#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>

namespace shard
{
	class SHARD_API PredefinedTypeSyntax : public TypeSyntax
	{
	public:
		const SyntaxToken TypeToken;

		inline PredefinedTypeSyntax(const SyntaxToken typeToken, SyntaxNode *const parent)
			: TypeSyntax(SyntaxKind::PredefinedType, parent), TypeToken(typeToken) { }

		inline PredefinedTypeSyntax(const PredefinedTypeSyntax& other) = delete;

		inline virtual ~PredefinedTypeSyntax()
		{

		}

		std::wstring ToString() override;
	};
}
