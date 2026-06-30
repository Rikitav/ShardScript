#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

namespace shard
{
	class SHARD_API PredefinedTypeSyntax : public TypeSyntax
	{
	public:
		const SyntaxToken TypeToken;

		inline PredefinedTypeSyntax(const SyntaxToken typeToken, SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::PredefinedType, parent), TypeToken(typeToken) { }

		inline PredefinedTypeSyntax(const PredefinedTypeSyntax& other) = delete;

		inline virtual ~PredefinedTypeSyntax()
		{

		}

		std::wstring ToString() override;
	};
}
