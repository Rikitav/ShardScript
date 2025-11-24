#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>
#include <sstream>
#include <string>

using namespace shard::syntax::nodes;

std::wstring GenericTypeSyntax::ToString()
{
	std::wostringstream result;
	result << UnderlayingType->ToString() << OpenListToken.Word;

	if (!TypeArguments.empty())
	{
		result << TypeArguments[0]->ToString();
		for (size_t i = 1; i < TypeArguments.size(); i++)
			result << L", " << TypeArguments[i]->ToString();
	}

	result << CloseListToken.Word;
	return result.str();
}
