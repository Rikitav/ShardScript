#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>

#include <string>

namespace shard::syntax::nodes
{
	class DelegateTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken DelegateToken;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline DelegateTypeSyntax(const SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::DelegateType, parent) { }

		std::wstring ToString() override;
	};
}
