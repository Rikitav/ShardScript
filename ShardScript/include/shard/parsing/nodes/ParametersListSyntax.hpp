#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API ParameterSyntax : public SyntaxNode
	{
	public:
		std::unique_ptr<TypeSyntax> Type;
		const SyntaxToken Identifier;
		shard::TypeSymbol* Symbol = nullptr;

		inline ParameterSyntax(std::unique_ptr<TypeSyntax> type, const SyntaxToken identifier, SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::Parameter, parent), Type(std::move(type)), Identifier(identifier) { }

		inline ParameterSyntax(const ParameterSyntax& other) = delete;
	
		inline virtual ~ParameterSyntax() = default;
	};

	class SHARD_API ParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<std::unique_ptr<ParameterSyntax>> Parameters;

		inline ParametersListSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {}

		inline ParametersListSyntax(const ParametersListSyntax& other) = delete;
	
		inline virtual ~ParametersListSyntax() = default;
	};
}
