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

gmt::Ref<StructSymbol> TypeBinder::resolve_type(gmt::Ref<const TypeSyntax> node)
{
	if (node.is_null())
		return gmt::nullref;

	node.as_ptr()->accept(*this);
	return lookup_symbol<StructSymbol>(node);
}

void TypeBinder::bind_arguments(gmt::Ref<const ArgumentsListSyntax> arguments)
{
	if (arguments.is_null())
		return;

	for (const gmt::Ref<ArgumentSyntax>& argument : arguments->get_arguments().as_span())
		bind_expression(argument->get_expression());
}

void TypeBinder::bind_expression(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null())
		return;

	switch (expression->get_kind())
	{
		case SyntaxKind::ObjectExpression:
		{
			visit_object_creation_expression(static_cast<const ObjectExpressionSyntax*>(expression.as_ptr()));
			break;
		}

		case SyntaxKind::BinaryExpression:
		case SyntaxKind::RangeExpression:
		{
			bind_expression(expression.as<BinaryExpressionSyntax>()->get_left());
			bind_expression(expression.as<BinaryExpressionSyntax>()->get_right());
			break;
		}

		case SyntaxKind::UnaryExpression:
		{
			bind_expression(expression.as<UnaryExpressionSyntax>()->get_operand());
			break;
		}

		case SyntaxKind::AwaitExpression:
		{
			bind_expression(expression.as<AwaitExpressionSyntax>()->get_expression());
			break;
		}

		case SyntaxKind::IfExpression:
		{
			bind_expression(expression.as<IfExpressionSyntax>()->get_condition());
			bind_expression(expression.as<IfExpressionSyntax>()->get_then_expression());
			bind_expression(expression.as<IfExpressionSyntax>()->get_else_expression());
			break;
		}

		case SyntaxKind::CastExpression:
		case SyntaxKind::IsExpression:
		{
			bind_expression(expression.as<CastExpressionSyntax>()->get_expression());
			resolve_type(expression.as<CastExpressionSyntax>()->get_target_type());
			break;
		}

		case SyntaxKind::CollectionExpression:
		{
			for (const gmt::Ref<ExpressionSyntax>& value : expression.as<CollectionExpressionSyntax>()->get_values().as_span())
				bind_expression(value);

			break;
		}

		case SyntaxKind::MemberAccessExpression:
		{
			bind_expression(expression.as<MemberAccessExpressionSyntax>()->get_previous());
			break;
		}

		case SyntaxKind::InvokationExpression:
		{
			gmt::Ref<const InvokationExpressionSyntax> invocation = expression.as<InvokationExpressionSyntax>();
			bind_expression(invocation->get_previous());

			gmt::Ref<const TypeArgumentsListSyntax> typeArguments = invocation->get_type_arguments();
			if (!typeArguments.is_null())
			{
				for (const gmt::Ref<TypeSyntax>& argument : typeArguments->get_types().as_span())
					argument.as_ptr()->accept(*this);
			}

			bind_arguments(invocation->get_arguments());
			break;
		}

		case SyntaxKind::IndexatorExpression:
		{
			bind_expression(expression.as<IndexatorExpressionSyntax>()->get_previous());
			bind_arguments(expression.as<IndexatorExpressionSyntax>()->get_arguments());
			break;
		}

		case SyntaxKind::LambdaExpression:
		{
			visit_lambda_expression(static_cast<const LambdaExpressionSyntax*>(expression.as_ptr()));
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

void TypeBinder::import_namespace(gmt::Ref<const UsingDirectiveSyntax> node, gmt::Ref<NamespaceSymbol> namespaceSymbol)
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
	for (const gmt::Ref<SyntaxSymbol>& member : namespaceSymbol->members)
	{
		if (member.is_null())
			continue;

		gmt::Ref<SyntaxSymbol> existing = current->find_local(member->get_name());
		if (!existing.is_null())
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

void TypeBinder::bind_where_clauses(gmt::Ref<const WhereClausesListSyntax> clauses, const gmt::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters)
{
	if (clauses.is_null())
		return;

	for (const gmt::Ref<WhereClauseSyntax>& clause : clauses->get_clauses().as_span())
	{
		if (clause.is_null())
			continue;

		const std::wstring_view parameterName = clause->get_identifier().get_lexeme();

		gmt::Ref<TypeParameterSymbol> typeParameter = gmt::nullref;
		for (const gmt::Ref<TypeParameterSymbol>& candidate : typeParameters)
		{
			if (!candidate.is_null() && candidate->get_name() == parameterName)
			{
				typeParameter = candidate;
				break;
			}
		}

		if (typeParameter.is_null())
		{
			get_diagnostics().report_error(clause->get_location(), L"Type parameter '" + gmt::wstring(parameterName) + L"' not found");
			continue;
		}

		for (const gmt::Ref<TypeSyntax>& constraintType : clause->get_constraint_types().as_span())
		{
			gmt::Ref<StructSymbol> constraint = resolve_type(constraintType);
			if (constraint.is_null())
			{
				get_diagnostics().report_error(clause->get_location(), L"Could not resolve constraint type for type parameter '" + gmt::wstring(parameterName) + L"'");
				continue;
			}

			typeParameter->add_constraint(constraint);
		}
	}
}

void TypeBinder::bind_base_types(gmt::Ref<StructSymbol> symbol, gmt::Ref<const BaseTypesListSyntax> baseTypes)
{
	if (baseTypes.is_null())
		return;

	for (const gmt::Ref<TypeSyntax>& baseType : baseTypes->get_types().as_span())
	{
		gmt::Ref<StructSymbol> baseSymbol = resolve_type(baseType);
		if (baseSymbol.is_null())
			continue;

		if (baseSymbol->get_kind() == SyntaxKind::InterfaceDeclaration)
		{
			symbol->interfaces.push_back(baseSymbol);
			continue;
		}

		if (baseSymbol->get_kind() == SyntaxKind::GenericType)
		{
			gmt::Ref<StructSymbol> underlaying = baseSymbol.as<GenericTypeSymbol>()->get_underlaying_type();
			if (!underlaying.is_null() && underlaying->get_kind() == SyntaxKind::InterfaceDeclaration)
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
	push_scope(gmt::nullref);

	for (const gmt::Ref<UsingDirectiveSyntax>& directive : node->get_usings().as_span())
		visit_using_directive(directive.as_ptr());

	if (node->has_namespace())
	{
		gmt::Ref<const NamespaceDirectiveSyntax> directive = node->get_namespace();
		gmt::Ref<NamespaceSymbol> namespaceSymbol = lookup_symbol<NamespaceSymbol>(directive);

		if (namespaceSymbol.is_null())
			throw std::runtime_error("namespace symbol not found");

		push_scope(namespaceSymbol);
	}

	// declare member symbols upfront so members can reference each other
	// regardless of declaration order; the binder re-declares into its own
	// fresh scopes, so this bypasses the owner hook on purpose - calling
	// declare() here would duplicate the member lists populated during
	// collection
	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if (!memberSymbol.is_null())
			get_current_scope()->declare_symbol(memberSymbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	pop_scope();

	if (node->has_namespace())
		pop_scope();
}

void TypeBinder::visit_using_directive(const UsingDirectiveSyntax* node)
{
	std::optional<gmt::Ref<const UsingDirectiveSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<NamespaceSymbol> current = get_symbol_table().get_root_namespace();
	for (const SyntaxToken& token : node->get_qualifier().as_span())
	{
		gmt::Ref<NamespaceSymbol> next = current->lookup(token.get_lexeme());
		if (next.is_null())
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
	std::optional<gmt::Ref<const StructDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<StructSymbol> symbol = lookup_symbol<StructSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("struct symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& typeParameter : symbol->type_parameters)
		get_current_scope()->declare_symbol(typeParameter);

	bind_where_clauses(node->get_where_clauses(), symbol->type_parameters);
	bind_base_types(symbol, node->get_base_types());

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if (!memberSymbol.is_null())
			get_current_scope()->declare_symbol(memberSymbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_interface_declaration(const InterfaceDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const InterfaceDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<InterfaceSymbol> symbol = lookup_symbol<InterfaceSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("interface symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& typeParameter : symbol->type_parameters)
		get_current_scope()->declare_symbol(typeParameter);

	bind_where_clauses(node->get_where_clauses(), symbol->type_parameters);

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> memberSymbol = lookup_symbol<SyntaxSymbol>(member);
		if (!memberSymbol.is_null())
			get_current_scope()->declare_symbol(memberSymbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_delegate_declaration(const DelegateDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const DelegateDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<DelegateTypeSymbol> symbol = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("delegate symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& typeParameter : symbol->get_type_parameters())
		get_current_scope()->declare_symbol(typeParameter);

	if (!node->get_return_type().is_null())
		symbol->set_return_type(resolve_type(node->get_return_type()));

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_enum_declaration(const EnumDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const EnumDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<EnumSymbol> symbol = lookup_symbol<EnumSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("enum symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<EnumFieldDeclarationSyntax>& field : node->get_fields().as_span())
		bind_expression(field->get_expression());

	pop_scope();
}

void TypeBinder::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const ConstructorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("constructor symbol not found");

	push_scope(symbol);

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const FunctionDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("method symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& typeParameter : symbol->get_type_parameters())
		get_current_scope()->declare_symbol(typeParameter);

	bind_where_clauses(node->get_where_clauses(), symbol->get_type_parameters());

	if (!node->get_return_type().is_null())
		symbol->set_return_type(resolve_type(node->get_return_type()));

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const OperatorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<OperatorSymbol> symbol = lookup_symbol<OperatorSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("operator symbol not found");

	push_scope(symbol);

	if (!node->get_return_type().is_null())
		symbol->set_return_type(resolve_type(node->get_return_type()));

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

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
	std::optional<gmt::Ref<const FieldDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<FieldSymbol> symbol = lookup_symbol<FieldSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("field symbol not found");

	if (!node->get_type().is_null())
		symbol->set_return_type(resolve_type(node->get_type()));

	bind_expression(node->get_expression());
}

void TypeBinder::visit_property_declaration(const PropertyDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const PropertyDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<PropertySymbol> symbol = lookup_symbol<PropertySymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("property symbol not found");

	if (node->get_type().is_null())
		return;

	push_scope(symbol);

	gmt::Ref<StructSymbol> propertyType = resolve_type(node->get_type());
	symbol->set_return_type(propertyType);

	gmt::Ref<FieldSymbol> backingField = symbol->get_backing_field();
	if (!backingField.is_null())
		backingField->set_return_type(propertyType);

	gmt::Ref<AccessorSymbol> getter = symbol->get_getter();
	if (!getter.is_null())
		getter->set_return_type(propertyType);

	gmt::Ref<AccessorSymbol> setter = symbol->get_setter();
	if (!setter.is_null() && !setter->get_parameters().empty())
		setter->get_parameters()[0]->set_type(propertyType);

	bind_expression(node->get_expression());

	if (!node->get_setter().is_null())
		visit_accessor_declaration(node->get_setter().as_ptr());

	if (!node->get_getter().is_null())
		visit_accessor_declaration(node->get_getter().as_ptr());

	pop_scope();
}

void TypeBinder::visit_indexator_declaration(const IndexatorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const IndexatorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<IndexatorSymbol> symbol = lookup_symbol<IndexatorSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("indexator symbol not found");

	push_scope(symbol);

	if (!node->get_type().is_null())
	{
		gmt::Ref<StructSymbol> propertyType = resolve_type(node->get_type());
		symbol->set_return_type(propertyType);

		gmt::Ref<AccessorSymbol> getter = symbol->get_getter();
		if (!getter.is_null())
			getter->set_return_type(propertyType);

		gmt::Ref<AccessorSymbol> setter = symbol->get_setter();
		if (!setter.is_null() && !setter->get_parameters().empty())
			setter->get_parameters()[0]->set_type(propertyType);
	}

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	if (!node->get_setter().is_null())
		visit_accessor_declaration(node->get_setter().as_ptr());

	if (!node->get_getter().is_null())
		visit_accessor_declaration(node->get_getter().as_ptr());

	pop_scope();
}

void TypeBinder::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const AccessorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<AccessorSymbol> symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("accessor symbol not found");

	gmt::Ref<const SyntaxNode> parentNode = node->get_parent();

	if (parentNode->get_kind() == SyntaxKind::IndexatorDeclaration)
	{
		// index parameters join each accessor's own parameter list; the
		// implicit setter 'value' parameter keeps the leading slot
		gmt::Ref<IndexatorSymbol> indexator = lookup_symbol<IndexatorSymbol>(parentNode);
		if (!indexator.is_null())
		{
			for (const gmt::Ref<ParameterSymbol>& indexParameter : indexator->get_parameters())
				symbol->on_symbol_declared(indexParameter);
		}
	}
	else
	{
		gmt::Ref<PropertySymbol> property = lookup_symbol<PropertySymbol>(parentNode);
		if (!property.is_null() && node->get_keyword().get_type() == TokenType::GetKeyword)
			symbol->set_return_type(property->get_return_type());
	}
}

void TypeBinder::visit_variable_statement(const VariableStatementSyntax* node)
{
	std::optional<gmt::Ref<const VariableStatementSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<VariableSymbol> symbol = lookup_symbol<VariableSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("variable symbol not found");

	if (!node->get_type().is_null())
		symbol->set_type(resolve_type(node->get_type()));

	bind_expression(node->get_expression());
}

void TypeBinder::visit_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_try_block().is_null())
		node->get_try_block().as_ptr()->accept(*this);

	for (const gmt::Ref<CatchClauseSyntax>& clause : node->get_catch_clauses().as_span())
	{
		gmt::Ref<StructSymbol> exceptionType = gmt::nullref;		if (!clause->get_exception_type().is_null())
			exceptionType = resolve_type(clause->get_exception_type());

		if (!exceptionType.is_null())
		{
			gmt::Ref<StructSymbol> throwable = get_symbol_table().get_standard_type(StandardTypes::IThrowable).as<StructSymbol>();
			if (!get_model().is_assignable_to(throwable, exceptionType))
				get_diagnostics().report_error(clause->get_exception_type()->get_location(), L"Catch type must implement IThrowable");
		}

		gmt::Ref<VariableSymbol> catchVariable = lookup_symbol<VariableSymbol>(clause);
		if (!catchVariable.is_null())
			catchVariable->set_type(exceptionType);

		if (!clause->get_body().is_null())
			clause->get_body().as_ptr()->accept(*this);
	}
}

void TypeBinder::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	std::optional<gmt::Ref<const LambdaExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	if (node->get_parameters().is_null())
		return;

	for (const gmt::Ref<ParameterSyntax>& parameter : node->get_parameters()->get_parameters().as_span())
	{
		gmt::Ref<ParameterSymbol> symbol = lookup_symbol<ParameterSymbol>(parameter);
		if (symbol.is_null())
			continue;

		if (!parameter->get_type().is_null())
			symbol->set_type(resolve_type(parameter->get_type()));
	}

	// the body belongs to the expression binder for values, but type syntaxes
	// inside it (and signatures of nested lambdas) are bound here
	gmt::Ref<MethodSymbol> anonymousMethod = lookup_symbol<MethodSymbol>(node->get_parameters());
	if (anonymousMethod.is_null())
		return;

	push_scope(anonymousMethod);

	for (const gmt::Ref<ParameterSymbol>& parameter : anonymousMethod->get_parameters())
		get_current_scope()->declare_symbol(parameter);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	std::optional<gmt::Ref<const ObjectExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	if (node->get_type().is_null())
		return;

	gmt::Ref<StructSymbol> createdType = resolve_type(node->get_type());

	bind_expression(node->get_array_size());
	bind_arguments(node->get_arguments());

	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, createdType);
}

void TypeBinder::visit_parameter(const ParameterSyntax* node)
{
	std::optional<gmt::Ref<const ParameterSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<ParameterSymbol> symbol = lookup_symbol<ParameterSymbol>(*nodeRef);
	if (symbol.is_null())
		throw std::runtime_error("parameter symbol not found");

	if (!node->get_type().is_null())
		symbol->set_type(resolve_type(node->get_type()));
}

void TypeBinder::visit_predefined_type(const PredefinedTypeSyntax* node)
{
	std::optional<gmt::Ref<const PredefinedTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	SymbolTable& table = get_symbol_table();
	gmt::Ref<StructSymbol> symbol = gmt::nullref;

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

	if (symbol.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Primitive type wasn't resolved");
		return;
	}

	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, symbol);
}

void TypeBinder::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	std::optional<gmt::Ref<const IdentifierNameTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	const std::wstring_view name = node->get_identifier().get_lexeme();

	if (name == L"Self")
	{
		gmt::Ref<StructSymbol> ownerType = get_owner_type();
		if (ownerType.is_null() || ownerType->get_kind() != SyntaxKind::StructDeclaration)
		{
			get_diagnostics().report_error(node->get_location(), L"'Self' can only be used inside a struct declaration");
			return;
		}

		if (ownerType->type_parameters.empty())
		{
			get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, ownerType);
			return;
		}

		gmt::inline_vector<gmt::Ref<StructSymbol>, 4> arguments;
		for (gmt::Ref<TypeParameterSymbol> typeParameter : ownerType->type_parameters)
			arguments.push_back(typeParameter);

		gmt::Ref<GenericTypeSymbol> selfType = get_symbol_table().find_or_create_generic_type(ownerType, arguments.get_vector());
		get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, selfType);
		return;
	}

	gmt::Ref<SyntaxSymbol> symbol = get_current_scope()->lookup(name);

	// types declared in the global namespace are always visible, like C#'s
	// global:: scope; locals and imports shadow them by resolving first
	if (symbol.is_null())
	{
		for (gmt::Ref<SyntaxSymbol> member : get_symbol_table().get_root_namespace()->members)
		{
			if (!member.is_null() && member->is_type() && member->get_name() == name)
			{
				symbol = member;
				break;
			}
		}
	}

	if (symbol.is_null())
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
		gmt::Ref<TypeParameterSymbol> typeParameter = symbol.as<TypeParameterSymbol>();
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

	gmt::Ref<StructSymbol> typeSymbol = symbol.as<StructSymbol>();
	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, typeSymbol);

	if (!is_symbol_accessible(typeSymbol, get_symbol_table().lookup_node(typeSymbol), *nodeRef))
		get_diagnostics().report_error(node->get_location(), L"Symbol inaccessible");
}

gmt::Ref<NamespaceSymbol> TypeBinder::resolve_namespace_type(gmt::Ref<const TypeSyntax> type)
{
	if (type.is_null())
		return gmt::nullref;

	if (type->get_kind() == SyntaxKind::IdentifierNameType)
	{
		gmt::Ref<const IdentifierNameTypeSyntax> identifier = type.as<IdentifierNameTypeSyntax>();
		return get_symbol_table().get_root_namespace()->lookup(identifier->get_identifier().get_lexeme());
	}

	if (type->get_kind() == SyntaxKind::QualifiedNameType)
	{
		gmt::Ref<const QualifiedNameTypeSyntax> qualified = type.as<QualifiedNameTypeSyntax>();
		gmt::Ref<NamespaceSymbol> parentNamespace = resolve_namespace_type(qualified->get_left());
		if (parentNamespace.is_null())
			return gmt::nullref;

		return parentNamespace->lookup(qualified->get_identifier().get_lexeme());
	}

	return gmt::nullref;
}

void TypeBinder::visit_qualified_name_type(const QualifiedNameTypeSyntax* node)
{
	std::optional<gmt::Ref<const QualifiedNameTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<NamespaceSymbol> namespaceSymbol = resolve_namespace_type(node->get_left());
	if (namespaceSymbol.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Left side of qualified type name must be a namespace");
		return;
	}

	for (const gmt::Ref<SyntaxSymbol>& member : namespaceSymbol->members)
	{
		if (member.is_null() || member->get_name() != node->get_identifier().get_lexeme())
			continue;

		if (!member->is_type())
			continue;

		get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, member.as<StructSymbol>());
		return;
	}

	get_diagnostics().report_error(node->get_location(), L"Type '" + gmt::wstring(node->get_identifier().get_lexeme()) + L"' not found in namespace");
}

void TypeBinder::visit_array_type(const ArrayTypeSyntax* node)
{
	std::optional<gmt::Ref<const ArrayTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<StructSymbol> underlaying = resolve_type(node->get_underlaying_type());
	if (underlaying.is_null())
		return;

	// the registry makes every reoccurring 'int[]' share one symbol
	m_factory.create_array_type(*nodeRef, underlaying);
}

void TypeBinder::visit_nullable_type(const NullableTypeSyntax* node)
{
	std::optional<gmt::Ref<const NullableTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<StructSymbol> underlaying = resolve_type(node->get_underlaying_type());
	if (!underlaying.is_null())
		m_factory.create_nullable_type(*nodeRef, underlaying);
}

void TypeBinder::visit_generic_type(const GenericTypeSyntax* node)
{
	std::optional<gmt::Ref<const GenericTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<StructSymbol> underlaying = resolve_type(node->get_underlaying_type());
	if (underlaying.is_null())
		return;

	if (node->get_type_arguments().is_null() || node->get_type_arguments()->get_types().as_span().empty())
	{
		get_diagnostics().report_error(node->get_location(), L"Generic type requires type arguments");
		return;
	}

	const auto argumentNodes = node->get_type_arguments()->get_types().as_span();
	const std::size_t argumentCount = argumentNodes.size();
	const std::size_t parameterCount = underlaying->type_parameters.size();

	if (argumentCount != parameterCount)
	{
		get_diagnostics().report_error(node->get_location(), gmt::wstring(L"Type requires " + std::to_wstring(parameterCount) + L" type arguments, but got " + std::to_wstring(argumentCount)));
		return;
	}

	gmt::inline_vector<gmt::Ref<StructSymbol>, 4> arguments;
	arguments.get_vector().resize(argumentCount, gmt::nullref);
	for (std::size_t i = 0; i < argumentCount; ++i)
		arguments.get_vector()[i] = resolve_type(argumentNodes[i]);

	// Resolve fully before creation so the registry never sees partially substituted symbols.
	// Refs are shared, so mutating one would corrupt every occurrence of the same shape.
	gmt::Ref<GenericTypeSymbol> symbol = get_symbol_table().find_or_create_generic_type(underlaying, arguments.get_vector());
	get_symbol_table().bind_symbol<StructSymbol>(*nodeRef, symbol);

	for (std::size_t i = 0; i < argumentCount; ++i)
	{
		gmt::Ref<StructSymbol> argument = arguments.get_vector()[i];
		if (argument.is_null())
		{
			get_diagnostics().report_error(node->get_location(), gmt::wstring(L"Type argument " + std::to_wstring(i + 1) + L" could not be resolved"));
			continue;
		}

		gmt::Ref<TypeParameterSymbol> typeParameter = underlaying->type_parameters[i];
		for (const gmt::Ref<StructSymbol>& constraint : typeParameter->get_constraints())
		{
			if (constraint.is_null())
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

	if (!node->get_block().is_null())
		node->get_block().as_ptr()->accept(*this);

	if (!node->get_next().is_null())
		node->get_next().as_ptr()->accept(*this);
}

void TypeBinder::visit_unless_statement(const UnlessStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_condition());

	if (!node->get_block().is_null())
		node->get_block().as_ptr()->accept(*this);

	if (!node->get_next().is_null())
		node->get_next().as_ptr()->accept(*this);
}

void TypeBinder::visit_else_statement(const ElseStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_block().is_null())
		node->get_block().as_ptr()->accept(*this);

	if (!node->get_next().is_null())
		node->get_next().as_ptr()->accept(*this);
}

void TypeBinder::visit_while_statement(const WhileStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_condition());

	if (!node->get_block().is_null())
		node->get_block().as_ptr()->accept(*this);
}

void TypeBinder::visit_until_statement(const UntilStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_condition());

	if (!node->get_block().is_null())
		node->get_block().as_ptr()->accept(*this);
}

void TypeBinder::visit_for_in_statement(const ForInStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bind_expression(node->get_range_expression());

	if (!node->get_block().is_null())
		node->get_block().as_ptr()->accept(*this);
}

void TypeBinder::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	// mirrors the collector: each block is an anonymous scope, so bound names resolve to the nearest enclosing binding
	push_scope(gmt::nullref);

	for (const gmt::Ref<StatementSyntax>& statement : node->get_statements().as_span())
		statement.as_ptr()->accept(*this);

	pop_scope();
}
