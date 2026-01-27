#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <vector>

namespace shard
{
	class SHARD_API ParameterSyntax : public SyntaxNode
	{
	public:
		const TypeSyntax* Type;
		const SyntaxToken Identifier;
		shard::TypeSymbol* Symbol = nullptr;

		inline ParameterSyntax(const TypeSyntax* type, const SyntaxToken identifier, const SyntaxNode* parent)
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

		inline ParametersListSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {}

		inline ParametersListSyntax(const ParametersListSyntax& other) = delete;
	
		inline virtual ~ParametersListSyntax()
		{
			for (const ParameterSyntax* parameter : Parameters)
				delete parameter;
		}
	};
}
