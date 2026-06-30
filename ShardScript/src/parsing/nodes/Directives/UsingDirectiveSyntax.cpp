#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <sstream>
#include <string>

using namespace shard;

std::wstring UsingDirectiveSyntax::ToString()
{
	std::wostringstream result;
	result << TokensList[0].Word;
	for (std::size_t i = 1; i < TokensList.size(); i++)
		result << L"." << TokensList[i].Word;

	return result.str();
}
