#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/TypeParametersListSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API MemberDeclarationSyntax : public SyntaxNode
	{
	public:
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken DeclareToken;
		SyntaxToken IdentifierToken;
		TypeParametersListSyntax* TypeParameters = nullptr;

		inline MemberDeclarationSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline MemberDeclarationSyntax(const MemberDeclarationSyntax& other) = delete;

		inline virtual ~MemberDeclarationSyntax()
		{

		}
	};
}
