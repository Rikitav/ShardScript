#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <vector>
#include <memory>

using namespace std;
using namespace shard::syntax;
using namespace shard::syntax::nodes;

namespace shard::parsing::structures
{
	struct MemberDeclarationInfo
	{
		vector<SyntaxToken> Modifiers;
		SyntaxToken ReturnType;
		SyntaxToken DeclareType;
		SyntaxToken Identifier;
		shared_ptr<ParametersListSyntax> Params;
	};
}
