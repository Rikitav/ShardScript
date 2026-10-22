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
		gmt::Ref<T> m_symbol;

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
		gmt::Ref<T> get() const { return m_symbol; }
		operator gmt::Ref<T>() const { return m_symbol; }
	};

	template<>
	class SHARD_API SymbolBuilder<NamespaceSymbol> : public SymbolBuilderBase<NamespaceSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<NamespaceSymbol> parent = gmt::nullref);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<NamespaceSymbol> parent = gmt::nullref);

		SymbolBuilder<MethodSymbol> add_method(const gmt::wstring& name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<StructSymbol> add_struct(const gmt::wstring& name, SymbolLinking linking = LINK_INSTANCE, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<InterfaceSymbol> add_interface(const gmt::wstring& name, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<EnumSymbol> add_enum(const gmt::wstring& name, bool isFlags = false, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<NamespaceSymbol> add_namespace(const gmt::wstring& name);
	};

	template<>
	class SHARD_API SymbolBuilder<StructSymbol> : public SymbolBuilderBase<StructSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, gmt::Ref<StructSymbol> symbol);
		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, gmt::Ref<StructSymbol> symbol);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<ConstructorSymbol> add_init(SymbolAccesibility access = ACS_PUBLIC);

		SymbolBuilder<MethodSymbol> add_method(const gmt::wstring& name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<FieldSymbol> add_field(const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<PropertySymbol> add_property(const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<IndexatorSymbol> add_indexator(gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<TypeParameterSymbol> add_type_parameter(const gmt::wstring& name);

		SymbolBuilder<OperatorSymbol> add_operator(TokenType operatorToken, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<OperatorSymbol> add_cast_operator(gmt::Ref<StructSymbol> targetType, SymbolLinking linking = LINK_STATIC, SymbolAccesibility access = ACS_PUBLIC);

		SymbolBuilder<StructSymbol>& implements(gmt::Ref<StructSymbol> interface);
		SymbolBuilder<StructSymbol>& declare_global();
	};

	template<>
	class SHARD_API SymbolBuilder<InterfaceSymbol> : public SymbolBuilderBase<InterfaceSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<MethodSymbol> add_method(const gmt::wstring& name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<PropertySymbol> add_property(const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<IndexatorSymbol> add_indexator(gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<TypeParameterSymbol> add_type_parameter(const gmt::wstring& name);

		SymbolBuilder<InterfaceSymbol>& implements(gmt::Ref<StructSymbol> interface);
		SymbolBuilder<InterfaceSymbol>& declare_global();
	};

	template<>
	class SHARD_API SymbolBuilder<EnumSymbol> : public SymbolBuilderBase<EnumSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, bool isFlags, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, bool isFlags, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<EnumSymbol>& set_flags(bool value = true);
		SymbolBuilder<EnumSymbol>& add_value(const gmt::wstring& name, std::int64_t value);
	};

	template<>
	class SHARD_API SymbolBuilder<FieldSymbol> : public SymbolBuilderBase<FieldSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, gmt::Ref<FieldSymbol> symbol);
		SymbolBuilder(SymbolTable& table, gmt::Ref<FieldSymbol> symbol);

		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);
	};

	template<>
	class SHARD_API SymbolBuilder<MethodSymbol> : public SymbolBuilderBase<MethodSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, gmt::Ref<MethodSymbol> symbol);
		SymbolBuilder(SymbolTable& table, gmt::Ref<MethodSymbol> symbol);

		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<MethodSymbol>& add_parameter(const gmt::wstring& name, gmt::Ref<StructSymbol> type);
		SymbolBuilder<MethodSymbol>& set_callback(MethodSymbolDelegate callback);
		SymbolBuilder<TypeParameterSymbol> add_type_parameter(const gmt::wstring& name);
		SymbolBuilder<MethodSymbol>& declare_global();

		SymbolBuilder<MethodSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<MethodSymbol>& set_throws_type(gmt::Ref<StructSymbol> type);
		SymbolBuilder<MethodSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<MethodSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<MethodSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<MethodSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<ConstructorSymbol> : public SymbolBuilderBase<ConstructorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<ConstructorSymbol>& add_parameter(const gmt::wstring& name, gmt::Ref<StructSymbol> type);
		SymbolBuilder<ConstructorSymbol>& set_callback(MethodSymbolDelegate callback);

		SymbolBuilder<ConstructorSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_throws_type(gmt::Ref<StructSymbol> type);
		SymbolBuilder<ConstructorSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<ConstructorSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<OperatorSymbol> : public SymbolBuilderBase<OperatorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, TokenType operatorToken, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, TokenType operatorToken, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<OperatorSymbol>& add_parameter(const gmt::wstring& name, gmt::Ref<StructSymbol> type);
		SymbolBuilder<OperatorSymbol>& set_callback(MethodSymbolDelegate callback);

		SymbolBuilder<OperatorSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_throws_type(gmt::Ref<StructSymbol> type);
		SymbolBuilder<OperatorSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<OperatorSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<PropertySymbol> : public SymbolBuilderBase<PropertySymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<FieldSymbol> add_backing_field();
		SymbolBuilder<AccessorSymbol> add_getter(SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<AccessorSymbol> add_setter(SymbolAccesibility access = ACS_PUBLIC);
	};

	template<>
	class SHARD_API SymbolBuilder<IndexatorSymbol> : public SymbolBuilderBase<IndexatorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<IndexatorSymbol>& add_parameter(const gmt::wstring& name, gmt::Ref<StructSymbol> type);

		SymbolBuilder<AccessorSymbol> add_getter(SymbolAccesibility access = ACS_PUBLIC);
		SymbolBuilder<AccessorSymbol> add_setter(SymbolAccesibility access = ACS_PUBLIC);
	};

	template<>
	class SHARD_API SymbolBuilder<AccessorSymbol> : public SymbolBuilderBase<AccessorSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, gmt::Ref<AccessorSymbol> symbol);
		SymbolBuilder(SymbolTable& table, gmt::Ref<AccessorSymbol> symbol);

		SymbolBuilder(CompilationContext& context, bool isGetter, SymbolAccesibility access, gmt::Ref<PropertySymbol> property);
		SymbolBuilder(SymbolTable& table, bool isGetter, SymbolAccesibility access, gmt::Ref<PropertySymbol> property);

		SymbolBuilder<AccessorSymbol>& add_parameter(const gmt::wstring& name, gmt::Ref<StructSymbol> type);
		SymbolBuilder<AccessorSymbol>& set_callback(MethodSymbolDelegate callback);

		SymbolBuilder<AccessorSymbol>& set_may_throw(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_throws_type(gmt::Ref<StructSymbol> type);
		SymbolBuilder<AccessorSymbol>& set_mutates_instance(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_mutates_static(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_mutates_arguments(bool value = true);
		SymbolBuilder<AccessorSymbol>& set_has_unknown_side_effects(bool value = true);
	};

	template<>
	class SHARD_API SymbolBuilder<TypeParameterSymbol> : public SymbolBuilderBase<TypeParameterSymbol>
	{
	public:
		SymbolBuilder(CompilationContext& context, const gmt::wstring& name, gmt::Ref<SyntaxSymbol> parent);
		SymbolBuilder(SymbolTable& table, const gmt::wstring& name, gmt::Ref<SyntaxSymbol> parent);

		SymbolBuilder<TypeParameterSymbol>& add_constraint(gmt::Ref<StructSymbol> constraint);
	};
}
