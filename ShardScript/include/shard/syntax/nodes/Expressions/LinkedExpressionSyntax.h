#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>

#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <vector>

namespace shard::syntax::nodes
{
	class LinkedExpressionNode : public ExpressionSyntax
	{
	public:
		SyntaxToken DelimeterToken;
		const ExpressionSyntax* PreviousExpression = nullptr;
		bool IsStaticContext = true;

		inline LinkedExpressionNode(const SyntaxKind kind, const ExpressionSyntax* previous, const SyntaxNode* parent)
			: ExpressionSyntax(kind, parent), PreviousExpression(previous) { }

		inline LinkedExpressionNode(const LinkedExpressionNode& other)
			: ExpressionSyntax(other), DelimeterToken(other.DelimeterToken), PreviousExpression(other.PreviousExpression) { }

		inline virtual ~LinkedExpressionNode()
		{
			if (PreviousExpression != nullptr)
				delete PreviousExpression;
		}
	};

	/*
	class LinkedExpressionSyntax : public ExpressionSyntax
	{
	public:
		std::vector<LinkedExpressionNode*> Nodes;
		LinkedExpressionNode* First = nullptr;
		LinkedExpressionNode* Last = nullptr;

		inline LinkedExpressionSyntax(const SyntaxNode* parent) : ExpressionSyntax(SyntaxKind::LinkedExpression, parent) { }
		inline LinkedExpressionSyntax(const LinkedExpressionSyntax& other) : ExpressionSyntax(SyntaxKind::LinkedExpression, other.Parent) { }

		inline virtual ~LinkedExpressionSyntax()
		{
			First = nullptr;
			Last = nullptr;

			for (const LinkedExpressionNode* node : Nodes)
			{
				node->~LinkedExpressionNode();
				delete node;
			}

			Nodes.~vector();
		}
	};
	*/
	
	class MemberAccessExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		shard::syntax::symbols::TypeSymbol* Type = nullptr;
		shard::syntax::symbols::FieldSymbol* FieldSymbol = nullptr;
		shard::syntax::symbols::PropertySymbol* PropertySymbol = nullptr;

		//bool IsType = false;
		//bool IsVariable = false;
		//bool IsProperty = false;
		
		inline MemberAccessExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, previous, parent), IdentifierToken(identifier) { }
		
		inline MemberAccessExpressionSyntax(const MemberAccessExpressionSyntax& other)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, other.PreviousExpression, other.Parent), IdentifierToken(other.IdentifierToken) {}

		inline virtual ~MemberAccessExpressionSyntax()
		{
			Type = nullptr;
			FieldSymbol = nullptr;
			PropertySymbol = nullptr;
		}
	};

	class InvokationExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::syntax::symbols::MethodSymbol* Symbol = nullptr;

		inline InvokationExpressionSyntax(SyntaxToken identifier, ExpressionSyntax* previous, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, previous, parent), IdentifierToken(identifier) { }

		inline InvokationExpressionSyntax(const InvokationExpressionSyntax& other)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, other.PreviousExpression, other.Parent), IdentifierToken(other.IdentifierToken), ArgumentsList(other.ArgumentsList) { }

		inline virtual ~InvokationExpressionSyntax()
		{
			ArgumentsList->~ArgumentsListSyntax();
			delete ArgumentsList;
		}
	};

	class IndexatorExpressionSyntax : public LinkedExpressionNode
	{
	public:
		MemberAccessExpressionSyntax* MemberAccess;
		IndexatorListSyntax* IndexatorList = nullptr;
		shard::syntax::symbols::MethodSymbol* Symbol = nullptr;

		inline IndexatorExpressionSyntax(MemberAccessExpressionSyntax* memberAccess, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::IndexatorExpression, memberAccess, parent), MemberAccess(memberAccess) { }

		inline IndexatorExpressionSyntax(const IndexatorExpressionSyntax& other)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, other.PreviousExpression, other.Parent), MemberAccess(other.MemberAccess), IndexatorList(other.IndexatorList) { }

		inline virtual ~IndexatorExpressionSyntax()
		{
			IndexatorList->~IndexatorListSyntax();
			delete IndexatorList;
		}
	};
}
