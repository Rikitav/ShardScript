#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <memory>

namespace shard
{
	class VariableSymbol;
	class MethodSymbol;

	class SHARD_API DeferStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken DeferToken;
		std::unique_ptr<StatementSyntax> Statement = nullptr;

		VariableSymbol* Variable = nullptr;
		MethodSymbol* DisposeMethod = nullptr;
		bool IsResourceDefer = false;

		inline DeferStatementSyntax(SyntaxToken deferToken, SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::DeferStatement, parent), DeferToken(deferToken) { }

		inline DeferStatementSyntax(const DeferStatementSyntax& other) = delete;

		inline virtual ~DeferStatementSyntax() = default;
	};
}
