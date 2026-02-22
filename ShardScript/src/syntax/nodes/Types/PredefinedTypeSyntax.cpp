#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>

using namespace shard;

std::wstring PredefinedTypeSyntax::ToString()
{
	return TypeToken.Word;
}
