#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

using namespace shard;

std::wstring PredefinedTypeSyntax::ToString()
{
	return TypeToken.Word;
}
