#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class FFISymbol : public SyntaxSymbol
	{
	public:
		std::vector<MethodSymbol*> Methods;

		inline FFISymbol() : SyntaxSymbol(L"", SyntaxKind::DllImportDirective) {}

		MethodSymbol* FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes);
	};
}
