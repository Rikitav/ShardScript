#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <string>

using namespace shard;

std::wstring IdentifierNameTypeSyntax::ToString()
{
	return Identifier.Word;
}
