#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <sstream>

using namespace std;
using namespace shard::syntax::nodes;

wstring IdentifierNameTypeSyntax::ToString()
{
	wostringstream result;
	if (!Identifiers.empty())
	{
		result << Identifiers[0].Word;
		for (size_t i = 1; i < Identifiers.size(); i++)
			result << L", " << Identifiers[i].Word;
	}

	return result.str();
}
