#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>

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

		inline MemberDeclarationSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: SyntaxNode(kind, parent) { }

		inline MemberDeclarationSyntax(const MemberDeclarationSyntax& other) = delete;

		inline virtual ~MemberDeclarationSyntax()
		{

		}
	};
}
