#include <stdexcept>
#include <string>
#include <random>
#include <climits>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <unordered_set>

#include <ShardScript.hpp>
#include <utilities/Console.hpp>

#include "ObjectDumper.hpp"

using namespace shard;

static bool RunningInTtyMode = true;

namespace
{
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
}

static void shard_constream_print(const CallState& context) noexcept(false)
{
    auto [instance] = context.GetArgs<ObjectInstance>();
    if (instance.IsNullInstance())
    {
        ConsoleHelper::Write(L"null");
        return;
    }

    InvokeResult result = context.TryInvokeMethod(TRAIT_PRINTABLE_ToString, { instance });
    if (!result)
        context.Propagate(result.Exception());

    ObjectInstance value = result.Value();
    ConsoleHelper::Write(value.AsString());
    context.Collector.CollectInstance(value);
}

static void shard_constream_println(const CallState& context) noexcept(false)
{
    auto [instance] = context.GetArgs<ObjectInstance>();
    if (instance.IsNullInstance())
    {
        ConsoleHelper::Write(L"null");
        return;
    }

    InvokeResult result = context.TryInvokeMethod(TRAIT_PRINTABLE_ToString, { instance });
    if (!result)
        context.Propagate(result.Exception());

    ObjectInstance value = result.Value();
    ConsoleHelper::WriteLine(value.AsString());
    context.Collector.CollectInstance(value);
}

static void shard_constream_println_enumerable(const CallState& context) noexcept(false)
{
    auto [enumerable] = context.GetArgs<ObjectInstance>();
    if (enumerable.IsNullInstance())
    {
        ConsoleHelper::WriteLine(L"[]");
        return;
    }

    ConsoleHelper::Write(L"[");
    for (ObjectInstance val : EnumerableAdapter(context, enumerable))
    {
        InvokeResult result = context.TryInvokeMethod(TRAIT_PRINTABLE_ToString, { val });
        if (!result)
            context.Propagate(result.Exception());

        ObjectInstance value = result.Value();
        ConsoleHelper::WriteLine(value.AsString());
    }

    ConsoleHelper::WriteLine(L"]");
    return;

    /*
    TypeSymbol* enumerableType = const_cast<TypeSymbol*>(enumerable->getInfo());
    MethodSymbol* getEnumerator = enumerableType->FindInterfaceImplementation(TRAIT_ENUMERABLE_GETENUMERATOR);
    if (getEnumerator == nullptr)
        throw std::runtime_error("Type '" + thinify(enumerableType->FullName.c_str()) + "' does not implement IEnumerable<T>");

    TypeSymbol* elementType = GetElementTypeOfEnumerable(enumerable);
    if (elementType == nullptr)
        throw std::runtime_error("Could not determine element type of IEnumerable<T>");

    enumerable->IncrementReference();
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
            ConsoleHelper::Write(L"[");
        else
            ConsoleHelper::Write(L" ");
        first = false;

        ObjectInstance* currentString = InvokeToString(context, current);
        ConsoleHelper::Write(currentString->AsString());
        context.Collector.CollectInstance(currentString);
    }

    context.Collector.CollectInstance(enumerator);
    ConsoleHelper::WriteLine(L"]");
    return nullptr;
    */
}

static void shard_constream_input(const CallState& context)
{
    std::wstring input;
    if (!std::getline(std::wcin, input))
        input = L"";

    context.PlaceReturned(context.Collector.FromString(input));
}

static void shard_constream_input_prompt(const CallState& context)
{
    auto [prompt] = context.GetArgs<const wchar_t*>();
    if (prompt != nullptr)
        ConsoleHelper::Write(prompt);

    std::wstring input;
    if (!std::getline(std::wcin, input))
        input = L"";

    context.PlaceReturned(context.Collector.FromString(input));
}

static void shard_constream_Clear(const CallState& context) noexcept
{
    std::wcout << L"\x1b[2J\x1b[H" << std::flush;
}

static void shard_constream_SetCursorPosition(const CallState& context) noexcept
{
    auto [x, y] = context.GetArgs<std::int64_t, std::int64_t>();
    std::wcout << L"\x1b[" << (y + 1) << L";" << (x + 1) << L"H" << std::flush;
}

static void shard_constream_SetCursorVisible(const CallState& context) noexcept
{
    auto [visible] = context.GetArgs<bool>();
    ConsoleHelper::Write(visible ? L"\x1b[?25h" : L"\x1b[?25l");
}

static void shard_constream_ReadKey(const CallState& context) noexcept
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
    context.PlaceReturned(context.Collector.FromString(resultStr));
}

static void shard_constream_dump(const CallState& context) noexcept(false)
{
    auto [value] = context.GetArgs<ObjectInstance>();
    DumpOptions options;

    ObjectDumper dumper(context, options);
    dumper.Dump(value);

    ConsoleHelper::WriteLine();
}

static void shard_constream_dump_with_options(const CallState& context) noexcept(false)
{
    auto [value, maxDepth, colorized] = context.GetArgs<ObjectInstance, std::int64_t, bool>();

    DumpOptions options;
    options.MaxDepth = static_cast<int>(maxDepth);
    options.Colorized = colorized;

    ObjectDumper dumper(context, options);
    dumper.Dump(value);

    ConsoleHelper::WriteLine();
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.stdio";
    lib.Description = L"Console IO with VT100 Support";
    lib.Version = L"1.1.0";
}

SHARDLIB_ENTRYPOINT
{
    shard::console::EnableColors();

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

    stdio.AddMethod(L"input", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
         .AddParameter(L"prompt", SymbolTable::Primitives::String)
         .SetCallback(&shard_constream_input_prompt);

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

    {
        auto dump = stdio.AddMethod(L"Dump", TYPE_VOID, LINK_STATIC, ACS_PUBLIC);
        TypeParameterSymbol* dumpTypeParam = dump.AddTypeParameter(L"T").Get();
        dump
            .AddParameter(L"obj", dumpTypeParam)
            .SetCallback(&shard_constream_dump);
    }

    {
        auto dump = stdio.AddMethod(L"Dump", TYPE_VOID, LINK_STATIC, ACS_PUBLIC);
        TypeParameterSymbol* dumpTypeParam = dump.AddTypeParameter(L"T").Get();
        dump
            .AddParameter(L"obj", dumpTypeParam)
            .AddParameter(L"maxDepth", TYPE_INT)
            .AddParameter(L"colorized", TYPE_BOOL)
            .SetCallback(&shard_constream_dump_with_options);
    }
}
