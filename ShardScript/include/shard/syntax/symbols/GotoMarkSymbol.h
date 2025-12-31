#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <string>

namespace shard
{
	class SHARD_API GotoMarkSymbol : public SyntaxSymbol
	{
	public:
		size_t BlockIndex = -1;

		inline GotoMarkSymbol(std::wstring name) : SyntaxSymbol(name, SyntaxKind::GotoMarkStatement)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline GotoMarkSymbol(const GotoMarkSymbol& other) = delete;

		inline virtual ~GotoMarkSymbol() override
		{
		}
	};
}