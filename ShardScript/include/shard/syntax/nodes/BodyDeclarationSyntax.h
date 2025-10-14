#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>

using namespace std;

namespace shard::syntax::nodes
{
	class BodyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken Semicolon;

		BodyDeclarationSyntax(SyntaxKind kind)
			: MemberDeclarationSyntax(kind) {}

		bool IsLiner()
		{
			return OpenBraceToken.IsMissing && CloseBraceToken.IsMissing && Semicolon.IsMissing;
		}
	};
}
