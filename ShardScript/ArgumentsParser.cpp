#include <string>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

namespace shard::utilities
{
	static string normalizePath(const string& messyPath)
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