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

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>
#include <span>

#include <cstdint>
#include <string_view>
#include <span>
#include <vector>

namespace shard
{
	class SHARD_API SymbolFactory
	{
		SymbolTable* m_table;

		void apply_modifiers(std::span<const SyntaxToken> modifiers, MemberSymbol& symbol);

	public:
		SymbolFactory(SymbolTable* table)
			: m_table(table) { }

		NamespaceSymbol* create_namespace(const NamespaceDirectiveSyntax* node);
		NamespaceSymbol* create_namespace(std::wstring_view qualifiedName);

		StructSymbol* create_struct(const StructDeclarationSyntax* node, SyntaxSymbol* parent);
		StructSymbol* create_struct(std::wstring_view name, SyntaxSymbol* parent);

		InterfaceSymbol* create_interface(const InterfaceDeclarationSyntax* node, SyntaxSymbol* parent);
		InterfaceSymbol* create_interface(std::wstring_view name, SyntaxSymbol* parent, SymbolAccesibility accesibility = ACS_PUBLIC);

		EnumSymbol* create_enum(const EnumDeclarationSyntax* node, SyntaxSymbol* parent);
		EnumSymbol* create_enum(std::wstring_view name, SyntaxSymbol* parent, bool isFlags = false);

		FieldSymbol* create_field(const FieldDeclarationSyntax* node, SyntaxSymbol* parent);
		FieldSymbol* create_field(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* type = nullptr, SymbolLinking linking = LINK_INSTANCE);
		FieldSymbol* create_enum_field(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* enumType, std::int64_t value);
		FieldSymbol* create_backing_field(PropertySymbol* property, SyntaxSymbol* parent);

		PropertySymbol* create_property(const PropertyDeclarationSyntax* node, SyntaxSymbol* parent);
		PropertySymbol* create_property(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType = nullptr, SymbolLinking linking = LINK_INSTANCE);

		MethodSymbol* create_method(const FunctionDeclarationSyntax* node, SyntaxSymbol* parent);
		MethodSymbol* create_method(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType = nullptr, SymbolLinking linking = LINK_INSTANCE);

		OperatorSymbol* create_operator(const OperatorDeclarationSyntax* node, SyntaxSymbol* parent);

		ConstructorSymbol* create_constructor(const ConstructorDeclarationSyntax* node, SyntaxSymbol* parent);
		ConstructorSymbol* create_constructor(SyntaxSymbol* parent, SymbolAccesibility accesibility);

		AccessorSymbol* create_accessor(const AccessorDeclarationSyntax* node, PropertySymbol* property);
		AccessorSymbol* create_getter(PropertySymbol* property);
		AccessorSymbol* create_setter(PropertySymbol* property);

		IndexatorSymbol* create_indexator(const IndexatorDeclarationSyntax* node, SyntaxSymbol* parent);
		IndexatorSymbol* create_indexator(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType = nullptr);
		IndexatorSymbol* create_indexator(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType, gmt::vector<ParameterSymbol*> parameters);

		ParameterSymbol* create_parameter(const ParameterSyntax* node, SyntaxSymbol* parent);
		ParameterSymbol* create_parameter(std::wstring_view name, SyntaxSymbol* parent);
		ParameterSymbol* create_parameter(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* type);

		VariableSymbol* create_variable(const VariableStatementSyntax* node, SyntaxSymbol* parent);
		VariableSymbol* create_variable(const ForInStatementSyntax* node, SyntaxSymbol* parent);
		VariableSymbol* create_variable(std::wstring_view name, SyntaxSymbol* parent);
		VariableSymbol* create_variable(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* type);

		TypeParameterSymbol* create_type_parameter(std::wstring_view name, StructSymbol* parent);
		TypeParameterSymbol* create_type_parameter(std::wstring_view name, MethodSymbol* parent);

		DelegateTypeSymbol* create_delegate(const DelegateDeclarationSyntax* node, SyntaxSymbol* parent);
		DelegateTypeSymbol* create_delegate(MethodSymbol* method, SyntaxSymbol* parent);
		DelegateTypeSymbol* create_delegate(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType, gmt::vector<ParameterSymbol*> parameters);

		ArrayTypeSymbol* create_array_type(const ArrayTypeSyntax* node, StructSymbol* underlayingType);
		ArrayTypeSymbol* create_array_type(StructSymbol* underlayingType);
		ArrayTypeSymbol* create_array_type(StructSymbol* underlayingType, std::size_t size);

		NullableTypeSymbol* create_nullable_type(const NullableTypeSyntax* node, StructSymbol* underlayingType);
		NullableTypeSymbol* create_nullable_type(StructSymbol* underlayingType);

		GenericTypeSymbol* create_generic_type(const GenericTypeSyntax* node, StructSymbol* underlayingType);
		GenericTypeSymbol* create_generic_type(StructSymbol* underlayingType);
		GenericTypeSymbol* create_generic_type(StructSymbol* underlayingType, std::span<StructSymbol* const> typeArguments);
	};
}
