#pragma once
#include <ShardScript.hpp>
#include <string>

using namespace shard;

static std::wstring EscapeString(const wchar_t* str)
{
    std::wstring result;
    if (str == nullptr)
        return result;

    for (const wchar_t* p = str; *p != L'\0'; ++p)
    {
        switch (*p)
        {
            case L'"':  result += L"\\\""; break;
            case L'\\': result += L"\\\\"; break;
            case L'\n': result += L"\\n"; break;
            case L'\r': result += L"\\r"; break;
            case L'\t': result += L"\\t"; break;
            default:    result += *p; break;
        }
    }

    return result;
}

struct DumpOptions
{
    int MaxDepth = 3;
    bool Colorized = true;
    bool ShowTypes = false;
    int IndentSize = 2;
};

struct DumpColors
{
    std::wstring Null = L"\x1b[35m";
    std::wstring String = L"\x1b[32m";
    std::wstring Number = L"\x1b[33m";
    std::wstring Boolean = L"\x1b[35m";
    std::wstring Key = L"\x1b[36m";
    std::wstring Type = L"\x1b[90m";
    std::wstring Reset = L"\x1b[0m";
};

class ObjectDumper
{
public:
    ObjectDumper(const CallState& callState, const DumpOptions& options)
        : context(callState), options(options), colors(options.Colorized ? DumpColors{} : DumpColors{})
    {
        if (!options.Colorized)
        {
            colors.Null.clear();
            colors.String.clear();
            colors.Number.clear();
            colors.Boolean.clear();
            colors.Key.clear();
            colors.Type.clear();
            colors.Reset.clear();
        }
    }

    void Dump(ObjectInstance value)
    {
        DumpInternal(value, 0);
    }

private:
    const CallState& context;
    DumpOptions options;
    DumpColors colors;
    std::unordered_set<std::byte*> visited;

    void Write(const std::wstring& text)
    {
        ConsoleHelper::Write(text);
    }

    std::wstring Indent(int depth) const
    {
        return std::wstring(depth * options.IndentSize, L' ');
    }

    std::wstring Color(const std::wstring& code, const std::wstring& text) const
    {
        return code + text + colors.Reset;
    }

    void DumpInternal(ObjectInstance value, int depth)
    {
        if (value.IsNullInstance())
        {
            Write(Color(colors.Null, L"null"));
            return;
        }

        if (depth > options.MaxDepth)
        {
            TypeSymbol* type = const_cast<TypeSymbol*>(value.getInfo());
            std::wstring typeName = type != nullptr ? type->Name : L"Object";
            Write(Color(colors.Type, L"[object " + typeName + L"]"));
            return;
        }

        if (visited.count(value.getMemory()))
        {
            Write(Color(colors.Type, L"[Circular]"));
            return;
        }

        TypeSymbol* type = const_cast<TypeSymbol*>(value.getInfo());
        if (type == nullptr)
        {
            Write(L"<unknown>");
            return;
        }

        // Primitives
        if (type == TYPE_BOOL)
        {
            Write(Color(colors.Boolean, value.AsBoolean() ? L"true" : L"false"));
            return;
        }

        if (type == TYPE_INT || type == TYPE_NINT || type == TYPE_BYTE)
        {
            std::int64_t number = (type == TYPE_BYTE)
                ? static_cast<std::int64_t>(value.AsByte())
                : value.AsInteger();
            
            Write(Color(colors.Number, std::to_wstring(number)));
            return;
        }

        if (type == TYPE_DOUBLE)
        {
            Write(Color(colors.Number, std::to_wstring(value.AsDouble())));
            return;
        }

        if (type == TYPE_CHAR)
        {
            std::wstring ch(1, value.AsCharacter());
            Write(Color(colors.String, L"'" + EscapeString(ch.c_str()) + L"'"));
            return;
        }

        if (type == TYPE_STRING)
        {
            Write(Color(colors.String, L"\"" + EscapeString(value.AsString()) + L"\""));
            return;
        }

        // Arrays
        if (type->Kind == SyntaxKind::ArrayType)
        {
            std::size_t length = value.GetArrayLength();
            Write(L"[");
            for (std::size_t i = 0; i < length; ++i)
            {
                if (i > 0)
                    Write(L", ");

                ObjectInstance element = value.GetElement(i);
                ObjectRef elementRef(element);
                DumpInternal(element, depth + 1);
            }

            Write(L"]");
            return;
        }

        // Enums
        if (type->Kind == SyntaxKind::EnumDeclaration)
        {
            std::int64_t enumValue = value.AsInteger();
            Write(Color(colors.Type, type->Name + L"."));

            FieldSymbol* matchingField = nullptr;
            for (FieldSymbol* field : type->Fields)
            {
                if (field != nullptr && field->IsEnumValue && field->EnumValue == enumValue)
                {
                    matchingField = field;
                    break;
                }
            }

            if (matchingField != nullptr)
                Write(Color(colors.Number, matchingField->Name));
            else
                Write(Color(colors.Number, std::to_wstring(enumValue)));

            return;
        }

        // Objects (classes / structs)
        visited.insert(value.getMemory());
        ObjectRef selfRef(value);

        Write(L"{");
        if (options.ShowTypes)
        {
            Write(L"\n" + Indent(depth + 1) + Color(colors.Type, L"#type: " + type->Name));
        }

        bool first = !options.ShowTypes;
        std::unordered_set<std::wstring> printedNames;

        auto shouldSkipField = [](const std::wstring& name) -> bool
        {
            return name.find(L"k__BackingField") != std::wstring::npos || name.find(L'<') != std::wstring::npos;
        };

        // Properties
        for (PropertySymbol* property : type->Properties)
        {
            if (property == nullptr || property->Getter == nullptr)
                continue;

            printedNames.insert(property->Name);
            if (!first)
                Write(L",");

            first = false;
            Write(L"\n" + Indent(depth + 1) + Color(colors.Key, property->Name) + L": ");

            ObjectInstance propertyValue = context.Runtimer.InvokeMethod(property->Getter, &value, 1);
            ObjectRef propertyRef(propertyValue);
            DumpInternal(propertyValue, depth + 1);
        }

        // Fields (skip compiler-generated backing fields and anything already printed as a property)
        for (FieldSymbol* field : type->Fields)
        {
            if (field == nullptr || shouldSkipField(field->Name) || printedNames.count(field->Name))
                continue;

            printedNames.insert(field->Name);
            if (!first)
                Write(L",");

            first = false;
            Write(L"\n" + Indent(depth + 1) + Color(colors.Key, field->Name) + L": ");

            ObjectInstance fieldValue = value.GetField(field->SlotIndex);
            DumpInternal(fieldValue, depth + 1);
        }

        if (!first)
            Write(L"\n" + Indent(depth));

        Write(L"}");
        visited.erase(value.getMemory());
    }
};