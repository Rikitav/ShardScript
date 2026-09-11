#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/AttributeSyntax.hpp>

// TODO:
/*
#include <shard/parsing/nodes/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/WhereClauseSyntax.hpp>
*/

#include <vector>

namespace shard
{
	class SHARD_API MemberDeclarationSyntax : public SyntaxNode
	{
		std::vector<AttributeSyntax> m_attributes;
		std::vector<SyntaxToken> m_modifiers;
		SyntaxToken m_dentifierToken;

		// TODO:
		/*
		TypeParametersListSyntax TypeParameters = nullptr;
		std::vector<WhereClauseSyntax> WhereClauses;
		*/

	public:
		MemberDeclarationSyntax(const SyntaxKind kind, SyntaxNode* parent);
		virtual ~MemberDeclarationSyntax() = default;
	};
}
