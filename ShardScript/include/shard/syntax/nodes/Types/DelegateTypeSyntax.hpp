#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>

#include <string>
#include <memory>

namespace shard
{
	class SHARD_API DelegateTypeSyntax : public TypeSyntax
	{
	public:
		SyntaxToken DelegateToken;
		std::unique_ptr<TypeSyntax> ReturnType = nullptr;
		std::unique_ptr<ParametersListSyntax> Params = nullptr;

		inline DelegateTypeSyntax(SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::DelegateType, parent) { }

		inline DelegateTypeSyntax(const DelegateTypeSyntax& other) = delete;

		inline virtual ~DelegateTypeSyntax() = default;

		std::wstring ToString() override;
	};
}
