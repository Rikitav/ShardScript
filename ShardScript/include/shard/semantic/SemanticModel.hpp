#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/semantic/SymbolTable.hpp>

#include <gmt/Ref.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace shard
{
	class ExpressionSyntax;
	class ConstructorSymbol;
	class FieldSymbol;
	class GenericTypeSymbol;
	class MethodSymbol;
	class PropertySymbol;
	class TypeParameterSymbol;

	class SHARD_API SemanticModel
	{
		shard::SyntaxTree& m_tree;
		shard::SymbolTable m_table;

		std::unordered_map<gmt::Ref<const ExpressionSyntax>, gmt::Ref<StructSymbol>> m_expressionTypes;

	public:
		using TypeParameterResolver = std::function<gmt::Ref<StructSymbol>(gmt::Ref<TypeParameterSymbol>)>;

		SemanticModel(shard::SyntaxTree& tree);

		SemanticModel(const SemanticModel&) = delete;
		SemanticModel& operator=(const SemanticModel&) = delete;

		SyntaxTree& get_tree() const;
		SymbolTable& get_table();

		gmt::Ref<StructSymbol> get_expression_type(gmt::Ref<const ExpressionSyntax> expression) const;
		void set_expression_type(gmt::Ref<const ExpressionSyntax> expression, gmt::Ref<StructSymbol> type);

		static bool are_types_equal(gmt::Ref<const StructSymbol> left, gmt::Ref<const StructSymbol> right);
		bool is_assignable_to(gmt::Ref<const StructSymbol> target, gmt::Ref<const StructSymbol> source) const;
		bool is_primitive_type(gmt::Ref<const StructSymbol> type) const;
		static bool is_task_or_task_like(gmt::Ref<StructSymbol> type);
		std::wstring get_type_display_name(gmt::Ref<const StructSymbol> type) const;

		gmt::Ref<StructSymbol> substitute_type(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> get_field_type(gmt::Ref<FieldSymbol> field, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> get_method_return_type(gmt::Ref<MethodSymbol> method, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> get_property_type(gmt::Ref<PropertySymbol> property, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> get_constructor_return_type(gmt::Ref<ConstructorSymbol> constructor, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);

		gmt::Ref<StructSymbol> find_type_by_name(std::wstring_view fullName);
		gmt::Ref<FieldSymbol> find_field_by_name(gmt::Ref<StructSymbol> type, std::wstring_view name) const;

		gmt::Ref<StructSymbol> resolve_runtime_type_argument(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolveParameter);
		bool try_resolve_generic_arguments(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolveParameter, gmt::Ref<StructSymbol>& baseType, std::vector<gmt::Ref<StructSymbol>>& genericArgs);
	};
}
