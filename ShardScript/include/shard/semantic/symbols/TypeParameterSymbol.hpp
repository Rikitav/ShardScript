#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Arena.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace shard
{
	class SHARD_API TypeParameterSymbol final : public StructSymbol
	{
		std::vector<gmt::Ref<StructSymbol>> m_constraints;
		std::uint16_t m_typeArgumentIndex = 0;

	public:
		TypeParameterSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		TypeParameterSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~TypeParameterSymbol() = default;

		TypeParameterSymbol(const TypeParameterSymbol& other) = delete;
		TypeParameterSymbol& operator=(const TypeParameterSymbol& other) = delete;

		TypeParameterSymbol(TypeParameterSymbol&& other) = delete;
		TypeParameterSymbol& operator=(TypeParameterSymbol&& other) = delete;

		const std::vector<gmt::Ref<StructSymbol>>& get_constraints() const;
		void add_constraint(gmt::Ref<StructSymbol> constraint);

		std::uint16_t get_type_argument_index() const;
		void set_type_argument_index(std::uint16_t index);
	};
}
