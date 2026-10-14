#include <shard/semantic/SymbolFactory.hpp>

#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <string>

using namespace shard;

namespace
{
	std::wstring concat(std::wstring_view left, std::wstring_view right)
	{
		std::wstring result;
		result.reserve(left.size() + right.size());
		result.append(left);
		result.append(right);
		return result;
	}
}

void SymbolFactory::apply_modifiers(gmt::Span<const SyntaxToken> modifiers, MemberSymbol& symbol)
{
	SymbolAccesibility accesibility = symbol.get_accesibility();
	SymbolLinking linking = symbol.get_linking();

	for (const SyntaxToken& modifier : modifiers.as_span())
	{
		switch (modifier.get_type())
		{
			case TokenType::PublicKeyword:
			{
				accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::StaticKeyword:
			{
				linking = LINK_STATIC;
				break;
			}

			default:
				break;
		}
	}

	symbol.set_accesibility(accesibility);
	symbol.set_linking(linking);
}

gmt::Ref<NamespaceSymbol> shard::SymbolFactory::create_namespace(gmt::Ref<const NamespaceDirectiveSyntax> node)
{
	gmt::Ref<NamespaceSymbol> current = m_table->get_root_namespace();
	for (const SyntaxToken& token : node->get_qualifier().as_span())
		current = current->lookup_or_create(token.get_lexeme(), *m_table);

	return current;
}

gmt::Ref<NamespaceSymbol> SymbolFactory::create_namespace(std::wstring_view qualifiedName)
{
	gmt::Ref<NamespaceSymbol> current = m_table->get_root_namespace();

	std::size_t start = 0;
	while (true)
	{
		const std::size_t dot = qualifiedName.find(L'.', start);
		if (dot == std::wstring_view::npos)
		{
			current = current->lookup_or_create(qualifiedName.substr(start), *m_table);
			break;
		}

		current = current->lookup_or_create(qualifiedName.substr(start, dot - start), *m_table);
		start = dot + 1;
	}

	return current;
}

gmt::Ref<StructSymbol> SymbolFactory::create_struct(gmt::Ref<const StructDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<StructSymbol> symbol = m_table->emplace<StructSymbol>(node->get_identifier().get_lexeme(), SyntaxKind::StructDeclaration, parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<StructSymbol>(node, symbol);
}

gmt::Ref<StructSymbol> SymbolFactory::create_struct(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<StructSymbol> symbol = m_table->emplace<StructSymbol>(name, SyntaxKind::StructDeclaration, parent);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

gmt::Ref<InterfaceSymbol> SymbolFactory::create_interface(gmt::Ref<const InterfaceDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<InterfaceSymbol> symbol = m_table->emplace<InterfaceSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<InterfaceSymbol>(node, symbol);
}

gmt::Ref<InterfaceSymbol> SymbolFactory::create_interface(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, SymbolAccesibility accesibility)
{
	gmt::Ref<InterfaceSymbol> symbol = m_table->emplace<InterfaceSymbol>(name, parent);
	symbol->set_accesibility(accesibility);
	return symbol;
}

gmt::Ref<EnumSymbol> SymbolFactory::create_enum(gmt::Ref<const EnumDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<EnumSymbol> symbol = m_table->emplace<EnumSymbol>(node->get_identifier().get_lexeme(), parent);
	symbol->set_is_flags(node->get_is_flags());
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<EnumSymbol>(node, symbol);
}

gmt::Ref<EnumSymbol> SymbolFactory::create_enum(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, bool isFlags)
{
	gmt::Ref<EnumSymbol> symbol = m_table->emplace<EnumSymbol>(name, parent);
	symbol->set_is_flags(isFlags);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

gmt::Ref<FieldSymbol> SymbolFactory::create_field(gmt::Ref<const FieldDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<FieldSymbol> symbol = m_table->emplace<FieldSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<FieldSymbol>(node, symbol);
}

gmt::Ref<FieldSymbol> SymbolFactory::create_field(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> type, SymbolLinking linking)
{
	gmt::Ref<FieldSymbol> symbol = m_table->emplace<FieldSymbol>(name, parent);
	symbol->set_return_type(type);
	symbol->set_linking(linking);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

gmt::Ref<FieldSymbol> SymbolFactory::create_enum_field(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> enumType, std::int64_t value)
{
	gmt::Ref<FieldSymbol> symbol = m_table->emplace<FieldSymbol>(name, parent);
	symbol->set_return_type(enumType);
	symbol->set_linking(LINK_STATIC);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_is_enum_value(true);
	symbol->set_enum_value(value);
	return symbol;
}

gmt::Ref<FieldSymbol> SymbolFactory::create_backing_field(gmt::Ref<PropertySymbol> property, gmt::Ref<SyntaxSymbol> parent)
{
	static constexpr std::wstring_view suffix = L">k__BackingField";
	const std::wstring_view propertyName = property->get_name();

	std::wstring backingName;
	backingName.reserve(1 + propertyName.size() + suffix.size());
	backingName.append(L"<");
	backingName.append(propertyName);
	backingName.append(suffix);

	gmt::Ref<FieldSymbol> backingField = create_field(backingName, parent, property->get_return_type(), property->get_linking());
	property->set_backing_field(backingField);
	return backingField;
}

gmt::Ref<PropertySymbol> SymbolFactory::create_property(gmt::Ref<const PropertyDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<PropertySymbol> symbol = m_table->emplace<PropertySymbol>(node->get_identifier().get_lexeme(), SyntaxKind::PropertyDeclaration, parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<PropertySymbol>(node, symbol);
}

gmt::Ref<PropertySymbol> SymbolFactory::create_property(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType, SymbolLinking linking)
{
	gmt::Ref<PropertySymbol> symbol = m_table->emplace<PropertySymbol>(name, SyntaxKind::PropertyDeclaration, parent);
	symbol->set_return_type(returnType);
	symbol->set_linking(linking);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

gmt::Ref<MethodSymbol> SymbolFactory::create_method(gmt::Ref<const FunctionDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<MethodSymbol> symbol = m_table->emplace<MethodSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<MethodSymbol>(node, symbol);
}

gmt::Ref<MethodSymbol> SymbolFactory::create_method(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType, SymbolLinking linking)
{
	gmt::Ref<MethodSymbol> symbol = m_table->emplace<MethodSymbol>(name, parent);
	symbol->set_return_type(returnType);
	symbol->set_linking(linking);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

gmt::Ref<OperatorSymbol> SymbolFactory::create_operator(gmt::Ref<const OperatorDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	const TokenType operatorToken = node->get_operator_token().get_type();
	gmt::Ref<OperatorSymbol> symbol = m_table->emplace<OperatorSymbol>(get_operator_method_name(operatorToken), operatorToken, parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<OperatorSymbol>(node, symbol);
}

gmt::Ref<ConstructorSymbol> SymbolFactory::create_constructor(gmt::Ref<const ConstructorDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<ConstructorSymbol> symbol = m_table->emplace<ConstructorSymbol>(L"init", parent);
	symbol->set_return_type(m_table->get_void_primitive());
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<ConstructorSymbol>(node, symbol);
}

gmt::Ref<ConstructorSymbol> SymbolFactory::create_constructor(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, SymbolAccesibility accesibility)
{
	gmt::Ref<ConstructorSymbol> symbol = m_table->emplace<ConstructorSymbol>(name, parent);
	symbol->set_return_type(m_table->get_void_primitive());
	symbol->set_accesibility(accesibility);
	return symbol;
}

gmt::Ref<AccessorSymbol> SymbolFactory::create_accessor(gmt::Ref<const AccessorDeclarationSyntax> node, gmt::Ref<PropertySymbol> property)
{
	const std::wstring_view propertyName = property->get_name();
	const std::wstring_view keyword = node->get_keyword().get_lexeme();

	std::wstring accessorName;
	accessorName.reserve(propertyName.size() + 1 + keyword.size());
	accessorName.append(propertyName);
	accessorName.append(L"_");
	accessorName.append(keyword);

	gmt::Ref<AccessorSymbol> symbol = m_table->emplace<AccessorSymbol>(accessorName, property);
	apply_modifiers(node->get_modifiers(), *symbol);
	symbol->set_linking(property->get_linking());

	switch (node->get_keyword().get_type())
	{
		case TokenType::GetKeyword:
		{
			symbol->set_return_type(property->get_return_type());
			property->set_getter(symbol);
			break;
		}

		case TokenType::SetKeyword:
		{
			gmt::Ref<ParameterSymbol> valueParam = create_parameter(L"value", symbol, property->get_return_type());
			symbol->on_symbol_declared(valueParam);
			
			symbol->set_return_type(m_table->get_void_primitive());
			property->set_setter(symbol);
			break;
		}

		default:
			break;
	}

	return m_table->bind_symbol<AccessorSymbol>(node, symbol);
}

gmt::Ref<AccessorSymbol> SymbolFactory::create_getter(gmt::Ref<PropertySymbol> property)
{
	gmt::Ref<AccessorSymbol> symbol = m_table->emplace<AccessorSymbol>(concat(L"get_", property->get_name()), property);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_linking(property->get_linking());
	symbol->set_return_type(property->get_return_type());

	property->set_getter(symbol);
	return symbol;
}

gmt::Ref<AccessorSymbol> SymbolFactory::create_setter(gmt::Ref<PropertySymbol> property)
{
	gmt::Ref<AccessorSymbol> symbol = m_table->emplace<AccessorSymbol>(concat(L"set_", property->get_name()), property);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_linking(property->get_linking());
	symbol->set_return_type(m_table->get_void_primitive());

	gmt::Ref<ParameterSymbol> valueParam = create_parameter(L"value", symbol, property->get_return_type());
	symbol->on_symbol_declared(valueParam);

	property->set_setter(symbol);
	return symbol;
}

gmt::Ref<IndexatorSymbol> SymbolFactory::create_indexator(gmt::Ref<const IndexatorDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<IndexatorSymbol> symbol = m_table->emplace<IndexatorSymbol>(L"index", parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<IndexatorSymbol>(node, symbol);
}

gmt::Ref<IndexatorSymbol> SymbolFactory::create_indexator(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType)
{
	gmt::Ref<IndexatorSymbol> symbol = m_table->emplace<IndexatorSymbol>(name, parent);
	symbol->set_return_type(returnType);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_linking(LINK_INSTANCE);
	return symbol;
}

gmt::Ref<IndexatorSymbol> SymbolFactory::create_indexator(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType, std::vector<gmt::Ref<ParameterSymbol>> parameters)
{
	gmt::Ref<IndexatorSymbol> symbol = create_indexator(name, parent, returnType);
	for (const gmt::Ref<ParameterSymbol>& parameter : parameters)
		symbol->on_symbol_declared(parameter);

	return symbol;
}

gmt::Ref<ParameterSymbol> SymbolFactory::create_parameter(gmt::Ref<const ParameterSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<ParameterSymbol> symbol = m_table->emplace<ParameterSymbol>(node->get_identifier().get_lexeme(), parent);
	return m_table->bind_symbol<ParameterSymbol>(node, symbol);
}

gmt::Ref<ParameterSymbol> SymbolFactory::create_parameter(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
{
	return m_table->emplace<ParameterSymbol>(name, parent);
}

gmt::Ref<ParameterSymbol> SymbolFactory::create_parameter(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> type)
{
	gmt::Ref<ParameterSymbol> symbol = m_table->emplace<ParameterSymbol>(name, parent);
	symbol->set_type(type);
	return symbol;
}

gmt::Ref<VariableSymbol> SymbolFactory::create_variable(gmt::Ref<const VariableStatementSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<VariableSymbol> symbol = m_table->emplace<VariableSymbol>(node->get_identifier().get_lexeme(), parent);
	return m_table->bind_symbol<VariableSymbol>(node, symbol);
}

gmt::Ref<VariableSymbol> SymbolFactory::create_variable(gmt::Ref<const ForInStatementSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<VariableSymbol> symbol = m_table->emplace<VariableSymbol>(node->get_identifier().get_lexeme(), parent);
	return m_table->bind_symbol<VariableSymbol>(node, symbol);
}

gmt::Ref<VariableSymbol> SymbolFactory::create_variable(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
{
	return m_table->emplace<VariableSymbol>(name, parent);
}

gmt::Ref<VariableSymbol> SymbolFactory::create_variable(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> type)
{
	gmt::Ref<VariableSymbol> symbol = m_table->emplace<VariableSymbol>(name, parent);
	symbol->set_type(type);
	return symbol;
}

gmt::Ref<TypeParameterSymbol> SymbolFactory::create_type_parameter(std::wstring_view name, gmt::Ref<StructSymbol> parent)
{
	gmt::Ref<TypeParameterSymbol> symbol = m_table->emplace<TypeParameterSymbol>(name, parent);
	symbol->set_type_argument_index(static_cast<std::uint16_t>(parent->type_parameters.size()));
	return symbol;
}

gmt::Ref<TypeParameterSymbol> SymbolFactory::create_type_parameter(std::wstring_view name, gmt::Ref<MethodSymbol> parent)
{
	gmt::Ref<TypeParameterSymbol> symbol = m_table->emplace<TypeParameterSymbol>(name, parent);
	symbol->set_type_argument_index(static_cast<std::uint16_t>(parent->get_type_parameters().size()));
	return symbol;
}

gmt::Ref<DelegateTypeSymbol> SymbolFactory::create_delegate(gmt::Ref<const DelegateDeclarationSyntax> node, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<DelegateTypeSymbol> symbol = m_table->emplace<DelegateTypeSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	symbol->set_return_type(m_table->get_void_primitive());

	if (!node->get_parameters_list().is_null())
	{
		auto& parameters = const_cast<std::vector<gmt::Ref<ParameterSymbol>>&>(symbol->get_parameters());
		for (const gmt::Ref<ParameterSyntax>& parameter : node->get_parameters_list()->get_parameters().as_span())
		{
			gmt::Ref<ParameterSymbol> parameterSymbol = create_parameter(parameter->get_identifier().get_lexeme(), symbol);
			parameters.push_back(parameterSymbol);
		}
	}

	return m_table->bind_symbol<DelegateTypeSymbol>(node, symbol);
}

gmt::Ref<DelegateTypeSymbol> SymbolFactory::create_delegate(gmt::Ref<MethodSymbol> method, gmt::Ref<SyntaxSymbol> parent)
{
	gmt::Ref<DelegateTypeSymbol> symbol = m_table->emplace<DelegateTypeSymbol>(method->get_name(), parent);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_return_type(method->get_return_type());
	symbol->set_parameters(method->get_parameters());
	symbol->set_is_async(method->get_is_async());

	for (const gmt::Ref<ParameterSymbol>& parameter : method->get_parameters())
		symbol->on_symbol_declared(parameter);

	for (const gmt::Ref<TypeParameterSymbol>& typeParameter : method->get_type_parameters())
		symbol->on_symbol_declared(typeParameter);

	return symbol;
}

gmt::Ref<DelegateTypeSymbol> SymbolFactory::create_delegate(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent, gmt::Ref<StructSymbol> returnType, std::vector<gmt::Ref<ParameterSymbol>> parameters)
{
	gmt::Ref<DelegateTypeSymbol> symbol = m_table->emplace<DelegateTypeSymbol>(name, parent);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_return_type(returnType);
	symbol->set_parameters(parameters);
	return symbol;
}

gmt::Ref<ArrayTypeSymbol> SymbolFactory::create_array_type(gmt::Ref<const ArrayTypeSyntax> node, gmt::Ref<StructSymbol> underlayingType)
{
	gmt::Ref<ArrayTypeSymbol> symbol = m_table->find_or_create_array_type(underlayingType);
	return m_table->bind_symbol<ArrayTypeSymbol>(node, symbol);
}

gmt::Ref<ArrayTypeSymbol> SymbolFactory::create_array_type(gmt::Ref<StructSymbol> underlayingType)
{
	return m_table->find_or_create_array_type(underlayingType);
}

gmt::Ref<ArrayTypeSymbol> SymbolFactory::create_array_type(gmt::Ref<StructSymbol> underlayingType, std::size_t size)
{
	return m_table->find_or_create_array_type(underlayingType, size);
}

gmt::Ref<NullableTypeSymbol> SymbolFactory::create_nullable_type(gmt::Ref<const NullableTypeSyntax> node, gmt::Ref<StructSymbol> underlayingType)
{
	gmt::Ref<NullableTypeSymbol> symbol = m_table->find_or_create_nullable_type(underlayingType);
	return m_table->bind_symbol<NullableTypeSymbol>(node, symbol);
}

gmt::Ref<NullableTypeSymbol> SymbolFactory::create_nullable_type(gmt::Ref<StructSymbol> underlayingType)
{
	return m_table->find_or_create_nullable_type(underlayingType);
}

gmt::Ref<GenericTypeSymbol> SymbolFactory::create_generic_type(gmt::Ref<const GenericTypeSyntax> node, gmt::Ref<StructSymbol> underlayingType)
{
	gmt::Ref<GenericTypeSymbol> symbol = m_table->find_or_create_generic_type(underlayingType, {});
	return m_table->bind_symbol<GenericTypeSymbol>(node, symbol);
}

gmt::Ref<GenericTypeSymbol> SymbolFactory::create_generic_type(gmt::Ref<StructSymbol> underlayingType)
{
	return m_table->find_or_create_generic_type(underlayingType, {});
}

gmt::Ref<GenericTypeSymbol> SymbolFactory::create_generic_type(gmt::Ref<StructSymbol> underlayingType, const std::vector<gmt::Ref<StructSymbol>>& typeArguments)
{
	return m_table->find_or_create_generic_type(underlayingType, typeArguments);
}
