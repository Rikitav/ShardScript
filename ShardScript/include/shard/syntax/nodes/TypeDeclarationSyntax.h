#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/BlockDeclarationSyntax.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>
#include <memory>
#include <vector>

using namespace std;
using namespace shard::parsing::structures;

namespace shard::syntax::nodes
{
	class TypeDeclarationSyntax : public BlockDeclarationSyntax
	{
	public:
		vector<shared_ptr<MemberDeclarationSyntax>> Members;

		TypeDeclarationSyntax(SyntaxKind kind)
			: BlockDeclarationSyntax(kind) {}
	};
}
