#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>
#include <sstream>

using namespace std;
using namespace shard::syntax::nodes;

wstring GenericTypeSyntax::ToString()
{
	wostringstream result;
	result << UnderlayingType->ToString() << L"<";

	if (!TypeArguments.empty())
	{
		result << TypeArguments[0]->ToString();
		for (size_t i = 1; i < TypeArguments.size(); i++)
			result << L", " << TypeArguments[i]->ToString();
	}

	return result.str();
}
