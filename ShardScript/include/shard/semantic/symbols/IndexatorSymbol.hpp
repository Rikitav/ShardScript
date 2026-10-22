#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>

#include <string_view>
#include <vector>

namespace shard
{
	class ParameterSymbol;

	class SHARD_API IndexatorSymbol final : public PropertySymbol
	{
		gmt::vector<gmt::Ref<ParameterSymbol>> m_parameters;

	public:
		IndexatorSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		IndexatorSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~IndexatorSymbol() = default;

		IndexatorSymbol(const IndexatorSymbol& other) = delete;
		IndexatorSymbol& operator=(const IndexatorSymbol& other) = delete;

		IndexatorSymbol(IndexatorSymbol&& other) = delete;
		IndexatorSymbol& operator=(IndexatorSymbol&& other) = delete;

		const gmt::vector<gmt::Ref<ParameterSymbol>>& get_parameters() const;

		void on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol) override;
	};
}
