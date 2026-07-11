#include <shard/parsing/nodes/Types/QualifiedNameTypeSyntax.hpp>

namespace shard
{
	std::wstring QualifiedNameTypeSyntax::ToString()
	{
		if (Left == nullptr)
			return Identifier.Word;

		return Left->ToString() + L"." + Identifier.Word;
	}
}
