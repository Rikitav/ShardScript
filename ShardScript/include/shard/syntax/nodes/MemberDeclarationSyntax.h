#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <vector>

namespace shard::syntax::nodes
{
	class MemberDeclarationSyntax : public SyntaxNode
	{
	public:
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken DeclareToken;
		SyntaxToken IdentifierToken;

		inline MemberDeclarationSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline MemberDeclarationSyntax(const MemberDeclarationSyntax& other)
			: SyntaxNode(other), Modifiers(other.Modifiers), DeclareToken(other.DeclareToken), IdentifierToken(other.IdentifierToken) { }

		inline virtual ~MemberDeclarationSyntax()
		{

		}
	};
}
