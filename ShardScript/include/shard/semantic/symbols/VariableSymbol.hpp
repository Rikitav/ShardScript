#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <string_view>

namespace shard
{
	class StructSymbol;

	class SHARD_API VariableSymbol final : public SyntaxSymbol
	{
		gmt::Ref<StructSymbol> m_type;

	public:
		VariableSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		VariableSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~VariableSymbol() = default;

		VariableSymbol(const VariableSymbol& other) = delete;
		VariableSymbol& operator=(const VariableSymbol& other) = delete;

		VariableSymbol(VariableSymbol&& other) = delete;
		VariableSymbol& operator=(VariableSymbol&& other) = delete;

		gmt::Ref<StructSymbol> get_type() const;
		void set_type(gmt::Ref<StructSymbol> type);
	};
}
