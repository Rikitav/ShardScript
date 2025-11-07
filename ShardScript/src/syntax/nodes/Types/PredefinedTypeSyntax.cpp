#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

using namespace std;
using namespace shard::syntax::nodes;

wstring PredefinedTypeSyntax::ToString()
{
	return TypeToken.Word;
}
