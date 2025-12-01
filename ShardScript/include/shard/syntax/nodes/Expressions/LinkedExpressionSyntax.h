#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>

#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>

namespace shard::syntax::nodes
{
	class SHARD_API LinkedExpressionNode : public ExpressionSyntax
	{
	public:
		SyntaxToken DelimeterToken;
		const ExpressionSyntax* PreviousExpression = nullptr;
		bool IsStaticContext = true;

		inline LinkedExpressionNode(const SyntaxKind kind, const ExpressionSyntax* previous, const SyntaxNode* parent)
			: ExpressionSyntax(kind, parent), PreviousExpression(previous) { }

		inline LinkedExpressionNode(const LinkedExpressionNode&) = delete;

		inline virtual ~LinkedExpressionNode()
		{
			if (PreviousExpression != nullptr)
				delete PreviousExpression;
		}
	};
	
	class SHARD_API MemberAccessExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		shard::syntax::symbols::TypeSymbol* Type = nullptr;
		shard::syntax::symbols::FieldSymbol* FieldSymbol = nullptr;
		shard::syntax::symbols::PropertySymbol* PropertySymbol = nullptr;
		shard::syntax::symbols::DelegateTypeSymbol* DelegateSymbol = nullptr;

		inline MemberAccessExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, previous, parent), IdentifierToken(identifier) { }

		MemberAccessExpressionSyntax(const MemberAccessExpressionSyntax&) = delete;

		inline virtual ~MemberAccessExpressionSyntax()
		{
			Type = nullptr;
			FieldSymbol = nullptr;
			PropertySymbol = nullptr;
		}
	};

	class SHARD_API InvokationExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::syntax::symbols::MethodSymbol* Symbol = nullptr;

		inline InvokationExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, previous, parent), IdentifierToken(identifier) { }

		InvokationExpressionSyntax(const InvokationExpressionSyntax&) = delete;

		inline virtual ~InvokationExpressionSyntax()
		{
			delete ArgumentsList;
		}
	};

	class SHARD_API IndexatorExpressionSyntax : public LinkedExpressionNode
	{
	public:
		MemberAccessExpressionSyntax* MemberAccess;
		IndexatorListSyntax* IndexatorList = nullptr;
		shard::syntax::symbols::MethodSymbol* Symbol = nullptr;

		inline IndexatorExpressionSyntax(MemberAccessExpressionSyntax* memberAccess, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::IndexatorExpression, memberAccess, parent), MemberAccess(memberAccess) { }

		IndexatorExpressionSyntax(const IndexatorExpressionSyntax&) = delete;

		inline virtual ~IndexatorExpressionSyntax()
		{
			delete IndexatorList;
		}
	};
}
