#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/ShardScriptAPI.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/parsing/nodes/BodyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <memory>
#include <vector>

namespace shard
{
	class SHARD_API EnumDeclarationSyntax : public BodyDeclarationSyntax
	{
	public:
		SyntaxToken ColonToken;
		SyntaxToken UnderlyingTypeToken;
		bool IsFlags = false;

		std::vector<std::unique_ptr<EnumFieldDeclarationSyntax>> Fields;

		inline EnumDeclarationSyntax(SyntaxNode* parent)
			: BodyDeclarationSyntax(SyntaxKind::EnumDeclaration, parent) { }

		inline EnumDeclarationSyntax(shard::MemberDeclarationInfo& info, SyntaxNode* parent)
			: BodyDeclarationSyntax(SyntaxKind::EnumDeclaration, parent)
		{
			Attributes = std::move(info.Attributes);
			Modifiers = info.Modifiers;
			IdentifierToken = info.Identifier;
		}

		inline EnumDeclarationSyntax(const EnumDeclarationSyntax& other) = delete;

		inline virtual ~EnumDeclarationSyntax() = default;
	};
}
