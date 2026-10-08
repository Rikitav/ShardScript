#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Interner.hpp>
#include <gmt/Ref.hpp>

#include <string_view>

namespace shard
{
	class StructSymbol;
	class ExpressionSyntax;

	class SHARD_API ParameterSymbol : public SyntaxSymbol
	{
		gmt::Ref<StructSymbol> m_type;

	public:
		ParameterSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		ParameterSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent);
		virtual ~ParameterSymbol() = default;

		ParameterSymbol(const ParameterSymbol& other) = delete;
		ParameterSymbol& operator=(const ParameterSymbol& other) = delete;

		ParameterSymbol(ParameterSymbol&& other) = delete;
		ParameterSymbol& operator=(ParameterSymbol&& other) = delete;

		gmt::Ref<StructSymbol> get_type() const;
		void set_type(gmt::Ref<StructSymbol> type);
	};
}
