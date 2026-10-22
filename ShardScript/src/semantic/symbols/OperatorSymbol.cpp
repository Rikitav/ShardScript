#include <shard/semantic/symbols/OperatorSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

OperatorSymbol::OperatorSymbol(std::wstring_view name, const TokenType operatorToken, gmt::Ref<SyntaxSymbol> parent)
	: OperatorSymbol(gmt::intern(name), operatorToken, parent) { }

OperatorSymbol::OperatorSymbol(gmt::StringReference name, const TokenType operatorToken, gmt::Ref<SyntaxSymbol> parent)
	: MethodSymbol(name, SyntaxKind::OperatorDeclaration, parent), m_operatorToken(operatorToken) { }

TokenType OperatorSymbol::get_operator_token() const
{
	return m_operatorToken;
}

void OperatorSymbol::set_operator_token(const TokenType operatorToken)
{
	m_operatorToken = operatorToken;
}