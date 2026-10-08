#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <string_view>
#include <vector>

namespace shard
{
	class ConstructorSymbol;
	class MethodSymbol;
	class OperatorSymbol;
	class FieldSymbol;
	class PropertySymbol;
	class IndexatorSymbol;
	class TypeParameterSymbol;

	class SHARD_API StructSymbol : public MemberSymbol
	{
	public:
		std::vector<gmt::Ref<StructSymbol>> interfaces;
		std::vector<gmt::Ref<TypeParameterSymbol>> type_parameters;

		std::vector<gmt::Ref<FieldSymbol>> fields;
		std::vector<gmt::Ref<PropertySymbol>> properties;
		std::vector<gmt::Ref<IndexatorSymbol>> indexators;
		std::vector<gmt::Ref<ConstructorSymbol>> constructors;
		std::vector<gmt::Ref<OperatorSymbol>> operators;
		std::vector<gmt::Ref<MethodSymbol>> methods;

		StructSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		StructSymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent);
		virtual ~StructSymbol() = default;

		StructSymbol(const StructSymbol& other) = delete;
		StructSymbol& operator=(const StructSymbol& other) = delete;

		StructSymbol(StructSymbol&& other) = delete;
		StructSymbol& operator=(StructSymbol&& other) = delete;

		bool is_type() const override;
		void on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol) override;
	};
}
