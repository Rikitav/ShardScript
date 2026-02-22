#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <string>

namespace shard
{
	class SHARD_API TypeParameterSymbol : public TypeSymbol
	{
	public:
		// Для type parameters без ограничений возвращаем Any
		// В будущем здесь можно добавить ограничения (constraints)
		//TypeSymbol* ConstraintType = nullptr;

		inline TypeParameterSymbol(std::wstring name) : TypeSymbol(name, SyntaxKind::TypeParameter)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline TypeParameterSymbol(const TypeParameterSymbol& other) = delete;

		inline virtual ~TypeParameterSymbol() override = default;
	};
}
