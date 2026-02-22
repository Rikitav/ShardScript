#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <string>
#include <vector>

namespace shard
{
	class NamespaceNode;
}

namespace shard
{
	class SHARD_API NamespaceSymbol : public SyntaxSymbol
	{
	public:
		std::vector<SyntaxSymbol*> Members;
		shard::NamespaceNode* Node = nullptr;

		inline NamespaceSymbol(std::wstring name)
			: SyntaxSymbol(name, SyntaxKind::NamespaceDeclaration) { }

		inline NamespaceSymbol(const NamespaceSymbol& other) = delete;

		void OnSymbolDeclared(SyntaxSymbol* symbol) override;

		inline virtual ~NamespaceSymbol()
		{
			for (SyntaxSymbol* member : Members)
				delete member;
		}
	};
}
