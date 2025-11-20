#pragma once
#include <vector>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard::syntax::nodes
{
	class ArgumentSyntax : public SyntaxNode
	{
	public:
		const ExpressionSyntax* Expression;
		const bool IsByReference;

		inline ArgumentSyntax(const ExpressionSyntax* expression, const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::Argument, parent), Expression(expression), IsByReference(false) { }

		inline ArgumentSyntax(const ArgumentSyntax& other)
			: SyntaxNode(other), Expression(other.Expression), IsByReference(false) { }

		inline virtual ~ArgumentSyntax()
		{
			Expression->~ExpressionSyntax();
			delete Expression;
		}
	};

	class ArgumentsListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		std::vector<ArgumentSyntax*> Arguments;

		inline ArgumentsListSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ArgumentsList, parent) { }

		inline ArgumentsListSyntax(const ArgumentsListSyntax& other)
			: SyntaxNode(other), OpenCurlToken(other.OpenCurlToken), CloseCurlToken(other.CloseCurlToken) { }

		inline virtual ~ArgumentsListSyntax()
		{
			for (const ArgumentSyntax* argument : Arguments)
			{
				argument->~ArgumentSyntax();
				delete argument;
			}
		}
	};

	class IndexatorListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		std::vector<ArgumentSyntax*> Arguments;

		inline IndexatorListSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::IndexatorList, parent) { }

		inline IndexatorListSyntax(const IndexatorListSyntax& other)
			: SyntaxNode(other), OpenSquareToken(other.OpenSquareToken), CloseSquareToken(other.CloseSquareToken) { }

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