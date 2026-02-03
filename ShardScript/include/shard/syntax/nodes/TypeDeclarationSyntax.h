#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/BodyDeclarationSyntax.h>

#include <vector>

namespace shard
{
	class SHARD_API TypeDeclarationSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<MemberDeclarationSyntax*> Members;

		inline TypeDeclarationSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: BodyDeclarationSyntax(kind, parent) { }

		inline TypeDeclarationSyntax(const TypeDeclarationSyntax& other) = delete;

		inline virtual ~TypeDeclarationSyntax()
		{
			for (const MemberDeclarationSyntax* member : Members)
				delete member;
		}
	};
}
