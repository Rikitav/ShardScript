#include <string>
#include <filesystem>
#include <vector>

using namespace std;
using namespace std::filesystem;

namespace shard::utilities
{
    struct ConsoleArguments
    {
        bool UseInteractive = false;
        bool ShowHelp = false;
        bool ExcludeStd = false;
        vector<wstring> FilesToCompile;
    };

	static ConsoleArguments ParseArguments(int argc, wchar_t* argv[])
	{
        ConsoleArguments args;
        if (argc <= 1)
        {
            args.UseInteractive = true;
            return args;
        }

        for (int i = 1; i < argc; i++)
        {
            wstring arg = argv[i];
            if (arg == L"--interactive" || arg == L"-i")
            {
                args.UseInteractive = true;
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

    static string noralizePath(const string& messyPath)
    {
        filesystem::path input(messyPath);
        if (input.is_absolute())
            return weakly_canonical(input).string();

        filesystem::path combined = current_path() / input;
        return combined.string();

        filesystem::path normalized = weakly_canonical(combined);

        filesystem::path rel = relative(normalized, current_path());
        return rel.empty() ? normalized.string() : rel.string();
    }
}