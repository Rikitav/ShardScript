#include  <shard/syntax/nodes/Types/ArrayTypeSyntax.h>

using namespace std;
using namespace shard::syntax::nodes;

wstring ArrayTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + L"[]";
}
