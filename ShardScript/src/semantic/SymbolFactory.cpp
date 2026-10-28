#include <shard/semantic/SymbolFactory.hpp>

#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <gmt/Containers.hpp>

#include <string>

using namespace shard;

namespace
{
	gmt::wstring concat(std::wstring_view left, std::wstring_view right)
	{
		gmt::wstring result;
		result.reserve(left.size() + right.size());
		result.append(left);
		result.append(right);
		return result;
	}
}

void SymbolFactory::apply_modifiers(std::span<const SyntaxToken> modifiers, MemberSymbol& symbol)
{
	SymbolAccesibility accesibility = symbol.get_accesibility();
	SymbolLinking linking = symbol.get_linking();

	for (const SyntaxToken& modifier : modifiers)
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

NamespaceSymbol* shard::SymbolFactory::create_namespace(const NamespaceDirectiveSyntax* node)
{
	NamespaceSymbol* current = m_table->get_root_namespace();
	for (const SyntaxToken& token : node->get_qualifier())
		current = current->lookup_or_create(token.get_lexeme(), *m_table);

	return current;
}

NamespaceSymbol* SymbolFactory::create_namespace(std::wstring_view qualifiedName)
{
	NamespaceSymbol* current = m_table->get_root_namespace();

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

StructSymbol* SymbolFactory::create_struct(const StructDeclarationSyntax* node, SyntaxSymbol* parent)
{
	StructSymbol* symbol = m_table->emplace<StructSymbol>(node->get_identifier().get_lexeme(), SyntaxKind::StructDeclaration, parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<StructSymbol>(node, symbol);
}

StructSymbol* SymbolFactory::create_struct(std::wstring_view name, SyntaxSymbol* parent)
{
	StructSymbol* symbol = m_table->emplace<StructSymbol>(name, SyntaxKind::StructDeclaration, parent);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

InterfaceSymbol* SymbolFactory::create_interface(const InterfaceDeclarationSyntax* node, SyntaxSymbol* parent)
{
	InterfaceSymbol* symbol = m_table->emplace<InterfaceSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<InterfaceSymbol>(node, symbol);
}

InterfaceSymbol* SymbolFactory::create_interface(std::wstring_view name, SyntaxSymbol* parent, SymbolAccesibility accesibility)
{
	InterfaceSymbol* symbol = m_table->emplace<InterfaceSymbol>(name, parent);
	symbol->set_accesibility(accesibility);
	return symbol;
}

EnumSymbol* SymbolFactory::create_enum(const EnumDeclarationSyntax* node, SyntaxSymbol* parent)
{
	EnumSymbol* symbol = m_table->emplace<EnumSymbol>(node->get_identifier().get_lexeme(), parent);
	symbol->set_is_flags(node->get_is_flags());
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<EnumSymbol>(node, symbol);
}

EnumSymbol* SymbolFactory::create_enum(std::wstring_view name, SyntaxSymbol* parent, bool isFlags)
{
	EnumSymbol* symbol = m_table->emplace<EnumSymbol>(name, parent);
	symbol->set_is_flags(isFlags);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

FieldSymbol* SymbolFactory::create_field(const FieldDeclarationSyntax* node, SyntaxSymbol* parent)
{
	FieldSymbol* symbol = m_table->emplace<FieldSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<FieldSymbol>(node, symbol);
}

FieldSymbol* SymbolFactory::create_field(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* type, SymbolLinking linking)
{
	FieldSymbol* symbol = m_table->emplace<FieldSymbol>(name, parent);
	symbol->set_return_type(type);
	symbol->set_linking(linking);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

FieldSymbol* SymbolFactory::create_enum_field(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* enumType, std::int64_t value)
{
	FieldSymbol* symbol = m_table->emplace<FieldSymbol>(name, parent);
	symbol->set_return_type(enumType);
	symbol->set_linking(LINK_STATIC);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_is_enum_value(true);
	symbol->set_enum_value(value);
	return symbol;
}

FieldSymbol* SymbolFactory::create_backing_field(PropertySymbol* property, SyntaxSymbol* parent)
{
	static constexpr std::wstring_view suffix = L">k__BackingField";
	const std::wstring_view propertyName = property->get_name();

	gmt::wstring backingName;
	backingName.reserve(1 + propertyName.size() + suffix.size());
	backingName.append(L"<");
	backingName.append(propertyName);
	backingName.append(suffix);

	FieldSymbol* backingField = create_field(backingName, parent, property->get_return_type(), property->get_linking());
	property->set_backing_field(backingField);
	return backingField;
}

PropertySymbol* SymbolFactory::create_property(const PropertyDeclarationSyntax* node, SyntaxSymbol* parent)
{
	PropertySymbol* symbol = m_table->emplace<PropertySymbol>(node->get_identifier().get_lexeme(), SyntaxKind::PropertyDeclaration, parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<PropertySymbol>(node, symbol);
}

PropertySymbol* SymbolFactory::create_property(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType, SymbolLinking linking)
{
	PropertySymbol* symbol = m_table->emplace<PropertySymbol>(name, SyntaxKind::PropertyDeclaration, parent);
	symbol->set_return_type(returnType);
	symbol->set_linking(linking);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

MethodSymbol* SymbolFactory::create_method(const FunctionDeclarationSyntax* node, SyntaxSymbol* parent)
{
	MethodSymbol* symbol = m_table->emplace<MethodSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<MethodSymbol>(node, symbol);
}

MethodSymbol* SymbolFactory::create_method(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType, SymbolLinking linking)
{
	MethodSymbol* symbol = m_table->emplace<MethodSymbol>(name, parent);
	symbol->set_return_type(returnType);
	symbol->set_linking(linking);
	symbol->set_accesibility(ACS_PRIVATE);
	return symbol;
}

OperatorSymbol* SymbolFactory::create_operator(const OperatorDeclarationSyntax* node, SyntaxSymbol* parent)
{
	const TokenType operatorToken = node->get_operator_token().get_type();
	OperatorSymbol* symbol = m_table->emplace<OperatorSymbol>(get_operator_method_name(operatorToken), operatorToken, parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<OperatorSymbol>(node, symbol);
}

ConstructorSymbol* SymbolFactory::create_constructor(const ConstructorDeclarationSyntax* node, SyntaxSymbol* parent)
{
	ConstructorSymbol* symbol = m_table->emplace<ConstructorSymbol>(parent);
	symbol->set_return_type(m_table->get_void_primitive());
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<ConstructorSymbol>(node, symbol);
}

ConstructorSymbol* SymbolFactory::create_constructor(SyntaxSymbol* parent, SymbolAccesibility accesibility)
{
	ConstructorSymbol* symbol = m_table->emplace<ConstructorSymbol>(parent);
	symbol->set_return_type(m_table->get_void_primitive());
	symbol->set_accesibility(accesibility);
	return symbol;
}

AccessorSymbol* SymbolFactory::create_accessor(const AccessorDeclarationSyntax* node, PropertySymbol* property)
{
	const std::wstring_view propertyName = property->get_name();
	const std::wstring_view keyword = node->get_keyword().get_lexeme();

	gmt::wstring accessorName;
	accessorName.reserve(propertyName.size() + 1 + keyword.size());
	accessorName.append(propertyName);
	accessorName.append(L"_");
	accessorName.append(keyword);

	AccessorSymbol* symbol = m_table->emplace<AccessorSymbol>(accessorName, property);
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
			ParameterSymbol* valueParam = create_parameter(L"value", symbol, property->get_return_type());
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

AccessorSymbol* SymbolFactory::create_getter(PropertySymbol* property)
{
	AccessorSymbol* symbol = m_table->emplace<AccessorSymbol>(concat(L"get_", property->get_name()), property);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_linking(property->get_linking());
	symbol->set_return_type(property->get_return_type());

	property->set_getter(symbol);
	return symbol;
}

AccessorSymbol* SymbolFactory::create_setter(PropertySymbol* property)
{
	AccessorSymbol* symbol = m_table->emplace<AccessorSymbol>(concat(L"set_", property->get_name()), property);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_linking(property->get_linking());
	symbol->set_return_type(m_table->get_void_primitive());

	ParameterSymbol* valueParam = create_parameter(L"value", symbol, property->get_return_type());
	symbol->on_symbol_declared(valueParam);

	property->set_setter(symbol);
	return symbol;
}

IndexatorSymbol* SymbolFactory::create_indexator(const IndexatorDeclarationSyntax* node, SyntaxSymbol* parent)
{
	IndexatorSymbol* symbol = m_table->emplace<IndexatorSymbol>(L"index", parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	return m_table->bind_symbol<IndexatorSymbol>(node, symbol);
}

IndexatorSymbol* SymbolFactory::create_indexator(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType)
{
	IndexatorSymbol* symbol = m_table->emplace<IndexatorSymbol>(name, parent);
	symbol->set_return_type(returnType);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_linking(LINK_INSTANCE);
	return symbol;
}

IndexatorSymbol* SymbolFactory::create_indexator(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType, gmt::vector<ParameterSymbol*> parameters)
{
	IndexatorSymbol* symbol = create_indexator(name, parent, returnType);
	for (ParameterSymbol* const& parameter : parameters)
		symbol->on_symbol_declared(parameter);

	return symbol;
}

ParameterSymbol* SymbolFactory::create_parameter(const ParameterSyntax* node, SyntaxSymbol* parent)
{
	ParameterSymbol* symbol = m_table->emplace<ParameterSymbol>(node->get_identifier().get_lexeme(), parent);
	return m_table->bind_symbol<ParameterSymbol>(node, symbol);
}

ParameterSymbol* SymbolFactory::create_parameter(std::wstring_view name, SyntaxSymbol* parent)
{
	return m_table->emplace<ParameterSymbol>(name, parent);
}

ParameterSymbol* SymbolFactory::create_parameter(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* type)
{
	ParameterSymbol* symbol = m_table->emplace<ParameterSymbol>(name, parent);
	symbol->set_type(type);
	return symbol;
}

VariableSymbol* SymbolFactory::create_variable(const VariableStatementSyntax* node, SyntaxSymbol* parent)
{
	VariableSymbol* symbol = m_table->emplace<VariableSymbol>(node->get_identifier().get_lexeme(), parent);
	return m_table->bind_symbol<VariableSymbol>(node, symbol);
}

VariableSymbol* SymbolFactory::create_variable(const ForInStatementSyntax* node, SyntaxSymbol* parent)
{
	VariableSymbol* symbol = m_table->emplace<VariableSymbol>(node->get_identifier().get_lexeme(), parent);
	return m_table->bind_symbol<VariableSymbol>(node, symbol);
}

VariableSymbol* SymbolFactory::create_variable(std::wstring_view name, SyntaxSymbol* parent)
{
	return m_table->emplace<VariableSymbol>(name, parent);
}

VariableSymbol* SymbolFactory::create_variable(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* type)
{
	VariableSymbol* symbol = m_table->emplace<VariableSymbol>(name, parent);
	symbol->set_type(type);
	return symbol;
}

TypeParameterSymbol* SymbolFactory::create_type_parameter(std::wstring_view name, StructSymbol* parent)
{
	TypeParameterSymbol* symbol = m_table->emplace<TypeParameterSymbol>(name, parent);
	symbol->set_type_argument_index(static_cast<std::uint16_t>(parent->type_parameters.size()));
	return symbol;
}

TypeParameterSymbol* SymbolFactory::create_type_parameter(std::wstring_view name, MethodSymbol* parent)
{
	TypeParameterSymbol* symbol = m_table->emplace<TypeParameterSymbol>(name, parent);
	symbol->set_type_argument_index(static_cast<std::uint16_t>(parent->get_type_parameters().size()));
	return symbol;
}

DelegateTypeSymbol* SymbolFactory::create_delegate(const DelegateDeclarationSyntax* node, SyntaxSymbol* parent)
{
	DelegateTypeSymbol* symbol = m_table->emplace<DelegateTypeSymbol>(node->get_identifier().get_lexeme(), parent);
	apply_modifiers(node->get_modifiers(), *symbol);
	symbol->set_return_type(m_table->get_void_primitive());

	if (!(node->get_parameters_list() == nullptr))
	{
		auto& parameters = const_cast<gmt::vector<ParameterSymbol*>&>(symbol->get_parameters());
		for (ParameterSyntax* const& parameter : node->get_parameters_list()->get_parameters())
			parameters.push_back(create_parameter(parameter, symbol));
	}

	return m_table->bind_symbol<DelegateTypeSymbol>(node, symbol);
}

DelegateTypeSymbol* SymbolFactory::create_delegate(MethodSymbol* method, SyntaxSymbol* parent)
{
	DelegateTypeSymbol* symbol = m_table->emplace<DelegateTypeSymbol>(method->get_name(), parent);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_return_type(method->get_return_type());
	symbol->set_parameters(method->get_parameters());
	symbol->set_is_async(method->get_is_async());

	for (ParameterSymbol* const& parameter : method->get_parameters())
		symbol->on_symbol_declared(parameter);

	for (TypeParameterSymbol* const& typeParameter : method->get_type_parameters())
		symbol->on_symbol_declared(typeParameter);

	return symbol;
}

DelegateTypeSymbol* SymbolFactory::create_delegate(std::wstring_view name, SyntaxSymbol* parent, StructSymbol* returnType, gmt::vector<ParameterSymbol*> parameters)
{
	DelegateTypeSymbol* symbol = m_table->emplace<DelegateTypeSymbol>(name, parent);
	symbol->set_accesibility(ACS_PUBLIC);
	symbol->set_return_type(returnType);
	symbol->set_parameters(parameters);
	return symbol;
}

ArrayTypeSymbol* SymbolFactory::create_array_type(const ArrayTypeSyntax* node, StructSymbol* underlayingType)
{
	ArrayTypeSymbol* symbol = m_table->find_or_create_array_type(underlayingType);
	return m_table->bind_symbol<ArrayTypeSymbol>(node, symbol);
}

ArrayTypeSymbol* SymbolFactory::create_array_type(StructSymbol* underlayingType)
{
	return m_table->find_or_create_array_type(underlayingType);
}

ArrayTypeSymbol* SymbolFactory::create_array_type(StructSymbol* underlayingType, std::size_t size)
{
	return m_table->find_or_create_array_type(underlayingType, size);
}

NullableTypeSymbol* SymbolFactory::create_nullable_type(const NullableTypeSyntax* node, StructSymbol* underlayingType)
{
	NullableTypeSymbol* symbol = m_table->find_or_create_nullable_type(underlayingType);
	return m_table->bind_symbol<NullableTypeSymbol>(node, symbol);
}

NullableTypeSymbol* SymbolFactory::create_nullable_type(StructSymbol* underlayingType)
{
	return m_table->find_or_create_nullable_type(underlayingType);
}

GenericTypeSymbol* SymbolFactory::create_generic_type(const GenericTypeSyntax* node, StructSymbol* underlayingType)
{
	GenericTypeSymbol* symbol = m_table->find_or_create_generic_type(underlayingType, {});
	return m_table->bind_symbol<GenericTypeSymbol>(node, symbol);
}

GenericTypeSymbol* SymbolFactory::create_generic_type(StructSymbol* underlayingType)
{
	return m_table->find_or_create_generic_type(underlayingType, {});
}

GenericTypeSymbol* SymbolFactory::create_generic_type(StructSymbol* underlayingType, std::span<StructSymbol* const> typeArguments)
{
	return m_table->find_or_create_generic_type(underlayingType, typeArguments);
}
