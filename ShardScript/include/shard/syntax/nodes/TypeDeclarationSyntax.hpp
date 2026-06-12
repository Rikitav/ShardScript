#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/BodyDeclarationSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API TypeDeclarationSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<std::unique_ptr<MemberDeclarationSyntax>> Members;

		inline TypeDeclarationSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: BodyDeclarationSyntax(kind, parent) { }

		inline TypeDeclarationSyntax(const TypeDeclarationSyntax& other) = delete;

		inline virtual ~TypeDeclarationSyntax() = default;
	};
}
