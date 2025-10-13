#include <shard/syntax/analysis/TextLocation.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/SourceReader.h>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace shard::syntax;
using namespace shard::parsing;

SourceReader::SourceReader()
{
	Line = 0;
    Offset = 0;
	Symbol = 0;
	PeekSymbol = 0;

	PeekBuffer = nullptr;
	ConsumeBuffer = nullptr;
}

SyntaxToken SourceReader::Current()
{
	if (ConsumeBuffer == nullptr)
		return Consume();

	return SyntaxToken(*ConsumeBuffer);
}

SyntaxToken SourceReader::Consume()
{
	if (PeekBuffer != nullptr)
	{
		ConsumeBuffer = PeekBuffer;
		PeekBuffer = nullptr;
		return SyntaxToken(*ConsumeBuffer);
	}
	
	if (!ReadNextToken(ConsumeBuffer))
	{
		ConsumeBuffer = make_shared<SyntaxToken>(TokenType::EndOfFile, "", TextLocation());
	}

	return SyntaxToken(*ConsumeBuffer);
}

SyntaxToken SourceReader::Peek()
{
	if (PeekBuffer == nullptr)
	{
		if (!ReadNextToken(PeekBuffer))
			PeekBuffer = make_shared<SyntaxToken>(TokenType::EndOfFile, "", TextLocation());
	}

	return SyntaxToken(*PeekBuffer);
}

bool SourceReader::CanConsume()
{
	return Current().Type != TokenType::EndOfFile;
}

bool SourceReader::CanPeek()
{
	return Peek().Type != TokenType::EndOfFile;
}

bool SourceReader::ReadNextToken(shared_ptr<SyntaxToken>& pToken)
{
	string word = "";
	TokenType type = TokenType::Unknown;

	if (!ReadNextWord(word, type))
		return false;

	pToken = make_shared<SyntaxToken>(type, word, GetLocation(word));
	return true;
}

bool SourceReader::ReadNextReal()
{
	while (PeekNext())
	{
		switch (PeekSymbol)
		{
			case '\n':
			{
				Line += 1;
				Offset = 0;

				ReadNext();
				continue;
			}

			default:
			{
				if (!isspace(PeekSymbol))
					return ReadNext();

				ReadNext();
				continue;
			}
		}
	}

	// EOF
	return false;
}

bool SourceReader::ReadNextWhileAlpha(string& word)
{
	word += Symbol;
	while (PeekNext())
	{
		if (!isalpha(PeekSymbol) && PeekSymbol != '_')
			return true;

		ReadNext();
		word += Symbol;
	}
	
	return false;
}

bool SourceReader::ReadNextWord(string& word, TokenType& type)
{
	type = TokenType::Unknown;
	word = "";
	if (!ReadNextReal())
	{
		type = TokenType::EndOfFile;
		return false;
	}

	if (IsPunctuation(word, type))
		return true;

	if (IsOperator(word, type))
		return true;

	bool wasClosed, dontEcran;
	if (IsStringLiteral(word, type, dontEcran))
		return ReadStringLiteral(word, dontEcran, wasClosed);

	if (IsNumberLiteral(word, type))
		return ReadNumberLiteral(word);

	if (!ReadNextWhileAlpha(word))
		return false;

	if (IsTypeDecl(word, type))
		return true;

	if (IsModifier(word, type))
		return true;

	if (IsType(word, type))
		return true;

	type = TokenType::Identifier;
	return true;
}

bool SourceReader::ReadStringLiteral(string& word, bool dontEcran, bool& wasClosed)
{
	bool ecran = false;

	while (ReadNext())
	{
		switch (Symbol)
		{
			case '\\':
			{
				if (dontEcran)
					break;

				if (!ecran)
				{
					ecran = true;
					continue;
				}

				break;
			}

			case '"':
			{
				if (dontEcran || !ecran)
				{
					wasClosed = true;
					PeekNext();
					return true;
				}

				break;
			}

			case 'n':
			{
				if (ecran)
				{
					Symbol = '\n';
					ecran = false;
				}

				break;
			}
		}

		word += Symbol;
		continue;
	}

	return false;
}

bool SourceReader::ReadNumberLiteral(string& word)
{
	short mode = 0;
	if (Symbol == '0')
	{
		if (!PeekNext())
			return false;

		switch (PeekSymbol)
		{
			case 'd':
				mode = 0;
				break;

			case 'x':
				mode = 1;
				break;

			case 'b':
				mode = 2;
				break;

			default:
				word += Symbol;
				break;
		}
	}
	else
	{
		word += Symbol;
	}

	while (PeekNext())
	{
		if (PeekSymbol == '_' || PeekSymbol == '`')
			continue;

		if (!isalnum(PeekSymbol))
			return true;

		word += PeekSymbol;
		ReadNext();
		continue;
	}

	return false;
}
