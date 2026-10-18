#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/nodes/directives/NamespaceDirectiveSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/types/NullableTypeSyntax.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

#include <cstdint>
#include <string_view>
#include <span>
#include <vector>

namespace shard
{
	class SHARD_API SymbolFactory
	{
		SymbolTable* m_table;

		void apply_modifiers(gmt::Span<const SyntaxToken> modifiers, MemberSymbol& symbol);

	public:
		SymbolFactory(SymbolTable* table)
			: m_table(table) { }

		gmt::Ref<NamespaceSymbol> create_namespace(gmt::Ref<const NamespaceDirectiveSyntax> node);
		gmt::Ref<NamespaceSymbol> create_namespace(std::wstring_view qualifiedName);

		gmt::Ref<StructSymbol> create_struct(gmt::Ref<const StructDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<StructSymbol> create_struct(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);

		gmt::Ref<InterfaceSymbol> create_interface(gmt::Ref<const InterfaceDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<InterfaceSymbol> create_interface(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, SymbolAccesibility accesibility = ACS_PUBLIC);

		gmt::Ref<EnumSymbol> create_enum(gmt::Ref<const EnumDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<EnumSymbol> create_enum(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, bool isFlags = false);

		gmt::Ref<FieldSymbol> create_field(gmt::Ref<const FieldDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<FieldSymbol> create_field(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> type = gmt::nullref, SymbolLinking linking = LINK_INSTANCE);
		gmt::Ref<FieldSymbol> create_enum_field(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> enumType, std::int64_t value);
		gmt::Ref<FieldSymbol> create_backing_field(gmt::Ref<PropertySymbol> property, gmt::Ref<SyntaxSymbol> parent);

		gmt::Ref<PropertySymbol> create_property(gmt::Ref<const PropertyDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<PropertySymbol> create_property(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType = gmt::nullref, SymbolLinking linking = LINK_INSTANCE);

		gmt::Ref<MethodSymbol> create_method(gmt::Ref<const FunctionDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<MethodSymbol> create_method(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType = gmt::nullref, SymbolLinking linking = LINK_INSTANCE);

		gmt::Ref<OperatorSymbol> create_operator(gmt::Ref<const OperatorDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);

		gmt::Ref<ConstructorSymbol> create_constructor(gmt::Ref<const ConstructorDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<ConstructorSymbol> create_constructor(gmt::Ref<SyntaxSymbol> parent, SymbolAccesibility accesibility);

		gmt::Ref<AccessorSymbol> create_accessor(gmt::Ref<const AccessorDeclarationSyntax> node, gmt::Ref<PropertySymbol> property);
		gmt::Ref<AccessorSymbol> create_getter(gmt::Ref<PropertySymbol> property);
		gmt::Ref<AccessorSymbol> create_setter(gmt::Ref<PropertySymbol> property);

		gmt::Ref<IndexatorSymbol> create_indexator(gmt::Ref<const IndexatorDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<IndexatorSymbol> create_indexator(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType = gmt::nullref);
		gmt::Ref<IndexatorSymbol> create_indexator(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType, std::vector<gmt::Ref<ParameterSymbol>> parameters);

		gmt::Ref<ParameterSymbol> create_parameter(gmt::Ref<const ParameterSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<ParameterSymbol> create_parameter(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<ParameterSymbol> create_parameter(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> type);

		gmt::Ref<VariableSymbol> create_variable(gmt::Ref<const VariableStatementSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<VariableSymbol> create_variable(gmt::Ref<const ForInStatementSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<VariableSymbol> create_variable(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<VariableSymbol> create_variable(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> type);

		gmt::Ref<TypeParameterSymbol> create_type_parameter(std::wstring_view name, gmt::Ref<StructSymbol> parent);
		gmt::Ref<TypeParameterSymbol> create_type_parameter(std::wstring_view name, gmt::Ref<MethodSymbol> parent);

		gmt::Ref<DelegateTypeSymbol> create_delegate(gmt::Ref<const DelegateDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<DelegateTypeSymbol> create_delegate(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> parent);
		gmt::Ref<DelegateTypeSymbol> create_delegate(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType, std::vector<gmt::Ref<ParameterSymbol>> parameters);

		gmt::Ref<ArrayTypeSymbol> create_array_type(gmt::Ref<const ArrayTypeSyntax> node, gmt::Ref<StructSymbol> underlayingType);
		gmt::Ref<ArrayTypeSymbol> create_array_type(gmt::Ref<StructSymbol> underlayingType);
		gmt::Ref<ArrayTypeSymbol> create_array_type(gmt::Ref<StructSymbol> underlayingType, std::size_t size);

		gmt::Ref<NullableTypeSymbol> create_nullable_type(gmt::Ref<const NullableTypeSyntax> node, gmt::Ref<StructSymbol> underlayingType);
		gmt::Ref<NullableTypeSymbol> create_nullable_type(gmt::Ref<StructSymbol> underlayingType);

		gmt::Ref<GenericTypeSymbol> create_generic_type(gmt::Ref<const GenericTypeSyntax> node, gmt::Ref<StructSymbol> underlayingType);
		gmt::Ref<GenericTypeSymbol> create_generic_type(gmt::Ref<StructSymbol> underlayingType);
		gmt::Ref<GenericTypeSymbol> create_generic_type(gmt::Ref<StructSymbol> underlayingType, std::span<const gmt::Ref<StructSymbol>> typeArguments);
	};
}
