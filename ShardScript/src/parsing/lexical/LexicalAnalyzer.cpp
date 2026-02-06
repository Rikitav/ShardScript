#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/lexical/LexicalAnalyzer.h>

#include <cctype>
#include <string>
#include <stdexcept>

using namespace shard;

LexicalAnalyzer::LexicalAnalyzer(SourceTextProvider& sourceText) : SourceText(&sourceText), Line(1), Offset(0), Symbol(-1), PeekSymbol(-1)
{
	ownsSourceTextProvider = false;
}

shard::LexicalAnalyzer::LexicalAnalyzer(SourceTextProvider* sourceText, bool ownsProvider) : SourceText(sourceText), Line(1), Offset(0), Symbol(-1), PeekSymbol(-1), ownsSourceTextProvider(ownsProvider)
{

}

LexicalAnalyzer::~LexicalAnalyzer()
{
	ReadBuffer.clear();

	if (ownsSourceTextProvider)
		delete SourceText;
}

SyntaxToken LexicalAnalyzer::Current()
{
	if (ReadBuffer.empty())
	{
		return Consume();
	}

	return SyntaxToken(ReadBuffer.front());
}

SyntaxToken LexicalAnalyzer::Consume()
{
	static int eofConsumeCounter = 0;
	switch (ReadBuffer.size())
	{
		case 0: // no tokens
			break;

		case 1: // no need to read peeks
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

		consumeBuffer = SyntaxToken(TokenType::EndOfFile, L"", TextLocation());
		ReadBuffer.push_back(consumeBuffer);
		return consumeBuffer;
	}
	else
	{
		eofConsumeCounter = 0;
		ReadBuffer.push_back(consumeBuffer);
		return consumeBuffer;
	}
}

SyntaxToken LexicalAnalyzer::Peek(int index)
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

bool LexicalAnalyzer::CanConsume()
{
	return Current().Type != TokenType::EndOfFile;
}

bool LexicalAnalyzer::CanPeek()
{
	return Peek().Type != TokenType::EndOfFile;
}

bool LexicalAnalyzer::ReadNextToken(SyntaxToken& token)
{
	std::wstring word = L"";
	TokenType type = TokenType::Unknown;

	if (!ReadNextWord(word, type))
		return false;

	token = SyntaxToken(type, word, GetLocation(word));
	return true;
}

bool LexicalAnalyzer::ReadNextReal()
{
	while (SourceText->ReadNext(Symbol))
	{
		switch (Symbol)
		{
			// encoding BOM marks
			case 0xEFBB:
			case 0xFEFF:
			case 0xFFFE:
			{
				continue;
			}

			case L'\n':
			{
				Line += 1;
				Offset = 0;

				continue;
			}

			case L'\0':
			{
				if (!SourceText->PeekNext(PeekSymbol))
					return false;

				if (PeekSymbol == L'\0')
					return false; // EOF

				continue;
			}

			default:
			{
				if (!isspace(Symbol))
					return true;

				continue;
			}
		}
	}

	// EOF
	return false;
}

bool LexicalAnalyzer::ReadNextWhileAlpha(std::wstring& word)
{
	word += Symbol;
	while (SourceText->PeekNext(PeekSymbol))
	{
		if (!isalnum(PeekSymbol) && PeekSymbol != '_')
			break;

		SourceText->ReadNext(Symbol);
		word += Symbol;
	}

	return true;
}

TextLocation LexicalAnalyzer::GetLocation(std::wstring& word)
{
	return TextLocation(SourceText->GetName(), Line, Offset, static_cast<int>(word.length()));
}

bool LexicalAnalyzer::ReadNextWord(std::wstring& word, TokenType& type)
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
	{
		// Handling comment
		if (type == TokenType::Trivia)
		{
			while (SourceText->ReadNext(PeekSymbol))
			{
				switch (Symbol)
				{
					case L'\n':
					{
						Line += 1;
						Offset = 0;

						return ReadNextWord(word, type);
					}

					case L'\0':
					{
						if (!SourceText->PeekNext(PeekSymbol))
							return false;

						if (PeekSymbol == L'\0')
							return false; // EOF

						continue;
					}
				}
			}

			return false; // EOF
		}

		return true;
	}

	bool wasClosed = false;
	bool dontEcran = false;
	if (IsStringLiteral(type, dontEcran))
		return ReadStringLiteral(word, dontEcran, wasClosed);

	if (IsNumberLiteral(type))
		return ReadNumberLiteral(word, type);

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

bool LexicalAnalyzer::ReadCharLiteral(std::wstring& word, bool notEcran, bool& wasClosed)
{
	while (SourceText->ReadNext(PeekSymbol))
	{
		switch (Symbol)
		{
			case '\'':
			{
				wasClosed = true;
				SourceText->PeekNext(PeekSymbol);
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

bool LexicalAnalyzer::ReadStringLiteral(std::wstring& word, bool dontEcran, bool& wasClosed)
{
	bool ecran = false;

	while (SourceText->ReadNext(PeekSymbol))
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
					SourceText->PeekNext(PeekSymbol);
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
				SourceText->PeekNext(PeekSymbol);
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

static bool IsNumberSymbol(wchar_t symbol)
{
	if (symbol >= L'0' && symbol <= L'9')
		return true;

	if (symbol >= L'A' && symbol <= L'F')
		return true;

	if (symbol >= L'a' && symbol <= L'f')
		return true;

	return false;
}

static bool IsVolumeRatio(wchar_t symbol)
{
	switch (symbol)
	{
		default:
			return false;

		case 'K':
		case 'G':
		case 'M':
		case 'T':
		case 'P':
			return false;
	}
}

bool LexicalAnalyzer::ReadNumberLiteral(std::wstring& word, TokenType& type)
{
	if (IsNumberSymbol(Symbol))
		word += Symbol;

	bool foundDelimeter = false;
	while (SourceText->PeekNext(PeekSymbol))
	{
		if (PeekSymbol == '`')
		{
			if (!SourceText->ReadNext(PeekSymbol))
				break;

			if (!SourceText->PeekNext(PeekSymbol))
				break;

			if (IsNumberSymbol(PeekSymbol))
				continue;
		}

		if (PeekSymbol == '.')
		{
			if (foundDelimeter)
				break;

			if (!SourceText->ReadNext(PeekSymbol))
				break;

			if (!SourceText->PeekNext(PeekSymbol))
				break;

			if (!IsNumberSymbol(PeekSymbol))
				break;

			foundDelimeter = true;
			type = TokenType::DoubleLiteral;

			word += '.';
			word += PeekSymbol;
			SourceText->ReadNext(PeekSymbol);
			continue;
		}

		if (IsNumberSymbol(PeekSymbol))
		{
			word += PeekSymbol;
			SourceText->ReadNext(PeekSymbol);
			continue;
		}

		// not literals' char ahead
		break;
	}

	return word.size() > 0;
}

bool LexicalAnalyzer::IsPunctuation(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::IsWordOperator(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::IsOperator(std::wstring& word, TokenType& type)
{
	switch (Symbol)
	{
		case '=':
		{
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::EqualsOperator;
					word = L"==";
					return true;
				}
				else if (PeekSymbol == '>')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::GreaterOrEqualsOperator;
					word = L">=";
					return true;
				}
				else if (PeekSymbol == '>')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::LessOrEqualsOperator;
					word = L"<=";
					return true;
				}
				else if (PeekSymbol == '<')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::AddAssignOperator;
					word = L"+=";
					return true;
				}
				else if (PeekSymbol == '+')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::SubAssignOperator;
					word = L"-=";
					return true;
				}
				else if (PeekSymbol == '-')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::DivAssignOperator;
					word = L"/=";
					return true;
				}
				else if (PeekSymbol == '/')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::Trivia;
					word = L"//";
					return true;
				}
			}

			word = L"/";
			type = TokenType::DivOperator;
			return true;
		}

		case '%':
		{
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::AndAssignOperator;
					word = L"&=";
					return true;
				}
				else if (PeekSymbol == '&')
				{
					SourceText->ReadNext(PeekSymbol);
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
			if (SourceText->PeekNext(PeekSymbol))
			{
				if (PeekSymbol == '=')
				{
					SourceText->ReadNext(PeekSymbol);
					type = TokenType::OrAssignOperator;
					word = L"|=";
					return true;
				}
				else if (PeekSymbol == '|')
				{
					SourceText->ReadNext(PeekSymbol);
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

bool LexicalAnalyzer::IsNullLiteral(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::IsBooleanLiteral(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::IsNumberLiteral(TokenType& type) const
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

bool LexicalAnalyzer::IsCharLiteral(TokenType& type, bool& dontEcran)
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

bool LexicalAnalyzer::IsStringLiteral(TokenType& type, bool& dontEcran)
{
	switch (Symbol)
	{
		case '@':
		{
			if (!SourceText->PeekNext(PeekSymbol))
				return false;

			if (PeekSymbol != '"')
				return false;

			SourceText->ReadNext(PeekSymbol);
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

bool LexicalAnalyzer::IsModifier(std::wstring& word, TokenType& type)
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
	/*
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
	*/
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

bool LexicalAnalyzer::IsDirectiveDecl(std::wstring& word, TokenType& type)
{
	if (word == L"using")
	{
		type = TokenType::UsingKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool LexicalAnalyzer::IsTypeDecl(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::IsType(std::wstring& word, TokenType& type)
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
	else if (word == L"double")
	{
		type = TokenType::DoubleKeyword;
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

bool LexicalAnalyzer::IsKeyword(std::wstring& word, TokenType& type)
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
	else if (word == L"indexer")
	{
		type = TokenType::IndexerKeyword;
		return true;
	}

	return false;
}

bool LexicalAnalyzer::IsConditionalKeyword(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::IsLoopKeyword(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::IsFunctionalKeyword(std::wstring& word, TokenType& type)
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
