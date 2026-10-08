#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <string_view>

namespace shard
{
	class SHARD_API AccessorSymbol final : public MethodSymbol
	{
	public:
		AccessorSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		AccessorSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~AccessorSymbol() = default;

		AccessorSymbol(const AccessorSymbol& other) = delete;
		AccessorSymbol& operator=(const AccessorSymbol& other) = delete;

		AccessorSymbol(AccessorSymbol&& other) = delete;
		AccessorSymbol& operator=(AccessorSymbol&& other) = delete;
	};
}
