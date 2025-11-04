#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <string>

namespace shard::syntax
{
	class SyntaxSymbol
	{
		inline static int counter = 0;

	public:
		const int TypeCode;
		const std::wstring Name;
		const SyntaxKind Kind;
		SymbolAccesibility Accesibility = SymbolAccesibility::Private;

		inline SyntaxSymbol(const std::wstring& name, const SyntaxKind kind)
			: TypeCode(counter++), Name(name), Kind(kind) { }

		inline virtual ~SyntaxSymbol()
		{

		}
	};
}