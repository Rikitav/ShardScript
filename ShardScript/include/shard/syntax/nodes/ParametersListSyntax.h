#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>

#include <vector>
#include <memory>

using namespace std;

namespace shard::syntax::nodes
{
	class ParameterSyntax : public SyntaxNode
	{
	public:
		SyntaxToken Type;
		SyntaxToken Identifier;

		ParameterSyntax(SyntaxToken type, SyntaxToken identifier)
			: SyntaxNode(SyntaxKind::Parameter), Type(type), Identifier(identifier) {}
	};

	class ParametersListSyntax : public SyntaxNode
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		vector<shared_ptr<ParameterSyntax>> Parameters;

		ParametersListSyntax()
			: SyntaxNode(SyntaxKind::ParametersList) {}
	};
}