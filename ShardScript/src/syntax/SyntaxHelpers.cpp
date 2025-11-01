#include <shard/syntax/SyntaxHelpers.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <vector>

using namespace std;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

void SetAccesibility(SyntaxSymbol* symbol, vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}
		}
	}
}

void SetAccesibility(TypeSymbol* symbol, vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}

			case TokenType::StaticKeyword:
			{
				symbol->IsStatic = true;
				break;
			}

			case TokenType::AbstractKeyword:
			{
				symbol->IsAbstract = true;
				break;
			}

			case TokenType::SealedKeyword:
			{
				symbol->IsSealed = true;
				break;
			}
		}
	}
}

void SetAccesibility(MethodSymbol* symbol, vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}

			case TokenType::StaticKeyword:
			{
				symbol->IsStatic = true;
				break;
			}

			case TokenType::OverrideKeyword:
			{
				symbol->IsOverride = true;
				break;
			}

			case TokenType::VirtualKeyword:
			{
				symbol->IsVirtual = true;
				break;
			}
		}
	}
}

void SetAccesibility(FieldSymbol* symbol, vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}

			case TokenType::StaticKeyword:
			{
				symbol->IsStatic = true;
				break;
			}
		}
	}
}
