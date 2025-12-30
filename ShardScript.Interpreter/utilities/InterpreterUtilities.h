#pragma once
#include <string>
#include <vector>

namespace shard
{
    struct ConsoleArguments
    {
        bool AssociateScriptFile = false;
        bool UseInteractive = false;
        bool ShowHelp = false;
        bool ExcludeStd = false;
        std::vector<std::wstring> FilesToCompile;
    };

    class ShardUtilities
    {
    public:
        static std::wstring NoralizePath(const std::wstring& messyPath);
        static ConsoleArguments ParseArguments(int argc, wchar_t* argv[]);
        static std::wstring GetFileVersion();
        static void AssociateRegistry();
    };
}
