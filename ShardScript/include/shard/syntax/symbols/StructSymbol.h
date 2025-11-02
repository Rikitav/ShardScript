#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/SyntaxKind.h>
#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class StructSymbol : public TypeSymbol
	{
	public:
		std::vector<MethodSymbol*> Constructors;

		inline StructSymbol(const std::wstring& name) : TypeSymbol(name, SyntaxKind::StructDeclaration)
		{
			MemoryBytesSize += 0;
			IsValueType = true;
		}
	};
}
