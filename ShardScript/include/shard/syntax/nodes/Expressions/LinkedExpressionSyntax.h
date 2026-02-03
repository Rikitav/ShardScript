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
#include <shard/syntax/symbols/IndexatorSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>

namespace shard
{
	class SHARD_API LinkedExpressionNode : public ExpressionSyntax
	{
	public:
		SyntaxToken DelimeterToken;
		ExpressionSyntax *const PreviousExpression = nullptr;
		bool IsStaticContext = true;

		inline LinkedExpressionNode(const SyntaxKind kind, ExpressionSyntax *const previous, SyntaxNode *const parent)
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
		shard::ParameterSymbol* ToParameter = nullptr;
		shard::VariableSymbol* ToVariable = nullptr;
		shard::FieldSymbol* ToField = nullptr;
		shard::PropertySymbol* ToProperty = nullptr;
		shard::DelegateTypeSymbol* ToDelegate = nullptr;

		const SyntaxToken IdentifierToken;

		inline MemberAccessExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode *const parent)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, previous, parent), IdentifierToken(identifier) { }

		inline MemberAccessExpressionSyntax(const MemberAccessExpressionSyntax&) = delete;

		inline virtual ~MemberAccessExpressionSyntax()
		{
			ToParameter = nullptr;
			ToVariable = nullptr;
			ToField = nullptr;
			ToProperty = nullptr;
			ToDelegate = nullptr;
		}
	};

	class SHARD_API InvokationExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::MethodSymbol* Symbol = nullptr;

		inline InvokationExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode *const parent)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, previous, parent), IdentifierToken(identifier) { }

		inline InvokationExpressionSyntax(const InvokationExpressionSyntax&) = delete;

		inline virtual ~InvokationExpressionSyntax()
		{
			delete ArgumentsList;
		}
	};

	class SHARD_API IndexatorExpressionSyntax : public MemberAccessExpressionSyntax
	{
	public:
		IndexatorListSyntax* IndexatorList = nullptr;
		shard::IndexatorSymbol* IndexatorSymbol = nullptr;

		inline IndexatorExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode *const parent)
			: MemberAccessExpressionSyntax(identifier, previous, parent)
		{
			// hehe
			SyntaxKind* pKind = const_cast<SyntaxKind*>(&Kind);
			*pKind = SyntaxKind::IndexatorExpression;
		}

		inline IndexatorExpressionSyntax(const IndexatorExpressionSyntax&) = delete;

		inline virtual ~IndexatorExpressionSyntax()
		{
			delete IndexatorList;
		}
	};
}
