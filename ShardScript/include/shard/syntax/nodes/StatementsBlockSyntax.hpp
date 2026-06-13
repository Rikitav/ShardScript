#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/BodyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API StatementsBlockSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<std::unique_ptr<StatementSyntax>> Statements;

		inline StatementsBlockSyntax(SyntaxNode *const parent)
			: BodyDeclarationSyntax(SyntaxKind::StatementsBlock, parent) { }

		inline StatementsBlockSyntax(const StatementsBlockSyntax& other) = delete;

		inline virtual ~StatementsBlockSyntax() = default;
	};
}
