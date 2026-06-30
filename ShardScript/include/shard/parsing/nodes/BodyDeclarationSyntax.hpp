#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

namespace shard
{
	class SHARD_API BodyDeclarationSyntax : public MemberDeclarationSyntax
	{
	public:
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;
		SyntaxToken SemicolonToken;
		
		inline BodyDeclarationSyntax(const SyntaxKind kind, SyntaxNode* parent)
			: MemberDeclarationSyntax(kind, parent) { }

		inline BodyDeclarationSyntax(const BodyDeclarationSyntax& other) = delete;

		inline virtual ~BodyDeclarationSyntax()
		{

		}
	};
}
