#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <string>
#include <cctype>
#include <vector>
#include <memory>

using namespace std;
using namespace shard::syntax;
using namespace shard::syntax::analysis;

namespace shard::parsing
{
	class SourceReader
	{
	protected:
		shared_ptr<SyntaxToken> PeekBuffer;
		shared_ptr<SyntaxToken> ConsumeBuffer;

		int Line;
		int Offset;
		char Symbol;
		char PeekSymbol;

	public:
		SourceReader();

		virtual SyntaxToken Current();
		virtual SyntaxToken Consume();
		virtual SyntaxToken Peek();
		
		virtual bool CanConsume();
		virtual bool CanPeek();
		virtual TextLocation GetLocation(string word) = 0;

	protected:
		virtual bool ReadNextToken(shared_ptr<SyntaxToken>& pToken);

		virtual bool ReadNext() = 0;
		virtual bool PeekNext() = 0;

		virtual bool ReadNextReal();
		virtual bool ReadNextWord(string& word, TokenType& type);
		virtual bool ReadNextWhileAlpha(string& word);

		bool ReadStringLiteral(string& word, bool notEcran, bool& wasClosed);
		bool ReadNumberLiteral(string& word);

		bool IsPunctuation(string& word, TokenType& type)
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

		bool IsOperator(string& word, TokenType& type)
		{
			switch (Symbol)
			{
				case '=':
				{
					if (PeekNext())
					{
						if (PeekSymbol == '=')
						{
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
							type = TokenType::NotEqualsOperator;
							word = "!=";
							return true;
						}
					}

					//type = TokenType::NullForgivingOperator;
					//word = "!";
					//return true;
					return false;
				}

				case '>':
				{
					if (PeekNext())
					{
						if (PeekSymbol == '=')
						{
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
					word = "+";
					type = TokenType::AddOperator;
					return true;
				}

				case '-':
				{
					word = "-";
					type = TokenType::SubOperator;
					return true;
				}

				case '*':
				{
					word = "*";
					type = TokenType::MultOperator;
					return true;
				}

				case '/':
				{
					word = "/";
					type = TokenType::DivOperator;
					return true;
				}

				case '%':
				{
					word = "%";
					type = TokenType::ModOperator;
					return true;
				}

				case '^':
				{
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

		bool IsBooleanLiteral(string& word, TokenType& type)
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

		bool IsNumberLiteral(string& word, TokenType& type)
		{
			if (isdigit(Symbol))
			{
				type = TokenType::NumberLiteral;
				return true;
			}

			return false;
		}

		bool IsStringLiteral(string& word, TokenType& type, bool& dontEcran)
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
					return IsStringLiteral(word, type, dontEcran);
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

		bool IsModifier(string word, TokenType& type)
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

		bool IsTypeDecl(string word, TokenType& type)
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

		bool IsType(string word, TokenType& type)
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
	};
}