#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <string>
#include <memory>

namespace shard::parsing
{
	class SourceReader
	{
	protected:
		std::shared_ptr<shard::syntax::SyntaxToken> PeekBuffer;
		std::shared_ptr<shard::syntax::SyntaxToken> ConsumeBuffer;

		int Line;
		int Offset;
		char Symbol;
		char PeekSymbol;

	public:
		SourceReader() : Line(0), Offset(0), Symbol(0), PeekSymbol(0) {}
		virtual ~SourceReader() = 0;

		virtual shard::syntax::SyntaxToken Current();
		virtual shard::syntax::SyntaxToken Consume();
		virtual shard::syntax::SyntaxToken Peek();
		
		virtual bool CanConsume();
		virtual bool CanPeek();
		virtual shard::syntax::analysis::TextLocation GetLocation(string& word) = 0;

	protected:
		virtual bool ReadNextToken(std::shared_ptr<shard::syntax::SyntaxToken>& pToken);

		virtual bool ReadNext() = 0;
		virtual bool PeekNext() = 0;

		virtual bool ReadNextReal();
		virtual bool ReadNextWord(std::string& word, shard::syntax::TokenType& type);
		virtual bool ReadNextWhileAlpha(std::string& word);

		bool ReadStringLiteral(std::string& word, bool notEcran, bool& wasClosed);
		bool ReadNumberLiteral(std::string& word);
		bool ReadNativeLiteral(std::string& word);

		bool IsNumberLiteral(shard::syntax::TokenType& type);
		bool IsNativeLiteral(shard::syntax::TokenType& type);
		bool IsStringLiteral(shard::syntax::TokenType& type, bool& dontEcran);

		bool IsPunctuation(std::string& word, shard::syntax::TokenType& type);
		bool IsOperator(std::string& word, shard::syntax::TokenType& type);
		bool IsNullLiteral(std::string& word, shard::syntax::TokenType& type);
		bool IsBooleanLiteral(std::string& word, shard::syntax::TokenType& type);
		bool IsModifier(std::string& word, shard::syntax::TokenType& type);
		bool IsDirectiveDecl(std::string& word, shard::syntax::TokenType& type);
		bool IsTypeDecl(std::string& word, shard::syntax::TokenType& type);
		bool IsType(std::string& word, shard::syntax::TokenType& type);
		bool IsLoopKeyword(std::string& word, shard::syntax::TokenType& type);
		bool IsFunctionalKeyword(std::string& word, shard::syntax::TokenType& type);
	};
}