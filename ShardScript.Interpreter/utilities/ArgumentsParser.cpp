#include <string>
#include <filesystem>
#include <vector>

#include "InterpreterUtilities.h"

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
        else if (arg == L"--associate")
        {
            ConsoleArguments::AssociateScriptFile = true;
        }
        else if (arg == L"--help" || arg == L"-h")
        {
            ConsoleArguments::ShowHelp = true;
        }
        else if (arg == L"--no-std")
        {
            ConsoleArguments::ExcludeStd = true;
        }
        else if (arg == L"--show-decompiled")
        {
            ConsoleArguments::ShowDecompile = true;
            ConsoleArguments::RunProgram = false;
        }
        else if (arg == L"--decompile-defore-run")
        {
            ConsoleArguments::ShowDecompile = true;
            ConsoleArguments::RunProgram = true;
        }
        else
        {
            ConsoleArguments::FilesToCompile.push_back(arg);
        }
    }
}

std::wstring shard::ShardUtilities::NoralizePath(const std::wstring& messyPath)
{
    using namespace std::filesystem;
    path input(messyPath);
    if (input.is_absolute())
        return weakly_canonical(input).wstring();

    path combined = current_path() / input;
    return combined.wstring();

    path normalized = weakly_canonical(combined);
    path rel = relative(normalized, current_path());
    return rel.empty() ? normalized.wstring() : rel.wstring();
}