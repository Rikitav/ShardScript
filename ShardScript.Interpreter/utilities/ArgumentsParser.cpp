#include <string>
#include <filesystem>
#include <vector>

#include "InterpreterUtilities.h"

shard::interpreter::utilities::ConsoleArguments shard::interpreter::utilities::ShardUtilities::ParseArguments(int argc, wchar_t* argv[])
{
    shard::interpreter::utilities::ConsoleArguments args;
    if (argc <= 1)
    {
        args.UseInteractive = true;
        return args;
    }

    for (int i = 1; i < argc; i++)
    {
        std::wstring arg = argv[i];
        if (arg == L"--interactive" || arg == L"-i")
        {
            args.UseInteractive = true;
        }
        else if (arg == L"--associate")
        {
            args.AssociateScriptFile = true;
        }
        else if (arg == L"--help" || arg == L"-h")
        {
            args.ShowHelp = true;
        }
        else if (arg == L"--no-std")
        {
            args.ExcludeStd = true;
        }
        else
        {
            args.FilesToCompile.push_back(arg);
        }
    }

    return args;
}

std::wstring shard::interpreter::utilities::ShardUtilities::NoralizePath(const std::wstring& messyPath)
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