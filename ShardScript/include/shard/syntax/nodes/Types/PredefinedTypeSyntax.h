#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/TypeSyntax.h>

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
