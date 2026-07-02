#include <string>
#include <filesystem>
#include <vector>
#include <iostream>

#include <utilities/InterpreterUtilities.hpp>

namespace fs = std::filesystem;

static std::vector<std::wstring> ExpandGlob(const std::wstring& pattern)
{
    std::vector<std::wstring> matches;

    if (pattern.find(L'*') == std::wstring::npos)
    {
        if (fs::exists(pattern))
            matches.push_back(pattern);
    
        return matches;
    }

    try
    {
        fs::path p(pattern);
        fs::path dir = p.parent_path();
        std::wstring filename_pattern = p.filename().wstring();

        if (dir.empty())
            dir = ".";

        if (!fs::exists(dir) || !fs::is_directory(dir))
            return matches;

        std::wstring ext_target = L"";
        if (filename_pattern.rfind(L"*.") == 0)
            ext_target = filename_pattern.substr(1);

        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {
                std::wstring ext = entry.path().extension().wstring();

                if (!ext_target.empty() && ext == ext_target)
                {
                    matches.push_back(entry.path().wstring());
                }
                else if (filename_pattern == L"*")
                {
                    matches.push_back(entry.path().wstring());
                }
            }
        }
    }
    catch (...)
    {
 
    }

    return matches;
}

void shard::ShardUtilities::ParseArguments(int argc, wchar_t* argv[])
{
    ConsoleArguments::RunProgram = true;

    for (int i = 1; i < argc; ++i)
    {
        std::wstring arg = argv[i];

        if (arg == L"-h" || arg == L"--help")
        {
            ConsoleArguments::ShowHelp = true;
            ConsoleArguments::RunProgram = false;
        }
        else if (arg == L"-r" || arg == L"--repl")
        {
            ConsoleArguments::UseInteractive = true;
        }
        else if (arg == L"--exclude-std")
        {
            ConsoleArguments::ExcludeStd = true;
        }
        else if (arg == L"--decompile")
        {
            ConsoleArguments::ShowDecompile = true;
            ConsoleArguments::RunProgram = false;
        }
        else if (arg == L"-l" || arg == L"--library")
        {
            if (i + 1 < argc)
            {
                std::wstring libPattern = argv[++i];

                auto expandedLibs = ExpandGlob(libPattern);
                for (const auto& lib : expandedLibs)
                {
                    ConsoleArguments::LibsToLoad.push_back(lib);
                }
            }
            else
            {
                std::wcout << L"Error: Missing value for --library option.\n";
                ConsoleArguments::ShowHelp = true;
                return;
            }
        }
        else if (arg.rfind(L"-", 0) == 0)
        {
            std::wcout << L"Error: Unknown option '" << arg << L"'.\n";
            ConsoleArguments::ShowHelp = true;
            return;
        }
        else
        {
            auto expandedSources = ExpandGlob(arg);
            if (expandedSources.empty())
            {
                if (arg.find(L'*') == std::wstring::npos)
                {
                    std::wcout << L"Error: Source file not found '" << arg << L"'.\n";
                    return;
                }
            }

            for (const auto& srcFile : expandedSources)
            {
                ConsoleArguments::FilesToCompile.push_back(srcFile);
            }
        }
    }
}
