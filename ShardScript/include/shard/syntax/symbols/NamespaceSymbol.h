#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <string>
#include <vector>

namespace shard::parsing::semantic
{
	class NamespaceNode;
}

namespace shard::syntax::symbols
{
	class SHARD_API NamespaceSymbol : public SyntaxSymbol
	{
	public:
		std::vector<SyntaxSymbol*> Members;
		shard::parsing::semantic::NamespaceNode* Node = nullptr;

		inline NamespaceSymbol(std::wstring name)
			: SyntaxSymbol(name, SyntaxKind::NamespaceDeclaration) { }

		inline NamespaceSymbol(const NamespaceSymbol& other) = delete;

		inline virtual ~NamespaceSymbol()
		{
			for (SyntaxSymbol* member : Members)
				delete member;
		}
	};
}
