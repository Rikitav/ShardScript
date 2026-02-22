#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <vector>

namespace shard
{
	class SHARD_API ParameterSyntax : public SyntaxNode
	{
	public:
		const TypeSyntax* Type;
		const SyntaxToken Identifier;
		shard::TypeSymbol* Symbol = nullptr;

		inline ParameterSyntax(const TypeSyntax* type, const SyntaxToken identifier, SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::Parameter, parent), Type(type), Identifier(identifier) { }

		inline ParameterSyntax(const ParameterSyntax& other) = delete;
	
		inline virtual ~ParameterSyntax()
		{

		}
	};

	class SHARD_API ParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenToken;
		SyntaxToken CloseToken;
		std::vector<ParameterSyntax*> Parameters;

		inline ParametersListSyntax(SyntaxNode *const parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {}

		inline ParametersListSyntax(const ParametersListSyntax& other) = delete;
	
		inline virtual ~ParametersListSyntax()
		{
			for (const ParameterSyntax* parameter : Parameters)
				delete parameter;
		}
	};
}
