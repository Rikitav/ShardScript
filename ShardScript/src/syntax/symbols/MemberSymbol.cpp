#include <shard/syntax/symbols/MemberSymbol.hpp>

using namespace shard;

void MemberSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	
}

bool MemberSymbol::IsType() const
{
	return false;
}

bool MemberSymbol::IsMember() const
{
	return true;
}
