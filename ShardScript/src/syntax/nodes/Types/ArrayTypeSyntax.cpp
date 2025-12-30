#include  <shard/syntax/nodes/Types/ArrayTypeSyntax.h>

using namespace shard;

std::wstring ArrayTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + L"[]";
}
