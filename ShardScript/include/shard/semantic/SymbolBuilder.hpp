#pragma once
#include <shard/Definitions.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/LibraryMetadata.hpp>

#include <shard/parsing/TokenType.hpp>

#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace shard
{
	template<typename T>
	class SymbolBuilder;

	template<typename T>
	class SymbolBuilderBase
	{
		static_assert(std::is_base_of_v<SyntaxSymbol, T>, "SymbolBuilder<T> can only be used with SyntaxSymbol-derived types");

	protected:
		SymbolTable* m_table;
		SymbolFactory m_factory;
		T* m_symbol = nullptr;

		SymbolBuilderBase(SymbolTable& table)
			: m_table(&table), m_factory(&table) { }

		SymbolBuilderBase(CompilationContext& context)
			: SymbolBuilderBase(context.get_semantic_model().get_table()) { }

	public:
		SymbolBuilderBase(const SymbolBuilderBase&) = delete;
		SymbolBuilderBase& operator=(const SymbolBuilderBase&) = delete;

		SymbolBuilderBase(SymbolBuilderBase&&) = default;
		SymbolBuilderBase& operator=(SymbolBuilderBase&&) = default;

		SymbolFactory& get_factory() { return m_factory; }
		T* get() const { return m_symbol; }
		operator T*() const { return m_symbol; }
	};

	template<>
	class SHARD_API SymbolBuilder<NamespaceSymbol> : public SymbolBuilderBase<NamespaceSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, std::wstring_view name, NamespaceSymbol* parent = nullptr);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, NamespaceSymbol* parent = nullptr);

		SymbolBuilder<MethodSymbol> add_method(std::wstring_view name, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<StructSymbol> add_struct(std::wstring_view name, SymbolLinking linking = LINK_INSTANCE, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<InterfaceSymbol> add_interface(std::wstring_view name, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<EnumSymbol> add_enum(std::wstring_view name, bool isFlags = false, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<NamespaceSymbol> add_namespace(std::wstring_view name);
	};

	template<>
	class SHARD_API SymbolBuilder<StructSymbol> : public SymbolBuilderBase<StructSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, StructSymbol* symbol);
		SymbolBuilder(CompilationContext& context, std::wstring_view name, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, StructSymbol* symbol);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, SyntaxSymbol* parent);

		SymbolBuilder<ConstructorSymbol> add_init(SymbolAccesibility access = ACS_PUBLIC);

		SymbolBuilder<MethodSymbol> add_method(std::wstring_view name, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<FieldSymbol> add_field(std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<PropertySymbol> add_property(std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<IndexatorSymbol> add_indexator(StructSymbol* type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<TypeParameterSymbol> add_type_parameter(std::wstring_view name);

		SymbolBuilder<OperatorSymbol> add_operator(TokenType operatorToken, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<OperatorSymbol> add_cast_operator(StructSymbol* targetType, SymbolLinking linking = LINK_STATIC, SymbolAccesibility access = ACS_PUBLIC);

		SymbolBuilder<StructSymbol>& implements(StructSymbol* interface);
		SymbolBuilder<StructSymbol>& declare_global();
	};

	template<>
	class SHARD_API SymbolBuilder<InterfaceSymbol> : public SymbolBuilderBase<InterfaceSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, std::wstring_view name, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, SyntaxSymbol* parent);

		SymbolBuilder<MethodSymbol> add_method(std::wstring_view name, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<PropertySymbol> add_property(std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<IndexatorSymbol> add_indexator(StructSymbol* type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<TypeParameterSymbol> add_type_parameter(std::wstring_view name);

		SymbolBuilder<InterfaceSymbol>& implements(StructSymbol* interface);
		SymbolBuilder<InterfaceSymbol>& declare_global();
	};

	template<>
	class SHARD_API SymbolBuilder<EnumSymbol> : public SymbolBuilderBase<EnumSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, std::wstring_view name, bool isFlags, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, bool isFlags, SyntaxSymbol* parent);

		SymbolBuilder<EnumSymbol>& set_flags(bool value = true);
		SymbolBuilder<EnumSymbol>& add_value(std::wstring_view name, std::int64_t value);
	};

	template<>
	class SHARD_API SymbolBuilder<FieldSymbol> : public SymbolBuilderBase<FieldSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, FieldSymbol* symbol);
		SymbolBuilder(SymbolTable& table, FieldSymbol* symbol);

		SymbolBuilder(CompilationContext& context, std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);
	};

	template<>
	class SHARD_API SymbolBuilder<MethodSymbol> : public SymbolBuilderBase<MethodSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, MethodSymbol* symbol);
		SymbolBuilder(SymbolTable& table, MethodSymbol* symbol);

		SymbolBuilder(CompilationContext& context, std::wstring_view name, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);

		SymbolBuilder<MethodSymbol>& add_parameter(std::wstring_view name, StructSymbol* type);
		SymbolBuilder<MethodSymbol>& set_callback(MethodSymbolDelegate callback);
		SymbolBuilder<TypeParameterSymbol> add_type_parameter(std::wstring_view name);
		SymbolBuilder<MethodSymbol>& declare_global();

		SymbolBuilder<MethodSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<MethodSymbol>& set_throws_type(StructSymbol* type);
		SymbolBuilder<MethodSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<MethodSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<MethodSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<MethodSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<ConstructorSymbol> : public SymbolBuilderBase<ConstructorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, SymbolAccesibility access, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, SymbolAccesibility access, SyntaxSymbol* parent);

		SymbolBuilder<ConstructorSymbol>& add_parameter(std::wstring_view name, StructSymbol* type);
		SymbolBuilder<ConstructorSymbol>& set_callback(MethodSymbolDelegate callback);

		SymbolBuilder<ConstructorSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_throws_type(StructSymbol* type);
		SymbolBuilder<ConstructorSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<OperatorSymbol> : public SymbolBuilderBase<OperatorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, TokenType operatorToken, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, TokenType operatorToken, StructSymbol* returnType, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);

		SymbolBuilder<OperatorSymbol>& add_parameter(std::wstring_view name, StructSymbol* type);
		SymbolBuilder<OperatorSymbol>& set_callback(MethodSymbolDelegate callback);

		SymbolBuilder<OperatorSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_throws_type(StructSymbol* type);
		SymbolBuilder<OperatorSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<PropertySymbol> : public SymbolBuilderBase<PropertySymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);

		SymbolBuilder<FieldSymbol> add_backing_field();
		SymbolBuilder<AccessorSymbol> add_getter(SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<AccessorSymbol> add_setter(SymbolAccesibility access = ACS_PUBLIC);
	};

	template<>
	class SHARD_API SymbolBuilder<IndexatorSymbol> : public SymbolBuilderBase<IndexatorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, StructSymbol* type, SymbolLinking linking, SymbolAccesibility access, SyntaxSymbol* parent);

		SymbolBuilder<IndexatorSymbol>& add_parameter(std::wstring_view name, StructSymbol* type);

		SymbolBuilder<AccessorSymbol> add_getter(SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<AccessorSymbol> add_setter(SymbolAccesibility access = ACS_PUBLIC);
	};

	template<>
	class SHARD_API SymbolBuilder<AccessorSymbol> : public SymbolBuilderBase<AccessorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, AccessorSymbol* symbol);
		SymbolBuilder(SymbolTable& table, AccessorSymbol* symbol);

		SymbolBuilder(CompilationContext& context, bool isGetter, SymbolAccesibility access, PropertySymbol* property);
		SymbolBuilder(SymbolTable& table, bool isGetter, SymbolAccesibility access, PropertySymbol* property);

		SymbolBuilder<AccessorSymbol>& add_parameter(std::wstring_view name, StructSymbol* type);
		SymbolBuilder<AccessorSymbol>& set_callback(MethodSymbolDelegate callback);

		SymbolBuilder<AccessorSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_throws_type(StructSymbol* type);
		SymbolBuilder<AccessorSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<TypeParameterSymbol> : public SymbolBuilderBase<TypeParameterSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, std::wstring_view name, SyntaxSymbol* parent);
		SymbolBuilder(SymbolTable& table, std::wstring_view name, SyntaxSymbol* parent);

		SymbolBuilder<TypeParameterSymbol>& add_constraint(StructSymbol* constraint);
	};
}
