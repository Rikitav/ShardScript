#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/parsing/analysis/TextLocation.hpp>
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/lexical/reading/SourceTextProvider.hpp>

#include <locale>
#include <string>
#include <deque>

namespace shard
{
	class SHARD_API LexicalAnalyzer : public SourceProvider
	{
	protected:
		int Line;
		int Offset;
		wchar_t Symbol;
		wchar_t PeekSymbol;

		bool ownsSourceTextProvider = false;
		SourceTextProvider* SourceText;
		std::deque<shard::SyntaxToken> ReadBuffer;

	public:
		LexicalAnalyzer(SourceTextProvider& sourceText);
		LexicalAnalyzer(SourceTextProvider* sourceText, bool ownsProvider);
		~LexicalAnalyzer();

		shard::SyntaxToken Current() override;
		shard::SyntaxToken Consume() override;
		shard::SyntaxToken Peek(int index = 0) override;

		bool CanConsume() override;
		bool CanPeek() override;

	protected:
		shard::TextLocation GetLocation(std::wstring& word);
		bool ReadNextToken(shard::SyntaxToken& pToken);

		bool ReadNextReal();
		bool ReadNextWord(std::wstring& word, shard::TokenType& type);
		bool ReadNextWhileAlpha(std::wstring& word);

		bool ReadNumberLiteral(std::wstring& word, shard::TokenType& type);
		bool ReadCharLiteral(std::wstring& word, bool notEcran, bool& wasClosed);
		bool ReadStringLiteral(std::wstring& word, bool notEcran, bool& wasClosed);

		bool IsNumberLiteral(shard::TokenType& type) const;
		bool IsCharLiteral(shard::TokenType& type, bool& dontEcran);
		bool IsStringLiteral(shard::TokenType& type, bool& dontEcran);

		bool IsPunctuation(std::wstring& word, shard::TokenType& type);
		bool IsOperator(std::wstring& word, shard::TokenType& type);
		bool IsWordOperator(std::wstring& word, shard::TokenType& type);
		bool IsNullLiteral(std::wstring& word, shard::TokenType& type);
		bool IsBooleanLiteral(std::wstring& word, shard::TokenType& type);
		bool IsModifier(std::wstring& word, shard::TokenType& type);
		bool IsDirectiveDecl(std::wstring& word, shard::TokenType& type);
		bool IsTypeDecl(std::wstring& word, shard::TokenType& type);
		bool IsType(std::wstring& word, shard::TokenType& type);
		bool IsKeyword(std::wstring& word, shard::TokenType& type);
		bool IsLoopKeyword(std::wstring& word, shard::TokenType& type);
		bool IsConditionalKeyword(std::wstring& word, shard::TokenType& type);
		bool IsFunctionalKeyword(std::wstring& word, shard::TokenType& type);
	};
}