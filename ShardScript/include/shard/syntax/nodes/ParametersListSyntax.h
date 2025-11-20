#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <vector>

namespace shard::syntax::nodes
{
	class ParameterSyntax : public SyntaxNode
	{
	public:
		const TypeSyntax* Type;
		const SyntaxToken Identifier;

		inline ParameterSyntax(const TypeSyntax* type, const SyntaxToken identifier, const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::Parameter, parent), Type(type), Identifier(identifier) { }

		inline ParameterSyntax(const ParameterSyntax& other)
			: SyntaxNode(other), Type(other.Type), Identifier(other.Identifier) { }
	
		inline virtual ~ParameterSyntax()
		{

		}
	};

	class ParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		std::vector<ParameterSyntax*> Parameters;

		inline ParametersListSyntax(const SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::ParametersList, parent) {}

		inline ParametersListSyntax(const ParametersListSyntax& other)
			: SyntaxNode(other), OpenCurlToken(other.OpenCurlToken), CloseCurlToken(other.CloseCurlToken), Parameters(other.Parameters) { }
	
		inline virtual ~ParametersListSyntax()
		{
			for (const ParameterSyntax* parameter : Parameters)
			{
				parameter->~ParameterSyntax();
				delete parameter;
			}
		}
	};
}