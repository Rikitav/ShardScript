#include  <shard/syntax/nodes/Types/NullableTypeSyntax.h>

using namespace shard;

std::wstring NullableTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + QuestionToken.Word;
}
