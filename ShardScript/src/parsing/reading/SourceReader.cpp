#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/reading/SourceReader.h>

#include <cctype>
#include <string>
#include <stdexcept>

using namespace shard::syntax;
using namespace shard::parsing;
using namespace shard::parsing::analysis;

SourceReader::SourceReader() : Line(1), Offset(0), Symbol(0), PeekSymbol(0)
{

}

SourceReader::~SourceReader()
{
	/*
	while (!ReadBuffer.empty())
	{
		delete ReadBuffer.front();
		ReadBuffer.pop_front();
	}
	*/

	ReadBuffer.clear();
}

SyntaxToken SourceReader::Current()
{
	if (ReadBuffer.empty())
	{
		return Consume();
	}

	return SyntaxToken(ReadBuffer.front());
}

SyntaxToken SourceReader::Consume()
{
	static int eofConsumeCounter = 0;
	switch (ReadBuffer.size())
	{
		case 0: // no tokens
			break;

		case 1: // no nned to read peeks
		{
			ReadBuffer.pop_front();
			break;
		}

		default: // read peeks
		{
			SyntaxToken token = ReadBuffer.front();
			ReadBuffer.pop_front();
			return SyntaxToken(token);
		}
	}

	SyntaxToken consumeBuffer = SyntaxToken();
	if (!ReadNextToken(consumeBuffer))
	{
		if (++eofConsumeCounter == 10)
			throw std::runtime_error("critical bug: eof consume overflow");

		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());
	}
	else
	{
		eofConsumeCounter = 0;
		ReadBuffer.push_back(consumeBuffer);
		return SyntaxToken(consumeBuffer);
	}
}

SyntaxToken SourceReader::Peek(int index)
{
	static int eofPeekCounter = 0;

	index += 1;
	int bufferSize = static_cast<int>(ReadBuffer.size());

	if ((index + 1) > bufferSize)
	{
		int extendFor = index - bufferSize + 1;
		for (int i = 0; i < extendFor; i++)
		{
			SyntaxToken peekToken = SyntaxToken();
			if (ReadNextToken(peekToken))
			{
				eofPeekCounter = 0;
				ReadBuffer.push_back(peekToken);
				continue;
			}

			if (++eofPeekCounter == 10)
				throw std::runtime_error("critical bug: eof peek overflow");

			peekToken = SyntaxToken(TokenType::EndOfFile, L"", TextLocation());
			ReadBuffer.push_back(peekToken);
			return SyntaxToken(peekToken);
		}
	}
	
	return SyntaxToken(ReadBuffer.at(index));
}

bool SourceReader::CanConsume()
{
	return Current().Type != TokenType::EndOfFile;
}

bool SourceReader::CanPeek()
{
	return Peek().Type != TokenType::EndOfFile;
}

bool SourceReader::ReadNextToken(SyntaxToken& token)
{
	std::wstring word = L"";
	TokenType type = TokenType::Unknown;

	if (!ReadNextWord(word, type))
		return false;

	token = SyntaxToken(type, word, GetLocation(word));
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
				
				if (!ReadNext())
					return false;
				
				continue;
			}

			default:
			{
				if (!ReadNext())
					return false;

				if (!isspace(PeekSymbol))
					return true;

				continue;
			}
		}
	}

	// EOF
	return false;
}

bool SourceReader::ReadNextWhileAlpha(std::wstring& word)
{
	word += Symbol;
	while (PeekNext())
	{
		if (!isalnum(PeekSymbol) && PeekSymbol != '_')
			break;

		ReadNext();
		word += Symbol;
	}
	
	return true;
}

bool SourceReader::ReadNextWord(std::wstring& word, TokenType& type)
{
	type = TokenType::Unknown;
	word = L"";

	// Reading next non whitespace character
	if (!ReadNextReal())
	{
		type = TokenType::EndOfFile;
		return false;
	}

	if (IsPunctuation(word, type))
		return true;

	if (IsOperator(word, type))
		return true;

	bool wasClosed = false;
	bool dontEcran = false;
	if (IsStringLiteral(type, dontEcran))
		return ReadStringLiteral(word, dontEcran, wasClosed);

	if (IsNumberLiteral(type))
		return ReadNumberLiteral(word);

	if (IsNativeLiteral(type))
		return ReadNativeLiteral(word);

	// Reading next non whitespace word
	if (!ReadNextWhileAlpha(word))
		return false;

	if (IsBooleanLiteral(word, type))
		return true;

	if (IsNullLiteral(word, type))
		return true;

	if (IsDirectiveDecl(word, type))
		return true;

	if (IsTypeDecl(word, type))
		return true;

	if (IsModifier(word, type))
		return true;

	if (IsWordOperator(word, type))
		return true;

	if (IsType(word, type))
		return true;

	if (IsKeyword(word, type))
		return true;

	type = TokenType::Identifier;
	return true;
}

bool SourceReader::ReadCharLiteral(std::wstring& word, bool notEcran, bool& wasClosed)
{
	while (ReadNext())
	{
		switch (Symbol)
		{
			case '\'':
			{
				wasClosed = true;
				PeekNext();
				return true;
			}
			
			default:
			{
				word += Symbol;
				continue;
			}
		}
	}

	return false;
}

bool SourceReader::ReadStringLiteral(std::wstring& word, bool dontEcran, bool& wasClosed)
{
	bool ecran = false;

	while (ReadNext())
	{
		switch (Symbol)
		{
			case L'\\':
			{
				if (dontEcran)
				{
					word += Symbol;
					break;
				}

				if (ecran)
				{
					Symbol = L'\\';
					word += Symbol;
					ecran = false;
					break;
				}

				ecran = true;
				break;
			}

			case L'"':
			{
				if (dontEcran)
				{
					wasClosed = true;
					PeekNext();
					return true;
				}

				if (ecran)
				{
					Symbol = L'"';
					word += Symbol;
					ecran = false;
					continue;
				}

				wasClosed = true;
				PeekNext();
				return true;
			}

			case L'n':
			{
				if (ecran)
				{
					Symbol = L'\n';
					ecran = false;
				}

				word += Symbol;
				break;
			}

			default:
			{
				word += Symbol;
				continue;
			}
		}
	}

	return false;
}

bool SourceReader::ReadNumberLiteral(std::wstring& word)
{
	short mode = 0;
	if (Symbol == '0')
	{
		if (!PeekNext())
		{
			word = L"0";
			return true;
		}

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

bool SourceReader::ReadNativeLiteral(std::wstring& word)
{
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

bool SourceReader::IsPunctuation(std::wstring& word, TokenType& type)
{
	switch (Symbol)
	{
		case '{':
		{
			type = TokenType::OpenBrace;
			word = L"{";
			return true;
		}

		case '}':
		{
			type = TokenType::CloseBrace;
			word = L"}";
			return true;
		}

		case '(':
		{
			type = TokenType::OpenCurl;
			word = L"(";
			return true;
		}

		case ')':
		{
			type = TokenType::CloseCurl;
			word = L")";
			return true;
		}

		case '[':
		{
			type = TokenType::OpenSquare;
			word = L"[";
			return true;
		}

		case ']':
		{
			type = TokenType::CloseSquare;
			word = L"]";
			return true;
		}

		case '.':
		{
			type = TokenType::Delimeter;
			word = '.';
			return true;
		}

		case ',':
		{
			type = TokenType::Comma;
			word = L",";
			return true;
		}

		case ':':
		{
			type = TokenType::Colon;
			word = L":";
			return true;
		}

		case ';':
		{
			type = TokenType::Semicolon;
			word = L";";
			return true;
		}

		case '?':
		{
			type = TokenType::Question;
			word = L"?";
			return true;
		}

		default:
		{
			return false;
		}
	}
}

bool SourceReader::IsWordOperator(std::wstring& word, TokenType& type)
{
	// Just because my friend asked this
	if (word == L"and")
	{
		type = TokenType::AndOperator;
		return true;
	}
	else if (word == L"or")
	{
		type = TokenType::OrOperator;
		return true;
	}
	else if (word == L"not")
	{
		type = TokenType::NotOperator;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsOperator(std::wstring& word, TokenType& type)
{
	switch (Symbol)
	{
		case '=':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::EqualsOperator;
					word = L"==";
					return true;
				}
				else if (PeekSymbol == '>')
				{
					ReadNext();
					type = TokenType::LambdaOperator;
					word = L"=>";
					return true;
				}
			}

			type = TokenType::AssignOperator;
			word = L"=";
			return true;
		}

		case '!':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::NotEqualsOperator;
					word = L"!=";
					return true;
				}
			}

			type = TokenType::NotOperator;
			word = L"!";
			return true;
		}

		case '>':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::GreaterOrEqualsOperator;
					word = L">=";
					return true;
				}
				else if (PeekSymbol == '>')
				{
					ReadNext();
					type = TokenType::RightShiftOperator;
					word = L">>";
					return true;
				}
			}

			type = TokenType::GreaterOperator;
			word = L">";
			return true;
		}

		case '<':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::LessOrEqualsOperator;
					word = L"<=";
					return true;
				}
				else if (PeekSymbol == '<')
				{
					ReadNext();
					type = TokenType::LeftShiftOperator;
					word = L"<<";
					return true;
				}
			}

			type = TokenType::LessOperator;
			word = L"<";
			return true;
		}

		case '+':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::AddAssignOperator;
					word = L"+=";
					return true;
				}
				else if (PeekSymbol == '+')
				{
					ReadNext();
					type = TokenType::IncrementOperator;
					word = L"++";
					return true;
				}
			}

			word = L"+";
			type = TokenType::AddOperator;
			return true;
		}

		case '-':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::SubAssignOperator;
					word = L"-=";
					return true;
				}
				else if (PeekSymbol == '-')
				{
					ReadNext();
					type = TokenType::DecrementOperator;
					word = L"--";
					return true;
				}
			}

			word = L"-";
			type = TokenType::SubOperator;
			return true;
		}

		case '*':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::MultAssignOperator;
					word = L"*=";
					return true;
				}
			}

			word = L"*";
			type = TokenType::MultOperator;
			return true;
		}

		case '/':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::DivAssignOperator;
					word = L"/=";
					return true;
				}
			}

			word = L"/";
			type = TokenType::DivOperator;
			return true;
		}

		case '%':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::ModAssignOperator;
					word = L"%=";
					return true;
				}
			}

			word = L"%";
			type = TokenType::ModOperator;
			return true;
		}

		case '^':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::PowAssignOperator;
					word = L"^=";
					return true;
				}
			}

			word = L"^";
			type = TokenType::PowOperator;
			return true;
		}

		case '&':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::AndAssignOperator;
					word = L"&=";
					return true;
				}
				else if (PeekSymbol == '&')
				{
					ReadNext();
					type = TokenType::AndOperator;
					word = L"&&";
					return true;
				}
			}

			word = L"&";
			type = TokenType::AndOperator;
			return true;
		}

		case '|':
		{
			if (PeekNext())
			{
				if (PeekSymbol == '=')
				{
					ReadNext();
					type = TokenType::OrAssignOperator;
					word = L"|=";
					return true;
				}
				else if (PeekSymbol == '|')
				{
					ReadNext();
					type = TokenType::OrOperator;
					word = L"||";
					return true;
				}
			}

			word = L"|";
			type = TokenType::OrOperator;
			return true;
		}

		default:
		{
			return false;
		}
	}
}

bool SourceReader::IsNullLiteral(std::wstring& word, TokenType& type)
{
	if (word == L"null")
	{
		type = TokenType::NullLiteral;
		return true;
	}
	else if (word == L"nil")
	{
		type = TokenType::NullLiteral;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsBooleanLiteral(std::wstring& word, TokenType& type)
{
	if (word == L"true")
	{
		type = TokenType::BooleanLiteral;
		return true;
	}
	else if (word == L"false")
	{
		type = TokenType::BooleanLiteral;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsNumberLiteral(TokenType& type) const
{
	if (isdigit(Symbol))
	{
		type = TokenType::NumberLiteral;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsNativeLiteral(TokenType& type)
{
	if (Symbol == 'n')
	{
		if (!PeekNext())
			return false;

		if (isdigit(PeekSymbol))
		{
			type = TokenType::NativeLiteral;
			return true;
		}
	}

	return false;
}

bool SourceReader::IsCharLiteral(TokenType& type, bool& dontEcran)
{
	switch (Symbol)
	{
		case '\'':
		{
			type = TokenType::CharLiteral;
			return true;
		}

		default:
			return false;
	}
}

bool SourceReader::IsStringLiteral(TokenType& type, bool& dontEcran)
{
	switch (Symbol)
	{
		case '@':
		{
			if (!PeekNext())
				return false;

			if (PeekSymbol != '"')
				return false;

			dontEcran = true;
			return IsStringLiteral(type, dontEcran);
		}

		case '"':
		{
			type = TokenType::StringLiteral;
			return true;
		}

		default:
			return false;
	}
}

bool SourceReader::IsModifier(std::wstring& word, TokenType& type)
{
	if (word == L"public")
	{
		type = TokenType::PublicKeyword;
		return true;
	}
	else if (word == L"private")
	{
		type = TokenType::PrivateKeyword;
		return true;
	}
	else if (word == L"protected")
	{
		type = TokenType::ProtectedKeyword;
		return true;
	}
	else if (word == L"internal")
	{
		type = TokenType::InternalKeyword;
		return true;
	}
	else if (word == L"static")
	{
		type = TokenType::StaticKeyword;
		return true;
	}
	else if (word == L"abstract")
	{
		type = TokenType::AbstractKeyword;
		return true;
	}
	else if (word == L"sealed")
	{
		type = TokenType::SealedKeyword;
		return true;
	}
	else if (word == L"partial")
	{
		type = TokenType::PartialKeyword;
		return true;
	}
	else if (word == L"override")
	{
		type = TokenType::OverrideKeyword;
		return true;
	}
	else if (word == L"virtual")
	{
		type = TokenType::VirtualKeyword;
		return true;
	}
	else if (word == L"extern")
	{
		type = TokenType::ExternKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsDirectiveDecl(std::wstring& word, TokenType& type)
{
	if (word == L"from")
	{
		type = TokenType::FromKeyword;
		return true;
	}
	else if (word == L"import")
	{
		type = TokenType::ImportKeyword;
		return true;
	}
	else if (word == L"using")
	{
		type = TokenType::UsingKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsTypeDecl(std::wstring& word, TokenType& type)
{
	if (word == L"method")
	{
		type = TokenType::MethodKeyword;
		return true;
	}
	else if (word == L"class")
	{
		type = TokenType::ClassKeyword;
		return true;
	}
	else if (word == L"struct")
	{
		type = TokenType::StructKeyword;
		return true;
	}
	else if (word == L"interface")
	{
		type = TokenType::InterfaceKeyword;
		return true;
	}
	else if (word == L"namespace")
	{
		type = TokenType::NamespaceKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsType(std::wstring& word, TokenType& type)
{
	if (word == L"void")
	{
		type = TokenType::VoidKeyword;
		return true;
	}
	else if (word == L"var")
	{
		type = TokenType::VarKeyword;
		return true;
	}
	else if (word == L"bool")
	{
		type = TokenType::BooleanKeyword;
		return true;
	}
	else if (word == L"string")
	{
		type = TokenType::StringKeyword;
		return true;
	}
	else if (word == L"int")
	{
		type = TokenType::IntegerKeyword;
		return true;
	}
	else if (word == L"lambda")
	{
		type = TokenType::LambdaKeyword;
		return true;
	}
	else if (word == L"delegate")
	{
		type = TokenType::DelegateKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsKeyword(std::wstring& word, TokenType& type)
{
	if (IsConditionalKeyword(word, type))
		return true;
	
	if (IsFunctionalKeyword(word, type))
		return true;

	if (IsLoopKeyword(word, type))
		return true;

	if (word == L"new")
	{
		type = TokenType::NewKeyword;
		return true;
	}
	else if (word == L"get")
	{
		type = TokenType::GetKeyword;
		return true;
	}
	else if (word == L"set")
	{
		type = TokenType::SetKeyword;
		return true;
	}
	else if (word == L"field")
	{
		type = TokenType::FieldKeyword;
		return true;
	}

	return false;
}

bool SourceReader::IsConditionalKeyword(std::wstring& word, TokenType& type)
{
	if (word == L"if")
	{
		type = TokenType::IfKeyword;
		return true;
	}
	else if (word == L"unless")
	{
		type = TokenType::UnlessKeyword;
		return true;
	}
	else if (word == L"else")
	{
		type = TokenType::ElseKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsLoopKeyword(std::wstring& word, TokenType& type)
{
	if (word == L"for")
	{
		type = TokenType::ForKeyword;
		return true;
	}
	else if (word == L"while")
	{
		type = TokenType::WhileKeyword;
		return true;
	}
	else if (word == L"until")
	{
		type = TokenType::UntilKeyword;
		return true;
	}
	else if (word == L"do")
	{
		type = TokenType::DoKeyword;
		return true;
	}
	else if (word == L"foreach")
	{
		type = TokenType::ForeachKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsFunctionalKeyword(std::wstring& word, TokenType& type)
{
	if (word == L"return")
	{
		type = TokenType::ReturnKeyword;
		return true;
	}
	else if (word == L"continue")
	{
		type = TokenType::ContinueKeyword;
		return true;
	}
	else if (word == L"break")
	{
		type = TokenType::BreakKeyword;
		return true;
	}
	else if (word == L"throw")
	{
		type = TokenType::ThrowKeyword;
		return true;
	}
	else
	{
		return false;
	}
}
