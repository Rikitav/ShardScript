#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API WhereClauseSyntax : public SyntaxNode
	{
	public:
		SyntaxToken WhereKeywordToken;
		SyntaxToken TypeParameterToken;
		SyntaxToken ColonToken;
		std::vector<std::unique_ptr<TypeSyntax>> ConstraintTypes;

		inline WhereClauseSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::WhereClause, parent) { }

		inline WhereClauseSyntax(const WhereClauseSyntax& other) = delete;

		inline virtual ~WhereClauseSyntax() = default;
	};
}
