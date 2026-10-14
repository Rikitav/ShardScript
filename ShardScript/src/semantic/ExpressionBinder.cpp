#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/lexical/SyntaxFacts.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/LeftDenotationSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Concat.hpp>

using namespace shard;

ExpressionBinder::ExpressionBinder(SemanticModel& model, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics), m_factory(&model.get_table()) { }

void ExpressionBinder::visit_expression(gmt::Ref<const ExpressionSyntax> expression)
{
	if (!expression.is_null())
		expression.as_ptr()->accept(*this);
}

void ExpressionBinder::set_expression_type(gmt::Ref<const ExpressionSyntax> expression, gmt::Ref<StructSymbol> type)
{
	get_model().set_expression_type(expression, type);
}

gmt::Ref<StructSymbol> ExpressionBinder::get_expression_type(gmt::Ref<const ExpressionSyntax> expression)
{
	return get_model().get_expression_type(expression);
}

void ExpressionBinder::visit_translation_unit(const TranslationUnitSyntax* node)
{
	push_scope(gmt::nullref);

	for (const gmt::Ref<UsingDirectiveSyntax>& directive : node->get_usings().as_span())
		visit_using_directive(directive.as_ptr());

	if (node->has_namespace())
	{
		gmt::Ref<NamespaceSymbol> symbol = find_namespace_by_name(node->get_namespace()->get_qualifier());
		if (!symbol.is_null())
		{
			push_scope(symbol);
			get_current_scope()->set_namespace(symbol);
		}
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> symbol = get_symbol_table().lookup_symbol(member);
		if (!symbol.is_null())
			declare(symbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member->accept(*this);

	if (node->has_namespace())
		pop_scope();

	pop_scope();
}

void ExpressionBinder::visit_using_directive(const UsingDirectiveSyntax* node)
{
	gmt::Ref<NamespaceSymbol> symbol = find_namespace_by_name(node->get_qualifier());
	if (symbol.is_null())
	{
		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Namespace '", node->get_qualifier_string(), L"' was not found"));
		return;
	}

	std::optional<gmt::Ref<const UsingDirectiveSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		import_namespace(*nodeRef, symbol);
}

void ExpressionBinder::import_namespace(gmt::Ref<const UsingDirectiveSyntax> node, gmt::Ref<NamespaceSymbol> namespaceSymbol)
{
	SemanticScope* scope = get_current_scope();
	if (scope->get_imported_namespaces().count(namespaceSymbol) != 0)
	{
		get_diagnostics().report_error(node->get_location(), L"Namespace is already imported");
		return;
	}

	scope->add_imported_namespace(namespaceSymbol);

	for (gmt::Ref<SyntaxSymbol> member : namespaceSymbol->members)
	{
		if (member.is_null())
			continue;

		gmt::Ref<SyntaxSymbol> existing = scope->find_local(member->get_name_reference());
		if (!existing.is_null() && existing != member)
		{
			bool isVariable = existing->get_kind() == SyntaxKind::VariableStatement;
			bool isParameter = existing->get_kind() == SyntaxKind::Parameter;
			if (!isVariable && !isParameter)
				m_ambiguousNames.insert(member->get_name());

			continue;
		}

		scope->declare_symbol(member);
	}
}

bool ExpressionBinder::is_ambiguous_name(std::wstring_view name) const
{
	return m_ambiguousNames.count(name) != 0;
}

gmt::Ref<NamespaceSymbol> ExpressionBinder::find_namespace_by_name(gmt::Span<const SyntaxToken> name)
{
	gmt::Ref<NamespaceSymbol> current = get_symbol_table().get_root_namespace();
	for (const SyntaxToken& token : name.as_span())
	{
		gmt::Ref<NamespaceSymbol> next = current->lookup(token.get_lexeme());
		if (next.is_null())
			return gmt::nullref;

		current = next;
	}

	return current;
}

void ExpressionBinder::visit_type_declaration(gmt::Ref<const TypeDeclarationSyntax> node)
{
	gmt::Ref<StructSymbol> symbol = lookup_symbol<StructSymbol>(node);
	if (symbol.is_null())
		return;

	push_scope(symbol);

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> memberSymbol = get_symbol_table().lookup_symbol(member);
		if (!memberSymbol.is_null())
			declare(memberSymbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member->accept(*this);

	pop_scope();
}

void ExpressionBinder::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const StructDeclarationSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		visit_type_declaration(*nodeRef);
}

void ExpressionBinder::visit_interface_declaration(const InterfaceDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const InterfaceDeclarationSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		visit_type_declaration(*nodeRef);
}

void ExpressionBinder::visit_enum_declaration(const EnumDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const EnumDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<StructSymbol> symbol = lookup_symbol<StructSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	push_scope(symbol);

	for (const gmt::Ref<EnumFieldDeclarationSyntax>& field : node->get_fields().as_span())
	{
		gmt::Ref<SyntaxSymbol> fieldSymbol = get_symbol_table().lookup_symbol(field);
		if (!fieldSymbol.is_null())
			declare(fieldSymbol);
	}

	pop_scope();
}

void ExpressionBinder::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const ConstructorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null() || !parent->is_type())
		return;

	if (node->get_body().is_null())
		return;

	push_scope(symbol);

	SemanticScope* scope = get_current_scope();

	for (gmt::Ref<TypeParameterSymbol> typeParameter : symbol->get_type_parameters())
		scope->declare_symbol(typeParameter);

	if (symbol->get_linking() == LINK_INSTANCE)
	{
		gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();
		scope->declare_symbol(m_factory.create_parameter(L"this", symbol, ownerType));
	}

	for (gmt::Ref<ParameterSymbol> parameter : symbol->get_parameters())
		scope->declare_symbol(parameter);

	node->get_body().as_ptr()->accept(*this);
	pop_scope();
}

void ExpressionBinder::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const FunctionDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	bool instanceInType = !parent.is_null() && parent->is_type() && symbol->get_linking() == LINK_INSTANCE;

	push_scope(symbol);

	SemanticScope* scope = get_current_scope();

	for (gmt::Ref<TypeParameterSymbol> typeParameter : symbol->get_type_parameters())
		scope->declare_symbol(typeParameter);

	if (instanceInType)
	{
		gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();
		scope->declare_symbol(m_factory.create_parameter(L"this", symbol, ownerType));
	}

	for (gmt::Ref<ParameterSymbol> parameter : symbol->get_parameters())
		scope->declare_symbol(parameter);

	node->get_body().as_ptr()->accept(*this);
	pop_scope();
}

void ExpressionBinder::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const OperatorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Cannot find parent node of operator.");
		return;
	}

	const std::vector<gmt::Ref<ParameterSymbol>>& parameters = symbol->get_parameters();

	push_scope(symbol);

	if (symbol->get_linking() == LINK_INSTANCE && parent->is_type())
	{
		gmt::Ref<StructSymbol> ownerType = parent.as<StructSymbol>();
		get_current_scope()->declare_symbol(m_factory.create_parameter(L"this", symbol, ownerType));
	}

	for (gmt::Ref<ParameterSymbol> parameter : parameters)
		get_current_scope()->declare_symbol(parameter);

	node->get_body().as_ptr()->accept(*this);

	if (node->get_operator_token().get_type() == TokenType::Delimeter)
	{
		bool hasSingleParameter = parameters.size() == 1;
		if (!hasSingleParameter || parameters[0]->get_type() != get_symbol_table().get_string_primitive())
			get_diagnostics().report_error(node->get_location(), L"Access operator ('.') parameter must be of type 'string'");
	}

	pop_scope();
}

void ExpressionBinder::visit_field_declaration(const FieldDeclarationSyntax* node)
{
	gmt::Ref<const ExpressionSyntax> initializer = node->get_expression();
	visit_expression(initializer);

	std::optional<gmt::Ref<const FieldDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<FieldSymbol> symbol = lookup_symbol<FieldSymbol>(*nodeRef);
	if (symbol.is_null() || initializer.is_null())
		return;

	gmt::Ref<StructSymbol> initializerType = get_expression_type(initializer);
	if (initializerType.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Field initializer expression type could not be determined");
		return;
	}

	TextLocation location = node->get_location();
	gmt::Ref<StructSymbol> returnType = symbol->get_return_type();
	SemanticModel& model = get_model();

	if (returnType.is_null())
	{
		get_diagnostics().report_error(location, L"Field return type not resolved");
		return;
	}

	if (!model.is_assignable_to(returnType, initializerType))
	{
		get_diagnostics().report_error(location, gmt::concat(L"Field initializer type mismatch: expected '", model.get_type_display_name(returnType), L"' but got '", model.get_type_display_name(initializerType), L"'"));
	}
}

void ExpressionBinder::visit_property_declaration(const PropertyDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const PropertyDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<PropertySymbol> symbol = lookup_symbol<PropertySymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	if (!symbol->get_parent().is_null() && !symbol->get_parent()->is_type())
		return;

	if (symbol->get_return_type() == get_symbol_table().get_void_primitive())
		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Property '", node->get_identifier().get_lexeme(), L"' must have return value"));

	visit_expression(node->get_expression());

	push_scope(symbol);

	if (!node->get_getter().is_null())
		visit_accessor_declaration(node->get_getter().as_ptr());

	if (!node->get_setter().is_null())
		visit_accessor_declaration(node->get_setter().as_ptr());

	pop_scope();
}

void ExpressionBinder::visit_indexator_declaration(const IndexatorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const IndexatorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<IndexatorSymbol> symbol = lookup_symbol<IndexatorSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	if (symbol->get_return_type() == get_symbol_table().get_void_primitive())
		get_diagnostics().report_error(node->get_location(), L"Indexator must have return value");

	push_scope(symbol);

	if (!node->get_getter().is_null())
		visit_accessor_declaration(node->get_getter().as_ptr());

	if (!node->get_setter().is_null())
		visit_accessor_declaration(node->get_setter().as_ptr());

	pop_scope();
}

void ExpressionBinder::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const AccessorDeclarationSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<AccessorSymbol> symbol = lookup_symbol<AccessorSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	push_scope(symbol);

	gmt::Ref<const SyntaxSymbol> parent = symbol->get_parent();
	if (parent.is_null() || !parent->is_member())
	{
		pop_scope();
		return;
	}

	gmt::Ref<const SyntaxSymbol> owner = parent->get_parent();
	bool isInstanceMember = symbol->get_linking() == LINK_INSTANCE && !owner.is_null();
	if (isInstanceMember && owner->is_type())
	{
		gmt::Ref<StructSymbol> ownerType = owner.as<StructSymbol>();
		get_current_scope()->declare_symbol(m_factory.create_parameter(L"this", symbol, ownerType));
	}

	for (gmt::Ref<ParameterSymbol> parameter : symbol->get_parameters())
		get_current_scope()->declare_symbol(parameter);

	// auto-accessors carry no body
	pop_scope();
}

void ExpressionBinder::visit_variable_statement(const VariableStatementSyntax* node)
{
	std::optional<gmt::Ref<const VariableStatementSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<VariableSymbol> symbol = lookup_symbol<VariableSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	gmt::Ref<const ExpressionSyntax> expression = node->get_expression();
	gmt::Ref<StructSymbol> symbolType = symbol->get_type();
	SemanticModel& model = get_model();

	get_current_scope()->declare_symbol(symbol);

	if (!expression.is_null())
	{
		gmt::Ref<LeftDenotationSymbol> denotation = get_symbol_table().emplace<LeftDenotationSymbol>(symbolType, symbol);
		push_scope(denotation);
		visit_expression(expression);
		pop_scope();
	}

	if (expression.is_null())
		return;

	gmt::Ref<StructSymbol> expressionType = get_expression_type(expression);
	if (expressionType.is_null())
		return;

	if (symbolType.is_null())
	{
		symbol->set_type(expressionType);
		return;
	}

	if (!model.is_assignable_to(symbolType, expressionType))
		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch: expected '", model.get_type_display_name(symbolType), L"' but got '", model.get_type_display_name(expressionType), L"'"));
}

gmt::Ref<StructSymbol> ExpressionBinder::find_target_return_type()
{
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		gmt::Ref<SyntaxSymbol> owner = scope->get_owner();
		if (owner.is_null())
			continue;

		switch (owner->get_kind())
		{
			case SyntaxKind::FunctionDeclaration:
			case SyntaxKind::OperatorDeclaration:
			case SyntaxKind::ConstructorDeclaration:
			case SyntaxKind::AccessorDeclaration:
			{
				return owner.as<MethodSymbol>()->get_return_type();
			}

			case SyntaxKind::FieldDeclaration:
			{
				return owner.as<FieldSymbol>()->get_return_type();
			}

			case SyntaxKind::PropertyDeclaration:
			case SyntaxKind::IndexatorDeclaration:
			{
				return owner.as<PropertySymbol>()->get_return_type();
			}

			default:
				break;
		}
	}

	return gmt::nullref;
}

gmt::Ref<StructSymbol> ExpressionBinder::resolve_left_denotation()
{
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		gmt::Ref<SyntaxSymbol> owner = scope->get_owner();
		if (owner.is_null())
			continue;

		if (owner->get_kind() == SyntaxKind::Argument)
			return owner.as<LeftDenotationSymbol>()->get_expected_type();
	}

	return gmt::nullref;
}

void ExpressionBinder::visit_return_statement(const ReturnStatementSyntax* node)
{
	TextLocation location = node->get_location();
	gmt::Ref<const ExpressionSyntax> expression = node->get_expression();
	SemanticModel& model = get_model();

	gmt::Ref<StructSymbol> returnType = find_target_return_type();
	if (returnType.is_null())
	{
		get_diagnostics().report_error(location, L"Couldn't find scope to return within");
		return;
	}

	if (returnType == get_symbol_table().get_void_primitive())
	{
		if (!expression.is_null())
		{
			visit_expression(expression);
			get_diagnostics().report_error(location, L"Void method cannot return a value");
		}

		return;
	}

	if (expression.is_null())
	{
		get_diagnostics().report_error(location, gmt::concat(L"Return statement must return a value of type '", model.get_type_display_name(returnType), L"'"));
		return;
	}

	visit_expression(expression);
	gmt::Ref<StructSymbol> expressionType = get_expression_type(expression);
	if (expressionType.is_null())
	{
		get_diagnostics().report_error(location, L"Return expression type could not be determined");
	}
	else if (returnType->get_kind() != SyntaxKind::TypeParameter && !model.is_assignable_to(returnType, expressionType))
	{
		get_diagnostics().report_error(location, gmt::concat(L"Return type mismatch: expected '", model.get_type_display_name(returnType), L"' but got '", model.get_type_display_name(expressionType), L"'"));
	}
}

void ExpressionBinder::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	visit_expression(node->get_expression());
}

void ExpressionBinder::visit_if_statement(const IfStatementSyntax* node)
{
	gmt::Ref<const ExpressionSyntax> condition = node->get_condition();

	if (!condition.is_null())
	{
		visit_expression(condition);
		gmt::Ref<StructSymbol> conditionType = get_expression_type(condition);

		if (conditionType.is_null())
			get_diagnostics().report_error(node->get_location(), L"If condition type could not be determined");
		else if (conditionType != get_symbol_table().get_boolean_primitive())
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"If condition must be boolean, got '", get_model().get_type_display_name(conditionType), L"'"));
	}

	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());

	if (!node->get_next().is_null())
		node->get_next().as_ptr()->accept(*this);
}

void ExpressionBinder::visit_unless_statement(const UnlessStatementSyntax* node)
{
	gmt::Ref<const ExpressionSyntax> condition = node->get_condition();

	if (!condition.is_null())
	{
		visit_expression(condition);
		gmt::Ref<StructSymbol> conditionType = get_expression_type(condition);

		if (conditionType.is_null())
			get_diagnostics().report_error(node->get_location(), L"Unless condition type could not be determined");
		else if (conditionType != get_symbol_table().get_boolean_primitive())
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Unless condition must be boolean, got '", get_model().get_type_display_name(conditionType), L"'"));
	}

	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());

	if (!node->get_next().is_null())
		node->get_next().as_ptr()->accept(*this);
}

void ExpressionBinder::visit_else_statement(const ElseStatementSyntax* node)
{
	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());

	if (!node->get_next().is_null())
		node->get_next().as_ptr()->accept(*this);
}

void ExpressionBinder::visit_while_statement(const WhileStatementSyntax* node)
{
	gmt::Ref<const ExpressionSyntax> condition = node->get_condition();

	if (!condition.is_null())
	{
		visit_expression(condition);
		gmt::Ref<StructSymbol> conditionType = get_expression_type(condition);

		if (conditionType.is_null())
			get_diagnostics().report_error(node->get_location(), L"While loop condition type could not be determined");
		else if (conditionType != get_symbol_table().get_boolean_primitive())
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"While loop condition must be boolean, got '", get_model().get_type_display_name(conditionType), L"'"));
	}

	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());
}

void ExpressionBinder::visit_until_statement(const UntilStatementSyntax* node)
{
	gmt::Ref<const ExpressionSyntax> condition = node->get_condition();

	if (!condition.is_null())
	{
		visit_expression(condition);
		gmt::Ref<StructSymbol> conditionType = get_expression_type(condition);

		if (conditionType.is_null())
			get_diagnostics().report_error(node->get_location(), L"Until loop condition type could not be determined");
		else if (conditionType != get_symbol_table().get_boolean_primitive())
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Until loop condition must be boolean, got '", get_model().get_type_display_name(conditionType), L"'"));
	}

	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());
}

void ExpressionBinder::visit_for_in_statement(const ForInStatementSyntax* node)
{
	std::optional<gmt::Ref<const ForInStatementSyntax>> nodeRef = ref_from(node);
	gmt::Ref<VariableSymbol> variable = nodeRef.has_value() ? gmt::Ref<VariableSymbol>(lookup_symbol<VariableSymbol>(*nodeRef)) : gmt::Ref<VariableSymbol>(gmt::nullref);
	gmt::Ref<const ExpressionSyntax> rangeExpression = node->get_range_expression();

	if (!rangeExpression.is_null())
	{
		visit_expression(rangeExpression);
		gmt::Ref<StructSymbol> rangeType = get_expression_type(rangeExpression);

		if (!rangeType.is_null() && rangeType->get_kind() == SyntaxKind::ArrayType)
		{
			gmt::Ref<StructSymbol> elementType = rangeType.as<ArrayTypeSymbol>()->get_underlaying_type();
			if (!variable.is_null() && variable->get_type().is_null())
				variable->set_type(elementType);
		}
		else
		{
			get_diagnostics().report_error(node->get_location(), L"'for-in' expression must be an array");
		}
	}

	if (!variable.is_null())
	{
		get_current_scope()->declare_symbol(variable);
		push_scope(variable);
	}

	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());

	if (!variable.is_null())
		pop_scope();
}

void ExpressionBinder::visit_try_statement(const TryStatementSyntax* node)
{
	if (!node->get_try_block().is_null())
		visit_statements_block(node->get_try_block().as_ptr());

	for (const gmt::Ref<CatchClauseSyntax>& clause : node->get_catch_clauses().as_span())
		visit_catch_clause(clause.as_ptr());
}

void ExpressionBinder::visit_catch_clause(const CatchClauseSyntax* node)
{
	std::optional<gmt::Ref<const CatchClauseSyntax>> clauseRef = ref_from(node);

	gmt::Ref<VariableSymbol> variable = gmt::nullref;
	if (clauseRef.has_value())
		variable = lookup_symbol<VariableSymbol>(*clauseRef);

	if (!variable.is_null())
	{
		get_current_scope()->declare_symbol(variable);
		push_scope(variable);
	}

	if (!node->get_body().is_null())
		visit_statements_block(node->get_body().as_ptr());

	if (!variable.is_null())
		pop_scope();
}

void ExpressionBinder::visit_defer_statement(const DeferStatementSyntax* node)
{
	visit_expression(node->get_expression());
}

void ExpressionBinder::visit_defer_block(const DeferBlockSyntax* node)
{
	if (!node->get_block().is_null())
		visit_statements_block(node->get_block().as_ptr());
}

void ExpressionBinder::visit_defer_variable(const DeferVariableSyntax* node)
{
	if (!node->get_variable().is_null())
		visit_variable_statement(node->get_variable().as_ptr());
}

void ExpressionBinder::visit_statements_block(const StatementsBlockSyntax* node)
{
	push_scope(gmt::nullref);

	for (const gmt::Ref<StatementSyntax>& statement : node->get_statements().as_span())
		statement->accept(*this);

	pop_scope();
}

void ExpressionBinder::visit_arrow_clause(const ArrowClauseSyntax* node)
{
	gmt::Ref<StructSymbol> returnType = find_target_return_type();
	if (returnType.is_null())
		return;

	visit_expression(node->get_expression());

	if (returnType == get_symbol_table().get_void_primitive())
		return;

	gmt::Ref<StructSymbol> expressionType = get_expression_type(node->get_expression());
	if (expressionType.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Expression body return type could not be determined");
		return;
	}

	SemanticModel& model = get_model();
	if (!model.is_assignable_to(returnType, expressionType))
		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Return type mismatch: expected '", model.get_type_display_name(returnType), L"' but got '", model.get_type_display_name(expressionType), L"'"));
}


void ExpressionBinder::visit_if_expression(const IfExpressionSyntax* node)
{
	visit_expression(node->get_condition());
	visit_expression(node->get_then_expression());
	visit_expression(node->get_else_expression());

	std::optional<gmt::Ref<const IfExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<StructSymbol> thenType = get_expression_type(node->get_then_expression());
	gmt::Ref<StructSymbol> elseType = get_expression_type(node->get_else_expression());

	gmt::Ref<StructSymbol> type = thenType;
	if (type.is_null())
	{
		type = elseType;
	}
	else if (!elseType.is_null() && !SemanticModel::are_types_equal(thenType, elseType))
	{
		if (get_model().is_assignable_to(thenType, elseType))
			type = elseType;
		else if (!get_model().is_assignable_to(elseType, thenType))
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch in conditional expression: '", get_model().get_type_display_name(thenType), L"' and '", get_model().get_type_display_name(elseType), L"'"));
	}

	set_expression_type(*nodeRef, type);
}
