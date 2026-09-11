#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/TokenType.hpp>

#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/SourceTextProvider.hpp>

#include <locale>
#include <string>
#include <deque>

namespace shard
{
	class SHARD_API LexicalAnalyzer : public SourceProvider
	{
	protected:
		int m_line = 1;
		int m_offset = 0;
		wchar_t m_symbol = -1;
		wchar_t m_peekSymbol = -1;

		bool m_hasPutbackSymbol = false;
		wchar_t m_putbackSymbol = L'\0';

		bool m_ownsSourceTextProvider;
		SourceTextProvider* m_sourceText;
		std::deque<SyntaxToken> m_readBuffer;

	public:
		LexicalAnalyzer(SourceTextProvider& sourceText);
		LexicalAnalyzer(SourceTextProvider* sourceText, bool ownsProvider);
		~LexicalAnalyzer();

		SyntaxToken current() override;
		SyntaxToken consume() override;
		SyntaxToken peek(int index = 0) override;
		void put_back(SyntaxToken token) override;

		bool can_consume() override;
		bool can_peek() override;

	protected:
		bool advance(wchar_t& ch);

		TextLocation get_current_location(std::wstring& word);
		bool read_next_token(SyntaxToken& pToken);

		bool read_next_real();
		bool read_next_word(std::wstring& word, TokenType& type);
		bool read_while_alpha(std::wstring& word);

		bool read_number_literal(std::wstring& word, TokenType& type);
		bool read_char_literal(std::wstring& word, bool notEcran, bool& wasClosed);
		bool read_string_literal(std::wstring& word, bool notEcran, bool& wasClosed);

		bool is_null_literal(std::wstring& word, TokenType& type);
		bool is_boolean_literal(std::wstring& word, TokenType& type);
		bool is_number_literal(TokenType& type) const;
		bool is_char_literal(TokenType& type, bool& dontEcran);
		bool is_string_literal(TokenType& type, bool& dontEcran);

		bool is_punctuation(std::wstring& word, TokenType& type);
		bool is_operator(std::wstring& word, TokenType& type);
		bool is_word_operator(std::wstring& word, TokenType& type);
		bool is_modifier(std::wstring& word, TokenType& type);
		bool is_directive_decl(std::wstring& word, TokenType& type);
		bool is_type_decl(std::wstring& word, TokenType& type);
		bool is_type(std::wstring& word, TokenType& type);
		bool is_keyword(std::wstring& word, TokenType& type);
		bool is_loop_keyword(std::wstring& word, TokenType& type);
		bool is_conditional_keyword(std::wstring& word, TokenType& type);
		bool is_functional_keyword(std::wstring& word, TokenType& type);
	};
}