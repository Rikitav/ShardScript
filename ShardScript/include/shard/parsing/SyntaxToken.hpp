#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <string>

namespace shard
{
	struct SHARD_API SyntaxToken
	{
	private:
		bool m_isMissing;
		TokenType m_type;
		TextLocation m_location;
		std::wstring m_lexeme;

	public:
		inline SyntaxToken() :
			m_isMissing(true),
			m_type(TokenType::Unknown),
			m_location(),
			m_lexeme()
		{ }

		inline SyntaxToken(
			const TokenType type,
			const std::wstring& lexeme,
			const TextLocation& location,
			const bool isMissing = false
		) :
			m_isMissing(isMissing),
			m_type(type),
			m_location(location),
			m_lexeme(std::move(lexeme))
		{ }
			
		inline bool get_is_missing() const { return m_isMissing; }
		inline TokenType get_type() const { return m_type; }
		inline const TextLocation& get_location() const { return m_location; }
		inline const wchar_t* get_lexeme() const { return m_lexeme.c_str(); }
	};
}
