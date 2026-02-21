#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <sstream>
#include <string>

using namespace shard;

std::wstring GenericTypeSyntax::ToString()
{
	std::wostringstream result;
	result << UnderlayingType->ToString() << Arguments->OpenToken.Word;

	if (!Arguments->Types.empty())
	{
		result << Arguments->Types[0]->ToString();
		for (size_t i = 1; i < Arguments->Types.size(); i++)
			result << L", " << Arguments->Types[i]->ToString();
	}

	result << Arguments->CloseToken.Word;
	return result.str();
}
