#include  <shard/syntax/nodes/Types/ArrayTypeSyntax.h>

using namespace shard::syntax::nodes;

std::wstring ArrayTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + L"[]";
}
