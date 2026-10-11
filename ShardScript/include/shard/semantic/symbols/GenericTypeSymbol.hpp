#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/FlatMap.hpp>
#include <gmt/Interner.hpp>
#include <gmt/Ref.hpp>

namespace shard
{
	class TypeParameterSymbol;

	class SHARD_API GenericTypeSymbol final : public StructSymbol
	{
		gmt::Ref<StructSymbol> m_underlayingType;
		gmt::FlatMap<gmt::StringReference, gmt::Ref<StructSymbol>> m_typeArguments;

	public:
		GenericTypeSymbol(gmt::Ref<StructSymbol> underlaying_type, gmt::Ref<SyntaxSymbol> parent);
		virtual ~GenericTypeSymbol() = default;

		GenericTypeSymbol(const GenericTypeSymbol& other) = delete;
		GenericTypeSymbol& operator=(const GenericTypeSymbol& other) = delete;

		GenericTypeSymbol(GenericTypeSymbol&& other) = delete;
		GenericTypeSymbol& operator=(GenericTypeSymbol&& other) = delete;

		gmt::Ref<StructSymbol> get_underlaying_type() const;

		bool add_type_parameter(gmt::Ref<TypeParameterSymbol> type_parameter, gmt::Ref<StructSymbol> argument_type);
		gmt::Ref<StructSymbol> substitute_type_parameter(gmt::Ref<TypeParameterSymbol> type_parameter) const;
	};
}
