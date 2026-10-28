#include <shard/semantic/SymbolBuilder.hpp>

#include <shard/lexical/SyntaxFacts.hpp>

#include <gmt/Containers.hpp>

#include <optional>
#include <stdexcept>

using namespace shard;

// =========================================================================
// NamespaceSymbol
// =========================================================================

SymbolBuilder<NamespaceSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<NamespaceSymbol> parent)
	: SymbolBuilderBase(table)
{
	gmt::Ref<NamespaceSymbol> anchor = parent;
	if (anchor.is_null())
		anchor = table.get_root_namespace();

	m_symbol = anchor->lookup_or_create(name, table);
}

SymbolBuilder<NamespaceSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<NamespaceSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, parent) { }

SymbolBuilder<MethodSymbol> SymbolBuilder<NamespaceSymbol>::add_method(std::wstring_view name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<MethodSymbol>(*m_table, name, returnType, linking, access, m_symbol);
}

SymbolBuilder<StructSymbol> SymbolBuilder<NamespaceSymbol>::add_struct(std::wstring_view name, SymbolLinking linking, SymbolAccesibility access)
{
	SymbolBuilder<StructSymbol> builder(*m_table, name, m_symbol);
	builder.get()->set_linking(linking);
	builder.get()->set_accesibility(access);
	return builder;
}

SymbolBuilder<InterfaceSymbol> SymbolBuilder<NamespaceSymbol>::add_interface(std::wstring_view name, SymbolAccesibility access)
{
	SymbolBuilder<InterfaceSymbol> builder(*m_table, name, m_symbol);
	builder.get()->set_accesibility(access);
	return builder;
}

SymbolBuilder<EnumSymbol> SymbolBuilder<NamespaceSymbol>::add_enum(std::wstring_view name, bool isFlags, SymbolAccesibility access)
{
	SymbolBuilder<EnumSymbol> builder(*m_table, name, isFlags, m_symbol);
	builder.get()->set_accesibility(access);
	return builder;
}

SymbolBuilder<NamespaceSymbol> SymbolBuilder<NamespaceSymbol>::add_namespace(std::wstring_view name)
{
	return SymbolBuilder<NamespaceSymbol>(*m_table, name, m_symbol);
}

// =========================================================================
// StructSymbol
// =========================================================================

SymbolBuilder<StructSymbol>::SymbolBuilder(SymbolTable& table, gmt::Ref<StructSymbol> symbol)
	: SymbolBuilderBase(table)
{
	m_symbol = symbol;
}

SymbolBuilder<StructSymbol>::SymbolBuilder(CompilationContext& context, gmt::Ref<StructSymbol> symbol)
	: SymbolBuilder(context.get_semantic_model().get_table(), symbol) { }

SymbolBuilder<StructSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_struct(name, parent);
	m_symbol->set_accesibility(ACS_PUBLIC);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<StructSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, parent) { }

SymbolBuilder<ConstructorSymbol> SymbolBuilder<StructSymbol>::add_init(SymbolAccesibility access)
{
	return SymbolBuilder<ConstructorSymbol>(*m_table, access, m_symbol);
}

SymbolBuilder<MethodSymbol> SymbolBuilder<StructSymbol>::add_method(std::wstring_view name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<MethodSymbol>(*m_table, name, returnType, linking, access, m_symbol);
}

SymbolBuilder<FieldSymbol> SymbolBuilder<StructSymbol>::add_field(std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<FieldSymbol>(*m_table, name, type, linking, access, m_symbol);
}

SymbolBuilder<PropertySymbol> SymbolBuilder<StructSymbol>::add_property(std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<PropertySymbol>(*m_table, name, type, linking, access, m_symbol);
}

SymbolBuilder<IndexatorSymbol> SymbolBuilder<StructSymbol>::add_indexator(gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<IndexatorSymbol>(*m_table, L"index", type, linking, access, m_symbol);
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<StructSymbol>::add_type_parameter(std::wstring_view name)
{
	return SymbolBuilder<TypeParameterSymbol>(*m_table, name, m_symbol);
}

SymbolBuilder<OperatorSymbol> SymbolBuilder<StructSymbol>::add_operator(TokenType operatorToken, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<OperatorSymbol>(*m_table, operatorToken, returnType, linking, access, m_symbol);
}

SymbolBuilder<OperatorSymbol> SymbolBuilder<StructSymbol>::add_cast_operator(gmt::Ref<StructSymbol> targetType, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<OperatorSymbol>(*m_table, TokenType::AsOperator, targetType, linking, access, m_symbol);
}

SymbolBuilder<StructSymbol>& SymbolBuilder<StructSymbol>::implements(gmt::Ref<StructSymbol> interface)
{
	m_symbol->interfaces.push_back(interface);
	return *this;
}

SymbolBuilder<StructSymbol>& SymbolBuilder<StructSymbol>::declare_global()
{
	m_table->get_root_namespace()->on_symbol_declared(m_symbol);
	return *this;
}

// =========================================================================
// InterfaceSymbol
// =========================================================================

SymbolBuilder<InterfaceSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_interface(name, parent, ACS_PUBLIC);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<InterfaceSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, parent) { }

SymbolBuilder<MethodSymbol> SymbolBuilder<InterfaceSymbol>::add_method(std::wstring_view name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<MethodSymbol>(*m_table, name, returnType, linking, access, m_symbol);
}

SymbolBuilder<PropertySymbol> SymbolBuilder<InterfaceSymbol>::add_property(std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<PropertySymbol>(*m_table, name, type, linking, access, m_symbol);
}

SymbolBuilder<IndexatorSymbol> SymbolBuilder<InterfaceSymbol>::add_indexator(gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access)
{
	return SymbolBuilder<IndexatorSymbol>(*m_table, L"index", type, linking, access, m_symbol);
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<InterfaceSymbol>::add_type_parameter(std::wstring_view name)
{
	return SymbolBuilder<TypeParameterSymbol>(*m_table, name, m_symbol);
}

SymbolBuilder<InterfaceSymbol>& SymbolBuilder<InterfaceSymbol>::implements(gmt::Ref<StructSymbol> interface)
{
	m_symbol->interfaces.push_back(interface);
	return *this;
}

SymbolBuilder<InterfaceSymbol>& SymbolBuilder<InterfaceSymbol>::declare_global()
{
	m_table->get_root_namespace()->on_symbol_declared(m_symbol);
	return *this;
}

// =========================================================================
// EnumSymbol
// =========================================================================

SymbolBuilder<EnumSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, bool isFlags, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_enum(name, parent, isFlags);
	m_symbol->set_accesibility(ACS_PUBLIC);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<EnumSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, bool isFlags, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, isFlags, parent) { }

SymbolBuilder<EnumSymbol>& SymbolBuilder<EnumSymbol>::set_flags(bool value)
{
	m_symbol->set_is_flags(value);
	return *this;
}

SymbolBuilder<EnumSymbol>& SymbolBuilder<EnumSymbol>::add_value(std::wstring_view name, std::int64_t value)
{
	gmt::Ref<FieldSymbol> field = m_factory.create_enum_field(name, m_symbol, m_symbol, value);
	m_symbol->on_symbol_declared(field);
	return *this;
}

// =========================================================================
// FieldSymbol
// =========================================================================

SymbolBuilder<FieldSymbol>::SymbolBuilder(SymbolTable& table, gmt::Ref<FieldSymbol> symbol)
	: SymbolBuilderBase(table)
{
	m_symbol = symbol;
}

SymbolBuilder<FieldSymbol>::SymbolBuilder(CompilationContext& context, gmt::Ref<FieldSymbol> symbol)
	: SymbolBuilder(context.get_semantic_model().get_table(), symbol) { }

SymbolBuilder<FieldSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_field(name, parent, type, linking);
	m_symbol->set_accesibility(access);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<FieldSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, type, linking, access, parent) { }

// =========================================================================
// MethodSymbol
// =========================================================================

SymbolBuilder<MethodSymbol>::SymbolBuilder(SymbolTable& table, gmt::Ref<MethodSymbol> symbol)
	: SymbolBuilderBase(table)
{
	m_symbol = symbol;
}

SymbolBuilder<MethodSymbol>::SymbolBuilder(CompilationContext& context, gmt::Ref<MethodSymbol> symbol)
	: SymbolBuilder(context.get_semantic_model().get_table(), symbol) { }

SymbolBuilder<MethodSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_method(name, parent, returnType, linking);
	m_symbol->set_accesibility(access);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<MethodSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, returnType, linking, access, parent) { }

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::add_parameter(std::wstring_view name, gmt::Ref<StructSymbol> type)
{
	gmt::Ref<ParameterSymbol> parameter = m_factory.create_parameter(name, m_symbol, type);
	m_symbol->on_symbol_declared(parameter);
	return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::set_callback(MethodSymbolDelegate callback)
{
	if (callback == nullptr)
		throw std::invalid_argument("method callback cannot be null");

	m_symbol->set_calling_convention(CallingConvention::ShardDecl);
	m_symbol->set_function_pointer(reinterpret_cast<void*>(callback));
	m_symbol->set_effects_computed(true);
	return *this;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<MethodSymbol>::add_type_parameter(std::wstring_view name)
{
	return SymbolBuilder<TypeParameterSymbol>(*m_table, name, m_symbol);
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::declare_global()
{
	m_table->get_root_namespace()->on_symbol_declared(m_symbol);
	return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::set_may_throw(bool value)
{
	m_symbol->get_effect_summary().mayThrow = value;
	return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::set_throws_type(gmt::Ref<StructSymbol> type)
{
	if (!type.is_null())
	{
		MethodEffectSummary& summary = m_symbol->get_effect_summary();
		summary.mayThrow = true;
		summary.thrownTypes.insert(type);
	}

	return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::set_mutates_instance(bool value)
{
	m_symbol->get_effect_summary().mutatesInstance = value;
	return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::set_mutates_static(bool value)
{
	m_symbol->get_effect_summary().mutatesStatic = value;
	return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::set_mutates_arguments(bool value)
{
	m_symbol->get_effect_summary().mutatesArguments = value;
	return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::set_has_unknown_side_effects(bool value)
{
	m_symbol->get_effect_summary().hasUnknownSideEffects = value;
	return *this;
}

// =========================================================================
// ConstructorSymbol
// =========================================================================

SymbolBuilder<ConstructorSymbol>::SymbolBuilder(SymbolTable& table, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_constructor(parent, access);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<ConstructorSymbol>::SymbolBuilder(CompilationContext& context, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), access, parent) { }

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::add_parameter(std::wstring_view name, gmt::Ref<StructSymbol> type)
{
	gmt::Ref<ParameterSymbol> parameter = m_factory.create_parameter(name, m_symbol, type);
	m_symbol->on_symbol_declared(parameter);
	return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::set_callback(MethodSymbolDelegate callback)
{
	if (callback == nullptr)
		throw std::invalid_argument("constructor callback cannot be null");

	m_symbol->set_calling_convention(CallingConvention::ShardDecl);
	m_symbol->set_function_pointer(reinterpret_cast<void*>(callback));
	m_symbol->set_effects_computed(true);
	return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::set_may_throw(bool value)
{
	m_symbol->get_effect_summary().mayThrow = value;
	return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::set_throws_type(gmt::Ref<StructSymbol> type)
{
	if (!type.is_null())
	{
		MethodEffectSummary& summary = m_symbol->get_effect_summary();
		summary.mayThrow = true;
		summary.thrownTypes.insert(type);
	}

	return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::set_mutates_instance(bool value)
{
	m_symbol->get_effect_summary().mutatesInstance = value;
	return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::set_mutates_static(bool value)
{
	m_symbol->get_effect_summary().mutatesStatic = value;
	return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::set_mutates_arguments(bool value)
{
	m_symbol->get_effect_summary().mutatesArguments = value;
	return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::set_has_unknown_side_effects(bool value)
{
	m_symbol->get_effect_summary().hasUnknownSideEffects = value;
	return *this;
}

// =========================================================================
// OperatorSymbol
// =========================================================================

SymbolBuilder<OperatorSymbol>::SymbolBuilder(SymbolTable& table, TokenType operatorToken, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_table->emplace<OperatorSymbol>(get_operator_method_name(operatorToken), operatorToken, parent);
	m_symbol->set_return_type(returnType);
	m_symbol->set_linking(linking);
	m_symbol->set_accesibility(access);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<OperatorSymbol>::SymbolBuilder(CompilationContext& context, TokenType operatorToken, gmt::Ref<StructSymbol> returnType, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), operatorToken, returnType, linking, access, parent) { }

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::add_parameter(std::wstring_view name, gmt::Ref<StructSymbol> type)
{
	gmt::Ref<ParameterSymbol> parameter = m_factory.create_parameter(name, m_symbol, type);
	m_symbol->on_symbol_declared(parameter);
	return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::set_callback(MethodSymbolDelegate callback)
{
	if (callback == nullptr)
		throw std::invalid_argument("operator callback cannot be null");

	m_symbol->set_calling_convention(CallingConvention::ShardDecl);
	m_symbol->set_function_pointer(reinterpret_cast<void*>(callback));
	m_symbol->set_effects_computed(true);
	return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::set_may_throw(bool value)
{
	m_symbol->get_effect_summary().mayThrow = value;
	return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::set_throws_type(gmt::Ref<StructSymbol> type)
{
	if (!type.is_null())
	{
		MethodEffectSummary& summary = m_symbol->get_effect_summary();
		summary.mayThrow = true;
		summary.thrownTypes.insert(type);
	}

	return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::set_mutates_instance(bool value)
{
	m_symbol->get_effect_summary().mutatesInstance = value;
	return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::set_mutates_static(bool value)
{
	m_symbol->get_effect_summary().mutatesStatic = value;
	return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::set_mutates_arguments(bool value)
{
	m_symbol->get_effect_summary().mutatesArguments = value;
	return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::set_has_unknown_side_effects(bool value)
{
	m_symbol->get_effect_summary().hasUnknownSideEffects = value;
	return *this;
}

// =========================================================================
// PropertySymbol
// =========================================================================

SymbolBuilder<PropertySymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_property(name, parent, type, linking);
	m_symbol->set_accesibility(access);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<PropertySymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, type, linking, access, parent) { }

SymbolBuilder<FieldSymbol> SymbolBuilder<PropertySymbol>::add_backing_field()
{
	gmt::Ref<const SyntaxSymbol> constParent = m_symbol->get_parent();
	std::optional<gmt::Ref<SyntaxSymbol>> parent = m_table->get_arena().ref_from_ptr(const_cast<SyntaxSymbol*>(constParent.as_ptr()));

	gmt::Ref<FieldSymbol> field = m_factory.create_backing_field(m_symbol, parent.value_or(gmt::nullref));
	if (parent.has_value())
		(*parent)->on_symbol_declared(field);

	return SymbolBuilder<FieldSymbol>(*m_table, field);
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<PropertySymbol>::add_getter(SymbolAccesibility access)
{
	return SymbolBuilder<AccessorSymbol>(*m_table, true, access, m_symbol);
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<PropertySymbol>::add_setter(SymbolAccesibility access)
{
	return SymbolBuilder<AccessorSymbol>(*m_table, false, access, m_symbol);
}

// =========================================================================
// IndexatorSymbol
// =========================================================================

SymbolBuilder<IndexatorSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	m_symbol = m_factory.create_indexator(name, parent, type);
	m_symbol->set_linking(linking);
	m_symbol->set_accesibility(access);

	if (!parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<IndexatorSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<StructSymbol> type, SymbolLinking linking, SymbolAccesibility access, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, type, linking, access, parent) { }

SymbolBuilder<IndexatorSymbol>& SymbolBuilder<IndexatorSymbol>::add_parameter(std::wstring_view name, gmt::Ref<StructSymbol> type)
{
	gmt::Ref<ParameterSymbol> parameter = m_factory.create_parameter(name, m_symbol, type);
	m_symbol->on_symbol_declared(parameter);
	return *this;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<IndexatorSymbol>::add_getter(SymbolAccesibility access)
{
	gmt::Ref<AccessorSymbol> accessor = m_table->emplace<AccessorSymbol>(L"get", m_symbol);
	accessor->set_return_type(m_symbol->get_return_type());
	accessor->set_linking(m_symbol->get_linking());
	accessor->set_accesibility(access);

	for (gmt::Ref<ParameterSymbol> parameter : m_symbol->get_parameters())
	{
		gmt::Ref<ParameterSymbol> copy = m_factory.create_parameter(parameter->get_name(), accessor, parameter->get_type());
		accessor->on_symbol_declared(copy);
	}

	m_symbol->set_getter(accessor);
	return SymbolBuilder<AccessorSymbol>(*m_table, accessor);
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<IndexatorSymbol>::add_setter(SymbolAccesibility access)
{
	gmt::Ref<AccessorSymbol> accessor = m_table->emplace<AccessorSymbol>(L"set", m_symbol);
	accessor->set_return_type(m_table->get_void_primitive());
	accessor->set_linking(m_symbol->get_linking());
	accessor->set_accesibility(access);

	for (gmt::Ref<ParameterSymbol> parameter : m_symbol->get_parameters())
	{
		gmt::Ref<ParameterSymbol> copy = m_factory.create_parameter(parameter->get_name(), accessor, parameter->get_type());
		accessor->on_symbol_declared(copy);
	}

	gmt::Ref<ParameterSymbol> value = m_factory.create_parameter(L"value", accessor, m_symbol->get_return_type());
	accessor->on_symbol_declared(value);

	m_symbol->set_setter(accessor);
	return SymbolBuilder<AccessorSymbol>(*m_table, accessor);
}

// =========================================================================
// AccessorSymbol
// =========================================================================

SymbolBuilder<AccessorSymbol>::SymbolBuilder(SymbolTable& table, gmt::Ref<AccessorSymbol> symbol)
	: SymbolBuilderBase(table)
{
	m_symbol = symbol;
}

SymbolBuilder<AccessorSymbol>::SymbolBuilder(CompilationContext& context, gmt::Ref<AccessorSymbol> symbol)
	: SymbolBuilder(context.get_semantic_model().get_table(), symbol) { }

SymbolBuilder<AccessorSymbol>::SymbolBuilder(SymbolTable& table, bool isGetter, SymbolAccesibility access, gmt::Ref<PropertySymbol> property)
	: SymbolBuilderBase(table)
{
	m_symbol = isGetter
		? m_factory.create_getter(property)
		: m_factory.create_setter(property);

	m_symbol->set_accesibility(access);
}

SymbolBuilder<AccessorSymbol>::SymbolBuilder(CompilationContext& context, bool isGetter, SymbolAccesibility access, gmt::Ref<PropertySymbol> property)
	: SymbolBuilder(context.get_semantic_model().get_table(), isGetter, access, property) { }

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::add_parameter(std::wstring_view name, gmt::Ref<StructSymbol> type)
{
	gmt::Ref<ParameterSymbol> parameter = m_factory.create_parameter(name, m_symbol, type);
	m_symbol->on_symbol_declared(parameter);
	return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::set_callback(MethodSymbolDelegate callback)
{
	if (callback == nullptr)
		throw std::invalid_argument("accessor callback cannot be null");

	m_symbol->set_calling_convention(CallingConvention::ShardDecl);
	m_symbol->set_function_pointer(reinterpret_cast<void*>(callback));
	m_symbol->set_effects_computed(true);
	return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::set_may_throw(bool value)
{
	m_symbol->get_effect_summary().mayThrow = value;
	return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::set_throws_type(gmt::Ref<StructSymbol> type)
{
	if (!type.is_null())
	{
		MethodEffectSummary& summary = m_symbol->get_effect_summary();
		summary.mayThrow = true;
		summary.thrownTypes.insert(type);
	}

	return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::set_mutates_instance(bool value)
{
	m_symbol->get_effect_summary().mutatesInstance = value;
	return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::set_mutates_static(bool value)
{
	m_symbol->get_effect_summary().mutatesStatic = value;
	return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::set_mutates_arguments(bool value)
{
	m_symbol->get_effect_summary().mutatesArguments = value;
	return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::set_has_unknown_side_effects(bool value)
{
	m_symbol->get_effect_summary().hasUnknownSideEffects = value;
	return *this;
}

// =========================================================================
// TypeParameterSymbol
// =========================================================================

SymbolBuilder<TypeParameterSymbol>::SymbolBuilder(SymbolTable& table, std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilderBase(table)
{
	bool isMethodParent = !parent.is_null() && parent->get_kind() == SyntaxKind::FunctionDeclaration;
	bool isTypeParent = !parent.is_null() && parent->is_type();

	if (isMethodParent)
		m_symbol = m_factory.create_type_parameter(name, parent.as<MethodSymbol>());
	else if (isTypeParent)
		m_symbol = m_factory.create_type_parameter(name, parent.as<StructSymbol>());

	if (!m_symbol.is_null() && !parent.is_null())
		parent->on_symbol_declared(m_symbol);
}

SymbolBuilder<TypeParameterSymbol>::SymbolBuilder(CompilationContext& context, std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: SymbolBuilder(context.get_semantic_model().get_table(), name, parent) { }

SymbolBuilder<TypeParameterSymbol>& SymbolBuilder<TypeParameterSymbol>::add_constraint(gmt::Ref<StructSymbol> constraint)
{
	if (!constraint.is_null())
		m_symbol->add_constraint(constraint);

	return *this;
}
