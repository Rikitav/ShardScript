#include <shard/syntax/analysis/TextLocation.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/SourceReader.h>
#include <cctype>
#include <string>
#include <memory>

using namespace std;
using namespace shard::syntax;
using namespace shard::parsing;

SourceReader::~SourceReader() { }

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
			break;

		ReadNext();
		word += Symbol;
	}
	
	return true;
}

bool SourceReader::ReadNextWord(string& word, TokenType& type)
{
	type = TokenType::Unknown;
	word = "";

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

	bool wasClosed, dontEcran;
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

	if (IsType(word, type))
		return true;

	if (IsKeyword(word, type))
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
		{
			word = "0";
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

bool SourceReader::ReadNativeLiteral(string& word)
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

bool SourceReader::IsPunctuation(string& word, TokenType& type)
{
	switch (Symbol)
	{
		case '{':
		{
			type = TokenType::OpenBrace;
			word = "{";
			return true;
		}

		case '}':
		{
			type = TokenType::CloseBrace;
			word = "}";
			return true;
		}

		case '(':
		{
			type = TokenType::OpenCurl;
			word = "(";
			return true;
		}

		case ')':
		{
			type = TokenType::CloseCurl;
			word = ")";
			return true;
		}

		case '[':
		{
			type = TokenType::OpenSquare;
			word = "[";
			return true;
		}

		case ']':
		{
			type = TokenType::CloseSquare;
			word = "]";
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
			word = ",";
			return true;
		}

		case ';':
		{
			type = TokenType::Semicolon;
			word = ";";
			return true;
		}

		default:
		{
			return false;
		}
	}
}

bool SourceReader::IsOperator(string& word, TokenType& type)
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
					word = "==";
					return true;
				}
			}

			type = TokenType::AssignOperator;
			word = "=";
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
					word = "!=";
					return true;
				}
			}

			type = TokenType::NotOperator;
			word = "!";
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
					word = ">=";
					return true;
				}
			}

			type = TokenType::GreaterOperator;
			word = ">";
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
					word = "<=";
					return true;
				}
			}

			type = TokenType::LessOperator;
			word = "<";
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
					word = "+=";
					return true;
				}
				else if (PeekSymbol == '+')
				{
					ReadNext();
					type = TokenType::IncrementOperator;
					word = "++";
					return true;
				}
			}

			word = "+";
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
					word = "-=";
					return true;
				}
				else if (PeekSymbol == '-')
				{
					ReadNext();
					type = TokenType::DecrementOperator;
					word = "--";
					return true;
				}
			}

			word = "-";
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
					word = "*=";
					return true;
				}
			}

			word = "*";
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
					word = "/=";
					return true;
				}
			}

			word = "/";
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
					word = "%=";
					return true;
				}
			}

			word = "%";
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
					word = "^=";
					return true;
				}
			}

			word = "^";
			type = TokenType::PowOperator;
			return true;
		}

		default:
		{
			return false;
		}
	}
}

bool SourceReader::IsNullLiteral(string& word, TokenType& type)
{
	if (word == "null")
	{
		type = TokenType::NullLiteral;
		return true;
	}
	else if (word == "nil")
	{
		type = TokenType::NullLiteral;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsBooleanLiteral(string& word, TokenType& type)
{
	if (word == "true")
	{
		type = TokenType::BooleanLiteral;
		return true;
	}
	else if (word == "false")
	{
		type = TokenType::BooleanLiteral;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsNumberLiteral(TokenType& type)
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

bool SourceReader::IsModifier(string& word, TokenType& type)
{
	if (word == "public")
	{
		type = TokenType::PublicKeyword;
		return true;
	}
	else if (word == "private")
	{
		type = TokenType::PrivateKeyword;
		return true;
	}
	else if (word == "protected")
	{
		type = TokenType::ProtectedKeyword;
		return true;
	}
	else if (word == "internal")
	{
		type = TokenType::InternalKeyword;
		return true;
	}
	else if (word == "static")
	{
		type = TokenType::StaticKeyword;
		return true;
	}
	else if (word == "abstract")
	{
		type = TokenType::AbstractKeyword;
		return true;
	}
	else if (word == "sealed")
	{
		type = TokenType::SealedKeyword;
		return true;
	}
	else if (word == "partial")
	{
		type = TokenType::PartialKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsDirectiveDecl(string& word, TokenType& type)
{
	if (word == "from")
	{
		type = TokenType::FromKeyword;
		return true;
	}
	else if (word == "import")
	{
		type = TokenType::ImportKeyword;
		return true;
	}
	else if (word == "using")
	{
		type = TokenType::UsingKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsTypeDecl(string& word, TokenType& type)
{
	if (word == "method")
	{
		type = TokenType::MethodKeyword;
		return true;
	}
	else if (word == "class")
	{
		type = TokenType::ClassKeyword;
		return true;
	}
	else if (word == "struct")
	{
		type = TokenType::StructKeyword;
		return true;
	}
	else if (word == "interface")
	{
		type = TokenType::InterfaceKeyword;
		return true;
	}
	else if (word == "namespace")
	{
		type = TokenType::NamespaceKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsType(string& word, TokenType& type)
{
	if (word == "void")
	{
		type = TokenType::VoidKeyword;
		return true;
	}
	else if (word == "string")
	{
		type = TokenType::StringKeyword;
		return true;
	}
	else if (word == "int")
	{
		type = TokenType::IntegerKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsKeyword(string& word, TokenType& type)
{
	if (IsConditionalKeyword(word, type))
		return true;
	
	if (IsFunctionalKeyword(word, type))
		return true;

	if (IsLoopKeyword(word, type))
		return true;
}

bool SourceReader::IsConditionalKeyword(string& word, TokenType& type)
{
	if (word == "if")
	{
		type = TokenType::IfKeyword;
		return true;
	}
	else if (word == "unless")
	{
		type = TokenType::UnlessKeyword;
		return true;
	}
	else if (word == "else")
	{
		type = TokenType::ElseKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsLoopKeyword(string& word, TokenType& type)
{
	if (word == "for")
	{
		type = TokenType::ForKeyword;
		return true;
	}
	else if (word == "while")
	{
		type = TokenType::WhileKeyword;
		return true;
	}
	else if (word == "do")
	{
		type = TokenType::DoKeyword;
		return true;
	}
	else if (word == "foreach")
	{
		type = TokenType::ForeachKeyword;
		return true;
	}
	else if (word == "forever")
	{
		type = TokenType::ForeverKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool SourceReader::IsFunctionalKeyword(string& word, TokenType& type)
{
	if (word == "return")
	{
		type = TokenType::ReturnKeyword;
		return true;
	}
	else if (word == "continue")
	{
		type = TokenType::ContinueKeyword;
		return true;
	}
	else if (word == "break")
	{
		type = TokenType::BreakKeyword;
		return true;
	}
	else
	{
		return false;
	}
}
