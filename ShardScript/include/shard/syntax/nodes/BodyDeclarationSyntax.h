#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard
{
	class SHARD_API BodyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken SemicolonToken;
		
		inline BodyDeclarationSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: MemberDeclarationSyntax(kind, parent) { }

		inline BodyDeclarationSyntax(const BodyDeclarationSyntax& other) = delete;

		inline virtual ~BodyDeclarationSyntax()
		{

		}
	};
}
