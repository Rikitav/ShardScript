#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <string>

using namespace shard::syntax::nodes;

std::wstring IdentifierNameTypeSyntax::ToString()
{
	return Identifier.Word;
}
