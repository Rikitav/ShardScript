#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>

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

		inline DelegateTypeSyntax(SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::DelegateType, parent) { }

		inline DelegateTypeSyntax(const DelegateTypeSyntax& other) = delete;

		inline virtual ~DelegateTypeSyntax() = default;

		std::wstring ToString() override;
	};
}
