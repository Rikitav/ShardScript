#include  <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>

using namespace shard;

std::wstring ArrayTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + L"[]";
}
