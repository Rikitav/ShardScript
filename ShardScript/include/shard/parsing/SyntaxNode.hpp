#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SyntaxVisitor;

	class SHARD_API SyntaxNode
	{
		SyntaxKind m_kind;
		gmt::Ref<SyntaxNode> m_parent;

	public:
		SyntaxNode(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~SyntaxNode();

		SyntaxNode(const SyntaxNode&) = delete;
		SyntaxNode& operator=(const SyntaxNode&) = delete;

		inline SyntaxKind get_kind() const { return m_kind; }
		inline gmt::Ref<const SyntaxNode> get_parent() const { return m_parent; }

		virtual TextLocation get_location() const = 0;
		virtual void accept(SyntaxVisitor& visitor) const = 0;
	};
}
