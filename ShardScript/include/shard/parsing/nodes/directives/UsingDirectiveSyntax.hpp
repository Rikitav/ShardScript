#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <string>
#include <vector>
#include <span>

namespace shard
{
	class SHARD_API UsingDirectiveSyntax final : public SyntaxNode
	{
		std::vector<SyntaxToken> m_qualifier;
		SyntaxToken m_usingKeyword;
		SyntaxToken m_semicolon;

		std::wstring m_qualifierStringCache;
		bool m_changed;

	public:
		UsingDirectiveSyntax(SyntaxNode* parent);
		virtual ~UsingDirectiveSyntax() = default;

		std::span<const SyntaxToken> get_qualifier() const;
		string_t get_qualifier_string() const;
		SyntaxToken get_semicolon() const;
		SyntaxToken get_using_keyword() const;

		void add_qulifier(const SyntaxToken& token);
		void set_using_keyword(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);

		virtual TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}