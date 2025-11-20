#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <sstream>
#include <string>

using namespace shard::syntax::nodes;

std::wstring IdentifierNameTypeSyntax::ToString()
{
	std::wostringstream result;
	if (!Identifiers.empty())
	{
		result << Identifiers[0].Word;
		for (size_t i = 1; i < Identifiers.size(); i++)
			result << L", " << Identifiers[i].Word;
	}

	return result.str();
}
