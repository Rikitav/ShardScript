#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>
#include <vector>

namespace shard::syntax::symbols
{
	class SHARD_API FFISymbol : public SyntaxSymbol
	{
	public:
		std::vector<MethodSymbol*> Methods;

		inline FFISymbol() : SyntaxSymbol(L"", SyntaxKind::DllImportDirective) {}

		inline FFISymbol(const FFISymbol& other) = delete;

		inline virtual ~FFISymbol() override
		{
			for (MethodSymbol* method : Methods)
				delete method;
		}
	};
}
