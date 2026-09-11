#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <shard/lexical/LexicalAnalyzer.hpp>

#include <cctype>
#include <cwctype>
#include <string>
#include <stdexcept>

using namespace shard;

bool LexicalAnalyzer::advance(wchar_t& ch)
{
	if (!m_sourceText->read_next(ch))
		return false;

	if (ch == L'\n')
	{
		m_line += 1;
		m_offset = 0;
	}
	else if (ch != L'\0')
	{
		m_offset += 1;
	}

	return true;
}

LexicalAnalyzer::LexicalAnalyzer(SourceTextProvider& sourceText) :
	m_ownsSourceTextProvider(false),
	m_sourceText(&sourceText)
{ }

LexicalAnalyzer::LexicalAnalyzer(SourceTextProvider* sourceText, bool ownsProvider) :
	m_ownsSourceTextProvider(ownsProvider),
	m_sourceText(sourceText)
{ }

LexicalAnalyzer::~LexicalAnalyzer()
{
	m_readBuffer.clear();

	if (m_ownsSourceTextProvider)
		delete m_sourceText;
}

SyntaxToken LexicalAnalyzer::current()
{
	if (m_readBuffer.empty())
	{
		return consume();
	}

	return SyntaxToken(m_readBuffer.front());
}

SyntaxToken LexicalAnalyzer::consume()
{
	switch (m_readBuffer.size())
	{
		case 0: // no tokens
			break;

		case 1: // consume the single buffered token
		{
			SyntaxToken token = m_readBuffer.front();
			m_readBuffer.pop_front();
			return SyntaxToken(token);
		}

		default: // read peeks
		{
			SyntaxToken token = m_readBuffer.front();
			m_readBuffer.pop_front();
			return SyntaxToken(token);
		}
	}

	SyntaxToken consumeBuffer = SyntaxToken();
	if (!read_next_token(consumeBuffer))
	{
		consumeBuffer = SyntaxToken(TokenType::EndOfFile, L"", TextLocation());
		m_readBuffer.push_back(consumeBuffer);
		return consumeBuffer;
	}
	else
	{
		m_readBuffer.push_back(consumeBuffer);
		return consumeBuffer;
	}
}

SyntaxToken LexicalAnalyzer::peek(int index)
{
	index += 1;
	int bufferSize = static_cast<int>(m_readBuffer.size());

	if ((index + 1) > bufferSize)
	{
		int extendFor = index - bufferSize + 1;
		for (int i = 0; i < extendFor; i++)
		{
			SyntaxToken peekToken = SyntaxToken();
			if (read_next_token(peekToken))
			{
				m_readBuffer.push_back(peekToken);
				continue;
			}

			peekToken = SyntaxToken(TokenType::EndOfFile, L"", TextLocation());
			m_readBuffer.push_back(peekToken);
			return SyntaxToken(peekToken);
		}
	}

	if (index < 0 || index >= static_cast<int>(m_readBuffer.size()))
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return SyntaxToken(m_readBuffer.at(index));
}

void LexicalAnalyzer::put_back(SyntaxToken token)
{
	m_readBuffer.push_front(token);
}

bool LexicalAnalyzer::can_consume()
{
	return current().get_type() != TokenType::EndOfFile;
}

bool LexicalAnalyzer::can_peek()
{
	return peek(0).get_type() != TokenType::EndOfFile;
}

bool LexicalAnalyzer::read_next_token(SyntaxToken& token)
{
	std::wstring word = L"";
	TokenType type = TokenType::Unknown;

	if (!read_next_word(word, type))
		return false;

	token = SyntaxToken(type, word, get_current_location(word));
	return true;
}

bool LexicalAnalyzer::read_next_real()
{
	if (m_hasPutbackSymbol)
	{
		m_symbol = m_putbackSymbol;
		m_hasPutbackSymbol = false;
		m_offset += 1;
		return true;
	}

	while (advance(m_symbol))
	{
		switch (m_symbol)
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
				continue;
			}

			case L'\0':
			{
				if (!m_sourceText->peek_next(m_peekSymbol))
					return false;

				if (m_peekSymbol == L'\0')
					return false; // EOF

				continue;
			}

			default:
			{
				if (!std::iswspace(static_cast<wint_t>(m_symbol)))
					return true;

				continue;
			}
		}
	}

	// EOF
	return false;
}

bool LexicalAnalyzer::read_while_alpha(std::wstring& word)
{
	word += m_symbol;
	while (m_sourceText->peek_next(m_peekSymbol))
	{
		if (!std::iswalnum(static_cast<wint_t>(m_peekSymbol)) && m_peekSymbol != '_')
			break;

		advance(m_symbol);
		word += m_symbol;
	}

	return true;
}

TextLocation LexicalAnalyzer::get_current_location(std::wstring& word)
{
	int length = static_cast<int>(word.length());
	int startOffset = m_offset - length + 1;
	if (startOffset < 1)
		startOffset = 1;

	return TextLocation(m_sourceText->get_name(), m_line, startOffset, length);
}

bool LexicalAnalyzer::read_next_word(std::wstring& word, TokenType& type)
{
	type = TokenType::Unknown;
	word = L"";

	// Reading next non whitespace character
	if (!read_next_real())
	{
		type = TokenType::EndOfFile;
		return false;
	}

	if (is_punctuation(word, type))
		return true;

	if (is_operator(word, type))
	{
		// Handling comment
		if (type == TokenType::Trivia)
		{
			while (advance(m_peekSymbol))
			{
				switch (m_peekSymbol)
				{
					case L'\n':
					{
						return read_next_word(word, type);
					}

					case L'\0':
					{
						if (!m_sourceText->peek_next(m_peekSymbol))
							return false;

						if (m_peekSymbol == L'\0')
							return false; // EOF

						continue;
					}

					default:
						continue;
				}
			}

			return false; // EOF
		}

		return true;
	}

	bool wasClosed = false;
	bool dontEcran = false;
	if (is_string_tliteral(type, dontEcran))
		return read_string_tliteral(word, dontEcran, wasClosed);

	if (is_char_literal(type, dontEcran))
		return read_char_literal(word, dontEcran, wasClosed);

	if (is_number_literal(type))
		return read_number_literal(word, type);

	// Reading next non whitespace word
	if (!read_while_alpha(word))
		return false;

	if (is_boolean_literal(word, type))
		return true;

	if (is_null_literal(word, type))
		return true;

	if (is_directive_decl(word, type))
		return true;

	if (is_type_decl(word, type))
		return true;

	if (is_modifier(word, type))
		return true;

	if (is_word_operator(word, type))
		return true;

	if (is_type(word, type))
		return true;

	if (is_keyword(word, type))
		return true;

	type = TokenType::Identifier;
	return true;
}

bool LexicalAnalyzer::read_char_literal(std::wstring& word, bool notEcran, bool& wasClosed)
{
	bool ecran = false;

	while (advance(m_symbol))
	{
		if (ecran)
		{
			switch (m_symbol)
			{
				case L'\'': word += L'\''; break;
				case L'\\': word += L'\\'; break;
				case L'n':  word += L'\n'; break;
				case L't':  word += L'\t'; break;
				case L'r':  word += L'\r'; break;
				case L'0':  word += L'\0'; break;
				default:    word += m_symbol; break;
			}

			ecran = false;
			continue;
		}

		switch (m_symbol)
		{
			case L'\'':
			{
				wasClosed = true;
				m_sourceText->peek_next(m_peekSymbol);
				return true;
			}

			case L'\\':
			{
				ecran = true;
				continue;
			}

			default:
			{
				word += m_symbol;
				continue;
			}
		}
	}

	return false;
}

bool LexicalAnalyzer::read_string_tliteral(std::wstring& word, bool dontEcran, bool& wasClosed)
{
	bool ecran = false;
	while (advance(m_symbol))
	{
		switch (m_symbol)
		{
			case L'\\':
			{
				if (dontEcran)
				{
					word += m_symbol;
					break;
				}

				if (ecran)
				{
					m_symbol = L'\\';
					word += m_symbol;
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
					m_sourceText->peek_next(m_peekSymbol);
					return true;
				}

				if (ecran)
				{
					m_symbol = L'"';
					word += m_symbol;
					ecran = false;
					continue;
				}

				wasClosed = true;
				m_sourceText->peek_next(m_peekSymbol);
				return true;
			}

			case L'n':
			{
				if (ecran)
				{
					m_symbol = L'\n';
					ecran = false;
				}

				word += m_symbol;
				break;
			}

			default:
			{
				word += m_symbol;
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

	// Base prefixes
	if (symbol == L'x' || symbol == L'X')
		return true;

	if (symbol == L'd' || symbol == L'D')
		return true;

	if (symbol == L'b' || symbol == L'B')
		return true;

	// Volume ratio suffix first letters (k, m, g, t, p)
	if (symbol == L'k' || symbol == L'K')
		return true;

	if (symbol == L'm' || symbol == L'M')
		return true;

	if (symbol == L'g' || symbol == L'G')
		return true;

	if (symbol == L't' || symbol == L'T')
		return true;

	if (symbol == L'p' || symbol == L'P')
		return true;

	return false;
}

static bool IsBasePrefixLetter(wchar_t symbol)
{
	switch (symbol)
	{
		case L'x': case L'X':
		case L'd': case L'D':
		case L'b': case L'B':
			return true;

		default:
			return false;
	}
}

bool LexicalAnalyzer::read_number_literal(std::wstring& word, TokenType& type)
{
	if (IsNumberSymbol(m_symbol))
		word += m_symbol;

	// Consume a base prefix immediately after a standalone 0, e.g. 0x, 0b, 0d.
	if (word == L"0" && m_sourceText->peek_next(m_peekSymbol) && IsBasePrefixLetter(m_peekSymbol))
	{
		advance(m_peekSymbol);
		word += m_peekSymbol;
	}

	bool foundDelimeter = false;
	while (m_sourceText->peek_next(m_peekSymbol))
	{
		if (m_peekSymbol == '`')
		{
			if (!advance(m_peekSymbol))
				break;

			if (!m_sourceText->peek_next(m_peekSymbol))
				break;

			if (IsNumberSymbol(m_peekSymbol))
				continue;
		}

		if (m_peekSymbol == '.')
		{
			if (foundDelimeter)
				break;

			// Tentatively consume the dot so we can inspect the following character.
			if (!advance(m_peekSymbol))
				break;

			wchar_t afterDot;
			if (!m_sourceText->peek_next(afterDot) || !IsNumberSymbol(afterDot))
			{
				// The dot does not start a fractional part (e.g. range operator ".."), o put it back for the next token.
				m_hasPutbackSymbol = true;
				m_putbackSymbol = L'.';
				m_offset -= 1;
				break;
			}

			advance(m_peekSymbol);

			foundDelimeter = true;
			type = TokenType::DoubleLiteral;

			word += '.';
			word += afterDot;
			continue;
		}

		if (IsNumberSymbol(m_peekSymbol))
		{
			word += m_peekSymbol;
			advance(m_peekSymbol);
			continue;
		}

		// not literals' char ahead
		break;
	}

	return word.size() > 0;
}

bool LexicalAnalyzer::is_punctuation(std::wstring& word, TokenType& type)
{
	switch (m_symbol)
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '.')
				{
					advance(m_peekSymbol);
					if (m_sourceText->peek_next(m_peekSymbol) && m_peekSymbol == '&')
					{
						advance(m_peekSymbol);
						type = TokenType::RangeInclusiveOperator;
						word = L"..&";
						return true;
					}

					type = TokenType::RangeOperator;
					word = L"..";
					return true;
				}
			}

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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::DeclareAssignOperator;
					word = L":=";
					return true;
				}
			}
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '?')
				{
					advance(m_peekSymbol);
					type = TokenType::NullCoalescingOperator;
					word = L"??";
					return true;
				}
			}

			type = TokenType::Question;
			word = L"?";
			return true;
		}

		case L'\u037E': // greek question mark, looks exactly like semicolon
		{
			type = TokenType::Semicolon;
			word = L";";
			return true;
		}

		default:
		{
			return false;
		}
	}
}

bool LexicalAnalyzer::is_word_operator(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::is_operator(std::wstring& word, TokenType& type)
{
	switch (m_symbol)
	{
		case '=':
		{
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::EqualsOperator;
					word = L"==";
					return true;
				}
				else if (m_peekSymbol == '>')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::GreaterOrEqualsOperator;
					word = L">=";
					return true;
				}
				else if (m_peekSymbol == '>')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::LessOrEqualsOperator;
					word = L"<=";
					return true;
				}
				else if (m_peekSymbol == '<')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::AddAssignOperator;
					word = L"+=";
					return true;
				}
				else if (m_peekSymbol == '+')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::SubAssignOperator;
					word = L"-=";
					return true;
				}
				else if (m_peekSymbol == '-')
				{
					advance(m_peekSymbol);
					type = TokenType::DecrementOperator;
					word = L"--";
					return true;
				}
				else if (m_peekSymbol == '>')
				{
					advance(m_peekSymbol);
					type = TokenType::ArrowOperator;
					word = L"->";
					return true;
				}
			}

			word = L"-";
			type = TokenType::SubOperator;
			return true;
		}

		case '*':
		{
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::DivAssignOperator;
					word = L"/=";
					return true;
				}
				else if (m_peekSymbol == '/')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::AndAssignOperator;
					word = L"&=";
					return true;
				}
				else if (m_peekSymbol == '&')
				{
					advance(m_peekSymbol);
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
			if (m_sourceText->peek_next(m_peekSymbol))
			{
				if (m_peekSymbol == '=')
				{
					advance(m_peekSymbol);
					type = TokenType::OrAssignOperator;
					word = L"|=";
					return true;
				}
				else if (m_peekSymbol == '|')
				{
					advance(m_peekSymbol);
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

bool LexicalAnalyzer::is_null_literal(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::is_boolean_literal(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::is_number_literal(TokenType& type) const
{
	if (m_symbol >= L'0' && m_symbol <= L'9')
	{
		type = TokenType::NumberLiteral;
		return true;
	}
	else
	{
		return false;
	}
}

bool LexicalAnalyzer::is_char_literal(TokenType& type, bool& dontEcran)
{
	switch (m_symbol)
	{
		case L'\'':
		{
			type = TokenType::CharLiteral;
			return true;
		}

		default:
			return false;
	}
}

bool LexicalAnalyzer::is_string_tliteral(TokenType& type, bool& dontEcran)
{
	switch (m_symbol)
	{
		case L'@':
		{
			if (!m_sourceText->peek_next(m_peekSymbol))
				return false;

			if (m_peekSymbol != '"')
				return false;

			advance(m_peekSymbol);
			dontEcran = true;
			return is_string_tliteral(type, dontEcran);
		}

		case L'"':
		{
			type = TokenType::StringLiteral;
			return true;
		}

		default:
			return false;
	}
}

bool LexicalAnalyzer::is_modifier(std::wstring& word, TokenType& type)
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
	else if (word == L"async")
	{
		type = TokenType::AsyncKeyword;
		return true;
	}
	else if (word == L"extern")
	{
		type = TokenType::ExternKeyword;
		return true;
	}
	else if (word == L"export")
	{
		type = TokenType::ExportKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool LexicalAnalyzer::is_directive_decl(std::wstring& word, TokenType& type)
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

bool LexicalAnalyzer::is_type_decl(std::wstring& word, TokenType& type)
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
	else if (word == L"enum")
	{
		type = TokenType::EnumKeyword;
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

bool LexicalAnalyzer::is_type(std::wstring& word, TokenType& type)
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
	else if (word == L"char")
	{
		type = TokenType::CharKeyword;
		return true;
	}
	else if (word == L"byte")
	{
		type = TokenType::ByteKeyword;
		return true;
	}
	else if (word == L"nint")
	{
		type = TokenType::NativeIntegerKeyword;
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

bool LexicalAnalyzer::is_keyword(std::wstring& word, TokenType& type)
{
	if (is_conditional_keyword(word, type))
		return true;

	if (is_functional_keyword(word, type))
		return true;

	if (is_loop_keyword(word, type))
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
	else if (word == L"try")
	{
		type = TokenType::TryKeyword;
		return true;
	}
	else if (word == L"catch")
	{
		type = TokenType::CatchKeyword;
		return true;
	}
	else if (word == L"defer")
	{
		type = TokenType::DeferKeyword;
		return true;
	}
	else if (word == L"where")
	{
		type = TokenType::WhereKeyword;
		return true;
	}
	else if (word == L"const")
	{
		type = TokenType::ConstKeyword;
		return true;
	}
	else if (word == L"func")
	{
		type = TokenType::FunctionKeyword;
		return true;
	}
	else if (word == L"init")
	{
		type = TokenType::InitKeyword;
		return true;
	}
	else if (word == L"operator")
	{
		type = TokenType::OperatorKeyword;
		return true;
	}
	else if (word == L"value")
	{
		type = TokenType::ValueKeyword;
		return true;
	}
	else if (word == L"is")
	{
		type = TokenType::IsOperator;
		return true;
	}
	else if (word == L"as")
	{
		type = TokenType::AsOperator;
		return true;
	}

	return false;
}

bool LexicalAnalyzer::is_conditional_keyword(std::wstring& word, TokenType& type)
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
	else if (word == L"switch")
	{
		type = TokenType::SwitchKeyword;
		return true;
	}
	else if (word == L"case")
	{
		type = TokenType::CaseKeyword;
		return true;
	}
	else if (word == L"default")
	{
		type = TokenType::DefaultKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool LexicalAnalyzer::is_loop_keyword(std::wstring& word, TokenType& type)
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
	else if (word == L"in")
	{
		type = TokenType::InKeyword;
		return true;
	}
	else
	{
		return false;
	}
}

bool LexicalAnalyzer::is_functional_keyword(std::wstring& word, TokenType& type)
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
	else if (word == L"try")
	{
		type = TokenType::TryKeyword;
		return true;
	}
	else if (word == L"catch")
	{
		type = TokenType::CatchKeyword;
		return true;
	}
	else if (word == L"await")
	{
		type = TokenType::AwaitKeyword;
		return true;
	}
	else
	{
		return false;
	}
}
