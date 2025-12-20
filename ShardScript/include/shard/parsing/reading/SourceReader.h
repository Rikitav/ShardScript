#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <locale>
#include <string>
#include <deque>

namespace shard::parsing
{
	class SHARD_API SourceReader
	{
	protected:
		std::deque<shard::syntax::SyntaxToken> ReadBuffer;

		int Line;
		int Offset;
		wchar_t Symbol;
		wchar_t PeekSymbol;

	public:
		SourceReader();
		virtual ~SourceReader();

		virtual shard::syntax::SyntaxToken Current();
		virtual shard::syntax::SyntaxToken Consume();
		virtual shard::syntax::SyntaxToken Peek(int index = 0);
		
		virtual bool CanConsume();
		virtual bool CanPeek();
		virtual shard::parsing::analysis::TextLocation GetLocation(std::wstring& word) = 0;

	protected:
		virtual bool ReadNextToken(shard::syntax::SyntaxToken& pToken);

		virtual bool ReadNext() = 0;
		virtual bool PeekNext() = 0;

		virtual bool ReadNextReal();
		virtual bool ReadNextWord(std::wstring& word, shard::syntax::TokenType& type);
		virtual bool ReadNextWhileAlpha(std::wstring& word);

		bool ReadNumberLiteral(std::wstring& word, shard::syntax::TokenType& type);
		bool ReadCharLiteral(std::wstring& word, bool notEcran, bool& wasClosed);
		bool ReadStringLiteral(std::wstring& word, bool notEcran, bool& wasClosed);

		bool IsNumberLiteral(shard::syntax::TokenType& type) const;
		bool IsCharLiteral(shard::syntax::TokenType& type, bool& dontEcran);
		bool IsStringLiteral(shard::syntax::TokenType& type, bool& dontEcran);

		bool IsPunctuation(std::wstring& word, shard::syntax::TokenType& type);
		bool IsOperator(std::wstring& word, shard::syntax::TokenType& type);
		bool IsWordOperator(std::wstring& word, shard::syntax::TokenType& type);
		bool IsNullLiteral(std::wstring& word, shard::syntax::TokenType& type);
		bool IsBooleanLiteral(std::wstring& word, shard::syntax::TokenType& type);
		bool IsModifier(std::wstring& word, shard::syntax::TokenType& type);
		bool IsDirectiveDecl(std::wstring& word, shard::syntax::TokenType& type);
		bool IsTypeDecl(std::wstring& word, shard::syntax::TokenType& type);
		bool IsType(std::wstring& word, shard::syntax::TokenType& type);
		bool IsKeyword(std::wstring& word, shard::syntax::TokenType& type);
		bool IsLoopKeyword(std::wstring& word, shard::syntax::TokenType& type);
		bool IsConditionalKeyword(std::wstring& word, shard::syntax::TokenType& type);
		bool IsFunctionalKeyword(std::wstring& word, shard::syntax::TokenType& type);
	};
}