#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

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
