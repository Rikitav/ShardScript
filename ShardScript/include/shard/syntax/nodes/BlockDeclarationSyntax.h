#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <string>
#include <vector>

using namespace std;

namespace shard::syntax::nodes
{
	class BlockDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken Semicolon;
		vector<string> Names;

		BlockDeclarationSyntax(SyntaxKind kind)
			: MemberDeclarationSyntax(kind) {}

		bool IsLiner()
		{
			return OpenBraceToken.IsMissing && CloseBraceToken.IsMissing && Semicolon.IsMissing;
		}
	};
}
