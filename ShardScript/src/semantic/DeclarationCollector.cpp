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
	inline bool is_namespace_scope(const gmt::Ref<const SyntaxSymbol> symbol)
	{
		return symbol->get_kind() == SyntaxKind::NamespaceDirective;
	}

	inline bool has_access_operator(const gmt::Ref<StructSymbol> type)
	{
		if (type.is_null())
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

gmt::Ref<SyntaxSymbol> DeclarationCollector::get_declaration_parent()
{
	SemanticScope* scope = get_current_scope();

	if (!scope->get_owner().is_null())
		return scope->get_owner(); 

	if (!scope->get_namespace().is_null())
		return scope->get_namespace();

	return gmt::nullref;
}

void DeclarationCollector::collect_type_parameters(gmt::Ref<StructSymbol> owner, gmt::Ref<const TypeParametersListSyntax> list, std::uint16_t ownerParameterOffset)
{
	if (list.is_null())
		return;

	if (owner->type_parameters.empty())
	{
		for (const gmt::Ref<TypeParameterSyntax>& parameter : list->get_parameters().as_span())
		{
			gmt::Ref<TypeParameterSymbol> symbol = m_factory.create_type_parameter(parameter->get_identifier().get_lexeme(), owner);
			symbol->set_type_argument_index(ownerParameterOffset + symbol->get_type_argument_index());
			declare(symbol);
		}
	}
	else
	{
		for (const gmt::Ref<TypeParameterSymbol>& typeParameter : owner->type_parameters)
		{
			if (get_current_scope()->find_local(typeParameter->get_name_reference()).is_null())
				get_current_scope()->declare_symbol(typeParameter);
		}
	}
}

void DeclarationCollector::collect_type_parameters(gmt::Ref<MethodSymbol> owner, gmt::Ref<const TypeParametersListSyntax> list, std::uint16_t ownerParameterOffset)
{
	if (list.is_null())
		return;

	if (owner->get_type_parameters().empty())
	{
		for (const gmt::Ref<TypeParameterSyntax>& parameter : list->get_parameters().as_span())
		{
			gmt::Ref<TypeParameterSymbol> symbol = m_factory.create_type_parameter(parameter->get_identifier().get_lexeme(), owner);
			symbol->set_type_argument_index(ownerParameterOffset + symbol->get_type_argument_index());
			declare(symbol);
		}
	}
	else
	{
		for (const gmt::Ref<TypeParameterSymbol>& typeParameter : owner->get_type_parameters())
		{
			if (get_current_scope()->find_local(typeParameter->get_name_reference()).is_null())
				get_current_scope()->declare_symbol(typeParameter);
		}
	}
}

void DeclarationCollector::visit_translation_unit(const TranslationUnitSyntax* node)
{
	std::optional<gmt::Ref<const TranslationUnitSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// nullref owner by convention represents the compilation unit itself
	push_scope(gmt::nullref);

	if (node->has_namespace())
	{
		gmt::Ref<const NamespaceDirectiveSyntax> namespaceDirective = node->get_namespace();
		gmt::Ref<NamespaceSymbol> namespaceSymbol = lookup_symbol<NamespaceSymbol>(namespaceDirective);

		if (namespaceSymbol.is_null())
		{
			namespaceSymbol = m_factory.create_namespace(namespaceDirective);
			get_symbol_table().bind_symbol<NamespaceSymbol>(namespaceDirective, namespaceSymbol);
		}

		get_current_scope()->set_namespace(namespaceSymbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const StructDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<StructSymbol> symbol = lookup_symbol<StructSymbol>(*nodeRef);
	const bool isNewDeclaration = symbol.is_null();

	if (isNewDeclaration)
	{
		symbol = m_factory.create_struct(*nodeRef, get_declaration_parent());
		declare(symbol);

		gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
		if (!parent.is_null() && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Structs can only be declared inside Namespace");
		}
	}

	push_scope(symbol);
	collect_type_parameters(symbol, node->get_type_parameters());

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	if (isNewDeclaration && symbol->get_linking() == LINK_INSTANCE && symbol->constructors.empty())
		declare(m_factory.create_constructor(symbol, ACS_PUBLIC));

	pop_scope();
}

void DeclarationCollector::visit_interface_declaration(const InterfaceDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const InterfaceDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<InterfaceSymbol> symbol = lookup_symbol<InterfaceSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_interface(*nodeRef, get_declaration_parent());
		declare(symbol);

		gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
		if (!parent.is_null() && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Interfaces can only be declared inside Namespace");
		}
	}

	push_scope(symbol);
	collect_type_parameters(symbol, node->get_type_parameters());

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if (memberSymbol.is_null())
			continue;

		if (memberSymbol->is_member())
			memberSymbol.as<MemberSymbol>()->set_accesibility(ACS_PUBLIC);

		if (memberSymbol->is_method())
		{
			gmt::Ref<MethodSymbol> method = memberSymbol.as<MethodSymbol>();
			method->set_is_virtual(true);
		}

		// properties and indexers on an interface expose virtual accessors
		bool isProperty = memberSymbol->get_kind() == SyntaxKind::PropertyDeclaration;
		bool isIndexator = memberSymbol->get_kind() == SyntaxKind::IndexatorDeclaration;
		if (isProperty || isIndexator)
		{
			gmt::Ref<PropertySymbol> property = memberSymbol.as<PropertySymbol>();

			gmt::Ref<AccessorSymbol> getter = property->get_getter();
			if (!getter.is_null())
				getter->set_is_virtual(true);

			gmt::Ref<AccessorSymbol> setter = property->get_setter();
			if (!setter.is_null())
				setter->set_is_virtual(true);
		}
	}

	pop_scope();
}

void DeclarationCollector::visit_delegate_declaration(const DelegateDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const DelegateDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<DelegateTypeSymbol> symbol = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_delegate(*nodeRef, get_declaration_parent());
		declare(symbol);

		gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
		if (!parent.is_null() && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Delegates can only be declared inside Namespace");
		}
	}

	push_scope(symbol);
	collect_type_parameters(symbol, node->get_type_parameters());

	if (!node->get_return_type().is_null())
		node->get_return_type().as_ptr()->accept(*this);

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_enum_declaration(const EnumDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const EnumDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<EnumSymbol> symbol = lookup_symbol<EnumSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_enum(*nodeRef, get_declaration_parent());
		declare(symbol);

		gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
		if (!parent.is_null() && !is_namespace_scope(parent))
		{
			get_diagnostics().report_error(node->get_location(), L"Enums can only be declared inside Namespace");
		}
	}

	push_scope(symbol);

	const auto fields = node->get_fields().as_span();
	std::int64_t previousValue = 0;
	for (std::size_t i = 0; i < fields.size(); ++i)
	{
		gmt::Ref<const EnumFieldDeclarationSyntax> fieldNode = fields[i];
		std::int64_t value = 0;

		if (!fieldNode->get_expression().is_null())
		{
			if (symbol->get_is_flags())
			{
				get_diagnostics().report_error(fieldNode->get_identifier(), L"Flags enum fields cannot have initializers.");
				continue;
			}

			std::optional<std::int64_t> parsed = read_enum_field_value(fieldNode->get_expression().as_ptr());
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

		gmt::Ref<FieldSymbol> fieldSymbol = m_factory.create_enum_field(fieldNode->get_identifier().get_lexeme(), symbol, symbol, value);
		declare(fieldSymbol);
		get_symbol_table().bind_symbol<FieldSymbol>(fieldNode, fieldSymbol);
	}

	pop_scope();
}

void DeclarationCollector::visit_field_declaration(const FieldDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const FieldDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<FieldSymbol> symbol = lookup_symbol<FieldSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_field(*nodeRef, get_declaration_parent());
		declare(symbol);

		gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
		if (parent.is_null() || !parent->is_type())
		{
			get_diagnostics().report_error(node->get_location(), L"Fields cannot be declared outside of Structures");
		}
		else if (parent->get_kind() == SyntaxKind::InterfaceDeclaration)
		{
			get_diagnostics().report_error(node->get_location(), L"Interfaces cannot contain fields");
		}
		else
		{
			gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();

			if (symbol->get_linking() == LINK_INSTANCE && ownerType->get_linking() == LINK_STATIC)
				get_diagnostics().report_error(node->get_location(), L"Cannot declare a non static Field in static Type");

			if (has_access_operator(ownerType) && symbol->get_accesibility() == ACS_PUBLIC)
				get_diagnostics().report_error(node->get_location(), L"Type declaring access operator ('.') cannot have public fields");
		}
	}

	push_scope(symbol);

	if (!node->get_type().is_null())
		node->get_type().as_ptr()->accept(*this);

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const FunctionDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_method(*nodeRef, get_declaration_parent());
		declare(symbol);

		for (const SyntaxToken& modifier : node->get_modifiers().as_span())
		{
			if (modifier.get_type() == TokenType::AsyncKeyword)
				symbol->set_is_async(true);
		}
	}

	push_scope(symbol);

	std::uint16_t ownerParameterOffset = 0;
	if (!symbol->get_parent().is_null() && symbol->get_parent()->is_type())
		ownerParameterOffset = static_cast<std::uint16_t>(symbol->get_parent().as<StructSymbol>()->type_parameters.size());

	collect_type_parameters(symbol, node->get_type_parameters(), ownerParameterOffset);

	if (!node->get_parameters_list().is_null())
	{
		const auto parameters = node->get_parameters_list()->get_parameters().as_span();
		for (const gmt::Ref<ParameterSyntax>& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null() || is_namespace_scope(parent))
	{
		if (symbol->get_linking() == LINK_INSTANCE)
			symbol->set_linking(LINK_STATIC);
	}
	else if (parent->is_type())
	{
		gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();
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

	if (node->get_body().is_null())
	{
		bool parentIsInterface = !parent.is_null();
		parentIsInterface = parentIsInterface && parent->get_kind() == SyntaxKind::InterfaceDeclaration;

		if (!parentIsInterface)
			get_diagnostics().report_error(node->get_location(), L"Method should have a Body, as it's not marked as 'abstract'");
	}
	else
	{
		bool parentIsInterface = !parent.is_null();
		parentIsInterface = parentIsInterface && parent->get_kind() == SyntaxKind::InterfaceDeclaration;

		if (parentIsInterface)
			get_diagnostics().report_error(node->get_location(), L"Method should not have a Body, as it's declared inside interface");
	}

	if (!node->get_return_type().is_null())
		node->get_return_type().as_ptr()->accept(*this);

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const OperatorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<OperatorSymbol> symbol = lookup_symbol<OperatorSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_operator(*nodeRef, get_declaration_parent());
		declare(symbol);

		if (symbol->get_accesibility() != ACS_PUBLIC)
			get_diagnostics().report_error(node->get_location(), L"Operator overloads must be declared public");

		if (node->get_operator_token().get_type() != TokenType::Delimeter && symbol->get_linking() != LINK_STATIC)
			get_diagnostics().report_error(node->get_location(), L"Operator overloads must be declared static");
	}

	push_scope(symbol);

	gmt::Ref<StructSymbol> ownerType = gmt::nullref;
	if (!symbol->get_parent().is_null() && symbol->get_parent()->is_type())
		ownerType = symbol->get_parent().as<StructSymbol>();

	if (!node->get_parameters_list().is_null())
	{
		const auto parameters = node->get_parameters_list()->get_parameters().as_span();
		for (const gmt::Ref<ParameterSyntax>& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null())
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

		if (node->get_body().is_null())
			get_diagnostics().report_error(node->get_location(), L"Operator overload should have a Body");
	}

	if (!node->get_return_type().is_null())
		node->get_return_type().as_ptr()->accept(*this);

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!ownerType.is_null() && node->get_operator_token().get_type() == TokenType::Delimeter)
	{
		std::size_t dotCount = 0;
		for (const gmt::Ref<OperatorSymbol>& op : ownerType->operators)
		{
			if (op->get_operator_token() == TokenType::Delimeter)
				++dotCount;
		}

		if (dotCount > 1)
			get_diagnostics().report_error(node->get_location(), L"Type can only declare one access operator ('.') overload");

		if (symbol->get_return_type() == get_symbol_table().get_void_primitive())
			get_diagnostics().report_error(node->get_location(), L"Access operator ('.') cannot return 'void'");

		for (const gmt::Ref<FieldSymbol>& field : ownerType->fields)
		{
			if (field->get_accesibility() == ACS_PUBLIC)
			{
				get_diagnostics().report_error(node->get_location(), L"Type declaring access operator ('.') cannot have public fields");
				break;
			}
		}

		if (node->get_parameters_list().is_null() || node->get_parameters_list()->get_parameters().as_span().size() != 1)
			get_diagnostics().report_error(node->get_location(), L"Access operator ('.') must take exactly one parameter");
	}

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const ConstructorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_constructor(*nodeRef, get_declaration_parent());
		declare(symbol);
	}

	push_scope(symbol);

	if (!node->get_parameters_list().is_null())
	{
		const auto parameters = node->get_parameters_list()->get_parameters().as_span();
		for (const gmt::Ref<ParameterSyntax>& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null())
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

			if (node->get_body().is_null())
				get_diagnostics().report_error(node->get_location(), L"Constructor should have a Body, as it's not marked as 'extern' or 'abstract'");
		}
	}

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_property_declaration(const PropertyDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const PropertyDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<PropertySymbol> symbol = lookup_symbol<PropertySymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_property(*nodeRef, get_declaration_parent());
		declare(symbol);

		bool isInterfaceProperty = !symbol->get_parent().is_null() && symbol->get_parent()->get_kind() == SyntaxKind::InterfaceDeclaration;
		bool isAutoProperty = !isInterfaceProperty && (!node->get_getter().is_null() || !node->get_setter().is_null());

		if (isAutoProperty)
			declare(m_factory.create_backing_field(symbol, symbol));

		gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
		if (parent.is_null() || !parent->is_type())
		{
			get_diagnostics().report_error(node->get_location(), L"Properties cannot be declared outside of Structures");
		}
		else
		{
			gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();
			if (symbol->get_linking() == LINK_INSTANCE && ownerType->get_linking() == LINK_STATIC)
				get_diagnostics().report_error(node->get_location(), L"Cannot declare a non static Property in static Type");
		}
	}

	push_scope(symbol);

	if (!node->get_getter().is_null())
		node->get_getter().as_ptr()->accept(*this);

	if (!node->get_setter().is_null())
		node->get_setter().as_ptr()->accept(*this);

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_indexator_declaration(const IndexatorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const IndexatorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<IndexatorSymbol> symbol = lookup_symbol<IndexatorSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_indexator(*nodeRef, get_declaration_parent());
		declare(symbol);
	}

	push_scope(symbol);

	if (!node->get_parameters_list().is_null())
	{
		const auto parameters = node->get_parameters_list()->get_parameters().as_span();
		for (const gmt::Ref<ParameterSyntax>& parameter : parameters)
			declare(m_factory.create_parameter(parameter, symbol));
	}

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Cannot resolve Indexators' owner Type");
	}
	else if (!parent->is_type())
	{
		get_diagnostics().report_error(node->get_location(), L"Indexators cannot be declared outside of Structures");
	}
	else
	{
		gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();
		if (symbol->get_linking() == LINK_INSTANCE && ownerType->get_linking() == LINK_STATIC)
			get_diagnostics().report_error(node->get_location(), L"Cannot declare a non static Indexator in static Type");
	}

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!node->get_getter().is_null())
		node->get_getter().as_ptr()->accept(*this);

	if (!node->get_setter().is_null())
		node->get_setter().as_ptr()->accept(*this);

	if (node->get_getter().is_null() && node->get_setter().is_null())
		get_diagnostics().report_error(node->get_location(), L"Indexator should have at least one accessor");

	pop_scope();
}

void DeclarationCollector::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const AccessorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<const SyntaxNode> parentNode = node->get_parent();
	if (parentNode->get_kind() != SyntaxKind::PropertyDeclaration && parentNode->get_kind() != SyntaxKind::IndexatorDeclaration)
	{
		get_diagnostics().report_error(node->get_location(), L"Accessors cannot be declared outside of Properties or Indexers");
		return;
	}

	gmt::Ref<PropertySymbol> propertySymbol = gmt::nullref;
	if (parentNode->get_kind() == SyntaxKind::PropertyDeclaration)
	{
		propertySymbol = lookup_symbol<PropertySymbol>(parentNode);
	}
	else
	{
		gmt::Ref<IndexatorSymbol> indexator = lookup_symbol<IndexatorSymbol>(parentNode);
		if (!indexator.is_null())
			propertySymbol = indexator;
	}

	if (propertySymbol.is_null())
		return;

	gmt::Ref<AccessorSymbol> symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_accessor(*nodeRef, propertySymbol);
		declare(symbol);

		if (symbol->get_parent().is_null())
			get_diagnostics().report_error(node->get_location(), L"Cannot resolve Accessors' owner Type");
	}

	/* TODO
	if (propertySymbol->get_parent().is_null() || propertySymbol->get_parent()->get_kind() != SyntaxKind::InterfaceDeclaration)
	{
		if (!node->get_body().is_null())
			get_diagnostics().report_error(node->get_location(), L"Method should not have a Body, as it's declared inside interface");
	}
	*/
}

void DeclarationCollector::visit_variable_statement(const VariableStatementSyntax* node)
{
	std::optional<gmt::Ref<const VariableStatementSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<VariableSymbol> symbol = lookup_symbol<VariableSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_variable(*nodeRef, get_owner_symbol());
		declare(symbol);
	}

	push_scope(symbol);

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_for_in_statement(const ForInStatementSyntax* node)
{
	std::optional<gmt::Ref<const ForInStatementSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<VariableSymbol> symbol = lookup_symbol<VariableSymbol>(*nodeRef);
	if (symbol.is_null())
	{
		symbol = m_factory.create_variable(*nodeRef, get_owner_symbol());
		declare(symbol);
	}

	push_scope(symbol);

	if (!node->get_range_expression().is_null())
		node->get_range_expression().as_ptr()->accept(*this);

	if (!node->get_block().is_null())
		node->get_block().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_try_block().is_null())
		node->get_try_block().as_ptr()->accept(*this);

	for (const gmt::Ref<CatchClauseSyntax>& clause : node->get_catch_clauses().as_span())
	{
		if (clause->get_identifier().get_type() == TokenType::Unknown)
		{
			if (!clause->get_body().is_null())
				clause->get_body().as_ptr()->accept(*this);

			continue;
		}

		gmt::Ref<VariableSymbol> symbol = lookup_symbol<VariableSymbol>(clause);
		if (symbol.is_null())
		{
			symbol = m_factory.create_variable(clause->get_identifier().get_lexeme(), get_owner_symbol());
			declare(symbol);
			get_symbol_table().bind_symbol<VariableSymbol>(clause, symbol);
		}

		push_scope(symbol);

		if (!clause->get_body().is_null())
			clause->get_body().as_ptr()->accept(*this);

		pop_scope();
	}
}

void DeclarationCollector::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	std::optional<gmt::Ref<const LambdaExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<SyntaxSymbol> owner = get_owner_symbol();
	gmt::Ref<MethodSymbol> anonymousMethod = m_factory.create_method(L"Lambda", owner, gmt::nullref, LINK_STATIC);

	push_scope(anonymousMethod);

	if (!node->get_parameters().is_null())
	{
		const auto parameters = node->get_parameters()->get_parameters().as_span();
		for (const gmt::Ref<ParameterSyntax>& parameter : parameters)
			declare(m_factory.create_parameter(parameter, anonymousMethod));
	}

	gmt::Ref<DelegateTypeSymbol> delegateType = m_factory.create_delegate(anonymousMethod, owner);
	get_symbol_table().bind_symbol<DelegateTypeSymbol>(*nodeRef, delegateType);

	// the delegate signature mirrors the anonymous method; the method itself
	// is bound to the parameters list node, so the expression binder can
	// recover it when binding the body
	if (!node->get_parameters().is_null())
		get_symbol_table().bind_symbol<MethodSymbol>(node->get_parameters(), anonymousMethod);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	pop_scope();
}

void DeclarationCollector::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	// statement blocks introduce an anonymous scope
	// so locals shadow outer bindings instead of colliding with them
	push_scope(gmt::nullref);

	for (const gmt::Ref<StatementSyntax>& statement : node->get_statements().as_span())
		statement.as_ptr()->accept(*this);

	pop_scope();
}
