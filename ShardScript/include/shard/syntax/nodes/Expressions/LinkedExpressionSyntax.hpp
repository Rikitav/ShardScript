#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>

#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/OperatorSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>

#include <string>
#include <memory>

namespace shard
{
	class SHARD_API LinkedExpressionNode : public ExpressionSyntax
	{
	public:
		SyntaxToken DelimeterToken;
		std::unique_ptr<ExpressionSyntax> PreviousExpression = nullptr;
		bool IsStaticContext = true;

		inline LinkedExpressionNode(const SyntaxKind kind, std::unique_ptr<ExpressionSyntax>&& previous, SyntaxNode* parent)
			: ExpressionSyntax(kind, parent), PreviousExpression(std::move(previous)) { }

		inline LinkedExpressionNode(const LinkedExpressionNode&) = delete;

		inline virtual ~LinkedExpressionNode() = default;
	};

	class SHARD_API MemberAccessExpressionSyntax : public LinkedExpressionNode
	{
	public:
		ParameterSymbol* ToParameter = nullptr;
		VariableSymbol* ToVariable = nullptr;
		FieldSymbol* ToField = nullptr;
		PropertySymbol* ToProperty = nullptr;
		OperatorSymbol* ToOperator = nullptr;
		DelegateTypeSymbol* ToDelegate = nullptr;

		const SyntaxToken IdentifierToken;

		inline MemberAccessExpressionSyntax(SyntaxToken identifier, std::unique_ptr<ExpressionSyntax>&& previous, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, std::move(previous), parent), IdentifierToken(identifier) { }

		inline MemberAccessExpressionSyntax(SyntaxToken identifier, std::unique_ptr<ExpressionSyntax>&& previous, SyntaxNode* parent, const SyntaxKind kind)
			: LinkedExpressionNode(kind, std::move(previous), parent), IdentifierToken(identifier) { }

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

	class SHARD_API IndexatorExpressionSyntax : public MemberAccessExpressionSyntax
	{
	public:
		std::unique_ptr<IndexatorListSyntax> IndexatorList = nullptr;
		//shard::IndexatorSymbol* IndexatorSymbol = nullptr;

		inline IndexatorExpressionSyntax(SyntaxToken& indexerToken, std::unique_ptr<ExpressionSyntax>&& previous, SyntaxNode* parent)
			: MemberAccessExpressionSyntax(indexerToken, std::move(previous), parent, SyntaxKind::IndexatorExpression) { }

		inline IndexatorExpressionSyntax(const IndexatorExpressionSyntax&) = delete;

		inline virtual ~IndexatorExpressionSyntax() = default;
	};

	class SHARD_API InvokationExpressionSyntax : public LinkedExpressionNode
	{
	public:
		const SyntaxToken IdentifierToken;
		std::unique_ptr<ArgumentsListSyntax> ArgumentsList = nullptr;
		MethodSymbol* Symbol = nullptr;
		TypeSymbol* ReceiverType = nullptr;
		bool IsDelegateInvocation = false;
		bool IsExtensionMethodInvocation = false;

		inline InvokationExpressionSyntax(SyntaxToken identifier, std::unique_ptr<ExpressionSyntax>&& previous, SyntaxNode* parent)
			: LinkedExpressionNode(SyntaxKind::InvokationExpression, std::move(previous), parent), IdentifierToken(identifier) { }

		inline InvokationExpressionSyntax(const InvokationExpressionSyntax&) = delete;

		inline virtual ~InvokationExpressionSyntax() = default;
	};
}
