#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <span>
#include <vector>

namespace shard
{
	class StructSymbol;

	class SHARD_API GenericMethodSymbol final : public MethodSymbol
	{
		gmt::Ref<MethodSymbol> m_underlayingMethod;

		// combined index space: owner type arguments first, then the method's own
		// type arguments; matches TypeParameterSymbol::get_type_argument_index
		gmt::vector<gmt::Ref<StructSymbol>> m_typeArguments;

	public:
		GenericMethodSymbol(gmt::Ref<MethodSymbol> underlaying_method, gmt::Ref<SyntaxSymbol> parent);
		virtual ~GenericMethodSymbol() = default;

		GenericMethodSymbol(const GenericMethodSymbol& other) = delete;
		GenericMethodSymbol& operator=(const GenericMethodSymbol& other) = delete;

		GenericMethodSymbol(GenericMethodSymbol&& other) = delete;
		GenericMethodSymbol& operator=(GenericMethodSymbol&& other) = delete;

		gmt::Ref<MethodSymbol> get_underlaying_method() const;

		bool get_is_generic_instantiation() const override;

		const gmt::vector<gmt::Ref<StructSymbol>>& get_type_arguments() const;
		void set_type_arguments(std::span<const gmt::Ref<StructSymbol>> type_arguments);
	};
}
