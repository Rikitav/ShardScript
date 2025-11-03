#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>

#include <vector>

namespace shard::syntax::nodes
{
	class LinkedExpressionNode : public SyntaxNode
	{
	public:
		SyntaxToken PrevDelimeterToken;
		SyntaxToken NextDelimeterToken;

		const LinkedExpressionNode* PrevNode = nullptr;
		LinkedExpressionNode* NextNode = nullptr;

		inline LinkedExpressionNode(const SyntaxKind kind, const LinkedExpressionNode* previous, const SyntaxNode* parent) : SyntaxNode(kind, parent)
		{
			if (previous != nullptr)
			{
				PrevNode = previous;
				PrevDelimeterToken = previous->NextDelimeterToken;
			}
		}

		inline LinkedExpressionNode(const LinkedExpressionNode& other)
			: SyntaxNode(other), PrevDelimeterToken(other.PrevDelimeterToken), NextDelimeterToken(other.NextDelimeterToken), PrevNode(other.PrevNode), NextNode(other.NextNode) { }

		inline virtual ~LinkedExpressionNode()
		{
			PrevNode = nullptr;
			NextNode = nullptr;
		}
	};

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
	
	class MemberAccessExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		shard::syntax::symbols::FieldSymbol* Symbol = nullptr;
		
		inline MemberAccessExpressionSyntax(SyntaxToken identifier, LinkedExpressionNode* previous, LinkedExpressionSyntax* parent)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, previous, parent), IdentifierToken(identifier) { }
		
		inline MemberAccessExpressionSyntax(const MemberAccessExpressionSyntax& other)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, other.PrevNode, other.Parent), IdentifierToken(other.IdentifierToken) {}

		inline virtual ~MemberAccessExpressionSyntax()
		{

		}
	};

	class InvokationExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		ArgumentsListSyntax* ArgumentsList = nullptr;
		shard::syntax::symbols::MethodSymbol* Symbol = nullptr;

		inline InvokationExpressionSyntax(SyntaxToken identifier, LinkedExpressionNode* previous, LinkedExpressionSyntax* parent)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, previous, parent), IdentifierToken(identifier) { }

		inline InvokationExpressionSyntax(const InvokationExpressionSyntax& other)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, other.PrevNode, other.Parent), IdentifierToken(other.IdentifierToken), ArgumentsList(other.ArgumentsList) { }

		inline virtual ~InvokationExpressionSyntax()
		{
			ArgumentsList->~ArgumentsListSyntax();
			delete ArgumentsList;
		}
	};

	class IndexatorExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		IndexatorListSyntax* IndexatorList = nullptr;

		inline IndexatorExpressionSyntax(SyntaxToken identifier, LinkedExpressionNode* previous, LinkedExpressionSyntax* parent)
			: LinkedExpressionNode(SyntaxKind::IndexatorExpression, previous, parent), IdentifierToken(identifier) { }

		inline IndexatorExpressionSyntax(const IndexatorExpressionSyntax& other)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, other.PrevNode, other.Parent), IdentifierToken(other.IdentifierToken), IndexatorList(other.IndexatorList) { }

		inline virtual ~IndexatorExpressionSyntax()
		{
			IndexatorList->~IndexatorListSyntax();
			delete IndexatorList;
		}
	};
}
