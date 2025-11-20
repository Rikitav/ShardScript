#include  <shard/syntax/nodes/Types/NullableTypeSyntax.h>

using namespace shard::syntax::nodes;

std::wstring NullableTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + L"?";
}
