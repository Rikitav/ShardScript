#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>

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
