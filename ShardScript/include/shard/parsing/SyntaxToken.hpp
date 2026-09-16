#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <gmt/Interner.hpp>

#include <string_view>
#include <optional>

namespace shard
{
	struct SHARD_API SyntaxToken
	{
	private:
		bool m_isMissing;
		TokenType m_type;
		TextLocation m_location;
		rs::stringintern::StringReference m_lexeme;

	public:
		inline SyntaxToken() :
			m_isMissing(true),
			m_type(TokenType::Unknown),
			m_location(),
			m_lexeme()
		{ }

		// interns the lexeme into the global interner
		inline SyntaxToken(
			const TokenType type,
			const std::wstring_view lexeme,
			const TextLocation& location,
			const bool isMissing = false
		) :
			SyntaxToken(type, gmt::intern(lexeme), location, isMissing)
		{ }

		inline SyntaxToken(
			const TokenType type,
			const rs::stringintern::StringReference lexeme,
			const TextLocation& location,
			const bool isMissing = false
		) :
			m_isMissing(isMissing),
			m_type(type),
			m_location(location),
			m_lexeme(lexeme)
		{ }

		inline bool is_missing() const { return m_isMissing; }
		inline TokenType get_type() const { return m_type; }
		inline const TextLocation& get_location() const { return m_location; }

		// zero-copy view into interned storage; empty for missing tokens
		inline std::wstring_view get_lexeme() const { return gmt::resolve(m_lexeme); }
	};
}
