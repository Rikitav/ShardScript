#include  <shard/syntax/nodes/Types/NullableTypeSyntax.hpp>

using namespace shard;

std::wstring NullableTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + QuestionToken.Word;
}
