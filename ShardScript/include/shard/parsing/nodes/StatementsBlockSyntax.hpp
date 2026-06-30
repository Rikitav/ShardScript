#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/BodyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API StatementsBlockSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<std::unique_ptr<StatementSyntax>> Statements;

		inline StatementsBlockSyntax(SyntaxNode* parent)
			: BodyDeclarationSyntax(SyntaxKind::StatementsBlock, parent) { }

		inline StatementsBlockSyntax(const StatementsBlockSyntax& other) = delete;

		inline virtual ~StatementsBlockSyntax() = default;
	};
}
