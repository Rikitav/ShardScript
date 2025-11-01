#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>

namespace shard::syntax::nodes
{
	class BodyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken SemicolonToken;
		
		inline BodyDeclarationSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: MemberDeclarationSyntax(kind, parent) { }

		inline BodyDeclarationSyntax(const BodyDeclarationSyntax& other)
			: MemberDeclarationSyntax(other), OpenBraceToken(other.OpenBraceToken), CloseBraceToken(other.CloseBraceToken), SemicolonToken(other.SemicolonToken) { }

		inline virtual ~BodyDeclarationSyntax()
		{

		}
	};
}
