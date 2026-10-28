#include <shard/semantic/DeclarationCollector.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <shard/parsing/nodes/expressions/LiteralExpressionsSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>

#include <optional>
#include <string>
#include <limits>
#include <ranges>

using namespace shard;

namespace
{
	inline bool is_namespace_scope(const SyntaxSymbol* const symbol)
	{
		return symbol->get_kind() == SyntaxKind::NamespaceDirective;
	}

	inline bool has_access_operator(StructSymbol* const type)
	{
		if ((type == nullptr))
			return false;
		
		return std::ranges::any_of(type->operators,
			[](TokenType token) { return token == TokenType::Delimeter; },
			&OperatorSymbol::get_operator_token
		);
	}

	inline std::optional<std::int64_t> parse_integer_literal(std::wstring_view text)
	{
		if (text.empty())
			return std::nullopt;

		int base = 10;
		std::size_t index = 0;

		if (text.size() > 2 && text[0] == L'0')
		{
			switch (text[1])
			{
				case L'x':
				case L'X':
				{
					base = 16;
					index = 2;
					break;
				}

				case L'b':
				case L'B':
				{
					base = 2;
					index = 2;
					break;
				}

				case L'd':
				case L'D':
				{
					index = 2;
					break;
				}

				default:
					break;
			}
		}

		std::int64_t value = 0;
		bool anyDigit = false;

		for (; index < text.size(); ++index)
		{
			const wchar_t symbol = text[index];
			if (symbol == L'`')
				continue;

			int digit;
			if (symbol >= L'0' && symbol <= L'9')
			{
				digit = symbol - L'0';
			}
			else if (base == 16 && symbol >= L'a' && symbol <= L'f')
			{
				digit = symbol - L'a' + 10;
			}
			else if (base == 16 && symbol >= L'A' && symbol <= L'F')
			{
				digit = symbol - L'A' + 10;
			}
			else
			{
				return std::nullopt;
			}

			if (digit >= base || value > (std::numeric_limits<std::int64_t>::max() - digit) / base)
				return std::nullopt;

			value = value * base + digit;
			anyDigit = true;
		}

		if (!anyDigit)
			return std::nullopt;

		return value;
	}

	inline std::optional<std::int64_t> read_enum_field_value(const ExpressionSyntax* expression)
	{
		if (expression == nullptr || expression->get_kind() != SyntaxKind::LiteralExpression)
			return std::nullopt;

		const auto* literal = static_cast<const LiteralExpressionSyntax*>(expression);
		if (literal->get_literal().get_type() != TokenType::NumberLiteral)
			return std::nullopt;

		return parse_integer_literal(literal->get_literal().get_lexeme());
	}
}

DeclarationCollector::DeclarationCollector(SemanticModel& model, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics), m_factory(&model.get_table()) { }

SyntaxSymbol* DeclarationCollector::get_declaration_parent()
{
	SemanticScope* scope = get_current_scope();

	if (!(scope->get_owner() == nullptr))
		return scope->get_owner(); 

	if (!(scope->get_namespace() == nullptr))
		return scope->get_namespace();

	return nullptr;
}

void DeclarationCollector::collect_type_parameters(StructSymbol* owner, const TypeParametersListSyntax* list, std::uint16_t ownerParameterOffset)
{
	if ((list == nullptr))
		return;

	if (owner->type_parameters.empty())
	{
		for (TypeParameterSyntax* const& parameter : list->get_parameters())
		{
			TypeParameterSymbol* symbol = m_factory.create_type_parameter(parameter->get_identifier().get_lexeme(), owner);
			symbol->set_type_argument_index(ownerParameterOffset + symbol->get_type_argument_index());
			declare(symbol);
		}
	}
	else
	{
		for (TypeParameterSymbol* const& typeParameter : owner->type_parameters)
		{
			if ((get_current_scope()->find_local(typeParameter->get_name_reference()) == nullptr))
				get_current_scope()->declare_symbol(typeParameter);
		}
	}
}

void DeclarationCollector::collect_type_parameters(MethodSymbol* owner, const TypeParametersListSyntax* list, std::uint16_t ownerParameterOffset)
{
	if ((list == nullptr))
		return;

	if (owner->get_type_parameters().empty())
	{
		for (TypeParameterSyntax* const& parameter : list->get_parameters())
		{
			TypeParameterSymbol* symbol = m_factory.create_type_parameter(parameter->get_identifier().get_lexeme(), owner);
			symbol->set_type_argument_index(ownerParameterOffset + symbol->get_type_argument_index());
			declare(symbol);
		}
	}
	else
	{
		for (TypeParameterSymbol* const& typeParameter : owner->get_type_parameters())
		{
			if ((get_current_scope()->find_local(typeParameter->get_name_reference()) == nullptr))
				get_current_scope()->declare_symbol(typeParameter);
		}
	}
}

void DeclarationCollector::visit_translation_unit(const TranslationUnitSyntax* node)
{
	std::optional<const TranslationUnitSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// nullref owner by convention represents the compilation unit itself
	push_scope(nullptr);

	if (node->has_namespace())
	{
		const NamespaceDirectiveSyntax* namespaceDirective = node->get_namespace();
		NamespaceSymbol* namespaceSymbol = lookup_symbol<NamespaceSymbol>(namespaceDirective);

		if ((namespaceSymbol == nullptr))
		{
			namespaceSymbol = m_factory.create_namespace(namespaceDirective);
			get_symbol_table().bind_symbol<NamespaceSymbol>(namespaceDirective, namespaceSymbol);
		}

		get_current_scope()->set_namespace(namespaceSymbol);
	}

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	std::optional<const StructDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	StructSymbol* symbol = lookup_symbol<StructSymbol>(*nodeRef);
	const bool isNewDeclaration = (symbol == nullptr);

	if (isNewDeclaration)
	{
		symbol = m_factory.create_struct(*nodeRef, get_declaration_parent());
		declare(symbol);

		const SyntaxSymbol* parent = symbol->get_parent();
		if (!(parent == nullptr) && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Structs can only be declared inside Namespace");
		}
	}

	push_scope(symbol);
	collect_type_parameters(symbol, node->get_type_parameters());

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);

	if (isNewDeclaration && symbol->get_linking() == LINK_INSTANCE && symbol->constructors.empty())
		declare(m_factory.create_constructor(symbol, ACS_PUBLIC));

	pop_scope();
}

void DeclarationCollector::visit_interface_declaration(const InterfaceDeclarationSyntax* node)
{
	std::optional<const InterfaceDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	InterfaceSymbol* symbol = lookup_symbol<InterfaceSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_interface(*nodeRef, get_declaration_parent());
		declare(symbol);

		const SyntaxSymbol* parent = symbol->get_parent();
		if (!(parent == nullptr) && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Interfaces can only be declared inside Namespace");
		}
	}

	push_scope(symbol);
	collect_type_parameters(symbol, node->get_type_parameters());

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);

	for (MemberDeclarationSyntax* const& member : node->get_members())
	{
		SyntaxSymbol* memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if ((memberSymbol == nullptr))
			continue;

		if (memberSymbol->is_member())
			static_cast<MemberSymbol*>(memberSymbol)->set_accesibility(ACS_PUBLIC);

		if (memberSymbol->is_method())
		{
			MethodSymbol* method = static_cast<MethodSymbol*>(memberSymbol);
			method->set_is_virtual(true);
		}

		// properties and indexers on an interface expose virtual accessors
		bool isProperty = memberSymbol->get_kind() == SyntaxKind::PropertyDeclaration;
		bool isIndexator = memberSymbol->get_kind() == SyntaxKind::IndexatorDeclaration;
		if (isProperty || isIndexator)
		{
			PropertySymbol* property = static_cast<PropertySymbol*>(memberSymbol);

			AccessorSymbol* getter = property->get_getter();
			if (!(getter == nullptr))
				getter->set_is_virtual(true);

			AccessorSymbol* setter = property->get_setter();
			if (!(setter == nullptr))
				setter->set_is_virtual(true);
		}
	}

	pop_scope();
}

void DeclarationCollector::visit_delegate_declaration(const DelegateDeclarationSyntax* node)
{
	std::optional<const DelegateDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	DelegateTypeSymbol* symbol = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_delegate(*nodeRef, get_declaration_parent());
		declare(symbol);

		const SyntaxSymbol* parent = symbol->get_parent();
		if (!(parent == nullptr) && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Delegates can only be declared inside Namespace");
		}
	}

	push_scope(symbol);
	collect_type_parameters(symbol, node->get_type_parameters());

	if (!(node->get_return_type() == nullptr))
		node->get_return_type()->accept(*this);

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_enum_declaration(const EnumDeclarationSyntax* node)
{
	std::optional<const EnumDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	EnumSymbol* symbol = lookup_symbol<EnumSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_enum(*nodeRef, get_declaration_parent());
		declare(symbol);

		const SyntaxSymbol* parent = symbol->get_parent();
		if (!(parent == nullptr) && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Enums can only be declared inside Namespace");
		}
	}

	push_scope(symbol);

	const auto fields = node->get_fields();
	std::int64_t previousValue = 0;
	for (std::size_t i = 0; i < fields.size(); ++i)
	{
		const EnumFieldDeclarationSyntax* fieldNode = fields[i];
		std::int64_t value = 0;

		if (!(fieldNode->get_expression() == nullptr))
		{
			if (symbol->get_is_flags())
			{
				get_diagnostics().report_error(fieldNode->get_identifier(), L"Flags enum fields cannot have initializers.");
				continue;
			}

			std::optional<std::int64_t> parsed = read_enum_field_value(fieldNode->get_expression());
			if (!parsed.has_value())
			{
				get_diagnostics().report_error(fieldNode->get_identifier(), L"Enum field initializer must be a constant integer literal");
				continue;
			}

			value = *parsed;
			previousValue = value;
		}
		else if (symbol->get_is_flags())
		{
			value = static_cast<std::int64_t>(1) << static_cast<std::int64_t>(i);
			previousValue = value;
		}
		else
		{
			value = i == 0 ? 0 : previousValue + 1;
			previousValue = value;
		}

		FieldSymbol* fieldSymbol = m_factory.create_enum_field(fieldNode->get_identifier().get_lexeme(), symbol, symbol, value);
		declare(fieldSymbol);
		get_symbol_table().bind_symbol<FieldSymbol>(fieldNode, fieldSymbol);
	}

	pop_scope();
}

void DeclarationCollector::visit_field_declaration(const FieldDeclarationSyntax* node)
{
	std::optional<const FieldDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	FieldSymbol* symbol = lookup_symbol<FieldSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_field(*nodeRef, get_declaration_parent());
		declare(symbol);

		const SyntaxSymbol* parent = symbol->get_parent();
		if ((parent == nullptr) || !parent->is_type())
		{
			get_diagnostics().report_error(node->get_location(), L"Fields cannot be declared outside of Structures");
		}
		else if (parent->get_kind() == SyntaxKind::InterfaceDeclaration)
		{
			get_diagnostics().report_error(node->get_location(), L"Interfaces cannot contain fields");
		}
		else
		{
			StructSymbol* ownerType = gmt::as<StructSymbol>(parent);

			if (symbol->get_linking() == LINK_INSTANCE && ownerType->get_linking() == LINK_STATIC)
				get_diagnostics().report_error(node->get_location(), L"Cannot declare a non static Field in static Type");

			if (has_access_operator(ownerType) && symbol->get_accesibility() == ACS_PUBLIC)
				get_diagnostics().report_error(node->get_location(), L"Type declaring access operator ('.') cannot have public fields");
		}
	}

	push_scope(symbol);

	if (!(node->get_type() == nullptr))
		node->get_type()->accept(*this);

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	std::optional<const FunctionDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_method(*nodeRef, get_declaration_parent());
		declare(symbol);

		for (const SyntaxToken& modifier : node->get_modifiers())
		{
			if (modifier.get_type() == TokenType::AsyncKeyword)
				symbol->set_is_async(true);
		}
	}

	push_scope(symbol);

	std::uint16_t ownerParameterOffset = 0;
	if (!(symbol->get_parent() == nullptr) && symbol->get_parent()->is_type())
		ownerParameterOffset = static_cast<std::uint16_t>(gmt::as<StructSymbol>(symbol->get_parent())->type_parameters.size());

	collect_type_parameters(symbol, node->get_type_parameters(), ownerParameterOffset);

	if (!(node->get_parameters_list() == nullptr))
	{
		const auto parameters = node->get_parameters_list()->get_parameters();
		for (ParameterSyntax* const& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	const SyntaxSymbol* parent = symbol->get_parent();
	if ((parent == nullptr) || is_namespace_scope(parent))
	{
		if (symbol->get_linking() == LINK_INSTANCE)
			symbol->set_linking(LINK_STATIC);
	}
	else if (parent->is_type())
	{
		StructSymbol* ownerType = gmt::as<StructSymbol>(parent);
		if (symbol->get_linking() == LINK_INSTANCE && ownerType->get_linking() == LINK_STATIC)
			get_diagnostics().report_error(node->get_location(), L"Cannot declare a non static Method in static Type");
	}
	else if (parent->is_member())
	{
		get_diagnostics().report_error(node->get_location(), L"Methods cannot be declared inside of members");
	}
	else
	{
		get_diagnostics().report_error(node->get_location(), L"Methods cannot be declared inside of UNKNOWN");
	}

	if ((node->get_body() == nullptr))
	{
		bool parentIsInterface = !(parent == nullptr);
		parentIsInterface = parentIsInterface && parent->get_kind() == SyntaxKind::InterfaceDeclaration;

		if (!parentIsInterface)
			get_diagnostics().report_error(node->get_location(), L"Method should have a Body, as it's not marked as 'abstract'");
	}
	else
	{
		bool parentIsInterface = !(parent == nullptr);
		parentIsInterface = parentIsInterface && parent->get_kind() == SyntaxKind::InterfaceDeclaration;

		if (parentIsInterface)
			get_diagnostics().report_error(node->get_location(), L"Method should not have a Body, as it's declared inside interface");
	}

	if (!(node->get_return_type() == nullptr))
		node->get_return_type()->accept(*this);

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	std::optional<const OperatorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	OperatorSymbol* symbol = lookup_symbol<OperatorSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_operator(*nodeRef, get_declaration_parent());
		declare(symbol);

		if (symbol->get_accesibility() != ACS_PUBLIC)
			get_diagnostics().report_error(node->get_location(), L"Operator overloads must be declared public");

		if (node->get_operator_token().get_type() != TokenType::Delimeter && symbol->get_linking() != LINK_STATIC)
			get_diagnostics().report_error(node->get_location(), L"Operator overloads must be declared static");
	}

	push_scope(symbol);

	StructSymbol* ownerType = nullptr;
	if (!(symbol->get_parent() == nullptr) && symbol->get_parent()->is_type())
		ownerType = gmt::as<StructSymbol>(symbol->get_parent());

	if (!(node->get_parameters_list() == nullptr))
	{
		const auto parameters = node->get_parameters_list()->get_parameters();
		for (ParameterSyntax* const& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	const SyntaxSymbol* parent = symbol->get_parent();
	if ((parent == nullptr))
	{
		get_diagnostics().report_error(node->get_location(), L"Cannot resolve Operators' owner Type");
	}
	else
	{
		if (!parent->is_type())
		{
			get_diagnostics().report_error(node->get_location(), L"Operators cannot be declared outside of Structures");
		}
		else
		{
			if (ownerType->get_linking() == LINK_STATIC && node->get_operator_token().get_type() != TokenType::Delimeter)
				get_diagnostics().report_error(node->get_location(), L"Cannot declare an operator overload in a static Type");
		}

		if ((node->get_body() == nullptr))
			get_diagnostics().report_error(node->get_location(), L"Operator overload should have a Body");
	}

	if (!(node->get_return_type() == nullptr))
		node->get_return_type()->accept(*this);

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(ownerType == nullptr) && node->get_operator_token().get_type() == TokenType::Delimeter)
	{
		std::size_t dotCount = 0;
		for (OperatorSymbol* const& op : ownerType->operators)
		{
			if (op->get_operator_token() == TokenType::Delimeter)
				++dotCount;
		}

		if (dotCount > 1)
			get_diagnostics().report_error(node->get_location(), L"Type can only declare one access operator ('.') overload");

		if (symbol->get_return_type() == get_symbol_table().get_void_primitive())
			get_diagnostics().report_error(node->get_location(), L"Access operator ('.') cannot return 'void'");

		for (FieldSymbol* const& field : ownerType->fields)
		{
			if (field->get_accesibility() == ACS_PUBLIC)
			{
				get_diagnostics().report_error(node->get_location(), L"Type declaring access operator ('.') cannot have public fields");
				break;
			}
		}

		if ((node->get_parameters_list() == nullptr) || node->get_parameters_list()->get_parameters().size() != 1)
			get_diagnostics().report_error(node->get_location(), L"Access operator ('.') must take exactly one parameter");
	}

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	std::optional<const ConstructorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	ConstructorSymbol* symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_constructor(*nodeRef, get_declaration_parent());
		declare(symbol);
	}

	push_scope(symbol);

	if (!(node->get_parameters_list() == nullptr))
	{
		const auto parameters = node->get_parameters_list()->get_parameters();
		for (ParameterSyntax* const& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	const SyntaxSymbol* parent = symbol->get_parent();
	if ((parent == nullptr))
	{
		get_diagnostics().report_error(node->get_location(), L"Cannot resolve Constructors' owner Type");
	}
	else
	{
		if (!parent->is_type())
		{
			get_diagnostics().report_error(node->get_location(), L"Constructors cannot be declared outside of Structures");
		}
		else
		{
			if (symbol->get_linking() == LINK_STATIC)
				get_diagnostics().report_error(node->get_location(), L"Type Constructors' cannot be static");

			if ((node->get_body() == nullptr))
				get_diagnostics().report_error(node->get_location(), L"Constructor should have a Body, as it's not marked as 'extern' or 'abstract'");
		}
	}

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_property_declaration(const PropertyDeclarationSyntax* node)
{
	std::optional<const PropertyDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	PropertySymbol* symbol = lookup_symbol<PropertySymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_property(*nodeRef, get_declaration_parent());
		declare(symbol);

		bool isInterfaceProperty = !(symbol->get_parent() == nullptr) && symbol->get_parent()->get_kind() == SyntaxKind::InterfaceDeclaration;
		bool isAutoProperty = !isInterfaceProperty && (!(node->get_getter() == nullptr) || !(node->get_setter() == nullptr));

		if (isAutoProperty)
			declare(m_factory.create_backing_field(symbol, symbol));

		const SyntaxSymbol* parent = symbol->get_parent();
		if ((parent == nullptr) || !parent->is_type())
		{
			get_diagnostics().report_error(node->get_location(), L"Properties cannot be declared outside of Structures");
		}
		else
		{
			StructSymbol* ownerType = gmt::as<StructSymbol>(parent);
			if (symbol->get_linking() == LINK_INSTANCE && ownerType->get_linking() == LINK_STATIC)
				get_diagnostics().report_error(node->get_location(), L"Cannot declare a non static Property in static Type");
		}
	}

	push_scope(symbol);

	if (!(node->get_getter() == nullptr))
		node->get_getter()->accept(*this);

	if (!(node->get_setter() == nullptr))
		node->get_setter()->accept(*this);

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_indexator_declaration(const IndexatorDeclarationSyntax* node)
{
	std::optional<const IndexatorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	IndexatorSymbol* symbol = lookup_symbol<IndexatorSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_indexator(*nodeRef, get_declaration_parent());
		declare(symbol);
	}

	push_scope(symbol);

	if (!(node->get_parameters_list() == nullptr))
	{
		const auto parameters = node->get_parameters_list()->get_parameters();
		for (ParameterSyntax* const& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	const SyntaxSymbol* parent = symbol->get_parent();
	if ((parent == nullptr))
	{
		get_diagnostics().report_error(node->get_location(), L"Cannot resolve Indexators' owner Type");
	}
	else if (!parent->is_type())
	{
		get_diagnostics().report_error(node->get_location(), L"Indexators cannot be declared outside of Structures");
	}
	else
	{
		StructSymbol* ownerType = gmt::as<StructSymbol>(parent);
		if (symbol->get_linking() == LINK_INSTANCE && ownerType->get_linking() == LINK_STATIC)
			get_diagnostics().report_error(node->get_location(), L"Cannot declare a non static Indexator in static Type");
	}

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(node->get_getter() == nullptr))
		node->get_getter()->accept(*this);

	if (!(node->get_setter() == nullptr))
		node->get_setter()->accept(*this);

	if ((node->get_getter() == nullptr) && (node->get_setter() == nullptr))
		get_diagnostics().report_error(node->get_location(), L"Indexator should have at least one accessor");

	pop_scope();
}

void DeclarationCollector::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	std::optional<const AccessorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	const SyntaxNode* parentNode = node->get_parent();
	if (parentNode->get_kind() != SyntaxKind::PropertyDeclaration && parentNode->get_kind() != SyntaxKind::IndexatorDeclaration)
	{
		get_diagnostics().report_error(node->get_location(), L"Accessors cannot be declared outside of Properties or Indexers");
		return;
	}

	PropertySymbol* propertySymbol = nullptr;
	if (parentNode->get_kind() == SyntaxKind::PropertyDeclaration)
	{
		propertySymbol = lookup_symbol<PropertySymbol>(parentNode);
	}
	else
	{
		IndexatorSymbol* indexator = lookup_symbol<IndexatorSymbol>(parentNode);
		if (!(indexator == nullptr))
			propertySymbol = indexator;
	}

	if ((propertySymbol == nullptr))
		return;

	AccessorSymbol* symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_accessor(*nodeRef, propertySymbol);
		declare(symbol);

		if ((symbol->get_parent() == nullptr))
			get_diagnostics().report_error(node->get_location(), L"Cannot resolve Accessors' owner Type");
	}

	/* TODO
	if ((propertySymbol->get_parent() == nullptr) || propertySymbol->get_parent()->get_kind() != SyntaxKind::InterfaceDeclaration)
	{
		if (!(node->get_body() == nullptr))
			get_diagnostics().report_error(node->get_location(), L"Method should not have a Body, as it's declared inside interface");
	}
	*/
}

void DeclarationCollector::visit_variable_statement(const VariableStatementSyntax* node)
{
	std::optional<const VariableStatementSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	VariableSymbol* symbol = lookup_symbol<VariableSymbol>(*nodeRef);
	if ((symbol == nullptr))
	{
		symbol = m_factory.create_variable(*nodeRef, get_owner_symbol());
		declare(symbol);
	}

	push_scope(symbol);

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_for_in_statement(const ForInStatementSyntax* node)
{
	std::optional<const ForInStatementSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	VariableSymbol* symbol = lookup_symbol<VariableSymbol>(*nodeRef);
	if ((symbol == nullptr))
		symbol = m_factory.create_variable(*nodeRef, get_owner_symbol());

	// the loop variable is scoped to the loop itself, so sibling loops may
	// reuse the same identifier
	push_scope(symbol);
	declare(symbol);

	if (!(node->get_range_expression() == nullptr))
		node->get_range_expression()->accept(*this);

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_try_block() == nullptr))
		node->get_try_block()->accept(*this);

	for (CatchClauseSyntax* const& clause : node->get_catch_clauses())
	{
		if (clause->get_identifier().get_type() == TokenType::Unknown)
		{
			if (!(clause->get_body() == nullptr))
				clause->get_body()->accept(*this);

			continue;
		}

		VariableSymbol* symbol = lookup_symbol<VariableSymbol>(clause);
		if ((symbol == nullptr))
		{
			symbol = m_factory.create_variable(clause->get_identifier().get_lexeme(), get_owner_symbol());
			declare(symbol);
			get_symbol_table().bind_symbol<VariableSymbol>(clause, symbol);
		}

		push_scope(symbol);

		if (!(clause->get_body() == nullptr))
			clause->get_body()->accept(*this);

		pop_scope();
	}
}

void DeclarationCollector::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	std::optional<const LambdaExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	SyntaxSymbol* owner = get_owner_symbol();

	// the lambda becomes a synthetic capture box: its body is the box's single
	// instance method and captured variables materialize as the box's fields
	StructSymbol* captureBox = get_symbol_table().emplace<StructSymbol>(L"CaptureBox", SyntaxKind::StructDeclaration, owner);
	captureBox->set_accesibility(ACS_PRIVATE);

	MethodSymbol* anonymousMethod = m_factory.create_method(L"Invoke", captureBox, nullptr, LINK_INSTANCE);
	captureBox->on_symbol_declared(anonymousMethod);

	push_scope(anonymousMethod);

	if (!(node->get_parameters() == nullptr))
	{
		const auto parameters = node->get_parameters()->get_parameters();
		for (ParameterSyntax* const& parameter : parameters)
			declare(m_factory.create_parameter(parameter, anonymousMethod));
	}

	DelegateTypeSymbol* delegateType = m_factory.create_delegate(anonymousMethod, owner);
	get_symbol_table().bind_symbol<DelegateTypeSymbol>(*nodeRef, delegateType);

	// the delegate signature mirrors the anonymous method; the method itself
	// is bound to the parameters list node, so the expression binder can
	// recover it when binding the body
	if (!(node->get_parameters() == nullptr))
		get_symbol_table().bind_symbol<MethodSymbol>(node->get_parameters(), anonymousMethod);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	// statement blocks introduce an anonymous scope
	// so locals shadow outer bindings instead of colliding with them
	push_scope(nullptr);

	for (StatementSyntax* const& statement : node->get_statements())
		statement->accept(*this);

	pop_scope();
}