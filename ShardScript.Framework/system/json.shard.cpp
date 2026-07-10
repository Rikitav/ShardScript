#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace shard;

namespace
{
    enum class JsonKind
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    struct JsonDom
    {
        JsonKind kind = JsonKind::Null;
        bool boolean = false;
        bool numberIsInteger = false;
        std::int64_t intValue = 0;
        double doubleValue = 0.0;
        std::wstring text;
        std::vector<JsonDom*> elements;
        std::vector<std::pair<std::wstring, JsonDom*>> members;
    };

    inline JsonDom* NewDom(JsonKind kind)
    {
        JsonDom* result = new JsonDom();
        result->kind = kind;

        return result;
    }

    inline bool IsWhitespace(wchar_t character)
    {
        return character == L' '
            || character == L'\t'
            || character == L'\n'
            || character == L'\r';
    }

    inline std::string Narrow(const std::wstring& wideText)
    {
        std::string result;
        result.reserve(wideText.size());

        for (wchar_t character : wideText)
        {
            result.push_back(static_cast<char>(static_cast<unsigned char>(character & 0xFF)));
        }

        return result;
    }

    inline double ParseDoubleClassic(const std::string& narrowText)
    {
        std::stringstream stream(narrowText);
        stream.imbue(std::locale::classic());

        double value = 0.0;
        stream >> value;

        return value;
    }

    inline double ParseDoubleClassic(const std::wstring& wideText)
    {
        return ParseDoubleClassic(Narrow(wideText));
    }

    inline std::int64_t DomToHandle(JsonDom* dom)
    {
        return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(dom));
    }

    inline JsonDom* HandleToDom(std::int64_t handle)
    {
        return reinterpret_cast<JsonDom*>(static_cast<std::uintptr_t>(handle));
    }

    ClassSymbol* g_jsonNodeClass = nullptr;
    FieldSymbol* g_jsonNodeHandleField = nullptr;

    inline ObjectInstance* WrapDom(JsonDom* dom, const CallState& context)
    {
        if (dom == nullptr)
        {
            return GarbageCollector::NullInstance;
        }

        ObjectInstance* wrapper = context.Collector.AllocateInstance(g_jsonNodeClass);
        wrapper->SetField(
            g_jsonNodeHandleField->SlotIndex,
            context.Collector.FromValue(DomToHandle(dom)));

        return wrapper;
    }

    inline JsonDom* UnwrapDom(ObjectInstance* wrapper)
    {
        if (wrapper == nullptr || wrapper == GarbageCollector::NullInstance)
        {
            return nullptr;
        }

        std::int64_t handle = wrapper->GetField(g_jsonNodeHandleField->SlotIndex)->AsInteger();
        if (handle == 0)
        {
            return nullptr;
        }

        return HandleToDom(handle);
    }

    inline TypeSymbol* EffectiveType(TypeSymbol* type)
    {
        if (type != nullptr && type->Kind == SyntaxKind::GenericType)
        {
            return static_cast<GenericTypeSymbol*>(type)->UnderlayingType;
        }

        return type;
    }

    inline bool IsSerializableField(FieldSymbol* field)
    {
        return field != nullptr
            && field->Linking != LINK_STATIC
            && field->Accesibility == ACS_PUBLIC;
    }

    inline void AppendHex4(std::wstring& output, unsigned value)
    {
        static const wchar_t hexDigits[] = L"0123456789abcdef";

        output += L"\\u";
        output.push_back(hexDigits[(value >> 12) & 0xF]);
        output.push_back(hexDigits[(value >> 8) & 0xF]);
        output.push_back(hexDigits[(value >> 4) & 0xF]);
        output.push_back(hexDigits[value & 0xF]);
    }

    inline void AppendCodepoint(std::wstring& output, unsigned codePoint)
    {
        if (sizeof(wchar_t) == 2 && codePoint > 0xFFFF)
        {
            codePoint -= 0x10000;

            wchar_t highSurrogate = static_cast<wchar_t>(0xD800 + (codePoint >> 10));
            wchar_t lowSurrogate = static_cast<wchar_t>(0xDC00 + (codePoint & 0x3FF));

            output.push_back(highSurrogate);
            output.push_back(lowSurrogate);
        }
        else
        {
            output.push_back(static_cast<wchar_t>(codePoint));
        }
    }

    inline void AppendCodepointEscape(std::wstring& output, unsigned codePoint)
    {
        if (codePoint <= 0xFFFF)
        {
            AppendHex4(output, codePoint);
        }
        else
        {
            codePoint -= 0x10000;

            unsigned highSurrogate = 0xD800 + (codePoint >> 10);
            unsigned lowSurrogate = 0xDC00 + (codePoint & 0x3FF);

            AppendHex4(output, highSurrogate);
            AppendHex4(output, lowSurrogate);
        }
    }

    void WriteJsonString(const std::wstring& input, std::wstring& output)
    {
        output.push_back(L'"');

        std::size_t index = 0;
        while (index < input.size())
        {
            unsigned codePoint;
            wchar_t current = input[index];

            if (sizeof(wchar_t) == 2 && current >= 0xD800 && current <= 0xDBFF && index + 1 < input.size())
            {
                wchar_t next = input[index + 1];

                if (next >= 0xDC00 && next <= 0xDFFF)
                {
                    codePoint = 0x10000u
                        + ((static_cast<unsigned>(current) - 0xD800u) << 10)
                        + (static_cast<unsigned>(next) - 0xDC00u);
                    index += 2;
                }
                else
                {
                    codePoint = static_cast<unsigned>(current);
                    index += 1;
                }
            }
            else
            {
                codePoint = static_cast<unsigned>(current);
                index += 1;
            }

            switch (codePoint)
            {
                case L'"':
                {
                    output += L"\\\"";
                    break;
                }

                case L'\\':
                {
                    output += L"\\\\";
                    break;
                }

                case L'\b':
                {
                    output += L"\\b";
                    break;
                }

                case L'\f':
                {
                    output += L"\\f";
                    break;
                }

                case L'\n':
                {
                    output += L"\\n";
                    break;
                }

                case L'\r':
                {
                    output += L"\\r";
                    break;
                }

                case L'\t':
                {
                    output += L"\\t";
                    break;
                }

                default:
                {
                    if (codePoint < 0x20)
                    {
                        AppendHex4(output, codePoint);
                    }
                    else if (codePoint < 0x7F)
                    {
                        output.push_back(static_cast<wchar_t>(codePoint));
                    }
                    else
                    {
                        AppendCodepointEscape(output, codePoint);
                    }

                    break;
                }
            }
        }

        output.push_back(L'"');
    }

    void FormatDouble(double value, std::wstring& output)
    {
        if (!std::isfinite(value))
        {
            throw std::runtime_error("JSON: cannot serialize non-finite number");
        }

        char buffer[64];

        std::snprintf(buffer, sizeof(buffer), "%.17g", value);

        for (int precision = 1; precision <= 17; ++precision)
        {
            std::snprintf(buffer, sizeof(buffer), "%.*g", precision, value);

            double parsed = std::strtod(buffer, nullptr);
            if (parsed == value)
            {
                break;
            }
        }

        for (char* pointer = buffer; *pointer != '\0'; ++pointer)
        {
            char character = *pointer;

            if (character == ',')
            {
                character = '.';
            }

            output.push_back(static_cast<wchar_t>(static_cast<unsigned char>(character)));
        }
    }

    void WriteDom(const JsonDom* dom, std::wstring& output)
    {
        if (dom == nullptr)
        {
            output += L"null";
            return;
        }

        switch (dom->kind)
        {
            case JsonKind::Null:
            {
                output += L"null";
                break;
            }

            case JsonKind::Boolean:
            {
                if (dom->boolean)
                {
                    output += L"true";
                }
                else
                {
                    output += L"false";
                }

                break;
            }

            case JsonKind::Number:
            {
                if (dom->numberIsInteger)
                {
                    output += std::to_wstring(dom->intValue);
                }
                else
                {
                    FormatDouble(dom->doubleValue, output);
                }

                break;
            }

            case JsonKind::String:
            {
                WriteJsonString(dom->text, output);
                break;
            }

            case JsonKind::Array:
            {
                output.push_back(L'[');

                for (std::size_t index = 0; index < dom->elements.size(); ++index)
                {
                    if (index != 0)
                    {
                        output.push_back(L',');
                    }

                    WriteDom(dom->elements[index], output);
                }

                output.push_back(L']');
                break;
            }

            case JsonKind::Object:
            {
                output.push_back(L'{');

                for (std::size_t index = 0; index < dom->members.size(); ++index)
                {
                    if (index != 0)
                    {
                        output.push_back(L',');
                    }

                    WriteJsonString(dom->members[index].first, output);
                    output.push_back(L':');
                    WriteDom(dom->members[index].second, output);
                }

                output.push_back(L'}');
                break;
            }
        }
    }

    class JsonParser
    {
    private:
        const std::wstring& source;
        std::size_t position = 0;

        [[noreturn]] void fail(const std::string& message) const
        {
            throw std::runtime_error("JSON: " + message + " at position " + std::to_string(position));
        }

        bool endOfFile() const
        {
            return position >= source.size();
        }

        wchar_t peek() const
        {
            if (endOfFile())
            {
                return L'\0';
            }

            return source[position];
        }

        void skipWhitespace()
        {
            while (!endOfFile() && IsWhitespace(source[position]))
            {
                ++position;
            }
        }

        bool matchLiteral(const wchar_t* literal)
        {
            std::size_t length = std::wcslen(literal);

            if (position + length > source.size())
            {
                return false;
            }

            for (std::size_t index = 0; index < length; ++index)
            {
                if (source[position + index] != literal[index])
                {
                    return false;
                }
            }

            wchar_t after = L'\0';

            if (position + length < source.size())
            {
                after = source[position + length];
            }

            bool isIdentifierContinuation = after == L'_'
                || (after >= L'0' && after <= L'9')
                || (after >= L'a' && after <= L'z')
                || (after >= L'A' && after <= L'Z')
                || after >= 0x80;

            if (isIdentifierContinuation)
            {
                fail(std::string("invalid literal after '") + Narrow(literal) + "'");
            }

            position += length;
            return true;
        }

        static bool isDigit(wchar_t character)
        {
            return character >= L'0' && character <= L'9';
        }

        unsigned parseHex4()
        {
            unsigned value = 0;

            for (int index = 0; index < 4; ++index)
            {
                if (endOfFile())
                {
                    fail("incomplete \\u escape");
                }

                wchar_t hexCharacter = source[position++];
                value <<= 4;

                if (hexCharacter >= L'0' && hexCharacter <= L'9')
                {
                    value |= static_cast<unsigned>(hexCharacter - L'0');
                }
                else if (hexCharacter >= L'a' && hexCharacter <= L'f')
                {
                    value |= static_cast<unsigned>(hexCharacter - L'a' + 10);
                }
                else if (hexCharacter >= L'A' && hexCharacter <= L'F')
                {
                    value |= static_cast<unsigned>(hexCharacter - L'A' + 10);
                }
                else
                {
                    fail("invalid hex digit in \\u escape");
                }
            }

            return value;
        }

        JsonDom* parseString()
        {
            if (peek() != L'"')
            {
                fail("expected '\"'");
            }

            ++position;

            std::wstring result;

            while (!endOfFile())
            {
                wchar_t current = source[position++];

                if (current == L'"')
                {
                    JsonDom* stringDom = NewDom(JsonKind::String);
                    stringDom->text = std::move(result);

                    return stringDom;
                }

                if (current == L'\\')
                {
                    if (endOfFile())
                    {
                        fail("trailing backslash in string");
                    }

                    wchar_t escape = source[position++];

                    switch (escape)
                    {
                        case L'"':
                        {
                            result += L'"';
                            break;
                        }

                        case L'\\':
                        {
                            result += L'\\';
                            break;
                        }

                        case L'/':
                        {
                            result += L'/';
                            break;
                        }

                        case L'b':
                        {
                            result += L'\b';
                            break;
                        }

                        case L'f':
                        {
                            result += L'\f';
                            break;
                        }

                        case L'n':
                        {
                            result += L'\n';
                            break;
                        }

                        case L'r':
                        {
                            result += L'\r';
                            break;
                        }

                        case L't':
                        {
                            result += L'\t';
                            break;
                        }

                        case L'u':
                        {
                            unsigned codePoint = parseHex4();

                            if (codePoint >= 0xD800 && codePoint <= 0xDBFF)
                            {
                                if (position + 6 <= source.size() && source[position] == L'\\' && source[position + 1] == L'u')
                                {
                                    position += 2;

                                    unsigned lowSurrogate = parseHex4();

                                    if (lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF)
                                    {
                                        codePoint = 0x10000u
                                            + ((codePoint - 0xD800u) << 10)
                                            + (lowSurrogate - 0xDC00u);
                                    }
                                    else
                                    {
                                        codePoint = 0xFFFD;
                                    }
                                }
                                else
                                {
                                    codePoint = 0xFFFD;
                                }
                            }
                            else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF)
                            {
                                codePoint = 0xFFFD;
                            }

                            AppendCodepoint(result, codePoint);
                            break;
                        }

                        default:
                        {
                            fail("invalid escape sequence");
                            break;
                        }
                    }

                    continue;
                }

                result += current;
            }

            fail("unterminated string");
        }

        JsonDom* parseNumber()
        {
            std::size_t startPosition = position;

            if (peek() == L'-')
            {
                ++position;
            }

            if (peek() == L'0')
            {
                ++position;
            }
            else if (!endOfFile() && isDigit(peek()))
            {
                while (!endOfFile() && isDigit(source[position]))
                {
                    ++position;
                }
            }
            else
            {
                fail("invalid number");
            }

            bool isFloat = false;

            if (!endOfFile() && source[position] == L'.')
            {
                isFloat = true;
                ++position;

                if (endOfFile() || !isDigit(peek()))
                {
                    fail("invalid number: missing digits after decimal point");
                }

                while (!endOfFile() && isDigit(source[position]))
                {
                    ++position;
                }
            }

            if (!endOfFile() && (source[position] == L'e' || source[position] == L'E'))
            {
                isFloat = true;
                ++position;

                if (!endOfFile() && (source[position] == L'+' || source[position] == L'-'))
                {
                    ++position;
                }

                if (endOfFile() || !isDigit(peek()))
                {
                    fail("invalid number: missing exponent digits");
                }

                while (!endOfFile() && isDigit(source[position]))
                {
                    ++position;
                }
            }

            JsonDom* numberDom = NewDom(JsonKind::Number);
            numberDom->text.assign(source, startPosition, position - startPosition);
            numberDom->numberIsInteger = !isFloat;

            if (numberDom->numberIsInteger)
            {
                try
                {
                    numberDom->intValue = std::stoll(Narrow(numberDom->text));
                    numberDom->doubleValue = static_cast<double>(numberDom->intValue);
                }
                catch (...)
                {
                    numberDom->numberIsInteger = false;
                    numberDom->doubleValue = ParseDoubleClassic(numberDom->text);
                }
            }
            else
            {
                numberDom->doubleValue = ParseDoubleClassic(numberDom->text);
            }

            return numberDom;
        }

        JsonDom* parseObject()
        {
            if (peek() != L'{')
            {
                fail("expected '{'");
            }

            ++position;
            skipWhitespace();

            JsonDom* objectDom = NewDom(JsonKind::Object);

            if (peek() == L'}')
            {
                ++position;
                return objectDom;
            }

            while (true)
            {
                skipWhitespace();

                if (peek() != L'"')
                {
                    fail("expected string key in object");
                }

                JsonDom* keyDom = parseString();
                std::wstring key = std::move(keyDom->text);
                delete keyDom;

                skipWhitespace();

                if (peek() != L':')
                {
                    fail("expected ':' after object key");
                }

                ++position;

                JsonDom* value = parseValue();
                objectDom->members.emplace_back(std::move(key), value);

                skipWhitespace();

                if (endOfFile())
                {
                    fail("unterminated object");
                }

                wchar_t separator = source[position++];

                if (separator == L'}')
                {
                    return objectDom;
                }

                if (separator != L',')
                {
                    fail("expected ',' or '}' in object");
                }

                skipWhitespace();
            }
        }

        JsonDom* parseArray()
        {
            if (peek() != L'[')
            {
                fail("expected '['");
            }

            ++position;
            skipWhitespace();

            JsonDom* arrayDom = NewDom(JsonKind::Array);

            if (peek() == L']')
            {
                ++position;
                return arrayDom;
            }

            while (true)
            {
                JsonDom* value = parseValue();
                arrayDom->elements.push_back(value);

                skipWhitespace();

                if (endOfFile())
                {
                    fail("unterminated array");
                }

                wchar_t separator = source[position++];

                if (separator == L']')
                {
                    return arrayDom;
                }

                if (separator != L',')
                {
                    fail("expected ',' or ']' in array");
                }

                skipWhitespace();
            }
        }

        JsonDom* parseValue()
        {
            skipWhitespace();

            if (endOfFile())
            {
                fail("unexpected end of input");
            }

            wchar_t current = peek();

            if (current == L'"')
            {
                return parseString();
            }

            if (current == L'{')
            {
                return parseObject();
            }

            if (current == L'[')
            {
                return parseArray();
            }

            if (current == L'-' || isDigit(current))
            {
                return parseNumber();
            }

            if (matchLiteral(L"true"))
            {
                JsonDom* booleanDom = NewDom(JsonKind::Boolean);
                booleanDom->boolean = true;

                return booleanDom;
            }

            if (matchLiteral(L"false"))
            {
                JsonDom* booleanDom = NewDom(JsonKind::Boolean);
                booleanDom->boolean = false;

                return booleanDom;
            }

            if (matchLiteral(L"null"))
            {
                return NewDom(JsonKind::Null);
            }

            fail("unexpected character");
        }

    public:
        explicit JsonParser(const std::wstring& sourceText)
            : source(sourceText)
        {
        }

        JsonDom* parse()
        {
            skipWhitespace();

            JsonDom* root = parseValue();

            skipWhitespace();

            if (!endOfFile())
            {
                fail("trailing characters after JSON value");
            }

            return root;
        }
    };

    std::int64_t DomAsInt(const JsonDom* dom)
    {
        if (dom == nullptr)
        {
            return 0;
        }

        if (dom->kind == JsonKind::Number)
        {
            return dom->numberIsInteger
                ? dom->intValue
                : static_cast<std::int64_t>(dom->doubleValue);
        }

        if (dom->kind == JsonKind::String)
        {
            try
            {
                return std::stoll(Narrow(dom->text));
            }
            catch (...)
            {
                return 0;
            }
        }

        if (dom->kind == JsonKind::Boolean)
        {
            return dom->boolean ? 1 : 0;
        }

        return 0;
    }

    double DomAsDouble(const JsonDom* dom)
    {
        if (dom == nullptr)
        {
            return 0.0;
        }

        if (dom->kind == JsonKind::Number)
        {
            return dom->numberIsInteger
                ? static_cast<double>(dom->intValue)
                : dom->doubleValue;
        }

        if (dom->kind == JsonKind::String)
        {
            return ParseDoubleClassic(dom->text);
        }

        if (dom->kind == JsonKind::Boolean)
        {
            return dom->boolean ? 1.0 : 0.0;
        }

        return 0.0;
    }

    const JsonDom* FindMember(const JsonDom* objectDom, const std::wstring& key)
    {
        if (objectDom == nullptr || objectDom->kind != JsonKind::Object)
        {
            return nullptr;
        }

        for (const auto& pair : objectDom->members)
        {
            if (pair.first == key)
            {
                return pair.second;
            }
        }

        return nullptr;
    }

    JsonDom* EncodeValue(const CallState& context, ObjectInstance* value, int depth);

    JsonDom* EncodeObject(const CallState& context, ObjectInstance* value, int depth)
    {
        TypeSymbol* objectType = EffectiveType(const_cast<TypeSymbol*>(value->getInfo()));

        JsonDom* objectDom = NewDom(JsonKind::Object);

        if (objectType == nullptr)
        {
            return objectDom;
        }

        std::unordered_set<std::wstring> seenFieldNames;

        for (FieldSymbol* field : objectType->Fields)
        {
            if (!IsSerializableField(field))
            {
                continue;
            }

            auto inserted = seenFieldNames.insert(field->Name);
            if (!inserted.second)
            {
                continue;
            }

#if JSON_DEBUG
            std::fprintf(stderr,
                "[DBG EncodeObject] inst=%ls field=%ls fieldParent=%ls match=%d\n",
                objectType->FullName.c_str(),
                field->Name.c_str(),
                field->Parent ? field->Parent->FullName.c_str() : L"<null>",
                objectType == field->Parent);
#endif

            ObjectInstance* fieldValue = value->GetField(field->SlotIndex);
            objectDom->members.emplace_back(field->Name, EncodeValue(context, fieldValue, depth + 1));
        }

        return objectDom;
    }

    JsonDom* EncodeValue(const CallState& context, ObjectInstance* value, int depth)
    {
        if (depth > 512)
        {
            throw std::runtime_error("JSON: nesting depth exceeded (possible cycle)");
        }

        if (value == nullptr || value == GarbageCollector::NullInstance)
        {
            return NewDom(JsonKind::Null);
        }

        TypeSymbol* type = const_cast<TypeSymbol*>(value->getInfo());

        if (type == nullptr)
        {
            return NewDom(JsonKind::Null);
        }

        if (type == TYPE_BOOL)
        {
            JsonDom* booleanDom = NewDom(JsonKind::Boolean);
            booleanDom->boolean = value->AsBoolean();

            return booleanDom;
        }

        if (type == TYPE_INT || type->Kind == SyntaxKind::EnumDeclaration)
        {
            JsonDom* numberDom = NewDom(JsonKind::Number);
            numberDom->numberIsInteger = true;
            numberDom->intValue = value->AsInteger();
            numberDom->doubleValue = static_cast<double>(numberDom->intValue);

            return numberDom;
        }

        if (type == TYPE_DOUBLE)
        {
            JsonDom* numberDom = NewDom(JsonKind::Number);
            numberDom->doubleValue = value->AsDouble();

            return numberDom;
        }

        if (type == TYPE_CHAR)
        {
            JsonDom* stringDom = NewDom(JsonKind::String);
            stringDom->text = std::wstring(1, value->AsCharacter());

            return stringDom;
        }

        if (type == TYPE_STRING)
        {
            JsonDom* stringDom = NewDom(JsonKind::String);
            stringDom->text.assign(value->AsString(), static_cast<std::size_t>(value->AsStringLength()));

            return stringDom;
        }

        if (type->Kind == SyntaxKind::ArrayType)
        {
            JsonDom* arrayDom = NewDom(JsonKind::Array);
            std::size_t length = value->GetArrayLength();

            arrayDom->elements.reserve(length);

            for (std::size_t index = 0; index < length; ++index)
            {
                arrayDom->elements.push_back(EncodeValue(context, value->GetElement(index), depth + 1));
            }

            return arrayDom;
        }

        if (type->Kind == SyntaxKind::ClassDeclaration
            || type->Kind == SyntaxKind::StructDeclaration
            || type->Kind == SyntaxKind::GenericType)
        {
            return EncodeObject(context, value, depth);
        }

        return NewDom(JsonKind::Null);
    }

    ObjectInstance* DecodeValue(const CallState& context, const JsonDom* dom,
        TypeSymbol* targetType, int depth);

    void FillFields(const CallState& context, ObjectInstance* object,
        TypeSymbol* effectiveType, const JsonDom* dom, int depth)
    {
        std::unordered_set<std::wstring> seenFieldNames;

        for (FieldSymbol* field : effectiveType->Fields)
        {
            if (!IsSerializableField(field))
            {
                continue;
            }

            auto inserted = seenFieldNames.insert(field->Name);
            if (!inserted.second)
            {
                continue;
            }

            const JsonDom* member = FindMember(dom, field->Name);

            if (member == nullptr)
            {
                continue;
            }

            TypeSymbol* fieldType = context.Frame->ResolveType(field->ReturnType);

            if (member->kind == JsonKind::Null)
            {
                if (fieldType != nullptr && fieldType->IsReferenceType())
                {
                    object->SetField(field->SlotIndex, GarbageCollector::NullInstance);
                }

                continue;
            }

            ObjectInstance* fieldValue = DecodeValue(context, member, fieldType, depth + 1);

            if (fieldValue == nullptr || fieldValue == GarbageCollector::NullInstance)
            {
                continue;
            }

            object->SetField(field->SlotIndex, fieldValue);
        }
    }

    ObjectInstance* DecodeValue(const CallState& context, const JsonDom* dom,
        TypeSymbol* targetType, int depth)
    {
        if (depth > 512)
        {
            throw std::runtime_error("JSON: nesting depth exceeded");
        }

        if (dom == nullptr)
        {
            return GarbageCollector::NullInstance;
        }

        CallStackFrame* frame = context.Frame;
        TypeSymbol* type = frame->ResolveType(targetType);

        if (type == nullptr)
        {
            return GarbageCollector::NullInstance;
        }

        if (dom->kind == JsonKind::Null)
        {
            return GarbageCollector::NullInstance;
        }

        if (type == TYPE_BOOL)
        {
            bool booleanValue = (dom->kind == JsonKind::Boolean)
                ? dom->boolean
                : (DomAsInt(dom) != 0);

            return context.Collector.FromValue(booleanValue);
        }

        if (type == TYPE_INT)
        {
            return context.Collector.FromValue(DomAsInt(dom));
        }

        if (type == TYPE_DOUBLE)
        {
            return context.Collector.FromValue(DomAsDouble(dom));
        }

        if (type == TYPE_CHAR)
        {
            wchar_t character = L'\0';

            if (dom->kind == JsonKind::String && !dom->text.empty())
            {
                character = dom->text[0];
            }

            return context.Collector.FromValue(character);
        }

        if (type == TYPE_STRING)
        {
            std::wstring text = (dom->kind == JsonKind::String) ? dom->text : std::wstring();

            return context.Collector.FromValue(text);
        }

        if (type->Kind == SyntaxKind::EnumDeclaration)
        {
            ObjectInstance* enumValue = context.Collector.AllocateInstance(type);
            enumValue->WriteInteger(DomAsInt(dom));

            return enumValue;
        }

        if (type->Kind == SyntaxKind::ArrayType)
        {
            if (dom->kind != JsonKind::Array)
            {
                throw std::runtime_error("JSON: expected array");
            }

            ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
            TypeSymbol* elementType = frame->ResolveType(arrayType->UnderlayingType);
            std::size_t count = dom->elements.size();

            ObjectInstance* array = context.Collector.AllocateArray(elementType, count);

            for (std::size_t index = 0; index < count; ++index)
            {
                ObjectInstance* child = DecodeValue(context, dom->elements[index], elementType, depth + 1);

                if (child == nullptr || child == GarbageCollector::NullInstance)
                {
                    if (elementType == nullptr || elementType->IsReferenceType())
                    {
                        array->SetElement(index, GarbageCollector::NullInstance, frame);
                    }

                    continue;
                }

                array->SetElement(index, child, frame);
            }

            return array;
        }

        if (type->Kind == SyntaxKind::ClassDeclaration
            || type->Kind == SyntaxKind::StructDeclaration)
        {
            if (dom->kind != JsonKind::Object)
            {
                throw std::runtime_error("JSON: expected object");
            }

            ObjectInstance* object = context.Collector.AllocateInstance(type);
            FillFields(context, object, type, dom, depth);

            return object;
        }

        if (type->Kind == SyntaxKind::GenericType)
        {
            if (dom->kind != JsonKind::Object)
            {
                throw std::runtime_error("JSON: expected object");
            }

            ObjectInstance* object = context.Collector.AllocateInstance(type);
            FillFields(context, object, EffectiveType(type), dom, depth);

            return object;
        }

        return GarbageCollector::NullInstance;
    }

    ObjectInstance* cb_JsonSerialize(const CallState& context)
    {
        if (context.Args.size() < 1)
        {
            throw std::runtime_error("JSON: Serialize expects a value");
        }

        JsonDom* dom = EncodeValue(context, context.Args[0], 0);

        std::wstring output;
        WriteDom(dom, output);

        return context.Collector.FromValue(output);
    }

    ObjectInstance* cb_JsonDeserialize(const CallState& context)
    {
        if (context.Args.size() < 1)
        {
            throw std::runtime_error("JSON: Deserialize expects a text argument");
        }

        if (context.Frame->TypeArguments.empty())
        {
            throw std::runtime_error("JSON: Deserialize requires a type argument, e.g. Deserialize<Person>(...)");
        }

        ObjectInstance* textArgument = context.Args[0];

        if (textArgument == nullptr || textArgument == GarbageCollector::NullInstance)
        {
            throw std::runtime_error("JSON: Deserialize text is null");
        }

        std::wstring text(textArgument->AsString(), static_cast<std::size_t>(textArgument->AsStringLength()));

        JsonParser parser(text);
        JsonDom* dom = parser.parse();

        TypeSymbol* targetType = context.Frame->TypeArguments[0];

        if (targetType == nullptr)
        {
            throw std::runtime_error("JSON: Deserialize type argument is null");
        }

        return DecodeValue(context, dom, targetType, 0);
    }

    ObjectInstance* cb_JsonNode_Parse(const CallState& context)
    {
        if (context.Args.size() < 1)
        {
            throw std::runtime_error("JSON: Parse expects a text argument");
        }

        ObjectInstance* textArgument = context.Args[0];

        if (textArgument == nullptr || textArgument == GarbageCollector::NullInstance)
        {
            throw std::runtime_error("JSON: Parse text is null");
        }

        std::wstring text(textArgument->AsString(), static_cast<std::size_t>(textArgument->AsStringLength()));

        JsonParser parser(text);
        JsonDom* dom = parser.parse();

        ObjectInstance* result = WrapDom(dom, context);

#if JSON_DEBUG
        std::fprintf(stderr,
            "[DBG cb_Parse] returning inst type=%ls\n",
            result->getInfo() ? result->getInfo()->FullName.c_str() : L"<null>");
#endif

        return result;
    }

    ObjectInstance* cb_JsonNode_Stringify(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        if (dom == nullptr)
        {
            throw std::runtime_error("JSON: Stringify on null node");
        }

        std::wstring output;
        WriteDom(dom, output);

        return context.Collector.FromValue(output);
    }

    const wchar_t* KindName(JsonKind kind)
    {
        switch (kind)
        {
            case JsonKind::Null:
            {
                return L"null";
            }

            case JsonKind::Boolean:
            {
                return L"bool";
            }

            case JsonKind::Number:
            {
                return L"number";
            }

            case JsonKind::String:
            {
                return L"string";
            }

            case JsonKind::Array:
            {
                return L"array";
            }

            case JsonKind::Object:
            {
                return L"object";
            }
        }

        return L"null";
    }

    ObjectInstance* cb_JsonNode_Kind(const CallState& context)
    {
#if JSON_DEBUG
        std::fprintf(stderr, "[DBG cb_Kind] entered, args=%zu\n", context.Args.size());
#endif

        JsonDom* dom = UnwrapDom(context.Args[0]);

#if JSON_DEBUG
        std::fprintf(stderr,
            "[DBG cb_Kind] dom=%p kind=%d\n",
            (void*)dom,
            dom ? (int)dom->kind : -1);
#endif

        return context.Collector.FromValue(std::wstring(dom ? KindName(dom->kind) : L"null"));
    }

    ObjectInstance* cb_JsonNode_IsNull(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        return context.Collector.FromValue(dom == nullptr || dom->kind == JsonKind::Null);
    }

    ObjectInstance* cb_JsonNode_IsObject(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        return context.Collector.FromValue(dom != nullptr && dom->kind == JsonKind::Object);
    }

    ObjectInstance* cb_JsonNode_IsArray(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        return context.Collector.FromValue(dom != nullptr && dom->kind == JsonKind::Array);
    }

    ObjectInstance* cb_JsonNode_AsInt(const CallState& context)
    {
        return context.Collector.FromValue(DomAsInt(UnwrapDom(context.Args[0])));
    }

    ObjectInstance* cb_JsonNode_AsDouble(const CallState& context)
    {
        return context.Collector.FromValue(DomAsDouble(UnwrapDom(context.Args[0])));
    }

    ObjectInstance* cb_JsonNode_AsBool(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);
        bool booleanValue = false;

        if (dom != nullptr)
        {
            booleanValue = (dom->kind == JsonKind::Boolean)
                ? dom->boolean
                : (DomAsInt(dom) != 0);
        }

        return context.Collector.FromValue(booleanValue);
    }

    ObjectInstance* cb_JsonNode_AsString(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);
        std::wstring text;

        if (dom != nullptr)
        {
            if (dom->kind == JsonKind::String)
            {
                text = dom->text;
            }
            else
            {
                WriteDom(dom, text);
            }
        }

        return context.Collector.FromValue(text);
    }

    ObjectInstance* cb_JsonNode_Get(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        if (dom == nullptr || dom->kind != JsonKind::Object)
        {
            return GarbageCollector::NullInstance;
        }

        std::wstring key(context.Args[1]->AsString(), static_cast<std::size_t>(context.Args[1]->AsStringLength()));

        for (auto& pair : dom->members)
        {
            if (pair.first == key)
            {
                return WrapDom(pair.second, context);
            }
        }

        return GarbageCollector::NullInstance;
    }

    ObjectInstance* cb_JsonNode_Set(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        if (dom == nullptr)
        {
            throw std::runtime_error("JSON: Set on null node");
        }

        if (dom->kind != JsonKind::Object)
        {
            dom->kind = JsonKind::Object;
        }

        std::wstring key(context.Args[1]->AsString(), static_cast<std::size_t>(context.Args[1]->AsStringLength()));

        JsonDom* value = UnwrapDom(context.Args[2]);

        if (value == nullptr)
        {
            value = NewDom(JsonKind::Null);
        }

        for (auto& pair : dom->members)
        {
            if (pair.first == key)
            {
                pair.second = value;

                return nullptr;
            }
        }

        dom->members.emplace_back(std::move(key), value);

        return nullptr;
    }

    ObjectInstance* cb_JsonNode_Contains(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        if (dom == nullptr || dom->kind != JsonKind::Object)
        {
            return context.Collector.FromValue(false);
        }

        std::wstring key(context.Args[1]->AsString(), static_cast<std::size_t>(context.Args[1]->AsStringLength()));

        for (auto& pair : dom->members)
        {
            if (pair.first == key)
            {
                return context.Collector.FromValue(true);
            }
        }

        return context.Collector.FromValue(false);
    }

    ObjectInstance* cb_JsonNode_Keys(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);
        std::size_t count = (dom != nullptr && dom->kind == JsonKind::Object)
            ? dom->members.size()
            : 0;

        ObjectInstance* array = context.Collector.AllocateArray(TYPE_STRING, count);

        for (std::size_t index = 0; index < count; ++index)
        {
            array->SetElement(index, context.Collector.FromValue(dom->members[index].first));
        }

        return array;
    }

    ObjectInstance* cb_JsonNode_Length(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);
        std::int64_t length = (dom != nullptr && dom->kind == JsonKind::Array)
            ? static_cast<std::int64_t>(dom->elements.size())
            : 0;

        return context.Collector.FromValue(length);
    }

    ObjectInstance* cb_JsonNode_At(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        if (dom == nullptr || dom->kind != JsonKind::Array)
        {
            return GarbageCollector::NullInstance;
        }

        std::int64_t index = context.Args[1]->AsInteger();

        if (index < 0 || static_cast<std::size_t>(index) >= dom->elements.size())
        {
            return GarbageCollector::NullInstance;
        }

        return WrapDom(dom->elements[static_cast<std::size_t>(index)], context);
    }

    ObjectInstance* cb_JsonNode_Add(const CallState& context)
    {
        JsonDom* dom = UnwrapDom(context.Args[0]);

        if (dom == nullptr)
        {
            throw std::runtime_error("JSON: Add on null node");
        }

        if (dom->kind != JsonKind::Array)
        {
            dom->kind = JsonKind::Array;
        }

        JsonDom* value = UnwrapDom(context.Args[1]);

        if (value == nullptr)
        {
            value = NewDom(JsonKind::Null);
        }

        dom->elements.push_back(value);

        return nullptr;
    }
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.json";
    lib.Description = L"JSON serializer/deserializer with typed model parsing";
    lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> jsonNamespace(context, L"json");
    SymbolFactory factory(context.GetSemanticModel().Table.get());

    SymbolBuilder<ClassSymbol> serializerClass = jsonNamespace.AddClass(L"JsonSerializer");

    {
        SymbolBuilder<MethodSymbol> serialize = serializerClass.AddMethod(L"Serialize", TYPE_STRING, LINK_STATIC);
        TypeParameterSymbol* serializeTypeParameter = serialize.AddTypeParameter(L"T");
        serialize.AddParameter(L"value", serializeTypeParameter).SetCallback(&cb_JsonSerialize);
    }

    {
        SymbolBuilder<MethodSymbol> deserialize = serializerClass.AddMethod(L"Deserialize", TYPE_ANY, LINK_STATIC);
        TypeParameterSymbol* deserializeTypeParameter = deserialize.AddTypeParameter(L"T");
        deserialize.Get()->ReturnType = deserializeTypeParameter;
        deserialize.AddParameter(L"text", TYPE_STRING).SetCallback(&cb_JsonDeserialize);
    }

    SymbolBuilder<ClassSymbol> jsonNodeClass = jsonNamespace.AddClass(L"JsonNode");
    g_jsonNodeClass = jsonNodeClass.Implements(TRAIT_PRINTABLE);

    g_jsonNodeHandleField = jsonNodeClass
        .AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

    ClassSymbol* nodeType = jsonNodeClass.Get();
    ArrayTypeSymbol* stringArrayType = factory.Array(TYPE_STRING);

    jsonNodeClass.AddMethod(L"Parse", nodeType, LINK_STATIC)
        .AddParameter(L"text", TYPE_STRING)
        .SetCallback(&cb_JsonNode_Parse);

    jsonNodeClass.AddMethod(L"ToString", TYPE_STRING, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_Stringify)
        .IsImplementationOf(TRAIT_PRINTABLE_ToString);

    jsonNodeClass.AddMethod(L"Kind", TYPE_STRING, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_Kind);

    jsonNodeClass.AddMethod(L"IsNull", TYPE_BOOL, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_IsNull);

    jsonNodeClass.AddMethod(L"IsObject", TYPE_BOOL, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_IsObject);

    jsonNodeClass.AddMethod(L"IsArray", TYPE_BOOL, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_IsArray);

    jsonNodeClass.AddMethod(L"AsInt", TYPE_INT, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_AsInt);

    jsonNodeClass.AddMethod(L"AsDouble", TYPE_DOUBLE, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_AsDouble);

    jsonNodeClass.AddMethod(L"AsBool", TYPE_BOOL, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_AsBool);

    jsonNodeClass.AddMethod(L"AsString", TYPE_STRING, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_AsString);

    jsonNodeClass.AddMethod(L"Get", nodeType, LINK_INSTANCE)
        .AddParameter(L"key", TYPE_STRING)
        .SetCallback(&cb_JsonNode_Get);

    jsonNodeClass.AddMethod(L"Set", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"key", TYPE_STRING)
        .AddParameter(L"value", nodeType)
        .SetCallback(&cb_JsonNode_Set);

    jsonNodeClass.AddMethod(L"Contains", TYPE_BOOL, LINK_INSTANCE)
        .AddParameter(L"key", TYPE_STRING)
        .SetCallback(&cb_JsonNode_Contains);

    jsonNodeClass.AddMethod(L"Keys", stringArrayType, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_Keys);

    jsonNodeClass.AddMethod(L"Length", TYPE_INT, LINK_INSTANCE)
        .SetCallback(&cb_JsonNode_Length);

    jsonNodeClass.AddMethod(L"At", nodeType, LINK_INSTANCE)
        .AddParameter(L"index", TYPE_INT)
        .SetCallback(&cb_JsonNode_At);

    jsonNodeClass.AddMethod(L"Add", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"value", nodeType)
        .SetCallback(&cb_JsonNode_Add);
}
