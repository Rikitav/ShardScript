#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace shard
{
	class SHARD_API SemanticModel
	{
		shard::SyntaxTree& m_tree;
		shard::SymbolTable m_table;

		gmt::unordered_map<const ExpressionSyntax*, StructSymbol*> m_expressionTypes;

	public:
		using TypeParameterResolver = std::function<StructSymbol*(TypeParameterSymbol*)>;

		SemanticModel(shard::SyntaxTree& tree);

		SemanticModel(const SemanticModel&) = delete;
		SemanticModel& operator=(const SemanticModel&) = delete;

		SyntaxTree& get_tree() const;
		SymbolTable& get_table();

		StructSymbol* get_expression_type(const ExpressionSyntax* expression) const;
		void set_expression_type(const ExpressionSyntax* expression, StructSymbol* type);

		static bool are_types_equal(const StructSymbol* left, const StructSymbol* right);
		bool is_assignable_to(const StructSymbol* target, const StructSymbol* source) const;
		bool type_implements_interface(StructSymbol* type, InterfaceSymbol* interface) const;
		bool type_satisfies_constraint(StructSymbol* typeArgument, StructSymbol* constraint) const;
		bool is_primitive_type(const StructSymbol* type) const;
		bool is_task_or_task_like(StructSymbol* type);
		gmt::wstring get_type_display_name(const StructSymbol* type) const;

		StructSymbol* substitute_type(StructSymbol* type, GenericTypeSymbol* within = nullptr);
		StructSymbol* substitute_type(StructSymbol* type, std::span<StructSymbol* const> typeArguments);
		StructSymbol* get_field_type(FieldSymbol* field, GenericTypeSymbol* within = nullptr);
		StructSymbol* get_method_return_type(MethodSymbol* method, GenericTypeSymbol* within = nullptr);
		StructSymbol* get_property_type(PropertySymbol* property, GenericTypeSymbol* within = nullptr);
		StructSymbol* get_constructor_return_type(ConstructorSymbol* constructor, GenericTypeSymbol* within = nullptr);

		StructSymbol* find_type_by_name(std::wstring_view fullName);
		FieldSymbol* find_field_by_name(StructSymbol* type, std::wstring_view name) const;
		bool resolve_indexable(StructSymbol* type, PropertySymbol*& outCount, IndexatorSymbol*& outIndexator);
		PropertySymbol* resolve_virtual_property(StructSymbol* type, PropertySymbol* interfaceMember, GenericTypeSymbol* within);
		IndexatorSymbol* resolve_virtual_indexator(StructSymbol* type, IndexatorSymbol* interfaceMember, GenericTypeSymbol* within);

		StructSymbol* resolve_runtime_type_argument(StructSymbol* type, const TypeParameterResolver& resolveParameter);
		bool try_resolve_generic_arguments(StructSymbol* type, const TypeParameterResolver& resolveParameter, StructSymbol*& baseType, gmt::vector<StructSymbol*>& genericArgs);
	};
}
