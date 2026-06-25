#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <cstdint>
#include <string>

namespace shard
{
	class TypeSymbol;
}

namespace shard
{
	class SHARD_API TypeParameterSymbol : public TypeSymbol
	{
	public:
		//TypeSymbol* ConstraintType = nullptr;

		std::uint16_t TypeArgumentIndex = 0;

		inline TypeParameterSymbol(std::wstring name) : TypeSymbol(name, SyntaxKind::TypeParameter)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline TypeParameterSymbol(const TypeParameterSymbol& other) = delete;

		inline virtual ~TypeParameterSymbol() override = default;
	};
}
