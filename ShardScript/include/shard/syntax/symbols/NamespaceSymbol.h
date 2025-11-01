#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class NamespaceSymbol : public SyntaxSymbol
	{
	public:
		std::vector<SyntaxSymbol*> Members;
		NamespaceSymbol* Parent = nullptr;

		inline NamespaceSymbol(std::wstring name)
			: SyntaxSymbol(name, SyntaxKind::NamespaceDeclaration) { }
	};
}
