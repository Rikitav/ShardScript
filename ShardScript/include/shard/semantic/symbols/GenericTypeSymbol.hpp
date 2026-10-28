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
		StructSymbol* m_underlayingType = nullptr;
		gmt::FlatMap<gmt::StringReference, StructSymbol*> m_typeArguments;

	public:
		GenericTypeSymbol(StructSymbol* underlaying_type, SyntaxSymbol* parent);
		virtual ~GenericTypeSymbol() = default;

		GenericTypeSymbol(const GenericTypeSymbol& other) = delete;
		GenericTypeSymbol& operator=(const GenericTypeSymbol& other) = delete;

		GenericTypeSymbol(GenericTypeSymbol&& other) = delete;
		GenericTypeSymbol& operator=(GenericTypeSymbol&& other) = delete;

		StructSymbol* get_underlaying_type() const;

		bool add_type_parameter(TypeParameterSymbol* type_parameter, StructSymbol* argument_type);
		StructSymbol* substitute_type_parameter(TypeParameterSymbol* type_parameter) const;
	};
}