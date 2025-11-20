#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

using namespace shard::syntax::nodes;

std::wstring PredefinedTypeSyntax::ToString()
{
	return TypeToken.Word;
}
