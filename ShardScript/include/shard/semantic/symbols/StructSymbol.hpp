#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
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
		gmt::vector<StructSymbol*> interfaces;
		gmt::vector<TypeParameterSymbol*> type_parameters;

		gmt::vector<FieldSymbol*> fields;
		gmt::vector<PropertySymbol*> properties;
		gmt::vector<IndexatorSymbol*> indexators;
		gmt::vector<ConstructorSymbol*> constructors;
		gmt::vector<OperatorSymbol*> operators;
		gmt::vector<MethodSymbol*> methods;

		StructSymbol(std::wstring_view name, const SyntaxKind kind, SyntaxSymbol* parent);
		StructSymbol(gmt::StringReference name, const SyntaxKind kind, SyntaxSymbol* parent);
		virtual ~StructSymbol() = default;

		StructSymbol(const StructSymbol& other) = delete;
		StructSymbol& operator=(const StructSymbol& other) = delete;

		StructSymbol(StructSymbol&& other) = delete;
		StructSymbol& operator=(StructSymbol&& other) = delete;

		bool is_type() const override;
		void on_symbol_declared(SyntaxSymbol* symbol) override;
	};
}
