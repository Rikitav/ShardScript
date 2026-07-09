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
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#define JSON_DEBUG 1

using namespace shard;

// =========================================================================
//  json — a System.Text.Json-style JSON engine for ShardScript.
//
//  Public API (namespace `json`):
//    JsonSerializer.Serialize<T>(value: T)   -> string
//    JsonSerializer.Deserialize<T>(text)     -> T
//    JsonNode.Parse(text)                    -> JsonNode   (static)
//    JsonNode: Stringify/Kind/IsNull/IsObject/IsArray,
//              AsInt/AsDouble/AsBool/AsString,
//              Get/Set/Contains/Keys, Length/At/Add
//
//  The engine is a strict recursive-descent parser producing an internal
//  DOM, plus two reflection-driven binders (ObjectInstance <-> DOM).
//
//  NOTE: parsed DOM trees are heap-allocated and intentionally not freed
//  when a JsonNode wrapper is collected (the runtime exposes no finalizer
//  hook). They are small and typically short-lived; a free-list can be
//  added later without changing the public API.
// =========================================================================

namespace
{
    // =====================================================================
    //  DOM
    // =====================================================================

    enum class JsonKind { Null, Boolean, Number, String, Array, Object };

    struct JsonDom
    {
        JsonKind kind = JsonKind::Null;
        bool boolean = false;
        std::wstring text;          // String content, or number lexeme
        bool numberIsInteger = false;
        std::int64_t intValue = 0;
        double doubleValue = 0.0;
        std::vector<JsonDom*> elements;                          // Array  (owned)
        std::vector<std::pair<std::wstring, JsonDom*>> members;  // Object (owned)
    };

    inline JsonDom* NewDom(JsonKind k)
    {
        JsonDom* d = new JsonDom();
        d->kind = k;
        return d;
    }

    inline bool IsWs(wchar_t c)
    {
        return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
    }

    inline std::string Narrow(const std::wstring& w)
    {
        std::string s;
        s.reserve(w.size());
        for (wchar_t c : w)
            s.push_back(static_cast<char>(static_cast<unsigned char>(c & 0xFF)));
        return s;
    }

    // =====================================================================
    //  Handle <-> DOM (for the JsonNode wrapper, like reflection.Type)
    // =====================================================================

    inline std::int64_t DomToHandle(JsonDom* d)
    {
        return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(d));
    }

    inline JsonDom* HandleToDom(std::int64_t h)
    {
        return reinterpret_cast<JsonDom*>(static_cast<std::uintptr_t>(h));
    }

    ClassSymbol* g_jsonNodeClass = nullptr;
    FieldSymbol* g_jsonNodeHandleField = nullptr;

    inline ObjectInstance* WrapDom(JsonDom* dom, const CallState& ctx)
    {
        if (dom == nullptr)
            return GarbageCollector::NullInstance;

        ObjectInstance* obj = ctx.Collector.AllocateInstance(g_jsonNodeClass);
#if JSON_DEBUG
        std::fprintf(stderr, "[DBG WrapDom] inst type=%ls | handle field parent=%ls | match=%d\n",
            obj->getInfo() ? obj->getInfo()->FullName.c_str() : L"<null>",
            (g_jsonNodeHandleField && g_jsonNodeHandleField->Parent) ? g_jsonNodeHandleField->Parent->FullName.c_str() : L"<null>",
            obj->getInfo() == (g_jsonNodeHandleField ? g_jsonNodeHandleField->Parent : nullptr));
#endif
        obj->SetField(g_jsonNodeHandleField->SlotIndex,
            ctx.Collector.FromValue(DomToHandle(dom)), ctx.Frame);
        return obj;
    }

    inline JsonDom* UnwrapDom(ObjectInstance* wrapper, CallStackFrame* frame)
    {
        if (wrapper == nullptr || wrapper == GarbageCollector::NullInstance)
            return nullptr;

#if JSON_DEBUG
        {
            const TypeSymbol* it = wrapper->getInfo();
            const SyntaxSymbol* fp = g_jsonNodeHandleField ? g_jsonNodeHandleField->Parent : nullptr;
            std::fprintf(stderr, "[DBG UnwrapDom] inst=%ls fieldParent=%ls inst==fieldParent=%d isRef=%d\n",
                it ? it->FullName.c_str() : L"<null>",
                fp ? fp->FullName.c_str() : L"<null>",
                static_cast<const SyntaxSymbol*>(it) == fp,
                it ? (int)it->IsReferenceType() : -1);
        }
#endif
        std::int64_t h = wrapper->GetField(g_jsonNodeHandleField->SlotIndex, frame)->AsInteger();
        return h == 0 ? nullptr : HandleToDom(h);
    }

    // =====================================================================
    //  Reflection helpers
    // =====================================================================

    inline TypeSymbol* EffectiveType(TypeSymbol* type)
    {
        if (type != nullptr && type->Kind == SyntaxKind::GenericType)
            return static_cast<GenericTypeSymbol*>(type)->UnderlayingType;
        return type;
    }

    // ShardScript fields are private by default; only `public` instance
    // fields are part of a model's serialized surface (matches System.Text.Json).
    inline bool IsSerializableField(FieldSymbol* field)
    {
        return field != nullptr
            && field->Linking != LINK_STATIC
            && field->Accesibility == ACS_PUBLIC;
    }

    // =====================================================================
    //  Writer (DOM -> wstring)
    // =====================================================================

    inline void AppendHex4(std::wstring& out, unsigned v)
    {
        static const wchar_t hex[] = L"0123456789abcdef";
        out += L"\\u";
        out.push_back(hex[(v >> 12) & 0xF]);
        out.push_back(hex[(v >> 8) & 0xF]);
        out.push_back(hex[(v >> 4) & 0xF]);
        out.push_back(hex[v & 0xF]);
    }

    inline void AppendCodepoint(std::wstring& out, unsigned cp)
    {
        if (sizeof(wchar_t) == 2 && cp > 0xFFFF)
        {
            cp -= 0x10000;
            out.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
        }
        else
        {
            out.push_back(static_cast<wchar_t>(cp));
        }
    }

    // Emits a code point as one or two \uXXXX JSON escapes (surrogate pair
    // for supplementary characters), keeping output pure ASCII.
    inline void AppendCodepointEscape(std::wstring& out, unsigned cp)
    {
        if (cp <= 0xFFFF)
            AppendHex4(out, cp);
        else
        {
            cp -= 0x10000;
            AppendHex4(out, 0xD800 + (cp >> 10));
            AppendHex4(out, 0xDC00 + (cp & 0x3FF));
        }
    }

    // Emits a JSON string literal, escaping control chars and all non-ASCII
    // as \uXXXX (with surrogate pairs) so the output is portable ASCII.
    void WriteJsonString(const std::wstring& s, std::wstring& out)
    {
        out.push_back(L'"');
        std::size_t i = 0;
        while (i < s.size())
        {
            unsigned cp;
            wchar_t c = s[i];
            if (sizeof(wchar_t) == 2 && c >= 0xD800 && c <= 0xDBFF && i + 1 < s.size())
            {
                wchar_t c2 = s[i + 1];
                if (c2 >= 0xDC00 && c2 <= 0xDFFF)
                {
                    cp = 0x10000u + ((static_cast<unsigned>(c) - 0xD800u) << 10)
                       + (static_cast<unsigned>(c2) - 0xDC00u);
                    i += 2;
                }
                else
                {
                    cp = static_cast<unsigned>(c);
                    i += 1;
                }
            }
            else
            {
                cp = static_cast<unsigned>(c);
                i += 1;
            }

            switch (cp)
            {
                case L'"':  out += L"\\\""; break;
                case L'\\': out += L"\\\\"; break;
                case L'\b': out += L"\\b";  break;
                case L'\f': out += L"\\f";  break;
                case L'\n': out += L"\\n";  break;
                case L'\r': out += L"\\r";  break;
                case L'\t': out += L"\\t";  break;
                default:
                    if (cp < 0x20)
                        AppendHex4(out, cp);
                    else if (cp < 0x7F)
                        out.push_back(static_cast<wchar_t>(cp));
                    else
                        AppendCodepointEscape(out, cp);
                    break;
            }
        }
        out.push_back(L'"');
    }

    void FormatDouble(double v, std::wstring& out)
    {
        if (!std::isfinite(v))
            throw std::runtime_error("JSON: cannot serialize non-finite number");

        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.17g", v);
        // Try to shorten to the fewest precision that still round-trips.
        for (int prec = 1; prec <= 17; ++prec)
        {
            std::snprintf(buf, sizeof(buf), "%.*g", prec, v);
            double parsed = std::strtod(buf, nullptr);
            if (parsed == v)
                break;
        }

        for (char* p = buf; *p != '\0'; ++p)
            out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
    }

    void WriteDom(const JsonDom* d, std::wstring& out)
    {
        if (d == nullptr)
        {
            out += L"null";
            return;
        }

        switch (d->kind)
        {
            case JsonKind::Null:
                out += L"null";
                break;

            case JsonKind::Boolean:
                out += (d->boolean ? L"true" : L"false");
                break;

            case JsonKind::Number:
                if (d->numberIsInteger)
                    out += std::to_wstring(d->intValue);
                else
                    FormatDouble(d->doubleValue, out);
                break;

            case JsonKind::String:
                WriteJsonString(d->text, out);
                break;

            case JsonKind::Array:
            {
                out.push_back(L'[');
                for (std::size_t i = 0; i < d->elements.size(); ++i)
                {
                    if (i != 0) out.push_back(L',');
                    WriteDom(d->elements[i], out);
                }
                out.push_back(L']');
                break;
            }

            case JsonKind::Object:
            {
                out.push_back(L'{');
                for (std::size_t i = 0; i < d->members.size(); ++i)
                {
                    if (i != 0) out.push_back(L',');
                    WriteJsonString(d->members[i].first, out);
                    out.push_back(L':');
                    WriteDom(d->members[i].second, out);
                }
                out.push_back(L'}');
                break;
            }
        }
    }

    // =====================================================================
    //  Parser (wstring -> DOM)
    // =====================================================================

    class JsonParser
    {
        const std::wstring& src;
        std::size_t pos = 0;

        [[noreturn]] void fail(const std::string& msg) const
        {
            throw std::runtime_error("JSON: " + msg + " at position "
                + std::to_string(pos));
        }

        bool eof() const { return pos >= src.size(); }
        wchar_t peek() const { return eof() ? L'\0' : src[pos]; }

        void skipWs()
        {
            while (!eof() && IsWs(src[pos]))
                ++pos;
        }

        bool matchLiteral(const wchar_t* lit)
        {
            std::size_t len = std::wcslen(lit);
            if (pos + len > src.size())
                return false;
            for (std::size_t k = 0; k < len; ++k)
                if (src[pos + k] != lit[k])
                    return false;

            // Must be followed by a non-identifier character.
            wchar_t after = (pos + len < src.size()) ? src[pos + len] : L'\0';
            if (after == L'_' || (after >= L'0' && after <= L'9') ||
                (after >= L'a' && after <= L'z') || (after >= L'A' && after <= L'Z') || after >= 0x80)
                fail(std::string("invalid literal after '") + Narrow(lit) + "'");

            pos += len;
            return true;
        }

        static bool isDigit(wchar_t c) { return c >= L'0' && c <= L'9'; }

        unsigned parseHex4()
        {
            unsigned v = 0;
            for (int k = 0; k < 4; ++k)
            {
                if (eof()) fail("incomplete \\u escape");
                wchar_t h = src[pos++];
                v <<= 4;
                if (h >= L'0' && h <= L'9') v |= static_cast<unsigned>(h - L'0');
                else if (h >= L'a' && h <= L'f') v |= static_cast<unsigned>(h - L'a' + 10);
                else if (h >= L'A' && h <= L'F') v |= static_cast<unsigned>(h - L'A' + 10);
                else fail("invalid hex digit in \\u escape");
            }
            return v;
        }

        JsonDom* parseString()
        {
            ++pos; // consume opening quote
            std::wstring out;
            while (!eof())
            {
                wchar_t c = src[pos++];
                if (c == L'"')
                {
                    JsonDom* d = NewDom(JsonKind::String);
                    d->text = std::move(out);
                    return d;
                }
                if (c == L'\\')
                {
                    if (eof()) fail("trailing backslash in string");
                    wchar_t e = src[pos++];
                    switch (e)
                    {
                        case L'"':  out.push_back(L'"');  break;
                        case L'\\': out.push_back(L'\\'); break;
                        case L'/':  out.push_back(L'/');  break;
                        case L'b':  out.push_back(L'\b'); break;
                        case L'f':  out.push_back(L'\f'); break;
                        case L'n':  out.push_back(L'\n'); break;
                        case L'r':  out.push_back(L'\r'); break;
                        case L't':  out.push_back(L'\t'); break;
                        case L'u':
                        {
                            unsigned cp = parseHex4();
                            if (cp >= 0xD800 && cp <= 0xDBFF)
                            {
                                // High surrogate; combine with a following \u low surrogate.
                                if (pos + 6 <= src.size() && src[pos] == L'\\' && src[pos + 1] == L'u')
                                {
                                    pos += 2;
                                    unsigned lo = parseHex4();
                                    if (lo >= 0xDC00 && lo <= 0xDFFF)
                                        cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                                    else
                                        cp = 0xFFFD; // dangling high surrogate
                                }
                                else
                                {
                                    cp = 0xFFFD; // dangling high surrogate
                                }
                            }
                            else if (cp >= 0xDC00 && cp <= 0xDFFF)
                            {
                                cp = 0xFFFD; // unexpected low surrogate
                            }
                            AppendCodepoint(out, cp);
                            break;
                        }
                        default:
                            fail("invalid escape sequence");
                    }
                }
                else
                {
                    out.push_back(c);
                }
            }
            fail("unterminated string");
        }

        JsonDom* parseNumber()
        {
            std::size_t start = pos;
            if (peek() == L'-') ++pos;
            while (!eof() && isDigit(src[pos])) ++pos;

            bool isFloat = false;
            if (!eof() && src[pos] == L'.')
            {
                isFloat = true;
                ++pos;
                while (!eof() && isDigit(src[pos])) ++pos;
            }
            if (!eof() && (src[pos] == L'e' || src[pos] == L'E'))
            {
                isFloat = true;
                ++pos;
                if (!eof() && (src[pos] == L'+' || src[pos] == L'-')) ++pos;
                while (!eof() && isDigit(src[pos])) ++pos;
            }

            JsonDom* d = NewDom(JsonKind::Number);
            d->text.assign(src, start, pos - start);
            d->numberIsInteger = !isFloat;

            std::string n = Narrow(d->text);
            if (d->numberIsInteger)
            {
                try
                {
                    d->intValue = std::stoll(n);
                    d->doubleValue = static_cast<double>(d->intValue);
                }
                catch (...)
                {
                    d->numberIsInteger = false;
                    try { d->doubleValue = std::stod(n); } catch (...) {}
                }
            }
            else
            {
                try { d->doubleValue = std::stod(n); } catch (...) {}
            }
            return d;
        }

        JsonDom* parseObject()
        {
            ++pos; // consume '{'
            JsonDom* d = NewDom(JsonKind::Object);
            skipWs();
            if (peek() == L'}') { ++pos; return d; }

            while (true)
            {
                skipWs();
                if (peek() != L'"') fail("expected string key in object");
                JsonDom* keyDom = parseString();
                std::wstring key = std::move(keyDom->text);
                delete keyDom;

                skipWs();
                if (peek() != L':') fail("expected ':' after object key");
                ++pos;

                JsonDom* value = parseValue();
                d->members.emplace_back(std::move(key), value);

                skipWs();
                wchar_t c = peek();
                if (c == L',') { ++pos; continue; }
                if (c == L'}') { ++pos; break; }
                fail("expected ',' or '}' in object");
            }
            return d;
        }

        JsonDom* parseArray()
        {
            ++pos; // consume '['
            JsonDom* d = NewDom(JsonKind::Array);
            skipWs();
            if (peek() == L']') { ++pos; return d; }

            while (true)
            {
                JsonDom* value = parseValue();
                d->elements.push_back(value);

                skipWs();
                wchar_t c = peek();
                if (c == L',') { ++pos; continue; }
                if (c == L']') { ++pos; break; }
                fail("expected ',' or ']' in array");
            }
            return d;
        }

        JsonDom* parseValue()
        {
            skipWs();
            if (eof()) fail("unexpected end of input");

            wchar_t c = peek();
            if (c == L'{') return parseObject();
            if (c == L'[') return parseArray();
            if (c == L'"') return parseString();
            if (c == L'-' || isDigit(c)) return parseNumber();
            if (matchLiteral(L"true"))  { JsonDom* d = NewDom(JsonKind::Boolean); d->boolean = true;  return d; }
            if (matchLiteral(L"false")) { JsonDom* d = NewDom(JsonKind::Boolean); d->boolean = false; return d; }
            if (matchLiteral(L"null"))  return NewDom(JsonKind::Null);
            fail("unexpected character");
        }

    public:
        explicit JsonParser(const std::wstring& s) : src(s) {}

        JsonDom* Parse()
        {
            skipWs();
            JsonDom* root = parseValue();
            skipWs();
            if (!eof()) fail("trailing characters after JSON value");
            return root;
        }
    };

    // =====================================================================
    //  Number coercion (lenient, like reading a loosely-typed JSON value)
    // =====================================================================

    std::int64_t DomAsInt(const JsonDom* d)
    {
        if (d == nullptr) return 0;
        if (d->kind == JsonKind::Number)
            return d->numberIsInteger ? d->intValue : static_cast<std::int64_t>(d->doubleValue);
        if (d->kind == JsonKind::String)
        {
            try { return std::stoll(Narrow(d->text)); } catch (...) { return 0; }
        }
        if (d->kind == JsonKind::Boolean) return d->boolean ? 1 : 0;
        return 0;
    }

    double DomAsDouble(const JsonDom* d)
    {
        if (d == nullptr) return 0.0;
        if (d->kind == JsonKind::Number)
            return d->numberIsInteger ? static_cast<double>(d->intValue) : d->doubleValue;
        if (d->kind == JsonKind::String)
        {
            try { return std::stod(Narrow(d->text)); } catch (...) { return 0.0; }
        }
        if (d->kind == JsonKind::Boolean) return d->boolean ? 1.0 : 0.0;
        return 0.0;
    }

    const JsonDom* FindMember(const JsonDom* obj, const std::wstring& key)
    {
        if (obj == nullptr || obj->kind != JsonKind::Object) return nullptr;
        for (const auto& m : obj->members)
            if (m.first == key) return m.second;
        return nullptr;
    }

    // =====================================================================
    //  Encoder: ObjectInstance -> DOM (reflection over public fields)
    // =====================================================================

    JsonDom* EncodeValue(const CallState& ctx, ObjectInstance* value, int depth);

    JsonDom* EncodeObject(const CallState& ctx, ObjectInstance* value, int depth)
    {
        TypeSymbol* type = EffectiveType(const_cast<TypeSymbol*>(value->getInfo()));
        JsonDom* d = NewDom(JsonKind::Object);
        if (type == nullptr) return d;

        for (FieldSymbol* field : type->Fields)
        {
            if (!IsSerializableField(field)) continue;
#if JSON_DEBUG
            std::fprintf(stderr, "[DBG EncodeObject] inst=%ls field=%ls fieldParent=%ls match=%d\n",
                type->FullName.c_str(),
                field->Name.c_str(),
                field->Parent ? field->Parent->FullName.c_str() : L"<null>",
                type == field->Parent);
#endif
            ObjectInstance* fieldValue = value->GetField(field->SlotIndex, ctx.Frame);
            d->members.emplace_back(field->Name, EncodeValue(ctx, fieldValue, depth + 1));
        }
        return d;
    }

    JsonDom* EncodeValue(const CallState& ctx, ObjectInstance* value, int depth)
    {
        if (depth > 512)
            throw std::runtime_error("JSON: nesting depth exceeded (possible cycle)");

        if (value == nullptr || value == GarbageCollector::NullInstance)
            return NewDom(JsonKind::Null);

        TypeSymbol* type = const_cast<TypeSymbol*>(value->getInfo());
        if (type == nullptr)
            return NewDom(JsonKind::Null);

        if (type == TYPE_BOOL)
        {
            JsonDom* d = NewDom(JsonKind::Boolean);
            d->boolean = value->AsBoolean();
            return d;
        }
        if (type == TYPE_INT || type->Kind == SyntaxKind::EnumDeclaration)
        {
            JsonDom* d = NewDom(JsonKind::Number);
            d->numberIsInteger = true;
            d->intValue = value->AsInteger();
            d->doubleValue = static_cast<double>(d->intValue);
            return d;
        }
        if (type == TYPE_DOUBLE)
        {
            JsonDom* d = NewDom(JsonKind::Number);
            d->doubleValue = value->AsDouble();
            return d;
        }
        if (type == TYPE_CHAR)
        {
            JsonDom* d = NewDom(JsonKind::String);
            d->text = std::wstring(1, value->AsCharacter());
            return d;
        }
        if (type == TYPE_STRING)
        {
            JsonDom* d = NewDom(JsonKind::String);
            d->text.assign(value->AsString(), static_cast<std::size_t>(value->AsStringLength()));
            return d;
        }
        if (type->Kind == SyntaxKind::ArrayType)
        {
            JsonDom* d = NewDom(JsonKind::Array);
            std::size_t len = value->GetArrayLength();
            d->elements.reserve(len);
            for (std::size_t i = 0; i < len; ++i)
                d->elements.push_back(EncodeValue(ctx, value->GetElement(i, ctx.Frame), depth + 1));
            return d;
        }
        if (type->Kind == SyntaxKind::ClassDeclaration ||
            type->Kind == SyntaxKind::StructDeclaration ||
            type->Kind == SyntaxKind::GenericType)
        {
            return EncodeObject(ctx, value, depth);
        }

        return NewDom(JsonKind::Null);
    }

    // =====================================================================
    //  Decoder: DOM -> ObjectInstance (reflection over public fields)
    // =====================================================================

    ObjectInstance* DecodeValue(const CallState& ctx, const JsonDom* dom,
                                TypeSymbol* targetType, int depth);

    void FillFields(const CallState& ctx, ObjectInstance* obj,
                    TypeSymbol* effType, const JsonDom* dom, int depth)
    {
        for (FieldSymbol* field : effType->Fields)
        {
            if (!IsSerializableField(field)) continue;

            const JsonDom* member = FindMember(dom, field->Name);
            if (member == nullptr)
                continue; // missing key -> leave default

            TypeSymbol* fieldType = ctx.Frame->ResolveType(field->ReturnType);

            if (member->kind == JsonKind::Null)
            {
                // null is only assignable to reference-type fields.
                if (fieldType != nullptr && fieldType->IsReferenceType())
                    obj->SetField(field->SlotIndex, GarbageCollector::NullInstance, ctx.Frame);
                continue;
            }

            ObjectInstance* fieldValue = DecodeValue(ctx, member, fieldType, depth + 1);
            if (fieldValue == nullptr || fieldValue == GarbageCollector::NullInstance)
                continue; // value-type + null already handled above

            obj->SetField(field->SlotIndex, fieldValue, ctx.Frame);
        }
    }

    ObjectInstance* DecodeValue(const CallState& ctx, const JsonDom* dom,
                                TypeSymbol* targetType, int depth)
    {
        if (depth > 512)
            throw std::runtime_error("JSON: nesting depth exceeded");

        if (dom == nullptr)
            return GarbageCollector::NullInstance;

        CallStackFrame* frame = ctx.Frame;
        TypeSymbol* type = frame->ResolveType(targetType);
        if (type == nullptr)
            return GarbageCollector::NullInstance;

        if (dom->kind == JsonKind::Null)
            return GarbageCollector::NullInstance;

        if (type == TYPE_BOOL)
        {
            bool b = (dom->kind == JsonKind::Boolean) ? dom->boolean : (DomAsInt(dom) != 0);
            return ctx.Collector.FromValue(b);
        }
        if (type == TYPE_INT)
            return ctx.Collector.FromValue(DomAsInt(dom));
        if (type == TYPE_DOUBLE)
            return ctx.Collector.FromValue(DomAsDouble(dom));
        if (type == TYPE_CHAR)
        {
            wchar_t ch = L'\0';
            if (dom->kind == JsonKind::String && !dom->text.empty())
                ch = dom->text[0];
            return ctx.Collector.FromValue(ch);
        }
        if (type == TYPE_STRING)
        {
            std::wstring s = (dom->kind == JsonKind::String) ? dom->text : std::wstring();
            return ctx.Collector.FromValue(s);
        }
        if (type->Kind == SyntaxKind::EnumDeclaration)
        {
            ObjectInstance* e = ctx.Collector.AllocateInstance(type);
            e->WriteInteger(DomAsInt(dom));
            return e;
        }
        if (type->Kind == SyntaxKind::ArrayType)
        {
            if (dom->kind != JsonKind::Array)
                throw std::runtime_error("JSON: expected array");

            auto* arrayType = static_cast<ArrayTypeSymbol*>(type);
            TypeSymbol* elemType = frame->ResolveType(arrayType->UnderlayingType);
            std::size_t n = dom->elements.size();

            ObjectInstance* arr = ctx.Collector.AllocateArray(elemType, n);
            for (std::size_t i = 0; i < n; ++i)
            {
                ObjectInstance* child = DecodeValue(ctx, dom->elements[i], elemType, depth + 1);
                if (child == nullptr || child == GarbageCollector::NullInstance)
                {
                    // null into a value-type element: leave it zeroed (AllocateArray zeroes).
                    if (elemType == nullptr || elemType->IsReferenceType())
                        arr->SetElement(i, GarbageCollector::NullInstance, frame);
                    continue;
                }
                arr->SetElement(i, child, frame);
            }
            return arr;
        }
        if (type->Kind == SyntaxKind::ClassDeclaration ||
            type->Kind == SyntaxKind::StructDeclaration)
        {
            if (dom->kind != JsonKind::Object)
                throw std::runtime_error("JSON: expected object");

            ObjectInstance* obj = ctx.Collector.AllocateInstance(type);
            FillFields(ctx, obj, type, dom, depth);
            return obj;
        }
        if (type->Kind == SyntaxKind::GenericType)
        {
            if (dom->kind != JsonKind::Object)
                throw std::runtime_error("JSON: expected object");

            ObjectInstance* obj = ctx.Collector.AllocateInstance(type);
            FillFields(ctx, obj, EffectiveType(type), dom, depth);
            return obj;
        }

        return GarbageCollector::NullInstance;
    }

    // =====================================================================
    //  Native callbacks
    // =====================================================================

    ObjectInstance* cb_JsonSerialize(const CallState& ctx)
    {
        if (ctx.Args.size() < 1)
            throw std::runtime_error("JSON: Serialize expects a value");

        JsonDom* dom = EncodeValue(ctx, ctx.Args[0], 0);
        std::wstring out;
        WriteDom(dom, out);
        return ctx.Collector.FromValue(out);
    }

    ObjectInstance* cb_JsonDeserialize(const CallState& ctx)
    {
        if (ctx.Args.size() < 1)
            throw std::runtime_error("JSON: Deserialize expects a text argument");

        if (ctx.Frame->TypeArguments.empty())
            throw std::runtime_error("JSON: Deserialize requires a type argument, e.g. Deserialize<Person>(...)");

        ObjectInstance* textArg = ctx.Args[0];
        if (textArg == nullptr || textArg == GarbageCollector::NullInstance)
            throw std::runtime_error("JSON: Deserialize text is null");

        std::wstring text(textArg->AsString(), static_cast<std::size_t>(textArg->AsStringLength()));

        JsonParser parser(text);
        JsonDom* dom = parser.Parse();

        TypeSymbol* targetType = ctx.Frame->TypeArguments[0];
        if (targetType == nullptr)
            throw std::runtime_error("JSON: Deserialize type argument is null");

        return DecodeValue(ctx, dom, targetType, 0);
    }

    // --- JsonNode ---

    ObjectInstance* cb_JsonNode_Parse(const CallState& ctx)
    {
        if (ctx.Args.size() < 1)
            throw std::runtime_error("JSON: Parse expects a text argument");

        ObjectInstance* textArg = ctx.Args[0];
        if (textArg == nullptr || textArg == GarbageCollector::NullInstance)
            throw std::runtime_error("JSON: Parse text is null");

        std::wstring text(textArg->AsString(), static_cast<std::size_t>(textArg->AsStringLength()));
        JsonParser parser(text);
        JsonDom* dom = parser.Parse();
        ObjectInstance* ret = WrapDom(dom, ctx);
#if JSON_DEBUG
        std::fprintf(stderr, "[DBG cb_Parse] returning inst type=%ls\n",
            ret->getInfo() ? ret->getInfo()->FullName.c_str() : L"<null>");
#endif
        return ret;
    }

    ObjectInstance* cb_JsonNode_Stringify(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        if (dom == nullptr) throw std::runtime_error("JSON: Stringify on null node");
        std::wstring out;
        WriteDom(dom, out);
        return ctx.Collector.FromValue(out);
    }

    const wchar_t* KindName(JsonKind k)
    {
        switch (k)
        {
            case JsonKind::Null:    return L"null";
            case JsonKind::Boolean: return L"bool";
            case JsonKind::Number:  return L"number";
            case JsonKind::String:  return L"string";
            case JsonKind::Array:   return L"array";
            case JsonKind::Object:  return L"object";
        }
        return L"null";
    }

    ObjectInstance* cb_JsonNode_Kind(const CallState& ctx)
    {
#if JSON_DEBUG
        std::fprintf(stderr, "[DBG cb_Kind] entered, args=%zu\n", ctx.Args.size());
#endif
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
#if JSON_DEBUG
        std::fprintf(stderr, "[DBG cb_Kind] dom=%p kind=%d\n", (void*)dom, dom ? (int)dom->kind : -1);
#endif
        return ctx.Collector.FromValue(std::wstring(dom ? KindName(dom->kind) : L"null"));
    }

    ObjectInstance* cb_JsonNode_IsNull(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        return ctx.Collector.FromValue(dom == nullptr || dom->kind == JsonKind::Null);
    }

    ObjectInstance* cb_JsonNode_IsObject(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        return ctx.Collector.FromValue(dom != nullptr && dom->kind == JsonKind::Object);
    }

    ObjectInstance* cb_JsonNode_IsArray(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        return ctx.Collector.FromValue(dom != nullptr && dom->kind == JsonKind::Array);
    }

    ObjectInstance* cb_JsonNode_AsInt(const CallState& ctx)
    {
        return ctx.Collector.FromValue(DomAsInt(UnwrapDom(ctx.Args[0], ctx.Frame)));
    }

    ObjectInstance* cb_JsonNode_AsDouble(const CallState& ctx)
    {
        return ctx.Collector.FromValue(DomAsDouble(UnwrapDom(ctx.Args[0], ctx.Frame)));
    }

    ObjectInstance* cb_JsonNode_AsBool(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        bool b = false;
        if (dom != nullptr)
            b = (dom->kind == JsonKind::Boolean) ? dom->boolean : (DomAsInt(dom) != 0);
        return ctx.Collector.FromValue(b);
    }

    ObjectInstance* cb_JsonNode_AsString(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        std::wstring s;
        if (dom != nullptr)
        {
            if (dom->kind == JsonKind::String)
                s = dom->text;
            else
                WriteDom(dom, s); // represent non-strings as their JSON literal
        }
        return ctx.Collector.FromValue(s);
    }

    ObjectInstance* cb_JsonNode_Get(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        if (dom == nullptr || dom->kind != JsonKind::Object)
            return GarbageCollector::NullInstance;

        std::wstring key(ctx.Args[1]->AsString(), static_cast<std::size_t>(ctx.Args[1]->AsStringLength()));
        for (auto& m : dom->members)
            if (m.first == key) return WrapDom(m.second, ctx);

        return GarbageCollector::NullInstance;
    }

    ObjectInstance* cb_JsonNode_Set(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        if (dom == nullptr) throw std::runtime_error("JSON: Set on null node");
        if (dom->kind != JsonKind::Object) dom->kind = JsonKind::Object;

        std::wstring key(ctx.Args[1]->AsString(), static_cast<std::size_t>(ctx.Args[1]->AsStringLength()));
        JsonDom* value = UnwrapDom(ctx.Args[2], ctx.Frame);
        if (value == nullptr) value = NewDom(JsonKind::Null);

        for (auto& m : dom->members)
        {
            if (m.first == key) { m.second = value; return nullptr; }
        }
        dom->members.emplace_back(std::move(key), value);
        return nullptr;
    }

    ObjectInstance* cb_JsonNode_Contains(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        if (dom == nullptr || dom->kind != JsonKind::Object)
            return ctx.Collector.FromValue(false);

        std::wstring key(ctx.Args[1]->AsString(), static_cast<std::size_t>(ctx.Args[1]->AsStringLength()));
        for (auto& m : dom->members)
            if (m.first == key) return ctx.Collector.FromValue(true);

        return ctx.Collector.FromValue(false);
    }

    ObjectInstance* cb_JsonNode_Keys(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        std::size_t n = (dom != nullptr && dom->kind == JsonKind::Object) ? dom->members.size() : 0;

        ObjectInstance* arr = ctx.Collector.AllocateArray(TYPE_STRING, n);
        for (std::size_t i = 0; i < n; ++i)
            arr->SetElement(i, ctx.Collector.FromValue(dom->members[i].first), ctx.Frame);

        return arr;
    }

    ObjectInstance* cb_JsonNode_Length(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        std::int64_t n = (dom != nullptr && dom->kind == JsonKind::Array)
            ? static_cast<std::int64_t>(dom->elements.size()) : 0;
        return ctx.Collector.FromValue(n);
    }

    ObjectInstance* cb_JsonNode_At(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        if (dom == nullptr || dom->kind != JsonKind::Array)
            return GarbageCollector::NullInstance;

        std::int64_t i = ctx.Args[1]->AsInteger();
        if (i < 0 || static_cast<std::size_t>(i) >= dom->elements.size())
            return GarbageCollector::NullInstance;

        return WrapDom(dom->elements[static_cast<std::size_t>(i)], ctx);
    }

    ObjectInstance* cb_JsonNode_Add(const CallState& ctx)
    {
        JsonDom* dom = UnwrapDom(ctx.Args[0], ctx.Frame);
        if (dom == nullptr) throw std::runtime_error("JSON: Add on null node");
        if (dom->kind != JsonKind::Array) dom->kind = JsonKind::Array;

        JsonDom* value = UnwrapDom(ctx.Args[1], ctx.Frame);
        if (value == nullptr) value = NewDom(JsonKind::Null);
        dom->elements.push_back(value);
        return nullptr;
    }
}

// =========================================================================
//  Library metadata and entry point
// =========================================================================

SHARDLIB_GETMETADATA
{
    lib.Name        = L"shard.json";
    lib.Description = L"JSON serializer/deserializer with typed model parsing";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> jsonNs(context, L"json");
    SymbolFactory factory(context.GetSemanticModel().Table.get());

    // --- JsonSerializer (static class with generic methods) ---

    SymbolBuilder<ClassSymbol> serializerClass = jsonNs.AddClass(L"JsonSerializer");

    {
        // Serialize<T>(value: T) -> string
        SymbolBuilder<MethodSymbol> serialize = serializerClass.AddMethod(L"Serialize", TYPE_STRING, LINK_STATIC);
        TypeParameterSymbol* serializeT = serialize.AddTypeParameter(L"T");
        serialize.AddParameter(L"value", serializeT).SetCallback(&cb_JsonSerialize);
    }

    {
        // Deserialize<T>(text: string) -> T
        //   Return type is T, which only exists after AddTypeParameter;
        //   OnSymbolDeclared does not snapshot the return type, so override it.
        SymbolBuilder<MethodSymbol> deserialize = serializerClass.AddMethod(L"Deserialize", TYPE_ANY, LINK_STATIC);
        TypeParameterSymbol* deserializeT = deserialize.AddTypeParameter(L"T");
        deserialize.Get()->ReturnType = deserializeT;
        deserialize.AddParameter(L"text", TYPE_STRING).SetCallback(&cb_JsonDeserialize);
    }

    // --- JsonNode (untyped DOM) ---

    SymbolBuilder<ClassSymbol> jsonNodeClass = jsonNs.AddClass(L"JsonNode");
    g_jsonNodeClass = jsonNodeClass.Get();
    g_jsonNodeHandleField = jsonNodeClass
        .AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE).Get();

    ClassSymbol* nodeType = jsonNodeClass.Get();
    ArrayTypeSymbol* stringArrayType = factory.Array(TYPE_STRING);

    jsonNodeClass.AddMethod(L"Parse", nodeType, LINK_STATIC)
        .AddParameter(L"text", TYPE_STRING)
        .SetCallback(&cb_JsonNode_Parse);

    jsonNodeClass.AddMethod(L"Stringify", TYPE_STRING, LINK_INSTANCE).SetCallback(&cb_JsonNode_Stringify);
    jsonNodeClass.AddMethod(L"Kind",      TYPE_STRING, LINK_INSTANCE).SetCallback(&cb_JsonNode_Kind);
    jsonNodeClass.AddMethod(L"IsNull",    TYPE_BOOL,   LINK_INSTANCE).SetCallback(&cb_JsonNode_IsNull);
    jsonNodeClass.AddMethod(L"IsObject",  TYPE_BOOL,   LINK_INSTANCE).SetCallback(&cb_JsonNode_IsObject);
    jsonNodeClass.AddMethod(L"IsArray",   TYPE_BOOL,   LINK_INSTANCE).SetCallback(&cb_JsonNode_IsArray);
    jsonNodeClass.AddMethod(L"AsInt",     TYPE_INT,    LINK_INSTANCE).SetCallback(&cb_JsonNode_AsInt);
    jsonNodeClass.AddMethod(L"AsDouble",  TYPE_DOUBLE, LINK_INSTANCE).SetCallback(&cb_JsonNode_AsDouble);
    jsonNodeClass.AddMethod(L"AsBool",    TYPE_BOOL,   LINK_INSTANCE).SetCallback(&cb_JsonNode_AsBool);
    jsonNodeClass.AddMethod(L"AsString",  TYPE_STRING, LINK_INSTANCE).SetCallback(&cb_JsonNode_AsString);

    jsonNodeClass.AddMethod(L"Get", nodeType, LINK_INSTANCE)
        .AddParameter(L"key", TYPE_STRING).SetCallback(&cb_JsonNode_Get);
    jsonNodeClass.AddMethod(L"Set", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"key", TYPE_STRING)
        .AddParameter(L"value", nodeType).SetCallback(&cb_JsonNode_Set);
    jsonNodeClass.AddMethod(L"Contains", TYPE_BOOL, LINK_INSTANCE)
        .AddParameter(L"key", TYPE_STRING).SetCallback(&cb_JsonNode_Contains);
    jsonNodeClass.AddMethod(L"Keys", stringArrayType, LINK_INSTANCE).SetCallback(&cb_JsonNode_Keys);

    jsonNodeClass.AddMethod(L"Length", TYPE_INT, LINK_INSTANCE).SetCallback(&cb_JsonNode_Length);
    jsonNodeClass.AddMethod(L"At", nodeType, LINK_INSTANCE)
        .AddParameter(L"index", TYPE_INT).SetCallback(&cb_JsonNode_At);
    jsonNodeClass.AddMethod(L"Add", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"value", nodeType).SetCallback(&cb_JsonNode_Add);
}
