#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <shard/lexical/LexicalAnalyzer.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Interner.hpp>

#include <cctype>
#include <cwctype>
#include <iterator>
#include <string>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

using namespace shard;

namespace
{
	// ASCII is the overwhelmingly common case; only fall back to the
	// locale-aware cwctype calls for non-ASCII code points
	bool is_space_character(wchar_t ch)
	{
		if (ch == L' ' || (ch >= L'\t' && ch <= L'\r'))
			return true;

		return ch > 0x7F && std::iswspace(static_cast<wint_t>(ch)) != 0;
	}

	bool is_word_character(wchar_t ch)
	{
		if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') ||
			(ch >= L'0' && ch <= L'9') || ch == L'_')
			return true;

		return ch > 0x7F && std::iswalnum(static_cast<wint_t>(ch)) != 0;
	}

	struct KeywordEntry
	{
		std::wstring_view text;
		TokenType type;
	};

	// every word token the lexer classifies, in one table; classification
	// used to run up to eight sequential scans over separate literal lists
	constexpr KeywordEntry KeywordTable[] =
	{
		{ L"true", TokenType::BooleanLiteral },
		{ L"false", TokenType::BooleanLiteral },
		{ L"null", TokenType::NullLiteral },
		{ L"nil", TokenType::NullLiteral },

		{ L"using", TokenType::UsingKeyword },
		{ L"namespace", TokenType::NamespaceKeyword },

		{ L"method", TokenType::MethodKeyword },
		{ L"class", TokenType::ClassKeyword },
		{ L"struct", TokenType::StructKeyword },
		{ L"interface", TokenType::InterfaceKeyword },
		{ L"enum", TokenType::EnumKeyword },

		{ L"public", TokenType::PublicKeyword },
		{ L"private", TokenType::PrivateKeyword },
		{ L"protected", TokenType::ProtectedKeyword },
		{ L"internal", TokenType::InternalKeyword },
		{ L"static", TokenType::StaticKeyword },
		{ L"async", TokenType::AsyncKeyword },

		{ L"and", TokenType::AndOperator },
		{ L"or", TokenType::OrOperator },
		{ L"not", TokenType::NotOperator },

		{ L"void", TokenType::VoidKeyword },
		{ L"var", TokenType::VarKeyword },
		{ L"bool", TokenType::BooleanKeyword },
		{ L"string", TokenType::StringKeyword },
		{ L"double", TokenType::DoubleKeyword },
		{ L"int", TokenType::IntegerKeyword },
		{ L"char", TokenType::CharKeyword },
		{ L"byte", TokenType::ByteKeyword },
		{ L"nint", TokenType::NativeIntegerKeyword },
		{ L"lambda", TokenType::LambdaKeyword },
		{ L"delegate", TokenType::DelegateKeyword },
		{ L"flags", TokenType::FlagsKeyword },

		{ L"if", TokenType::IfKeyword },
		{ L"unless", TokenType::UnlessKeyword },
		{ L"else", TokenType::ElseKeyword },
		{ L"switch", TokenType::SwitchKeyword },
		{ L"case", TokenType::CaseKeyword },
		{ L"default", TokenType::DefaultKeyword },

		{ L"return", TokenType::ReturnKeyword },
		{ L"continue", TokenType::ContinueKeyword },
		{ L"break", TokenType::BreakKeyword },
		{ L"throw", TokenType::ThrowKeyword },
		{ L"try", TokenType::TryKeyword },
		{ L"catch", TokenType::CatchKeyword },
		{ L"await", TokenType::AwaitKeyword },

		{ L"for", TokenType::ForKeyword },
		{ L"while", TokenType::WhileKeyword },
		{ L"until", TokenType::UntilKeyword },
		{ L"do", TokenType::DoKeyword },
		{ L"foreach", TokenType::ForeachKeyword },
		{ L"in", TokenType::InKeyword },

		{ L"new", TokenType::NewKeyword },
		{ L"get", TokenType::GetKeyword },
		{ L"set", TokenType::SetKeyword },
		{ L"this", TokenType::ThisKeyword },
		{ L"self", TokenType::SelfKeyword },
		{ L"field", TokenType::FieldKeyword },
		{ L"indexer", TokenType::IndexerKeyword },
		{ L"defer", TokenType::DeferKeyword },
		{ L"where", TokenType::WhereKeyword },
		{ L"const", TokenType::ConstKeyword },
		{ L"func", TokenType::FunctionKeyword },
		{ L"init", TokenType::InitKeyword },
		{ L"operator", TokenType::OperatorKeyword },
		{ L"value", TokenType::ValueKeyword },
		{ L"is", TokenType::IsOperator },
		{ L"as", TokenType::AsOperator },
	};

	TokenType classify_word(std::wstring_view word)
	{
		static const std::unordered_map<std::wstring_view, TokenType> table = []
		{
			std::unordered_map<std::wstring_view, TokenType> entries;
			entries.reserve(std::size(KeywordTable));
			for (const KeywordEntry& entry : KeywordTable)
				entries.emplace(entry.text, entry.type);
			return entries;
		}();

		const auto choice = table.find(word);
		return choice != table.end() ? choice->second : TokenType::Identifier;
	}
}

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
{
	m_fileName = gmt::resolve(gmt::intern(m_sourceText->get_name()));
}

LexicalAnalyzer::LexicalAnalyzer(SourceTextProvider* sourceText, bool ownsProvider) :
	m_ownsSourceTextProvider(ownsProvider),
	m_sourceText(sourceText)
{
	m_fileName = gmt::resolve(gmt::intern(m_sourceText->get_name()));
}

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
			return token;
		}

		default: // read peeks
		{
			SyntaxToken token = m_readBuffer.front();
			m_readBuffer.pop_front();
			return token;
		}
	}

	SyntaxToken consumeBuffer{};
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
			SyntaxToken peekToken{};
			if (read_next_token(peekToken))
			{
				m_readBuffer.push_back(peekToken);
				continue;
			}

			peekToken = SyntaxToken(TokenType::EndOfFile, L"", TextLocation());
			m_readBuffer.push_back(peekToken);
			return peekToken;
		}
	}

	if (index < 0 || index >= static_cast<int>(m_readBuffer.size()))
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return m_readBuffer.at(index);
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
	gmt::wstring word = L"";
	TokenType type = TokenType::Unknown;

	if (!read_next_word(word, type))
		return false;

	token = SyntaxToken(type, gmt::intern(word), get_current_location(word));
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
				if (!is_space_character(m_symbol))
					return true;

				continue;
			}
		}
	}

	// EOF
	return false;
}

bool LexicalAnalyzer::read_while_alpha(gmt::wstring& word)
{
	word += m_symbol;
	while (m_sourceText->peek_next(m_peekSymbol))
	{
		if (!is_word_character(m_peekSymbol))
			break;

		advance(m_symbol);
		word += m_symbol;
	}

	return true;
}

TextLocation LexicalAnalyzer::get_current_location(gmt::wstring& word)
{
	int length = static_cast<int>(word.size());
	int startOffset = m_offset - length + 1;

	if (startOffset < 1)
		startOffset = 1;

	return TextLocation(m_fileName, m_line, startOffset, length);
}

bool LexicalAnalyzer::read_next_word(gmt::wstring& word, TokenType& type)
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
	if (is_string_literal(type, dontEcran))
		return read_string_literal(word, dontEcran, wasClosed);

	if (is_char_literal(type, dontEcran))
		return read_char_literal(word, dontEcran, wasClosed);

	if (is_number_literal(type))
		return read_number_literal(word, type);

	// Reading next non whitespace word
	if (!read_while_alpha(word))
		return false;

	type = classify_word(word);
	return true;
}

bool LexicalAnalyzer::read_char_literal(gmt::wstring& word, bool notEcran, bool& wasClosed)
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

bool LexicalAnalyzer::read_string_literal(gmt::wstring& word, bool dontEcran, bool& wasClosed)
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

bool LexicalAnalyzer::read_number_literal(gmt::wstring& word, TokenType& type)
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
			if (!m_sourceText->peek_next(afterDot) || afterDot < L'0' || afterDot > L'9')
			{
				// The dot does not start a fractional part (a range operator
				// '..' or member access '11.ToString'), so put it back for
				// the next token.
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

bool LexicalAnalyzer::is_punctuation(gmt::wstring& word, TokenType& type)
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

				if (m_peekSymbol == ':')
				{
					advance(m_peekSymbol);
					type = TokenType::NamespaceQualifier;
					word = L"::";
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

bool LexicalAnalyzer::is_operator(gmt::wstring& word, TokenType& type)
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
			/*
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
			*/

			word = L"&";
			type = TokenType::AmpersandOperator;
			return true;
		}

		case '|':
		{
			/*
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
			*/

			word = L"|";
			type = TokenType::PipeOperator;
			return true;
		}

		default:
		{
			return false;
		}
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

bool LexicalAnalyzer::is_string_literal(TokenType& type, bool& dontEcran)
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
			return is_string_literal(type, dontEcran);
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
