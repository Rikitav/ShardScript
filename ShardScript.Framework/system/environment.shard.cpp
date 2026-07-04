#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include <sstream>
#include <codecvt>
#include <locale>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace shard;

#ifdef _WIN32
static void SetGlobalEnv_Windows(const std::wstring& name, const std::wstring& value, bool is_user_scope = true)
{
    HKEY hKey;
    LPCWSTR subkey = is_user_scope ? L"Environment" : L"System\\CurrentControlSet\\Control\\Session Manager\\Environment";
    HKEY baseKey = is_user_scope ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

    if (RegOpenKeyExW(baseKey, subkey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (value.empty())
        {
            RegDeleteValueW(hKey, name.c_str());
        }
        else
        {
            RegSetValueExW(hKey, name.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (value.size() + 1) * sizeof(wchar_t));
        }

        RegCloseKey(hKey);

        DWORD_PTR dwReturnValue;
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, &dwReturnValue);
    }
}

#else
static void SetGlobalEnv_Linux(const std::wstring& name, const std::wstring& value)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string u8_name = converter.to_bytes(name);
    std::string u8_value = converter.to_bytes(value);

    /* TODO
    const char* home = std::getenv("HOME");
    if (home)
    {
        std::string profile_path = std::string(home) + "/.profile";
        std::ofstream outfile;

        outfile.open(profile_path, std::ios_base::app);
        if (outfile.is_open())
        {
            outfile << "\nexport " << u8_name << "=\"" << u8_value << "\"\n";
        }
    }
    */
}

#endif

static std::wstring getEnvVar(const wchar_t* name)
{
#ifdef _WIN32
    const wchar_t* value = _wgetenv(name);
    return std::wstring(value ? value : L"");
#else
    std::string narrowName(name, name + std::wcslen(name));
    const char* value = std::getenv(narrowName.c_str());

    if (value == nullptr)
        value = "";

    return std::wstring(value, value + std::strlen(value));
#endif
}

static void setEnvVar(const wchar_t* name, const wchar_t* value)
{
#ifdef _WIN32
    _wputenv_s(name, value);
#else
    std::string narrowName(name, name + std::wcslen(name));
    std::string narrowValue(value, value + std::wcslen(value));
    setenv(narrowName.c_str(), narrowValue.c_str(), 1);
#endif
}

static void setGlobalEnvVar(const wchar_t* name, const wchar_t* value)
{
#ifdef _WIN32
    SetGlobalEnv_Windows(name, value, true);
#else
    SetGlobalEnv_Linux(name, value);
#endif
}

static ObjectInstance* shard_Environment_op_Delimeter(const CallState& context)
{
    const wchar_t* name = context.Args[0]->AsString();
    return context.Collector.FromValue(getEnvVar(name));
}

static ObjectInstance* shard_Environment_GetEnvVar(const CallState& context)
{
    const wchar_t* name = context.Args[0]->AsString();
    return context.Collector.FromValue(getEnvVar(name));
}

static ObjectInstance* shard_Environment_SetEnvVar(const CallState& context)
{
    const wchar_t* name = context.Args[0]->AsString();
    const wchar_t* value = context.Args[1]->AsString();
    setEnvVar(name, value);
    return nullptr;
}

SHARDLIB_GETMETADATA
{
    lib.Name        = L"environment";
    lib.Description = L"Access environment variables via static access operator";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    auto envNamespace = SymbolBuilder<NamespaceSymbol>(context, L"environment");
    auto envClass = envNamespace.AddClass(L"Environment", LINK_STATIC);

    envClass.AddOperator(shard::TokenType::Delimeter, SymbolTable::Primitives::String, LINK_STATIC)
        .AddParameter(L"name", SymbolTable::Primitives::String)
        .SetCallback(&shard_Environment_op_Delimeter);

    envClass.AddMethod(L"GetVariable", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"name", SymbolTable::Primitives::String)
        .SetCallback(&shard_Environment_GetEnvVar);

    envClass.AddMethod(L"SetVariable", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"name", SymbolTable::Primitives::String)
        .SetCallback(&shard_Environment_SetEnvVar);
}
