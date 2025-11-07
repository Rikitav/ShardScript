#include  <shard/syntax/nodes/Types/NullableTypeSyntax.h>

using namespace std;
using namespace shard::syntax::nodes;

wstring NullableTypeSyntax::ToString()
{
	return UnderlayingType->ToString() + L"?";
}
