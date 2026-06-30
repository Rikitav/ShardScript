#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/NamespaceTree.hpp>

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

		inline virtual ~NamespaceSymbol() = default;

		void OnSymbolDeclared(SyntaxSymbol* symbol) override;
	};
}
