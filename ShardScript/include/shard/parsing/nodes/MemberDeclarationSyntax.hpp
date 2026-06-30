#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/nodes/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/AttributeSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API MemberDeclarationSyntax : public SyntaxNode
	{
	public:
		std::vector<SyntaxToken> Modifiers;
		SyntaxToken DeclareToken;
		SyntaxToken IdentifierToken;

		std::vector<std::unique_ptr<AttributeSyntax>> Attributes;
		std::unique_ptr<TypeParametersListSyntax> TypeParameters = nullptr;

		inline MemberDeclarationSyntax(const SyntaxKind kind, SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline MemberDeclarationSyntax(const MemberDeclarationSyntax& other) = delete;

		inline virtual ~MemberDeclarationSyntax() = default;
	};
}
