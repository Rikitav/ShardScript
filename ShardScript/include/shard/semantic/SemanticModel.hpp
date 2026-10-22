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

		gmt::unordered_map<gmt::Ref<const ExpressionSyntax>, gmt::Ref<StructSymbol>> m_expressionTypes;

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
		bool type_implements_interface(gmt::Ref<StructSymbol> type, gmt::Ref<InterfaceSymbol> interface) const;
		bool type_satisfies_constraint(gmt::Ref<StructSymbol> typeArgument, gmt::Ref<StructSymbol> constraint) const;
		bool is_primitive_type(gmt::Ref<const StructSymbol> type) const;
		bool is_task_or_task_like(gmt::Ref<StructSymbol> type);
		gmt::wstring get_type_display_name(gmt::Ref<const StructSymbol> type) const;

		gmt::Ref<StructSymbol> substitute_type(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> substitute_type(gmt::Ref<StructSymbol> type, std::span<const gmt::Ref<StructSymbol>> typeArguments);
		gmt::Ref<StructSymbol> get_field_type(gmt::Ref<FieldSymbol> field, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> get_method_return_type(gmt::Ref<MethodSymbol> method, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> get_property_type(gmt::Ref<PropertySymbol> property, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);
		gmt::Ref<StructSymbol> get_constructor_return_type(gmt::Ref<ConstructorSymbol> constructor, gmt::Ref<GenericTypeSymbol> within = gmt::nullref);

		gmt::Ref<StructSymbol> find_type_by_name(std::wstring_view fullName);
		gmt::Ref<FieldSymbol> find_field_by_name(gmt::Ref<StructSymbol> type, std::wstring_view name) const;
		bool resolve_indexable(gmt::Ref<StructSymbol> type, gmt::Ref<PropertySymbol>& outCount, gmt::Ref<IndexatorSymbol>& outIndexator);
		gmt::Ref<PropertySymbol> resolve_virtual_property(gmt::Ref<StructSymbol> type, gmt::Ref<PropertySymbol> interfaceMember, gmt::Ref<GenericTypeSymbol> within);
		gmt::Ref<IndexatorSymbol> resolve_virtual_indexator(gmt::Ref<StructSymbol> type, gmt::Ref<IndexatorSymbol> interfaceMember, gmt::Ref<GenericTypeSymbol> within);

		gmt::Ref<StructSymbol> resolve_runtime_type_argument(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolveParameter);
		bool try_resolve_generic_arguments(gmt::Ref<StructSymbol> type, const TypeParameterResolver& resolveParameter, gmt::Ref<StructSymbol>& baseType, gmt::vector<gmt::Ref<StructSymbol>>& genericArgs);
	};
}
