#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/BodyDeclarationSyntax.h>
#include <shard/syntax/SyntaxNode.h>
#include <vector>

namespace shard::syntax::nodes
{
	class TypeDeclarationSyntax : public BodyDeclarationSyntax
	{
	public:
		std::vector<MemberDeclarationSyntax*> Members;

		inline TypeDeclarationSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: BodyDeclarationSyntax(kind, parent) { }

		inline TypeDeclarationSyntax(const TypeDeclarationSyntax& other)
			: BodyDeclarationSyntax(other), Members(other.Members) { }

		inline virtual ~TypeDeclarationSyntax()
		{
			for (const MemberDeclarationSyntax* member : Members)
			{
				member->~MemberDeclarationSyntax();
				delete member;
			}
		}
	};
}
