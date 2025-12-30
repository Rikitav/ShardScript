#include <shard/ShardScriptVersion.h>
#include <Windows.h>

using namespace shard;

const int ShardScriptVersion::Major = 0;
const int ShardScriptVersion::Minor = 1;

/*
enum StringCompareResult
{
    Greater = CSTR_GREATER_THAN,
    Equals = CSTR_EQUAL,
    Less = CSTR_LESS_THAN,
};

static StringCompareResult compareVersionStrings(const wchar_t* v1, const wchar_t* v2)
{
    int result = CompareStringEx(
        LOCALE_NAME_INVARIANT, SORT_DIGITSASNUMBERS,
        v1, -1, v2, -1, NULL, NULL, 0);

    return static_cast<StringCompareResult>(result);
}

bool shard::IsCompatibleWith(const wchar_t* version)
{
    StringCompareResult result = compareVersionStrings(Version, version);
    return result == Greater || result == Equals;
}
*/

bool ShardScriptVersion::IsCompatibleWith(const int major, const int minor)
{
    if (major > Major)
        return false;

    if (minor > Minor)
        return false;

    return true;
}