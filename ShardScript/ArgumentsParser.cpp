#include <string>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

namespace shard::utilities
{
    struct ConsoleArguments
    {
        bool UseInterpreter = false;
        vector<string> FilesToCompile;
    };

	static ConsoleArguments ParseArguments(int argc, char** argv)
	{
        ConsoleArguments args;
        if (argc <= 1)
        {
            args.UseInterpreter = true;
            return args;
        }

        for (int i = 1; i < argc; i++)
        {
            string arg = argv[i];
            if (arg == "-interpret")
            {
                args.UseInterpreter = true;
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