#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <memory>
#include <string>

namespace shard
{
	class SHARD_API QualifiedNameTypeSyntax : public TypeSyntax
	{
	public:
		std::unique_ptr<TypeSyntax> Left;
		SyntaxToken Identifier;

		inline QualifiedNameTypeSyntax(std::unique_ptr<TypeSyntax>&& left, SyntaxNode* parent)
			: TypeSyntax(SyntaxKind::QualifiedNameType, parent), Left(std::move(left)) { }

		inline QualifiedNameTypeSyntax(const QualifiedNameTypeSyntax& other) = delete;

		inline virtual ~QualifiedNameTypeSyntax() = default;

		std::wstring ToString() override;
	};
}
