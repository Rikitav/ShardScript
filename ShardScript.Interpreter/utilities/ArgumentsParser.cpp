#include <string>
#include <filesystem>
#include <vector>

#include <utilities/InterpreterUtilities.hpp>

void shard::ShardUtilities::ParseArguments(int argc, wchar_t* argv[])
{
    if (argc <= 1)
    {
        ConsoleArguments::RunProgram = false;
        ConsoleArguments::UseInteractive = true;
        return;
    }

    for (int i = 1; i < argc; i++)
    {
        std::wstring arg = argv[i];
        if (arg == L"--interactive" || arg == L"-i")
        {
            ConsoleArguments::RunProgram = false;
            ConsoleArguments::UseInteractive = true;
        }
        else if (arg == L"--decompiled" || arg == L"-d")
        {
            ConsoleArguments::RunProgram = false;
            ConsoleArguments::ShowDecompile = true;
        }
        else if (arg == L"--help" || arg == L"-h")
        {
            ConsoleArguments::ShowHelp = true;
        }
        else if (arg == L"--no-std")
        {
            ConsoleArguments::ExcludeStd = true;
        }
        else
        {
            ConsoleArguments::FilesToCompile.push_back(arg);
        }
    }
}
