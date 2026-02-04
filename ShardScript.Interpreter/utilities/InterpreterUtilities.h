#pragma once
#include <string>
#include <vector>

namespace shard
{
    struct ConsoleArguments
    {
        static inline bool AssociateScriptFile = false;
        static inline bool UseInteractive = false;
        static inline bool ShowHelp = false;
        static inline bool ExcludeStd = false;
        static inline bool ShowDecompile = false;
        static inline bool RunProgram = false;
        static inline std::vector<std::wstring> FilesToCompile;
    };

    class ShardUtilities
    {
    public:
        static std::wstring NoralizePath(const std::wstring& messyPath);
        static void ParseArguments(int argc, wchar_t* argv[]);
        static std::wstring GetFileVersion();
        static void AssociateRegistry();
    };
}
