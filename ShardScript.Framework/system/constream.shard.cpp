#include <stdexcept>
#include <string>
#include <random>
#include <climits>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <ShardScript.hpp>

#ifdef _WIN32
    #include <Windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
#endif

using namespace shard;

namespace
{
    static MethodSymbol* GetIPrintableToString()
    {
        static std::wstring methodName = L"ToString";
        static MethodSymbol* method = TRAIT_PRINTABLE->FindMethod(methodName, std::vector<TypeSymbol*>());
        return method;
    }

    static inline std::string thinify(const wchar_t* wstr)
    {
        size_t length = wcslen(wstr) + 1;
        std::string narrow(length, '\0');
        size_t converted = 0;

#ifdef _WIN32
        wcstombs_s(&converted, narrow.data(), length, wstr, _TRUNCATE);
#else
        wcstombs(narrow.data(), wstr, length);
#endif

        return narrow;
    }

    static ObjectInstance* InvokeToString(const CallState& context, ObjectInstance* instance)
    {
        TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
        MethodSymbol* implementation = type->FindInterfaceImplementation(GetIPrintableToString());
        if (implementation == nullptr)
            throw std::runtime_error("Type '" + thinify(type->FullName.c_str()) + "' does not implement IPrintable");

        context.Runtimer.InvokeMethod(implementation, { instance });
        ObjectInstance* result = context.Runtimer.CurrentFrame()->PopStack();

        if (result == nullptr || result->getInfo() != SymbolTable::Primitives::String)
            throw std::runtime_error("ToString did not return a string");

        return result;
    }

    static TypeSymbol* GetElementTypeOfEnumerable(ObjectInstance* enumerable)
    {
        TypeSymbol* type = const_cast<TypeSymbol*>(enumerable->getInfo());
        if (type == nullptr)
            return nullptr;

        if (type->Kind == SyntaxKind::ArrayType)
            return static_cast<ArrayTypeSymbol*>(type)->UnderlayingType;

        GenericTypeSymbol* genericType = nullptr;
        TypeSymbol* searchType = type;
        if (type->Kind == SyntaxKind::GenericType)
        {
            genericType = static_cast<GenericTypeSymbol*>(type);
            searchType = genericType->UnderlayingType;
        }

        for (TypeSymbol* iface : searchType->Interfaces)
        {
            if (iface->Kind != SyntaxKind::GenericType)
                continue;

            GenericTypeSymbol* genericIface = static_cast<GenericTypeSymbol*>(iface);
            if (genericIface->UnderlayingType != TRAIT_ENUMERABLE)
                continue;

            TypeSymbol* elementType = genericIface->SubstituteTypeParameters(TRAIT_ENUMERABLE->TypeParameters[0]);
            if (elementType != nullptr && elementType->Kind == SyntaxKind::TypeParameter && genericType != nullptr)
            {
                TypeSymbol* resolved = genericType->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(elementType));
                if (resolved != nullptr)
                    elementType = resolved;
            }

            return elementType;
        }

        return nullptr;
    }

    static void TryEnableVT100Processing()
    {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
            return;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode))
            return;

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif
    }
}

static ObjectInstance* shard_constream_print(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    if (instance == nullptr || instance == context.Collector.NullInstance)
    {
        ConsoleHelper::Write(L"null");
        return nullptr;
    }

    ObjectInstance* result = InvokeToString(context, instance);
    ConsoleHelper::Write(result->AsString());
    context.Collector.CollectInstance(result);
    return nullptr; // void
}

static ObjectInstance* shard_constream_println(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    if (instance == nullptr || instance == context.Collector.NullInstance)
    {
        ConsoleHelper::WriteLine(L"null");
        return nullptr;
    }

    ObjectInstance* result = InvokeToString(context, instance);
    ConsoleHelper::WriteLine(result->AsString());
    context.Collector.CollectInstance(result);
    return nullptr; // void
}

static ObjectInstance* shard_constream_println_enumerable(const CallState& context) noexcept(false)
{
    ObjectInstance* enumerable = context.Args[0];
    if (enumerable == nullptr || enumerable == context.Collector.NullInstance)
    {
        ConsoleHelper::WriteLine(L"");
        return nullptr;
    }

    TypeSymbol* enumerableType = const_cast<TypeSymbol*>(enumerable->getInfo());
    MethodSymbol* getEnumerator = enumerableType->FindInterfaceImplementation(TRAIT_ENUMERABLE_GETENUMERATOR);
    if (getEnumerator == nullptr)
        throw std::runtime_error("Type '" + thinify(enumerableType->FullName.c_str()) + "' does not implement IEnumerable<T>");

    TypeSymbol* elementType = GetElementTypeOfEnumerable(enumerable);
    if (elementType == nullptr)
        throw std::runtime_error("Could not determine element type of IEnumerable<T>");

    enumerable->IncrementReference();
    context.Runtimer.SetPendingTypeArguments({ elementType });
    ObjectInstance* enumerator = context.Runtimer.InvokeMethod(getEnumerator, &enumerable, 1);
    if (enumerator == nullptr)
        throw std::runtime_error("GetEnumerator returned null");

    TypeSymbol* enumeratorType = const_cast<TypeSymbol*>(enumerator->getInfo());
    MethodSymbol* moveNext = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_MOVENEXT);
    MethodSymbol* currentGetter = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_CURRENT_GET);
    if (moveNext == nullptr)
        throw std::runtime_error("Enumerator does not implement MoveNext");
    if (currentGetter == nullptr)
        throw std::runtime_error("Enumerator does not implement Current getter");

    bool first = true;
    while (true)
    {
        enumerator->IncrementReference();
        ObjectInstance* moveResult = context.Runtimer.InvokeMethod(moveNext, &enumerator, 1);
        if (moveResult == nullptr || !moveResult->AsBoolean())
            break;

        enumerator->IncrementReference();
        ObjectInstance* current = context.Runtimer.InvokeMethod(currentGetter, &enumerator, 1);

        if (!first)
            ConsoleHelper::Write(L" ");
        first = false;

        ObjectInstance* currentString = InvokeToString(context, current);
        ConsoleHelper::Write(currentString->AsString());
        context.Collector.CollectInstance(currentString);
    }

    context.Collector.CollectInstance(enumerator);
    ConsoleHelper::WriteLine(L"");
    return nullptr;
}

static ObjectInstance* shard_constream_input(const CallState& context)
{
    std::wstring input;
    if (std::getline(std::wcin, input))
    {
        return context.Collector.FromValue(input);
    }

    return context.Collector.FromValue(L"");
}

static ObjectInstance* shard_constream_Clear(const CallState& context) noexcept
{
    std::wcout << L"\x1b[2J\x1b[H" << std::flush;
    return nullptr;
}

static ObjectInstance* shard_constream_SetCursorPosition(const CallState& context) noexcept
{
    int64_t x = context.Args[0]->AsInteger();
    int64_t y = context.Args[1]->AsInteger();

    std::wcout << L"\x1b[" << (y + 1) << L";" << (x + 1) << L"H" << std::flush;
    return nullptr;
}

static ObjectInstance* shard_constream_SetCursorVisible(const CallState& context) noexcept
{
    bool visible = context.Args[0]->AsBoolean();
    if (visible)
        std::wcout << L"\x1b[?25h" << std::flush; // Показать
    else
        std::wcout << L"\x1b[?25l" << std::flush; // Скрыть
    return nullptr;
}

static ObjectInstance* shard_constream_ReadKey(const CallState& context) noexcept
{
    wchar_t ch = L'\0';

#ifdef _WIN32
    ch = static_cast<wchar_t>(_getwch());
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::wcin.get(ch);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    std::wstring resultStr(1, ch);
    return context.Collector.FromValue(resultStr);
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.stdio";
    lib.Description = L"Console IO with VT100 Support";
    lib.Version = L"1.1.0";
}

SHARDLIB_ENTRYPOINT
{
    TryEnableVT100Processing();

    SymbolBuilder<NamespaceSymbol> stdio(context, L"stdio");

    stdio.AddMethod(L"print", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
         .AddParameter(L"message", TRAIT_PRINTABLE)
         .SetCallback(&shard_constream_print);

    stdio.AddMethod(L"println", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
         .AddParameter(L"message", TRAIT_PRINTABLE)
         .SetCallback(&shard_constream_println);

    {
        auto printlnEnumerable = stdio.AddMethod(L"println", TYPE_VOID, LINK_STATIC, ACS_PUBLIC);
        TypeParameterSymbol* enumerableTypeParam = printlnEnumerable.AddTypeParameter(L"T").Get();
        printlnEnumerable
            .AddParameter(L"enumerable", stdio.GetFactory().GenericType(TRAIT_ENUMERABLE, { { L"T", enumerableTypeParam } }))
            .SetCallback(&shard_constream_println_enumerable);
    }

    stdio.AddMethod(L"input", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
         .SetCallback(&shard_constream_input);

    stdio.AddMethod(L"clear", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
        .SetCallback(&shard_constream_Clear);

    stdio.AddMethod(L"setCursor", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
        .AddParameter(L"x", SymbolTable::Primitives::Integer)
        .AddParameter(L"y", SymbolTable::Primitives::Integer)
        .SetCallback(&shard_constream_SetCursorPosition);

    stdio.AddMethod(L"setCursorVisible", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
        .AddParameter(L"visible", SymbolTable::Primitives::Boolean)
        .SetCallback(&shard_constream_SetCursorVisible);

    stdio.AddMethod(L"readKey", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
        .SetCallback(&shard_constream_ReadKey);
}
