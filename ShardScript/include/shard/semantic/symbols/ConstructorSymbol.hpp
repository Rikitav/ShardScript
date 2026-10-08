#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <string_view>

namespace shard
{
	class SHARD_API ConstructorSymbol final : public MethodSymbol
	{
	public:
		ConstructorSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		ConstructorSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~ConstructorSymbol() = default;

		ConstructorSymbol(const ConstructorSymbol& other) = delete;
		ConstructorSymbol& operator=(const ConstructorSymbol& other) = delete;

		ConstructorSymbol(ConstructorSymbol&& other) = delete;
		ConstructorSymbol& operator=(ConstructorSymbol&& other) = delete;
	};
}
