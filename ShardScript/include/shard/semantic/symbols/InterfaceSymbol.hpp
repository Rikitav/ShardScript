#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <string_view>

namespace shard
{
	class SHARD_API InterfaceSymbol final : public StructSymbol
	{
	public:
		InterfaceSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		InterfaceSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~InterfaceSymbol() = default;

		InterfaceSymbol(const InterfaceSymbol& other) = delete;
		InterfaceSymbol& operator=(const InterfaceSymbol& other) = delete;

		InterfaceSymbol(InterfaceSymbol&& other) = delete;
		InterfaceSymbol& operator=(InterfaceSymbol&& other) = delete;
	};
}
