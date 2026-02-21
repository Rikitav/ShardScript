#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <string>

using namespace shard;

std::wstring IdentifierNameTypeSyntax::ToString()
{
	return Identifier.Word;
}
