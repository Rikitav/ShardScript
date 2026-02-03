#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>

#include <string>

namespace shard
{
	class SHARD_API DelegateTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken DelegateToken;
		TypeSyntax* ReturnType = nullptr;
		ParametersListSyntax* Params = nullptr;

		inline DelegateTypeSyntax(SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::DelegateType, parent) { }

		inline DelegateTypeSyntax(const DelegateTypeSyntax& other) = delete;

		inline virtual ~DelegateTypeSyntax()
		{
			if (Params != nullptr)
				delete Params;
		}

		std::wstring ToString() override;
	};
}
