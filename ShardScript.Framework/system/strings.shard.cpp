#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cwctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

using namespace shard;

namespace
{
    inline std::wstring GetArgString(const CallState& context, std::size_t index)
    {
        if (index >= context.Args.size() || context.Args[index] == nullptr || context.Args[index] == GarbageCollector::NullInstance)
        {
            return std::wstring();
        }

        return context.Args[index]->AsString();
    }

    inline wchar_t GetArgChar(const CallState& context, std::size_t index)
    {
        if (index >= context.Args.size() || context.Args[index] == nullptr || context.Args[index] == GarbageCollector::NullInstance)
        {
            return L'\0';
        }

        return context.Args[index]->AsCharacter();
    }

    inline std::int64_t GetArgInt(const CallState& context, std::size_t index)
    {
        if (index >= context.Args.size() || context.Args[index] == nullptr || context.Args[index] == GarbageCollector::NullInstance)
        {
            return 0;
        }

        return context.Args[index]->AsInteger();
    }

    inline double GetArgDouble(const CallState& context, std::size_t index)
    {
        if (index >= context.Args.size() || context.Args[index] == nullptr || context.Args[index] == GarbageCollector::NullInstance)
        {
            return 0.0;
        }

        return context.Args[index]->AsDouble();
    }

    inline ObjectInstance* GetArgArray(const CallState& context, std::size_t index)
    {
        if (index >= context.Args.size() || context.Args[index] == nullptr || context.Args[index] == GarbageCollector::NullInstance)
        {
            return nullptr;
        }

        return context.Args[index];
    }

    inline bool IsStringWhitespace(wchar_t character)
    {
        return character == L' '
            || character == L'\t'
            || character == L'\n'
            || character == L'\r'
            || character == L'\f'
            || character == L'\v';
    }

    inline std::wstring TrimString(const std::wstring& value)
    {
        std::size_t start = 0;

        while (start < value.size() && IsStringWhitespace(value[start]))
        {
            ++start;
        }

        if (start == value.size())
        {
            return std::wstring();
        }

        std::size_t end = value.size();

        while (end > start && IsStringWhitespace(value[end - 1]))
        {
            --end;
        }

        return value.substr(start, end - start);
    }

    inline std::wstring TrimStartString(const std::wstring& value)
    {
        std::size_t start = 0;

        while (start < value.size() && IsStringWhitespace(value[start]))
        {
            ++start;
        }

        return value.substr(start);
    }

    inline std::wstring TrimEndString(const std::wstring& value)
    {
        if (value.empty())
        {
            return value;
        }

        std::size_t end = value.size();

        while (end > 0 && IsStringWhitespace(value[end - 1]))
        {
            --end;
        }

        return value.substr(0, end);
    }

    inline std::wstring TrimCharsString(const std::wstring& value, const std::wstring& chars)
    {
        if (chars.empty())
        {
            return value;
        }

        std::size_t start = 0;

        while (start < value.size() && chars.find(value[start]) != std::wstring::npos)
        {
            ++start;
        }

        if (start == value.size())
        {
            return std::wstring();
        }

        std::size_t end = value.size();

        while (end > start && chars.find(value[end - 1]) != std::wstring::npos)
        {
            --end;
        }

        return value.substr(start, end - start);
    }

    inline std::wstring SubstringSafe(const std::wstring& value, std::size_t startIndex, std::size_t length)
    {
        if (startIndex > value.size())
        {
            throw std::out_of_range("Substring: startIndex is out of range");
        }

        if (length > value.size() - startIndex)
        {
            length = value.size() - startIndex;
        }

        return value.substr(startIndex, length);
    }

    inline std::vector<std::wstring> SplitString(const std::wstring& value, const std::wstring& separator)
    {
        if (separator.empty())
        {
            return { value };
        }

        std::vector<std::wstring> result;
        std::size_t start = 0;

        while (true)
        {
            std::size_t position = value.find(separator, start);

            if (position == std::wstring::npos)
            {
                result.push_back(value.substr(start));
                break;
            }

            result.push_back(value.substr(start, position - start));
            start = position + separator.size();
        }

        return result;
    }

    inline std::wstring FormatString(const std::wstring& format, const std::vector<std::wstring>& args)
    {
        std::wstring result;
        std::size_t index = 0;

        while (index < format.size())
        {
            wchar_t current = format[index];

            if (current == L'{' && index + 1 < format.size())
            {
                if (format[index + 1] == L'{')
                {
                    result += L'{';
                    index += 2;

                    continue;
                }

                std::size_t end = index + 1;

                while (end < format.size() && format[end] >= L'0' && format[end] <= L'9')
                {
                    ++end;
                }

                if (end < format.size() && format[end] == L'}')
                {
                    int argIndex = std::stoi(format.substr(index + 1, end - index - 1));

                    if (argIndex < 0 || static_cast<std::size_t>(argIndex) >= args.size())
                    {
                        throw std::runtime_error("Format: argument index out of range");
                    }

                    result += args[argIndex];
                    index = end + 1;

                    continue;
                }
            }
            else if (current == L'}' && index + 1 < format.size() && format[index + 1] == L'}')
            {
                result += L'}';
                index += 2;

                continue;
            }

            result += current;
            ++index;
        }

        return result;
    }

    inline double ParseDoubleClassic(const std::string& text)
    {
        char* endPointer = nullptr;
        double value = std::strtod(text.c_str(), &endPointer);

        if (endPointer == text.c_str())
        {
            throw std::invalid_argument("ParseDouble: invalid number format");
        }

        return value;
    }

    inline std::string NarrowString(const std::wstring& value)
    {
        std::string result;
        result.reserve(value.size());

        for (wchar_t character : value)
        {
            result.push_back(static_cast<char>(static_cast<unsigned char>(character & 0xFF)));
        }

        return result;
    }

    inline ObjectInstance* StringArrayFromVector(const CallState& context, const std::vector<std::wstring>& values)
    {
        ObjectInstance* array = context.Collector.AllocateArray(TYPE_STRING, values.size());

        for (std::size_t index = 0; index < values.size(); ++index)
        {
            array->SetElement(index, context.Collector.FromValue(values[index]), context.Frame);
        }

        return array;
    }

    // =====================================================================
    //  Null / empty checks
    // =====================================================================

    static ObjectInstance* strings_IsNullOrEmpty(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        return context.Collector.FromValue(value.empty());
    }

    static ObjectInstance* strings_IsNullOrWhiteSpace(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);

        for (wchar_t character : value)
        {
            if (!IsStringWhitespace(character))
            {
                return context.Collector.FromValue(false);
            }
        }

        return context.Collector.FromValue(true);
    }

    // =====================================================================
    //  Length
    // =====================================================================

    static ObjectInstance* strings_Length(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        return context.Collector.FromValue(static_cast<std::int64_t>(value.size()));
    }

    // =====================================================================
    //  Case conversion
    // =====================================================================

    static ObjectInstance* strings_ToUpper(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring result;
        result.reserve(value.size());

        for (wchar_t character : value)
        {
            result.push_back(static_cast<wchar_t>(std::towupper(static_cast<wint_t>(character))));
        }

        return context.Collector.FromValue(result);
    }

    static ObjectInstance* strings_ToLower(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring result;
        result.reserve(value.size());

        for (wchar_t character : value)
        {
            result.push_back(static_cast<wchar_t>(std::towlower(static_cast<wint_t>(character))));
        }

        return context.Collector.FromValue(result);
    }

    // =====================================================================
    //  Trimming
    // =====================================================================

    static ObjectInstance* strings_Trim(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        return context.Collector.FromValue(TrimString(value));
    }

    static ObjectInstance* strings_TrimStart(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        return context.Collector.FromValue(TrimStartString(value));
    }

    static ObjectInstance* strings_TrimEnd(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        return context.Collector.FromValue(TrimEndString(value));
    }

    static ObjectInstance* strings_TrimChars(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring chars = GetArgString(context, 1);

        return context.Collector.FromValue(TrimCharsString(value, chars));
    }

    // =====================================================================
    //  Padding
    // =====================================================================

    static ObjectInstance* strings_PadLeft(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t totalWidth = GetArgInt(context, 1);

        if (totalWidth <= 0 || static_cast<std::int64_t>(value.size()) >= totalWidth)
        {
            return context.Collector.FromValue(value);
        }

        std::wstring result(static_cast<std::size_t>(totalWidth) - value.size(), L' ');
        result += value;

        return context.Collector.FromValue(result);
    }

    static ObjectInstance* strings_PadLeftChar(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t totalWidth = GetArgInt(context, 1);
        wchar_t paddingChar = GetArgChar(context, 2);

        if (totalWidth <= 0 || static_cast<std::int64_t>(value.size()) >= totalWidth)
        {
            return context.Collector.FromValue(value);
        }

        std::wstring result(static_cast<std::size_t>(totalWidth) - value.size(), paddingChar);
        result += value;

        return context.Collector.FromValue(result);
    }

    static ObjectInstance* strings_PadRight(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t totalWidth = GetArgInt(context, 1);

        if (totalWidth <= 0 || static_cast<std::int64_t>(value.size()) >= totalWidth)
        {
            return context.Collector.FromValue(value);
        }

        std::wstring result = value;
        result.append(static_cast<std::size_t>(totalWidth) - value.size(), L' ');

        return context.Collector.FromValue(result);
    }

    static ObjectInstance* strings_PadRightChar(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t totalWidth = GetArgInt(context, 1);
        wchar_t paddingChar = GetArgChar(context, 2);

        if (totalWidth <= 0 || static_cast<std::int64_t>(value.size()) >= totalWidth)
        {
            return context.Collector.FromValue(value);
        }

        std::wstring result = value;
        result.append(static_cast<std::size_t>(totalWidth) - value.size(), paddingChar);

        return context.Collector.FromValue(result);
    }

    // =====================================================================
    //  Substring / manipulation
    // =====================================================================

    static ObjectInstance* strings_SubstringStart(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t startIndex = GetArgInt(context, 1);

        if (startIndex < 0)
        {
            throw std::out_of_range("Substring: startIndex cannot be negative");
        }

        return context.Collector.FromValue(SubstringSafe(value, static_cast<std::size_t>(startIndex), value.size()));
    }

    static ObjectInstance* strings_SubstringStartLength(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t startIndex = GetArgInt(context, 1);
        std::int64_t length = GetArgInt(context, 2);

        if (startIndex < 0)
        {
            throw std::out_of_range("Substring: startIndex cannot be negative");
        }

        if (length < 0)
        {
            throw std::out_of_range("Substring: length cannot be negative");
        }

        return context.Collector.FromValue(SubstringSafe(value, static_cast<std::size_t>(startIndex), static_cast<std::size_t>(length)));
    }

    static ObjectInstance* strings_RemoveStart(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t startIndex = GetArgInt(context, 1);

        if (startIndex < 0)
        {
            throw std::out_of_range("Remove: startIndex cannot be negative");
        }

        if (startIndex > static_cast<std::int64_t>(value.size()))
        {
            return context.Collector.FromValue(value);
        }

        return context.Collector.FromValue(value.substr(0, static_cast<std::size_t>(startIndex)));
    }

    static ObjectInstance* strings_RemoveStartCount(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t startIndex = GetArgInt(context, 1);
        std::int64_t count = GetArgInt(context, 2);

        if (startIndex < 0)
        {
            throw std::out_of_range("Remove: startIndex cannot be negative");
        }

        if (count < 0)
        {
            throw std::out_of_range("Remove: count cannot be negative");
        }

        if (startIndex > static_cast<std::int64_t>(value.size()) || count == 0)
        {
            return context.Collector.FromValue(value);
        }

        std::size_t start = static_cast<std::size_t>(startIndex);
        std::size_t removeCount = static_cast<std::size_t>(count);

        if (start + removeCount > value.size())
        {
            removeCount = value.size() - start;
        }

        std::wstring result = value.substr(0, start) + value.substr(start + removeCount);

        return context.Collector.FromValue(result);
    }

    static ObjectInstance* strings_Insert(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t startIndex = GetArgInt(context, 1);
        std::wstring insertValue = GetArgString(context, 2);

        if (startIndex < 0 || startIndex > static_cast<std::int64_t>(value.size()))
        {
            throw std::out_of_range("Insert: startIndex is out of range");
        }

        std::wstring result = value;
        result.insert(static_cast<std::size_t>(startIndex), insertValue);

        return context.Collector.FromValue(result);
    }

    // =====================================================================
    //  Searching
    // =====================================================================

    static ObjectInstance* strings_IndexOfString(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring substring = GetArgString(context, 1);

        std::size_t position = value.find(substring);

        if (position == std::wstring::npos)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(-1));
        }

        return context.Collector.FromValue(static_cast<std::int64_t>(position));
    }

    static ObjectInstance* strings_IndexOfChar(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        wchar_t character = GetArgChar(context, 1);

        std::size_t position = value.find(character);

        if (position == std::wstring::npos)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(-1));
        }

        return context.Collector.FromValue(static_cast<std::int64_t>(position));
    }

    static ObjectInstance* strings_LastIndexOfString(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring substring = GetArgString(context, 1);

        std::size_t position = value.rfind(substring);

        if (position == std::wstring::npos)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(-1));
        }

        return context.Collector.FromValue(static_cast<std::int64_t>(position));
    }

    static ObjectInstance* strings_LastIndexOfChar(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        wchar_t character = GetArgChar(context, 1);

        std::size_t position = value.rfind(character);

        if (position == std::wstring::npos)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(-1));
        }

        return context.Collector.FromValue(static_cast<std::int64_t>(position));
    }

    static ObjectInstance* strings_Contains(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring substring = GetArgString(context, 1);

        return context.Collector.FromValue(value.find(substring) != std::wstring::npos);
    }

    static ObjectInstance* strings_StartsWith(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring substring = GetArgString(context, 1);

        if (substring.size() > value.size())
        {
            return context.Collector.FromValue(false);
        }

        return context.Collector.FromValue(value.compare(0, substring.size(), substring) == 0);
    }

    static ObjectInstance* strings_EndsWith(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring substring = GetArgString(context, 1);

        if (substring.size() > value.size())
        {
            return context.Collector.FromValue(false);
        }

        return context.Collector.FromValue(value.compare(value.size() - substring.size(), substring.size(), substring) == 0);
    }

    // =====================================================================
    //  Replacement
    // =====================================================================

    static ObjectInstance* strings_Replace(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring oldValue = GetArgString(context, 1);
        std::wstring newValue = GetArgString(context, 2);

        if (oldValue.empty())
        {
            return context.Collector.FromValue(value);
        }

        std::wstring result;
        std::size_t start = 0;

        while (true)
        {
            std::size_t position = value.find(oldValue, start);

            if (position == std::wstring::npos)
            {
                result += value.substr(start);
                break;
            }

            result += value.substr(start, position - start);
            result += newValue;
            start = position + oldValue.size();
        }

        return context.Collector.FromValue(result);
    }

    // =====================================================================
    //  Split / Join / Concat
    // =====================================================================

    static ObjectInstance* strings_Split(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring separator = GetArgString(context, 1);

        return StringArrayFromVector(context, SplitString(value, separator));
    }

    static ObjectInstance* strings_Join(const CallState& context)
    {
        std::wstring separator = GetArgString(context, 0);
        ObjectInstance* values = GetArgArray(context, 1);

        if (values == nullptr)
        {
            return context.Collector.FromValue(std::wstring());
        }

        std::wstring result;

        for (std::size_t index = 0; index < values->GetArrayLength(); ++index)
        {
            if (index != 0)
            {
                result += separator;
            }

            result += values->GetElement(index, context.Frame)->AsString();
        }

        return context.Collector.FromValue(result);
    }

    static ObjectInstance* strings_Concat(const CallState& context)
    {
        ObjectInstance* values = GetArgArray(context, 0);

        if (values == nullptr)
        {
            return context.Collector.FromValue(std::wstring());
        }

        std::wstring result;

        for (std::size_t index = 0; index < values->GetArrayLength(); ++index)
        {
            result += values->GetElement(index, context.Frame)->AsString();
        }

        return context.Collector.FromValue(result);
    }

    // =====================================================================
    //  Formatting
    // =====================================================================

    static ObjectInstance* strings_Format(const CallState& context)
    {
        std::wstring format = GetArgString(context, 0);
        ObjectInstance* argsArray = GetArgArray(context, 1);

        if (argsArray == nullptr)
        {
            return context.Collector.FromValue(format);
        }

        std::vector<std::wstring> args;
        args.reserve(argsArray->GetArrayLength());

        for (std::size_t index = 0; index < argsArray->GetArrayLength(); ++index)
        {
            args.push_back(argsArray->GetElement(index, context.Frame)->AsString());
        }

        return context.Collector.FromValue(FormatString(format, args));
    }

    // =====================================================================
    //  Char array conversion
    // =====================================================================

    static ObjectInstance* strings_ToCharArray(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        ObjectInstance* array = context.Collector.AllocateArray(TYPE_CHAR, value.size());

        for (std::size_t index = 0; index < value.size(); ++index)
        {
            array->SetElement(index, context.Collector.FromValue(value[index]), context.Frame);
        }

        return array;
    }

    static ObjectInstance* strings_FromCharArray(const CallState& context)
    {
        ObjectInstance* charArray = GetArgArray(context, 0);

        if (charArray == nullptr)
        {
            return context.Collector.FromValue(std::wstring());
        }

        std::wstring result;
        result.reserve(charArray->GetArrayLength());

        for (std::size_t index = 0; index < charArray->GetArrayLength(); ++index)
        {
            result.push_back(charArray->GetElement(index, context.Frame)->AsCharacter());
        }

        return context.Collector.FromValue(result);
    }

    // =====================================================================
    //  Reverse / Repeat
    // =====================================================================

    static ObjectInstance* strings_Reverse(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring result = value;
        std::reverse(result.begin(), result.end());

        return context.Collector.FromValue(result);
    }

    static ObjectInstance* strings_Repeat(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t count = GetArgInt(context, 1);

        if (count <= 0)
        {
            return context.Collector.FromValue(std::wstring());
        }

        std::wstring result;
        result.reserve(value.size() * static_cast<std::size_t>(count));

        for (std::int64_t index = 0; index < count; ++index)
        {
            result += value;
        }

        return context.Collector.FromValue(result);
    }

    // =====================================================================
    //  Comparison
    // =====================================================================

    static ObjectInstance* strings_Equals(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring other = GetArgString(context, 1);

        return context.Collector.FromValue(value == other);
    }

    static ObjectInstance* strings_Compare(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::wstring other = GetArgString(context, 1);

        if (value < other)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(-1));
        }

        if (value > other)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(1));
        }

        return context.Collector.FromValue(static_cast<std::int64_t>(0));
    }

    static ObjectInstance* strings_CompareOrdinal(const CallState& context)
    {
        std::wstring first = GetArgString(context, 0);
        std::wstring second = GetArgString(context, 1);

        if (first < second)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(-1));
        }

        if (first > second)
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(1));
        }

        return context.Collector.FromValue(static_cast<std::int64_t>(0));
    }

    // =====================================================================
    //  Parsing
    // =====================================================================

    static ObjectInstance* strings_ParseInt(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);

        try
        {
            std::int64_t result = std::stoll(NarrowString(value));
            return context.Collector.FromValue(result);
        }
        catch (...)
        {
            throw std::runtime_error("ParseInt: invalid integer format");
        }
    }

    static ObjectInstance* strings_ParseDouble(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);

        try
        {
            double result = ParseDoubleClassic(NarrowString(value));
            return context.Collector.FromValue(result);
        }
        catch (...)
        {
            throw std::runtime_error("ParseDouble: invalid number format");
        }
    }

    static ObjectInstance* strings_TryParseInt(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        std::int64_t defaultValue = GetArgInt(context, 1);

        try
        {
            return context.Collector.FromValue(static_cast<std::int64_t>(std::stoll(NarrowString(value))));
        }
        catch (...)
        {
            return context.Collector.FromValue(defaultValue);
        }
    }

    static ObjectInstance* strings_TryParseDouble(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        double defaultValue = GetArgDouble(context, 1);

        try
        {
            return context.Collector.FromValue(ParseDoubleClassic(NarrowString(value)));
        }
        catch (...)
        {
            return context.Collector.FromValue(defaultValue);
        }
    }

    static ObjectInstance* strings_IsInt(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);

        try
        {
            [[maybe_unused]] std::int64_t parsed = std::stoll(NarrowString(value));
            return context.Collector.FromValue(true);
        }
        catch (...)
        {
            return context.Collector.FromValue(false);
        }
    }

    static ObjectInstance* strings_IsDouble(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);

        try
        {
            [[maybe_unused]] double parsed = ParseDoubleClassic(NarrowString(value));
            return context.Collector.FromValue(true);
        }
        catch (...)
        {
            return context.Collector.FromValue(false);
        }
    }

    // =====================================================================
    //  Character helpers
    // =====================================================================

    static ObjectInstance* chars_ToUpper(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(static_cast<wchar_t>(std::towupper(value)));
    }

    static ObjectInstance* chars_ToLower(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(static_cast<wchar_t>(std::towlower(value)));
    }

    static ObjectInstance* chars_IsUpper(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswupper(value) != 0);
    }

    static ObjectInstance* chars_IsLower(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswlower(value) != 0);
    }

    static ObjectInstance* chars_IsDigit(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswdigit(value) != 0);
    }

    static ObjectInstance* chars_IsLetter(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswalpha(value) != 0);
    }

    static ObjectInstance* chars_IsLetterOrDigit(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswalnum(value) != 0);
    }

    static ObjectInstance* chars_IsWhitespace(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswspace(value) != 0);
    }

    static ObjectInstance* chars_IsPunctuation(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswpunct(value) != 0);
    }

    static ObjectInstance* chars_IsControl(const CallState& context)
    {
        wchar_t value = GetArgChar(context, 0);
        return context.Collector.FromValue(std::iswcntrl(value) != 0);
    }

    static ObjectInstance* chars_Parse(const CallState& context)
    {
        std::wstring value = GetArgString(context, 0);
        if (value.size() != 1)
            return context.Collector.FromValue(L'\0');

        return context.Collector.FromValue(value[0]);
    }

    // =====================================================================
    //  Static helpers
    // =====================================================================

    static ObjectInstance* strings_Empty(const CallState& context)
    {
        return context.Collector.FromValue(std::wstring());
    }
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"strings";
    lib.Description = L"String utilities, formatting, parsing and extension methods";
    lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> stringsNamespace(context, L"strings");
    SymbolFactory factory(context.GetSemanticModel().Table.get());

    // =====================================================================
    //  string extension methods
    // =====================================================================

    stringsNamespace.AddMethod(L"IsNullOrEmpty", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsNullOrEmpty);

    stringsNamespace.AddMethod(L"IsNullOrWhiteSpace", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsNullOrWhiteSpace);

    stringsNamespace.AddMethod(L"Length", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_Length);

    stringsNamespace.AddMethod(L"ToUpper", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ToUpper);

    stringsNamespace.AddMethod(L"ToLower", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ToLower);

    stringsNamespace.AddMethod(L"Trim", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_Trim);

    stringsNamespace.AddMethod(L"TrimStart", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_TrimStart);

    stringsNamespace.AddMethod(L"TrimEnd", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_TrimEnd);

    stringsNamespace.AddMethod(L"TrimChars", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"chars", TYPE_STRING)
        .SetCallback(&strings_TrimChars);

    stringsNamespace.AddMethod(L"PadLeft", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .SetCallback(&strings_PadLeft);

    stringsNamespace.AddMethod(L"PadLeft", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .AddParameter(L"paddingChar", TYPE_CHAR)
        .SetCallback(&strings_PadLeftChar);

    stringsNamespace.AddMethod(L"PadRight", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .SetCallback(&strings_PadRight);

    stringsNamespace.AddMethod(L"PadRight", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .AddParameter(L"paddingChar", TYPE_CHAR)
        .SetCallback(&strings_PadRightChar);

    stringsNamespace.AddMethod(L"Substring", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .SetCallback(&strings_SubstringStart);

    stringsNamespace.AddMethod(L"Substring", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .AddParameter(L"length", TYPE_INT)
        .SetCallback(&strings_SubstringStartLength);

    stringsNamespace.AddMethod(L"Remove", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .SetCallback(&strings_RemoveStart);

    stringsNamespace.AddMethod(L"Remove", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .AddParameter(L"count", TYPE_INT)
        .SetCallback(&strings_RemoveStartCount);

    stringsNamespace.AddMethod(L"Insert", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .AddParameter(L"insertValue", TYPE_STRING)
        .SetCallback(&strings_Insert);

    stringsNamespace.AddMethod(L"IndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_IndexOfString);

    stringsNamespace.AddMethod(L"IndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"character", TYPE_CHAR)
        .SetCallback(&strings_IndexOfChar);

    stringsNamespace.AddMethod(L"LastIndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_LastIndexOfString);

    stringsNamespace.AddMethod(L"LastIndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"character", TYPE_CHAR)
        .SetCallback(&strings_LastIndexOfChar);

    stringsNamespace.AddMethod(L"Contains", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_Contains);

    stringsNamespace.AddMethod(L"StartsWith", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_StartsWith);

    stringsNamespace.AddMethod(L"EndsWith", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_EndsWith);

    stringsNamespace.AddMethod(L"Replace", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"oldValue", TYPE_STRING)
        .AddParameter(L"newValue", TYPE_STRING)
        .SetCallback(&strings_Replace);

    stringsNamespace.AddMethod(L"Split", factory.Array(TYPE_STRING), LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"separator", TYPE_STRING)
        .SetCallback(&strings_Split);

    stringsNamespace.AddMethod(L"Join", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"separator", TYPE_STRING)
        .AddParameter(L"values", factory.Array(TYPE_STRING))
        .SetCallback(&strings_Join);

    stringsNamespace.AddMethod(L"Concat", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"values", factory.Array(TYPE_STRING))
        .SetCallback(&strings_Concat);

    stringsNamespace.AddMethod(L"Format", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"format", TYPE_STRING)
        .AddParameter(L"args", factory.Array(TYPE_STRING))
        .SetCallback(&strings_Format);

    stringsNamespace.AddMethod(L"ToCharArray", factory.Array(TYPE_CHAR), LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ToCharArray);

    stringsNamespace.AddMethod(L"FromCharArray", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"chars", factory.Array(TYPE_CHAR))
        .SetCallback(&strings_FromCharArray);

    stringsNamespace.AddMethod(L"Reverse", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_Reverse);

    stringsNamespace.AddMethod(L"Repeat", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"count", TYPE_INT)
        .SetCallback(&strings_Repeat);

    stringsNamespace.AddMethod(L"Equals", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"other", TYPE_STRING)
        .SetCallback(&strings_Equals);

    stringsNamespace.AddMethod(L"Compare", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"other", TYPE_STRING)
        .SetCallback(&strings_Compare);

    stringsNamespace.AddMethod(L"CompareOrdinal", TYPE_INT, LINK_STATIC)
        .AddParameter(L"first", TYPE_STRING)
        .AddParameter(L"second", TYPE_STRING)
        .SetCallback(&strings_CompareOrdinal);

    stringsNamespace.AddMethod(L"ParseInt", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ParseInt);

    stringsNamespace.AddMethod(L"ParseDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ParseDouble);

    stringsNamespace.AddMethod(L"TryParseInt", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"defaultValue", TYPE_INT)
        .SetCallback(&strings_TryParseInt);

    stringsNamespace.AddMethod(L"TryParseDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"defaultValue", TYPE_DOUBLE)
        .SetCallback(&strings_TryParseDouble);

    stringsNamespace.AddMethod(L"IsInt", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsInt);

    stringsNamespace.AddMethod(L"IsDouble", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsDouble);

    // =====================================================================
    //  Static string methods
    // =====================================================================

    SymbolBuilder<ClassSymbol> stringClass = SymbolBuilder<ClassSymbol>(context.GetSemanticModel().Table.get(), static_cast<ClassSymbol*>(TYPE_STRING));

    stringClass.AddMethod(L"IsNullOrEmpty", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsNullOrEmpty);

    stringClass.AddMethod(L"IsNullOrWhiteSpace", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsNullOrWhiteSpace);

    stringClass.AddMethod(L"Length", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_Length);

    stringClass.AddMethod(L"ToUpper", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ToUpper);

    stringClass.AddMethod(L"ToLower", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ToLower);

    stringClass.AddMethod(L"Trim", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_Trim);

    stringClass.AddMethod(L"TrimStart", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_TrimStart);

    stringClass.AddMethod(L"TrimEnd", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_TrimEnd);

    stringClass.AddMethod(L"TrimChars", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"chars", TYPE_STRING)
        .SetCallback(&strings_TrimChars);

    stringClass.AddMethod(L"PadLeft", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .SetCallback(&strings_PadLeft);

    stringClass.AddMethod(L"PadLeft", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .AddParameter(L"paddingChar", TYPE_CHAR)
        .SetCallback(&strings_PadLeftChar);

    stringClass.AddMethod(L"PadRight", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .SetCallback(&strings_PadRight);

    stringClass.AddMethod(L"PadRight", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"totalWidth", TYPE_INT)
        .AddParameter(L"paddingChar", TYPE_CHAR)
        .SetCallback(&strings_PadRightChar);

    stringClass.AddMethod(L"Substring", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .SetCallback(&strings_SubstringStart);

    stringClass.AddMethod(L"Substring", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .AddParameter(L"length", TYPE_INT)
        .SetCallback(&strings_SubstringStartLength);

    stringClass.AddMethod(L"Remove", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .SetCallback(&strings_RemoveStart);

    stringClass.AddMethod(L"Remove", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .AddParameter(L"count", TYPE_INT)
        .SetCallback(&strings_RemoveStartCount);

    stringClass.AddMethod(L"Insert", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"startIndex", TYPE_INT)
        .AddParameter(L"insertValue", TYPE_STRING)
        .SetCallback(&strings_Insert);

    stringClass.AddMethod(L"IndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_IndexOfString);

    stringClass.AddMethod(L"IndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"character", TYPE_CHAR)
        .SetCallback(&strings_IndexOfChar);

    stringClass.AddMethod(L"LastIndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_LastIndexOfString);

    stringClass.AddMethod(L"LastIndexOf", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"character", TYPE_CHAR)
        .SetCallback(&strings_LastIndexOfChar);

    stringClass.AddMethod(L"Contains", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_Contains);

    stringClass.AddMethod(L"StartsWith", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_StartsWith);

    stringClass.AddMethod(L"EndsWith", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"substring", TYPE_STRING)
        .SetCallback(&strings_EndsWith);

    stringClass.AddMethod(L"Replace", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"oldValue", TYPE_STRING)
        .AddParameter(L"newValue", TYPE_STRING)
        .SetCallback(&strings_Replace);

    stringClass.AddMethod(L"Split", factory.Array(TYPE_STRING), LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"separator", TYPE_STRING)
        .SetCallback(&strings_Split);

    stringClass.AddMethod(L"Join", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"separator", TYPE_STRING)
        .AddParameter(L"values", factory.Array(TYPE_STRING))
        .SetCallback(&strings_Join);

    stringClass.AddMethod(L"Concat", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"values", factory.Array(TYPE_STRING))
        .SetCallback(&strings_Concat);

    stringClass.AddMethod(L"Format", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"format", TYPE_STRING)
        .AddParameter(L"args", factory.Array(TYPE_STRING))
        .SetCallback(&strings_Format);

    stringClass.AddMethod(L"ToCharArray", factory.Array(TYPE_CHAR), LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ToCharArray);

    stringClass.AddMethod(L"FromCharArray", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"chars", factory.Array(TYPE_CHAR))
        .SetCallback(&strings_FromCharArray);

    stringClass.AddMethod(L"Reverse", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_Reverse);

    stringClass.AddMethod(L"Repeat", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"count", TYPE_INT)
        .SetCallback(&strings_Repeat);

    stringClass.AddMethod(L"Equals", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"other", TYPE_STRING)
        .SetCallback(&strings_Equals);

    stringClass.AddMethod(L"Compare", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"other", TYPE_STRING)
        .SetCallback(&strings_Compare);

    stringClass.AddMethod(L"CompareOrdinal", TYPE_INT, LINK_STATIC)
        .AddParameter(L"first", TYPE_STRING)
        .AddParameter(L"second", TYPE_STRING)
        .SetCallback(&strings_CompareOrdinal);

    stringClass.AddMethod(L"ParseInt", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ParseInt);

    stringClass.AddMethod(L"ParseDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_ParseDouble);

    stringClass.AddMethod(L"TryParseInt", TYPE_INT, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"defaultValue", TYPE_INT)
        .SetCallback(&strings_TryParseInt);

    stringClass.AddMethod(L"TryParseDouble", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .AddParameter(L"defaultValue", TYPE_DOUBLE)
        .SetCallback(&strings_TryParseDouble);

    stringClass.AddMethod(L"IsInt", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsInt);

    stringClass.AddMethod(L"IsDouble", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&strings_IsDouble);

    stringClass.AddMethod(L"Empty", TYPE_STRING, LINK_STATIC)
        .SetCallback(&strings_Empty);

    // =====================================================================
    //  character extension methods
    // =====================================================================

    SymbolBuilder<StructSymbol> charClass(context, static_cast<StructSymbol*>(TYPE_CHAR));

    charClass.AddMethod(L"ToUpper", TYPE_CHAR, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_ToUpper);

    charClass.AddMethod(L"ToLower", TYPE_CHAR, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_ToLower);

    charClass.AddMethod(L"IsUpper", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsUpper);

    charClass.AddMethod(L"IsLower", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsLower);

    charClass.AddMethod(L"IsDigit", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsDigit);

    charClass.AddMethod(L"IsLetter", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsLetter);

    charClass.AddMethod(L"IsLetterOrDigit", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsLetterOrDigit);

    charClass.AddMethod(L"IsWhitespace", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsWhitespace);

    charClass.AddMethod(L"IsPunctuation", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsPunctuation);

    charClass.AddMethod(L"IsControl", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsControl);

    charClass.AddMethod(L"Parse", TYPE_CHAR, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&chars_Parse);

    // =====================================================================
    //  Static character helper class
    // =====================================================================

    SymbolBuilder<StructSymbol> charStaticClass = SymbolBuilder<StructSymbol>(context.GetSemanticModel().Table.get(), static_cast<StructSymbol*>(TYPE_CHAR));
    
    charStaticClass.AddMethod(L"ToUpper", TYPE_CHAR, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_ToUpper);

    charStaticClass.AddMethod(L"ToLower", TYPE_CHAR, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_ToLower);

    charStaticClass.AddMethod(L"IsUpper", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsUpper);

    charStaticClass.AddMethod(L"IsLower", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsLower);

    charStaticClass.AddMethod(L"IsDigit", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsDigit);

    charStaticClass.AddMethod(L"IsLetter", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsLetter);

    charStaticClass.AddMethod(L"IsLetterOrDigit", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsLetterOrDigit);

    charStaticClass.AddMethod(L"IsWhitespace", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsWhitespace);

    charStaticClass.AddMethod(L"IsPunctuation", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsPunctuation);

    charStaticClass.AddMethod(L"IsControl", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"value", TYPE_CHAR)
        .SetCallback(&chars_IsControl);

    charStaticClass.AddMethod(L"Parse", TYPE_CHAR, LINK_STATIC)
        .AddParameter(L"value", TYPE_STRING)
        .SetCallback(&chars_Parse);
}
