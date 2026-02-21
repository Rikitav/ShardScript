#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <vector>

namespace shard
{
	class SHARD_API ArgumentSyntax : public SyntaxNode
	{
	public:
		const ExpressionSyntax* Expression;
		const bool IsByReference;

		inline ArgumentSyntax(const ExpressionSyntax* expression, SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::Argument, parent), Expression(expression), IsByReference(false) { }

		inline ArgumentSyntax(const ArgumentSyntax& other) = delete;

		inline virtual ~ArgumentSyntax()
		{
			Expression->~ExpressionSyntax();
			delete Expression;
		}
	};

	class SHARD_API ArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		std::vector<ArgumentSyntax*> Arguments;

		inline ArgumentsListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ArgumentsList, parent) { }

		inline ArgumentsListSyntax(const ArgumentsListSyntax& other) = delete;

		inline virtual ~ArgumentsListSyntax()
		{
			for (const ArgumentSyntax* argument : Arguments)
			{
				argument->~ArgumentSyntax();
				delete argument;
			}
		}
	};

	class SHARD_API IndexatorListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		std::vector<ArgumentSyntax*> Arguments;

		inline IndexatorListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::IndexatorList, parent) { }

		inline IndexatorListSyntax(const IndexatorListSyntax& other) = delete;

		inline virtual ~IndexatorListSyntax()
		{
			for (const ArgumentSyntax* argument : Arguments)
			{
				argument->~ArgumentSyntax();
				delete argument;
			}
		}
	};
}