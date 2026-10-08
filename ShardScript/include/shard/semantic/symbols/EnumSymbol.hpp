#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <string_view>

namespace shard
{
	class SHARD_API EnumSymbol final : public StructSymbol
	{
		bool m_isFlags = false;

	public:
		EnumSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		EnumSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~EnumSymbol() = default;

		EnumSymbol(const EnumSymbol& other) = delete;
		EnumSymbol& operator=(const EnumSymbol& other) = delete;

		EnumSymbol(EnumSymbol&& other) = delete;
		EnumSymbol& operator=(EnumSymbol&& other) = delete;

		bool get_is_flags() const;
		void set_is_flags(bool is_flags);
	};
}
