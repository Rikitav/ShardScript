#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <locale>
#include <string>
#include <deque>

namespace shard
{
	class SHARD_API SourceReader
	{
	protected:
		std::deque<shard::SyntaxToken> ReadBuffer;

		int Line;
		int Offset;
		wchar_t Symbol;
		wchar_t PeekSymbol;

	public:
		SourceReader();
		virtual ~SourceReader();

		virtual shard::SyntaxToken Current();
		virtual shard::SyntaxToken Consume();
		virtual shard::SyntaxToken Peek(int index = 0);
		
		virtual bool CanConsume();
		virtual bool CanPeek();
		virtual shard::TextLocation GetLocation(std::wstring& word) = 0;

	protected:
		virtual bool ReadNextToken(shard::SyntaxToken& pToken);

		virtual bool ReadNext() = 0;
		virtual bool PeekNext() = 0;

		virtual bool ReadNextReal();
		virtual bool ReadNextWord(std::wstring& word, shard::TokenType& type);
		virtual bool ReadNextWhileAlpha(std::wstring& word);

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