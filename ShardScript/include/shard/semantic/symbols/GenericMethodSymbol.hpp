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
		MethodSymbol* m_underlayingMethod = nullptr;
		gmt::vector<StructSymbol*> m_typeArguments;

	public:
		GenericMethodSymbol(MethodSymbol* underlaying_method, SyntaxSymbol* parent);
		virtual ~GenericMethodSymbol() = default;

		GenericMethodSymbol(const GenericMethodSymbol& other) = delete;
		GenericMethodSymbol& operator=(const GenericMethodSymbol& other) = delete;

		GenericMethodSymbol(GenericMethodSymbol&& other) = delete;
		GenericMethodSymbol& operator=(GenericMethodSymbol&& other) = delete;

		MethodSymbol* get_underlaying_method() const;

		bool get_is_generic_instantiation() const override;

		const gmt::vector<StructSymbol*>& get_type_arguments() const;
		void set_type_arguments(std::span<StructSymbol* const> type_arguments);
	};
}
