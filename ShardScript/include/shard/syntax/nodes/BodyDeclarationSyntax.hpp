#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>

namespace shard
{
	class SHARD_API BodyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken SemicolonToken;
		
		inline BodyDeclarationSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: MemberDeclarationSyntax(kind, parent) { }

		inline BodyDeclarationSyntax(const BodyDeclarationSyntax& other) = delete;

		inline virtual ~BodyDeclarationSyntax()
		{

		}
	};
}
