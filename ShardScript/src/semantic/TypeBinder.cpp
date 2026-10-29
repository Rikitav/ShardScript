#include <shard/semantic/TypeBinder.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/TokenType.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>

#include <stdexcept>
#include <string>

using namespace shard;

namespace
{
	bool is_overloadable_declaration(SyntaxKind kind)
	{
		switch (kind)
		{
			case SyntaxKind::FunctionDeclaration:
			case SyntaxKind::ConstructorDeclaration:
			case SyntaxKind::AccessorDeclaration:
			case SyntaxKind::OperatorDeclaration:
				return true;

			default:
				return false;
		}
	}
}

TypeBinder::TypeBinder(SemanticModel& model, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics), m_factory(&model.get_table()) { }

StructSymbol* TypeBinder::resolve_type(const TypeSyntax* node)
{
	if (node == nullptr)
		return nullptr;

	node->accept(*this);
	return lookup_symbol<StructSymbol>(node);
}

void TypeBinder::bind_arguments(const ArgumentsListSyntax* arguments)
{
	if (arguments == nullptr)
		return;

	for (ArgumentSyntax* const& argument : arguments->get_arguments())
		bind_expression(argument->get_expression());
}

void TypeBinder::bind_expression(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	switch (expression->get_kind())
	{
		case SyntaxKind::ObjectExpression:
		{
			visit_object_creation_expression(static_cast<const ObjectExpressionSyntax*>(expression));
			break;
		}

		case SyntaxKind::BinaryExpression:
		case SyntaxKind::RangeExpression:
		{
			bind_expression(gmt::as<BinaryExpressionSyntax>(expression)->get_left());
			bind_expression(gmt::as<BinaryExpressionSyntax>(expression)->get_right());
			break;
		}

		case SyntaxKind::UnaryExpression:
		{
			bind_expression(gmt::as<UnaryExpressionSyntax>(expression)->get_operand());
			break;
		}

		case SyntaxKind::AwaitExpression:
		{
			bind_expression(gmt::as<AwaitExpressionSyntax>(expression)->get_expression());
			break;
		}

		case SyntaxKind::IfExpression:
		{
			bind_expression(gmt::as<IfExpressionSyntax>(expression)->get_condition());
			bind_expression(gmt::as<IfExpressionSyntax>(expression)->get_then_expression());
			bind_expression(gmt::as<IfExpressionSyntax>(expression)->get_else_expression());
			break;
		}

		case SyntaxKind::CastExpression:
		case SyntaxKind::IsExpression:
		{
			bind_expression(gmt::as<CastExpressionSyntax>(expression)->get_expression());
			resolve_type(gmt::as<CastExpressionSyntax>(expression)->get_target_type());
			break;
		}

		case SyntaxKind::CollectionExpression:
		{
			for (ExpressionSyntax* const& value : gmt::as<CollectionExpressionSyntax>(expression)->get_values())
				bind_expression(value);

			break;
		}

		case SyntaxKind::MemberAccessExpression:
		{
			bind_expression(gmt::as<MemberAccessExpressionSyntax>(expression)->get_previous());
			break;
		}

		case SyntaxKind::InvokationExpression:
		{
			const InvokationExpressionSyntax* invocation = gmt::as<InvokationExpressionSyntax>(expression);
			bind_expression(invocation->get_previous());

			const TypeArgumentsListSyntax* typeArguments = invocation->get_type_arguments();
			if (!(typeArguments == nullptr))
			{
				for (TypeSyntax* const& argument : typeArguments->get_types())
					argument->accept(*this);
			}

			bind_arguments(invocation->get_arguments());
			break;
		}

		case SyntaxKind::IndexatorExpression:
		{
			bind_expression(gmt::as<IndexatorExpressionSyntax>(expression)->get_previous());
			bind_arguments(gmt::as<IndexatorExpressionSyntax>(expression)->get_arguments());
			break;
		}

		case SyntaxKind::LambdaExpression:
		{
			visit_lambda_expression(static_cast<const LambdaExpressionSyntax*>(expression));
			break;
		}

		default:
		{
			// literals and identifier-name operands carry no type information
			// for this pass; everything else resolves later
			break;
		}
	}
}

void TypeBinder::import_namespace(const UsingDirectiveSyntax* node, NamespaceSymbol* namespaceSymbol)
{
	SemanticScope* current = get_current_scope();

	if (current->get_imported_namespaces().count(namespaceSymbol) != 0)
	{
		get_diagnostics().report_error(node->get_location(), L"Namespace is already imported");
		return;
	}

	current->add_imported_namespace(namespaceSymbol);

	// namespace members become visible without qualification; a name that
	// already resolves to a different symbol is ambiguous from now on
	for (SyntaxSymbol* const& member : namespaceSymbol->members)
	{
		if (member == nullptr)
			continue;

		SyntaxSymbol* existing = current->find_local(member->get_name());
		if (!(existing == nullptr))
		{
			if (existing != member)
			{
				bool overloadablePair = is_overloadable_declaration(existing->get_kind()) && is_overloadable_declaration(member->get_kind());
				if (overloadablePair)
					current->declare_symbol(member);
				else
					m_ambiguousNames.insert(member->get_name());
			}

			continue;
		}

		current->declare_symbol(member);
	}
}

bool TypeBinder::is_ambiguous_name(std::wstring_view name)
{
	return m_ambiguousNames.count(name) != 0;
}

void TypeBinder::bind_where_clauses(const WhereClausesListSyntax* clauses, const gmt::vector<TypeParameterSymbol*>& typeParameters)
{
	if (clauses == nullptr)
		return;

	for (WhereClauseSyntax* const& clause : clauses->get_clauses())
	{
		if (clause == nullptr)
			continue;

		const std::wstring_view parameterName = clause->get_identifier().get_lexeme();

		TypeParameterSymbol* typeParameter = nullptr;
		for (TypeParameterSymbol* const& candidate : typeParameters)
		{
			if (!(candidate == nullptr) && candidate->get_name() == parameterName)
			{
				typeParameter = candidate;
				break;
			}
		}

		if (typeParameter == nullptr)
		{
			get_diagnostics().report_error(clause->get_location(), L"Type parameter '" + gmt::wstring(parameterName) + L"' not found");
			continue;
		}

		for (TypeSyntax* const& constraintType : clause->get_constraint_types())
		{
			StructSymbol* constraint = resolve_type(constraintType);
			if (constraint == nullptr)
			{
				get_diagnostics().report_error(clause->get_location(), L"Could not resolve constraint type for type parameter '" + gmt::wstring(parameterName) + L"'");
				continue;
			}

			typeParameter->add_constraint(constraint);
		}
	}
}

void TypeBinder::bind_base_types(StructSymbol* symbol, const BaseTypesListSyntax* baseTypes)
{
	if (baseTypes == nullptr)
		return;

	for (TypeSyntax* const& baseType : baseTypes->get_types())
	{
		StructSymbol* baseSymbol = resolve_type(baseType);
		if (baseSymbol == nullptr)
			continue;

		if (baseSymbol->get_kind() == SyntaxKind::InterfaceDeclaration)
		{
			symbol->interfaces.push_back(baseSymbol);
			continue;
		}

		if (baseSymbol->get_kind() == SyntaxKind::GenericType)
		{
			StructSymbol* underlaying = static_cast<GenericTypeSymbol*>(baseSymbol)->get_underlaying_type();
			if (!(underlaying == nullptr) && underlaying->get_kind() == SyntaxKind::InterfaceDeclaration)
			{
				symbol->interfaces.push_back(baseSymbol);
				continue;
			}
		}

		get_diagnostics().report_error(baseType->get_location(), L"Base type must be an interface");
	}
}

void TypeBinder::visit_translation_unit(const TranslationUnitSyntax* node)
{
	push_scope(nullptr);

	for (UsingDirectiveSyntax* const& directive : node->get_usings())
		visit_using_directive(directive);

	if (node->has_namespace())
	{
		const NamespaceDirectiveSyntax* directive = node->get_namespace();
		NamespaceSymbol* namespaceSymbol = lookup_symbol<NamespaceSymbol>(directive);

		if (namespaceSymbol == nullptr)
			throw std::runtime_error("namespace symbol not found");

		push_scope(namespaceSymbol);
	}

	// declare member symbols upfront so members can reference each other
	// regardless of declaration order; the binder re-declares into its own
	// fresh scopes, so this bypasses the owner hook on purpose - calling
	// declare() here would duplicate the member lists populated during
	// collection
	for (MemberDeclarationSyntax* const& member : node->get_members())
	{
		SyntaxSymbol* memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if (!(memberSymbol == nullptr))
			get_current_scope()->declare_symbol(memberSymbol);
	}

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);

	pop_scope();

	if (node->has_namespace())
		pop_scope();
}

void TypeBinder::visit_using_directive(const UsingDirectiveSyntax* node)
{
	std::optional<const UsingDirectiveSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	NamespaceSymbol* current = get_symbol_table().get_root_namespace();
	for (const SyntaxToken& token : node->get_qualifier())
	{
		NamespaceSymbol* next = current->lookup(token.get_lexeme());
		if (next == nullptr)
		{
			get_diagnostics().report_error(node->get_location(), L"Namespace '" + gmt::wstring(token.get_lexeme()) + L"' does not exist");
			return;
		}

		current = next;
	}

	if (nodeRef.has_value())
		get_symbol_table().bind_symbol<NamespaceSymbol>(*nodeRef, current);

	import_namespace(*nodeRef, current);
}

void TypeBinder::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	std::optional<const StructDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	StructSymbol* symbol = lookup_symbol<StructSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("struct symbol not found");

	push_scope(symbol);

	for (TypeParameterSymbol* const& typeParameter : symbol->type_parameters)
		get_current_scope()->declare_symbol(typeParameter);

	bind_where_clauses(node->get_where_clauses(), symbol->type_parameters);
	bind_base_types(symbol, node->get_base_types());

	for (MemberDeclarationSyntax* const& member : node->get_members())
	{
		SyntaxSymbol* memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if (!(memberSymbol == nullptr))
			get_current_scope()->declare_symbol(memberSymbol);
	}

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);

	pop_scope();
}

void TypeBinder::visit_interface_declaration(const InterfaceDeclarationSyntax* node)
{
	std::optional<const InterfaceDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	InterfaceSymbol* symbol = lookup_symbol<InterfaceSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("interface symbol not found");

	push_scope(symbol);

	for (TypeParameterSymbol* const& typeParameter : symbol->type_parameters)
		get_current_scope()->declare_symbol(typeParameter);

	bind_where_clauses(node->get_where_clauses(), symbol->type_parameters);

	for (MemberDeclarationSyntax* const& member : node->get_members())
	{
		SyntaxSymbol* memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if (!(memberSymbol == nullptr))
			get_current_scope()->declare_symbol(memberSymbol);
	}

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);

	pop_scope();
}

void TypeBinder::visit_delegate_declaration(const DelegateDeclarationSyntax* node)
{
	std::optional<const DelegateDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	DelegateTypeSymbol* symbol = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("delegate symbol not found");

	push_scope(symbol);

	for (TypeParameterSymbol* const& typeParameter : symbol->get_type_parameters())
		get_current_scope()->declare_symbol(typeParameter);

	if (!(node->get_return_type() == nullptr))
		symbol->set_return_type(resolve_type(node->get_return_type()));

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_enum_declaration(const EnumDeclarationSyntax* node)
{
	std::optional<const EnumDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	EnumSymbol* symbol = lookup_symbol<EnumSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("enum symbol not found");

	push_scope(symbol);

	for (EnumFieldDeclarationSyntax* const& field : node->get_fields())
		bind_expression(field->get_expression());

	pop_scope();
}

void TypeBinder::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	std::optional<const ConstructorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	ConstructorSymbol* symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("constructor symbol not found");

	push_scope(symbol);

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	std::optional<const FunctionDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("method symbol not found");

	push_scope(symbol);

	for (TypeParameterSymbol* const& typeParameter : symbol->get_type_parameters())
		get_current_scope()->declare_symbol(typeParameter);

	bind_where_clauses(node->get_where_clauses(), symbol->get_type_parameters());

	if (!(node->get_return_type() == nullptr))
		symbol->set_return_type(resolve_type(node->get_return_type()));

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	std::optional<const OperatorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	OperatorSymbol* symbol = lookup_symbol<OperatorSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("operator symbol not found");

	push_scope(symbol);

	if (!(node->get_return_type() == nullptr))
		symbol->set_return_type(resolve_type(node->get_return_type()));

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	if (symbol->get_operator_token() == TokenType::AsOperator)
	{
		if (symbol->get_linking() != LINK_STATIC)
			get_diagnostics().report_error(node->get_location(), L"Conversion operator 'as' must be static");

		if (symbol->get_parameters().size() != 1)
			get_diagnostics().report_error(node->get_location(), L"Conversion operator 'as' must take exactly one parameter");
	}

	pop_scope();
}

void TypeBinder::visit_field_declaration(const FieldDeclarationSyntax* node)
{
	std::optional<const FieldDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	FieldSymbol* symbol = lookup_symbol<FieldSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("field symbol not found");

	if (!(node->get_type() == nullptr))
		symbol->set_return_type(resolve_type(node->get_type()));

	bind_expression(node->get_expression());
}

void TypeBinder::visit_property_declaration(const PropertyDeclarationSyntax* node)
{
	std::optional<const PropertyDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	PropertySymbol* symbol = lookup_symbol<PropertySymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("property symbol not found");

	if ((node->get_type() == nullptr))
		return;

	push_scope(symbol);

	StructSymbol* propertyType = resolve_type(node->get_type());
	symbol->set_return_type(propertyType);

	FieldSymbol* backingField = symbol->get_backing_field();
	if (!(backingField == nullptr))
		backingField->set_return_type(propertyType);

	AccessorSymbol* getter = symbol->get_getter();
	if (!(getter == nullptr))
		getter->set_return_type(propertyType);

	AccessorSymbol* setter = symbol->get_setter();
	if (!(setter == nullptr) && !setter->get_parameters().empty())
		setter->get_parameters()[0]->set_type(propertyType);

	bind_expression(node->get_expression());

	if (!(node->get_setter() == nullptr))
		visit_accessor_declaration(node->get_setter());

	if (!(node->get_getter() == nullptr))
		visit_accessor_declaration(node->get_getter());

	pop_scope();
}

void TypeBinder::visit_indexator_declaration(const IndexatorDeclarationSyntax* node)
{
	std::optional<const IndexatorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	IndexatorSymbol* symbol = lookup_symbol<IndexatorSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("indexator symbol not found");

	push_scope(symbol);

	if (!(node->get_type() == nullptr))
	{
		StructSymbol* propertyType = resolve_type(node->get_type());
		symbol->set_return_type(propertyType);

		AccessorSymbol* getter = symbol->get_getter();
		if (!(getter == nullptr))
			getter->set_return_type(propertyType);

		AccessorSymbol* setter = symbol->get_setter();
		if (!(setter == nullptr) && !setter->get_parameters().empty())
			setter->get_parameters()[0]->set_type(propertyType);
	}

	if (!(node->get_parameters_list() == nullptr))
		node->get_parameters_list()->accept(*this);

	if (!(node->get_setter() == nullptr))
		visit_accessor_declaration(node->get_setter());

	if (!(node->get_getter() == nullptr))
		visit_accessor_declaration(node->get_getter());

	pop_scope();
}

void TypeBinder::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	std::optional<const AccessorDeclarationSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	AccessorSymbol* symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("accessor symbol not found");

	const SyntaxNode* parentNode = node->get_parent();

	if (parentNode->get_kind() == SyntaxKind::IndexatorDeclaration)
	{
		// index parameters join each accessor's own parameter list; the
		// implicit setter 'value' parameter keeps the leading slot
		IndexatorSymbol* indexator = lookup_symbol<IndexatorSymbol>(parentNode);
		if (!(indexator == nullptr))
		{
			for (ParameterSymbol* const& indexParameter : indexator->get_parameters())
				symbol->on_symbol_declared(indexParameter);
		}
	}
	else
	{
		PropertySymbol* property = lookup_symbol<PropertySymbol>(parentNode);
		if (!(property == nullptr) && node->get_keyword().get_type() == TokenType::GetKeyword)
			symbol->set_return_type(property->get_return_type());
	}
}

void TypeBinder::visit_variable_statement(const VariableStatementSyntax* node)
{
	std::optional<const VariableStatementSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	VariableSymbol* symbol = lookup_symbol<VariableSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("variable symbol not found");

	if (!(node->get_type() == nullptr))
		symbol->set_type(resolve_type(node->get_type()));

	bind_expression(node->get_expression());
}

void TypeBinder::visit_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_try_block() == nullptr))
		node->get_try_block()->accept(*this);

	for (CatchClauseSyntax* const& clause : node->get_catch_clauses())
	{
		StructSymbol* exceptionType = nullptr;		if (!(clause->get_exception_type() == nullptr))
			exceptionType = resolve_type(clause->get_exception_type());

		if (!(exceptionType == nullptr))
		{
			StructSymbol* throwable = static_cast<StructSymbol*>(get_symbol_table().get_standard_type(StandardTypes::IThrowable));
			if (!get_model().is_assignable_to(throwable, exceptionType))
				get_diagnostics().report_error(clause->get_exception_type()->get_location(), L"Catch type must implement IThrowable");
		}

		VariableSymbol* catchVariable = lookup_symbol<VariableSymbol>(clause);
		if (!(catchVariable == nullptr))
			catchVariable->set_type(exceptionType);

		if (!(clause->get_body() == nullptr))
			clause->get_body()->accept(*this);
	}
}

void TypeBinder::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	std::optional<const LambdaExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	if ((node->get_parameters() == nullptr))
		return;

	for (ParameterSyntax* const& parameter : node->get_parameters()->get_parameters())
	{
		ParameterSymbol* symbol = lookup_symbol<ParameterSymbol>(parameter);
		if (symbol == nullptr)
			continue;

		if (!(parameter->get_type() == nullptr))
			symbol->set_type(resolve_type(parameter->get_type()));
	}

	// the body belongs to the expression binder for values, but type syntaxes
	// inside it (and signatures of nested lambdas) are bound here
	MethodSymbol* anonymousMethod = lookup_symbol<MethodSymbol>(node->get_parameters());
	if (anonymousMethod == nullptr)
		return;

	push_scope(anonymousMethod);

	for (ParameterSymbol* const& parameter : anonymousMethod->get_parameters())
		get_current_scope()->declare_symbol(parameter);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	std::optional<const ObjectExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	if ((node->get_type() == nullptr))
		return;

	StructSymbol* createdType = resolve_type(node->get_type());

	bind_expression(node->get_array_size());
	bind_arguments(node->get_arguments());

	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, createdType);
}

void TypeBinder::visit_parameter(const ParameterSyntax* node)
{
	std::optional<const ParameterSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	ParameterSymbol* symbol = lookup_symbol<ParameterSymbol>(*nodeRef);
	if (symbol == nullptr)
		throw std::runtime_error("parameter symbol not found");

	if (!(node->get_type() == nullptr))
		symbol->set_type(resolve_type(node->get_type()));
}

void TypeBinder::visit_predefined_type(const PredefinedTypeSyntax* node)
{
	std::optional<const PredefinedTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	SymbolTable& table = get_symbol_table();
	StructSymbol* symbol = nullptr;

	switch (node->get_type_token().get_type())
	{
		case TokenType::BooleanKeyword: symbol = table.get_boolean_primitive(); break;
		case TokenType::IntegerKeyword: symbol = table.get_integer_primitive(); break;
		case TokenType::DoubleKeyword: symbol = table.get_double_primitive(); break;
		case TokenType::CharKeyword: symbol = table.get_char_primitive(); break;
		case TokenType::ByteKeyword: symbol = table.get_byte_primitive(); break;
		case TokenType::NativeIntegerKeyword: symbol = table.get_nint_primitive(); break;
		case TokenType::StringKeyword: symbol = table.get_string_primitive(); break;
		case TokenType::VoidKeyword: symbol = table.get_void_primitive(); break;

		case TokenType::VarKeyword:
		{
			get_diagnostics().report_error(node->get_location(), L"Implicitly typed variables are not supported");
			return;
		}

		default:
		{
			get_diagnostics().report_error(node->get_location(), L"Unknown primitive type");
			return;
		}
	}

	if (symbol == nullptr)
	{
		get_diagnostics().report_error(node->get_location(), L"Primitive type wasn't resolved");
		return;
	}

	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, symbol);
}

void TypeBinder::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	std::optional<const IdentifierNameTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	const std::wstring_view name = node->get_identifier().get_lexeme();

	if (name == L"Self")
	{
		StructSymbol* ownerType = get_owner_type();
		if ((ownerType == nullptr) || ownerType->get_kind() != SyntaxKind::StructDeclaration)
		{
			get_diagnostics().report_error(node->get_location(), L"'Self' can only be used inside a struct declaration");
			return;
		}

		if (ownerType->type_parameters.empty())
		{
			get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, ownerType);
			return;
		}

		gmt::inline_vector<StructSymbol*, 4> arguments;
		for (TypeParameterSymbol* typeParameter : ownerType->type_parameters)
			arguments.push_back(typeParameter);

		GenericTypeSymbol* selfType = get_symbol_table().find_or_create_generic_type(ownerType, arguments.get_vector());
		get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, selfType);
		return;
	}

	SyntaxSymbol* symbol = get_current_scope()->lookup(name);

	// types declared in the global namespace are always visible, like C#'s
	// global:: scope; locals and imports shadow them by resolving first
	if (symbol == nullptr)
	{
		for (SyntaxSymbol* member : get_symbol_table().get_root_namespace()->members)
		{
			if (!(member == nullptr) && member->is_type() && member->get_name() == name)
			{
				symbol = member;
				break;
			}
		}
	}

	if (symbol == nullptr)
	{
		if (is_ambiguous_name(name))
		{
			get_diagnostics().report_error(node->get_location(), L"Ambiguous reference: '" + gmt::wstring(name) + L"' is defined in multiple imported namespaces");
			return;
		}

		get_diagnostics().report_error(node->get_location(), L"Symbol wasn't found in current scope");
		return;
	}

	if (symbol->get_kind() == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* typeParameter = static_cast<TypeParameterSymbol*>(symbol);
		get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, typeParameter);

		if (!is_symbol_accessible(typeParameter, get_symbol_table().lookup_node(typeParameter), *nodeRef))
			get_diagnostics().report_error(node->get_location(), L"Symbol inaccessible");

		return;
	}

	if (!symbol->is_type())
	{
		get_diagnostics().report_error(node->get_location(), L"Symbol is not a type");
		return;
	}

	StructSymbol* typeSymbol = static_cast<StructSymbol*>(symbol);
	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, typeSymbol);

	if (!is_symbol_accessible(typeSymbol, get_symbol_table().lookup_node(typeSymbol), *nodeRef))
		get_diagnostics().report_error(node->get_location(), L"Symbol inaccessible");
}

NamespaceSymbol* TypeBinder::resolve_namespace_type(const TypeSyntax* type)
{
	if (type == nullptr)
		return nullptr;

	if (type->get_kind() == SyntaxKind::IdentifierNameType)
	{
		const IdentifierNameTypeSyntax* identifier = gmt::as<IdentifierNameTypeSyntax>(type);
		return get_symbol_table().get_root_namespace()->lookup(identifier->get_identifier().get_lexeme());
	}

	if (type->get_kind() == SyntaxKind::QualifiedNameType)
	{
		const QualifiedNameTypeSyntax* qualified = gmt::as<QualifiedNameTypeSyntax>(type);
		NamespaceSymbol* parentNamespace = resolve_namespace_type(qualified->get_left());
		if (parentNamespace == nullptr)
			return nullptr;

		return parentNamespace->lookup(qualified->get_identifier().get_lexeme());
	}

	return nullptr;
}

void TypeBinder::visit_qualified_name_type(const QualifiedNameTypeSyntax* node)
{
	std::optional<const QualifiedNameTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	NamespaceSymbol* namespaceSymbol = resolve_namespace_type(node->get_left());
	if (namespaceSymbol == nullptr)
	{
		get_diagnostics().report_error(node->get_location(), L"Left side of qualified type name must be a namespace");
		return;
	}

	for (SyntaxSymbol* const& member : namespaceSymbol->members)
	{
		if ((member == nullptr) || member->get_name() != node->get_identifier().get_lexeme())
			continue;

		if (!member->is_type())
			continue;

		get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, static_cast<StructSymbol*>(member));
		return;
	}

	get_diagnostics().report_error(node->get_location(), L"Type '" + gmt::wstring(node->get_identifier().get_lexeme()) + L"' not found in namespace");
}

void TypeBinder::visit_array_type(const ArrayTypeSyntax* node)
{
	std::optional<const ArrayTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	StructSymbol* underlaying = resolve_type(node->get_underlaying_type());
	if (underlaying == nullptr)
		return;

	// the registry makes every reoccurring 'int[]' share one symbol
	m_factory.create_array_type(*nodeRef, underlaying);
}

void TypeBinder::visit_nullable_type(const NullableTypeSyntax* node)
{
	std::optional<const NullableTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	StructSymbol* underlaying = resolve_type(node->get_underlaying_type());
	if (!(underlaying == nullptr))
		m_factory.create_nullable_type(*nodeRef, underlaying);
}

void TypeBinder::visit_generic_type(const GenericTypeSyntax* node)
{
	std::optional<const GenericTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	StructSymbol* underlaying = resolve_type(node->get_underlaying_type());
	if (underlaying == nullptr)
		return;

	if ((node->get_type_arguments() == nullptr) || node->get_type_arguments()->get_types().empty())
	{
		get_diagnostics().report_error(node->get_location(), L"Generic type requires type arguments");
		return;
	}

	const auto argumentNodes = node->get_type_arguments()->get_types();
	const std::size_t argumentCount = argumentNodes.size();
	const std::size_t parameterCount = underlaying->type_parameters.size();

	if (argumentCount != parameterCount)
	{
		get_diagnostics().report_error(node->get_location(), gmt::wstring(L"Type requires " + std::to_wstring(parameterCount) + L" type arguments, but got " + std::to_wstring(argumentCount)));
		return;
	}

	gmt::inline_vector<StructSymbol*, 4> arguments;
	arguments.get_vector().resize(argumentCount, nullptr);
	for (std::size_t i = 0; i < argumentCount; ++i)
		arguments.get_vector()[i] = resolve_type(argumentNodes[i]);

	// Resolve fully before creation so the registry never sees partially substituted symbols.
	// Refs are shared, so mutating one would corrupt every occurrence of the same shape.
	GenericTypeSymbol* symbol = get_symbol_table().find_or_create_generic_type(underlaying, arguments.get_vector());
	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, symbol);

	for (std::size_t i = 0; i < argumentCount; ++i)
	{
		StructSymbol* argument = arguments.get_vector()[i];
		if (argument == nullptr)
		{
			get_diagnostics().report_error(node->get_location(), gmt::wstring(L"Type argument " + std::to_wstring(i + 1) + L" could not be resolved"));
			continue;
		}

		TypeParameterSymbol* typeParameter = underlaying->type_parameters[i];
		for (StructSymbol* const& constraint : typeParameter->get_constraints())
		{
			if (constraint == nullptr)
				continue;

			if (!get_model().type_satisfies_constraint(argument, constraint))
			{
				get_diagnostics().report_error(node->get_location(), L"Type argument '" + gmt::wstring(argument->get_name()) + L"' does not satisfy the constraint '" + gmt::wstring(constraint->get_name()) + L"' on type parameter '" + gmt::wstring(typeParameter->get_name()) + L"'");
			}
		}
	}
}

void TypeBinder::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_expression());
}

void TypeBinder::visit_return_statement(const ReturnStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_expression());
}

void TypeBinder::visit_arrow_clause(const ArrowClauseSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_expression());
}

void TypeBinder::visit_throw_statement(const ThrowStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_expression());
}

void TypeBinder::visit_defer_statement(const DeferStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_expression());
}

void TypeBinder::visit_if_statement(const IfStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_condition());

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);

	if (!(node->get_next() == nullptr))
		node->get_next()->accept(*this);
}

void TypeBinder::visit_unless_statement(const UnlessStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_condition());

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);

	if (!(node->get_next() == nullptr))
		node->get_next()->accept(*this);
}

void TypeBinder::visit_else_statement(const ElseStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);

	if (!(node->get_next() == nullptr))
		node->get_next()->accept(*this);
}

void TypeBinder::visit_while_statement(const WhileStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_condition());

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);
}

void TypeBinder::visit_until_statement(const UntilStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_condition());

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);
}

void TypeBinder::visit_for_in_statement(const ForInStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_range_expression());

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);
}

void TypeBinder::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	// mirrors the collector: each block is an anonymous scope, so bound names resolve to the nearest enclosing binding
	push_scope(nullptr);

	for (StatementSyntax* const& statement : node->get_statements())
		statement->accept(*this);

	pop_scope();
}
