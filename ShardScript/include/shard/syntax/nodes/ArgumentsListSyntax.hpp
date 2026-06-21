#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API ArgumentSyntax : public SyntaxNode
	{
	public:
		std::unique_ptr<ExpressionSyntax> Expression;
		const bool IsByReference;

		inline ArgumentSyntax(std::unique_ptr<ExpressionSyntax> expression, SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::Argument, parent), Expression(std::move(expression)), IsByReference(false) { }

		inline ArgumentSyntax(const ArgumentSyntax& other) = delete;

		inline virtual ~ArgumentSyntax() = default;
	};

	class SHARD_API ArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		std::vector<std::unique_ptr<ArgumentSyntax>> Arguments;

		inline ArgumentsListSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ArgumentsList, parent) { }

		inline ArgumentsListSyntax(const ArgumentsListSyntax& other) = delete;

		inline virtual ~ArgumentsListSyntax() = default;
	};

	class SHARD_API IndexatorListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		std::vector<std::unique_ptr<ArgumentSyntax>> Arguments;

		inline IndexatorListSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::IndexatorList, parent) { }

		inline IndexatorListSyntax(const IndexatorListSyntax& other) = delete;

		inline virtual ~IndexatorListSyntax() = default;
	};
}