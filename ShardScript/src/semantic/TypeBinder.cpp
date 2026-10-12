#include <shard/semantic/TypeBinder.hpp>

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
			bind_expression(expression.as<InvokationExpressionSyntax>()->get_previous());
			bind_arguments(expression.as<InvokationExpressionSyntax>()->get_arguments());
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
				m_ambiguousNames.insert(member->get_name());

			continue;
		}

		current->declare_symbol(member);
	}
}

bool TypeBinder::is_ambiguous_name(std::wstring_view name)
{
	return m_ambiguousNames.count(name) != 0;
}

void TypeBinder::bind_where_clauses(gmt::Ref<const WhereClausesListSyntax> clauses, const std::vector<gmt::Ref<TypeParameterSymbol>>& type_parameters)
{
	if (clauses.is_null())
		return;

	for (const gmt::Ref<WhereClauseSyntax>& clause : clauses->get_clauses().as_span())
	{
		if (clause.is_null())
			continue;

		const std::wstring_view parameter_name = clause->get_identifier().get_lexeme();

		gmt::Ref<TypeParameterSymbol> type_parameter = gmt::nullref;
		for (const gmt::Ref<TypeParameterSymbol>& candidate : type_parameters)
		{
			if (!candidate.is_null() && candidate->get_name() == parameter_name)
			{
				type_parameter = candidate;
				break;
			}
		}

		if (type_parameter.is_null())
		{
			get_diagnostics().report_error(clause->get_location(), L"Type parameter '" + std::wstring(parameter_name) + L"' not found");
			continue;
		}

		for (const gmt::Ref<TypeSyntax>& constraint_type : clause->get_constraint_types().as_span())
		{
			gmt::Ref<StructSymbol> constraint = resolve_type(constraint_type);
			if (constraint.is_null())
			{
				get_diagnostics().report_error(clause->get_location(), L"Could not resolve constraint type for type parameter '" + std::wstring(parameter_name) + L"'");
				continue;
			}

			type_parameter->add_constraint(constraint);
		}
	}
}

void TypeBinder::bind_base_types(gmt::Ref<StructSymbol> symbol, gmt::Ref<const BaseTypesListSyntax> base_types)
{
	if (base_types.is_null())
		return;

	for (const gmt::Ref<TypeSyntax>& base_type : base_types->get_types().as_span())
	{
		gmt::Ref<StructSymbol> base_symbol = resolve_type(base_type);
		if (base_symbol.is_null())
			continue;

		if (base_symbol->get_kind() == SyntaxKind::InterfaceDeclaration)
		{
			symbol->interfaces.push_back(base_symbol);
			continue;
		}

		if (base_symbol->get_kind() == SyntaxKind::GenericType)
		{
			gmt::Ref<StructSymbol> underlaying = base_symbol.as<GenericTypeSymbol>()->get_underlaying_type();
			if (!underlaying.is_null() && underlaying->get_kind() == SyntaxKind::InterfaceDeclaration)
			{
				symbol->interfaces.push_back(base_symbol);
				continue;
			}
		}

		get_diagnostics().report_error(base_type->get_location(), L"Base type must be an interface");
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
		gmt::Ref<NamespaceSymbol> namespace_symbol = lookup_symbol<NamespaceSymbol>(directive);

		if (namespace_symbol.is_null())
			throw std::runtime_error("namespace symbol not found");

		push_scope(namespace_symbol);
	}

	// declare member symbols upfront so members can reference each other
	// regardless of declaration order; the binder re-declares into its own
	// fresh scopes, so this bypasses the owner hook on purpose - calling
	// declare() here would duplicate the member lists populated during
	// collection
	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> member_symbol = lookup_symbol<SyntaxSymbol>(member);
		if (!member_symbol.is_null())
			get_current_scope()->declare_symbol(member_symbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	pop_scope();

	if (node->has_namespace())
		pop_scope();
}

void TypeBinder::visit_using_directive(const UsingDirectiveSyntax* node)
{
	std::optional<gmt::Ref<const UsingDirectiveSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<NamespaceSymbol> current = get_symbol_table().get_root_namespace();
	for (const SyntaxToken& token : node->get_qualifier().as_span())
	{
		gmt::Ref<NamespaceSymbol> next = current->lookup(token.get_lexeme());
		if (next.is_null())
		{
			get_diagnostics().report_error(node->get_location(), L"Namespace '" + std::wstring(token.get_lexeme()) + L"' does not exist");
			return;
		}

		current = next;
	}

	if (node_ref.has_value())
		get_symbol_table().bind_symbol<NamespaceSymbol>(*node_ref, current);

	import_namespace(*node_ref, current);
}

void TypeBinder::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const StructDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<StructSymbol> symbol = lookup_symbol<StructSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("struct symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& type_parameter : symbol->type_parameters)
		get_current_scope()->declare_symbol(type_parameter);

	bind_where_clauses(node->get_where_clauses(), symbol->type_parameters);
	bind_base_types(symbol, node->get_base_types());

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> member_symbol = lookup_symbol<SyntaxSymbol>(member);
		if (!member_symbol.is_null())
			get_current_scope()->declare_symbol(member_symbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_interface_declaration(const InterfaceDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const InterfaceDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<InterfaceSymbol> symbol = lookup_symbol<InterfaceSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("interface symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& type_parameter : symbol->type_parameters)
		get_current_scope()->declare_symbol(type_parameter);

	bind_where_clauses(node->get_where_clauses(), symbol->type_parameters);

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
	{
		gmt::Ref<SyntaxSymbol> member_symbol = lookup_symbol<SyntaxSymbol>(member);
		if (!member_symbol.is_null())
			get_current_scope()->declare_symbol(member_symbol);
	}

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_delegate_declaration(const DelegateDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const DelegateDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<DelegateTypeSymbol> symbol = lookup_symbol<DelegateTypeSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("delegate symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& type_parameter : symbol->get_type_parameters())
		get_current_scope()->declare_symbol(type_parameter);

	if (!node->get_return_type().is_null())
		symbol->set_return_type(resolve_type(node->get_return_type()));

	if (!node->get_parameters_list().is_null())
		node->get_parameters_list().as_ptr()->accept(*this);

	pop_scope();
}

void TypeBinder::visit_enum_declaration(const EnumDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const EnumDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<EnumSymbol> symbol = lookup_symbol<EnumSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("enum symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<EnumFieldDeclarationSyntax>& field : node->get_fields().as_span())
		bind_expression(field->get_expression());

	pop_scope();
}

void TypeBinder::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const ConstructorDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*node_ref);
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
	std::optional<gmt::Ref<const FunctionDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("method symbol not found");

	push_scope(symbol);

	for (const gmt::Ref<TypeParameterSymbol>& type_parameter : symbol->get_type_parameters())
		get_current_scope()->declare_symbol(type_parameter);

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
	std::optional<gmt::Ref<const OperatorDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<OperatorSymbol> symbol = lookup_symbol<OperatorSymbol>(*node_ref);
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
	std::optional<gmt::Ref<const FieldDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<FieldSymbol> symbol = lookup_symbol<FieldSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("field symbol not found");

	if (!node->get_type().is_null())
		symbol->set_return_type(resolve_type(node->get_type()));

	bind_expression(node->get_expression());
}

void TypeBinder::visit_property_declaration(const PropertyDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const PropertyDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<PropertySymbol> symbol = lookup_symbol<PropertySymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("property symbol not found");

	if (node->get_type().is_null())
		return;

	push_scope(symbol);

	gmt::Ref<StructSymbol> property_type = resolve_type(node->get_type());
	symbol->set_return_type(property_type);

	gmt::Ref<FieldSymbol> backing_field = symbol->get_backing_field();
	if (!backing_field.is_null())
		backing_field->set_return_type(property_type);

	gmt::Ref<AccessorSymbol> getter = symbol->get_getter();
	if (!getter.is_null())
		getter->set_return_type(property_type);

	gmt::Ref<AccessorSymbol> setter = symbol->get_setter();
	if (!setter.is_null() && !setter->get_parameters().empty())
		setter->get_parameters()[0]->set_type(property_type);

	bind_expression(node->get_expression());

	if (!node->get_setter().is_null())
		visit_accessor_declaration(node->get_setter().as_ptr());

	if (!node->get_getter().is_null())
		visit_accessor_declaration(node->get_getter().as_ptr());

	pop_scope();
}

void TypeBinder::visit_indexator_declaration(const IndexatorDeclarationSyntax* node)
{
	std::optional<gmt::Ref<const IndexatorDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<IndexatorSymbol> symbol = lookup_symbol<IndexatorSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("indexator symbol not found");

	push_scope(symbol);

	if (!node->get_type().is_null())
	{
		gmt::Ref<StructSymbol> property_type = resolve_type(node->get_type());
		symbol->set_return_type(property_type);

		gmt::Ref<AccessorSymbol> getter = symbol->get_getter();
		if (!getter.is_null())
			getter->set_return_type(property_type);

		gmt::Ref<AccessorSymbol> setter = symbol->get_setter();
		if (!setter.is_null() && !setter->get_parameters().empty())
			setter->get_parameters()[0]->set_type(property_type);
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
	std::optional<gmt::Ref<const AccessorDeclarationSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<AccessorSymbol> symbol = lookup_symbol<AccessorSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("accessor symbol not found");

	gmt::Ref<const SyntaxNode> parent_node = node->get_parent();

	if (parent_node->get_kind() == SyntaxKind::IndexatorDeclaration)
	{
		// index parameters join each accessor's own parameter list; the
		// implicit setter 'value' parameter keeps the leading slot
		gmt::Ref<IndexatorSymbol> indexator = lookup_symbol<IndexatorSymbol>(parent_node);
		if (!indexator.is_null())
		{
			for (const gmt::Ref<ParameterSymbol>& index_parameter : indexator->get_parameters())
				symbol->on_symbol_declared(index_parameter);
		}
	}
	else
	{
		gmt::Ref<PropertySymbol> property = lookup_symbol<PropertySymbol>(parent_node);
		if (!property.is_null() && node->get_keyword().get_type() == TokenType::GetKeyword)
			symbol->set_return_type(property->get_return_type());
	}
}

void TypeBinder::visit_variable_statement(const VariableStatementSyntax* node)
{
	std::optional<gmt::Ref<const VariableStatementSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<VariableSymbol> symbol = lookup_symbol<VariableSymbol>(*node_ref);
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
		gmt::Ref<StructSymbol> exception_type = gmt::nullref;		if (!clause->get_exception_type().is_null())
			exception_type = resolve_type(clause->get_exception_type());

		gmt::Ref<VariableSymbol> catch_variable = lookup_symbol<VariableSymbol>(clause);
		if (!catch_variable.is_null())
			catch_variable->set_type(exception_type);

		if (!clause->get_body().is_null())
			clause->get_body().as_ptr()->accept(*this);
	}
}

void TypeBinder::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	std::optional<gmt::Ref<const LambdaExpressionSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	// only the signature is bound here; the body belongs to the expression
	// binder, same split the declaration collector makes
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
}

void TypeBinder::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	std::optional<gmt::Ref<const ObjectExpressionSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	if (node->get_type().is_null())
		return;

	gmt::Ref<StructSymbol> created_type = resolve_type(node->get_type());

	bind_expression(node->get_array_size());
	bind_arguments(node->get_arguments());

	get_symbol_table().bind_symbol<StructSymbol>(*node_ref, created_type);
}

void TypeBinder::visit_parameter(const ParameterSyntax* node)
{
	std::optional<gmt::Ref<const ParameterSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<ParameterSymbol> symbol = lookup_symbol<ParameterSymbol>(*node_ref);
	if (symbol.is_null())
		throw std::runtime_error("parameter symbol not found");

	if (!node->get_type().is_null())
		symbol->set_type(resolve_type(node->get_type()));
}

void TypeBinder::visit_predefined_type(const PredefinedTypeSyntax* node)
{
	std::optional<gmt::Ref<const PredefinedTypeSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
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

	get_symbol_table().bind_symbol<StructSymbol>(*node_ref, symbol);
}

void TypeBinder::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	std::optional<gmt::Ref<const IdentifierNameTypeSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	const std::wstring_view name = node->get_identifier().get_lexeme();

	if (name == L"Self")
	{
		gmt::Ref<StructSymbol> owner_type = get_owner_type();
		if (owner_type.is_null() || owner_type->get_kind() != SyntaxKind::StructDeclaration)
		{
			get_diagnostics().report_error(node->get_location(), L"'Self' can only be used inside a struct declaration");
			return;
		}

		if (owner_type->type_parameters.empty())
		{
			get_symbol_table().bind_symbol<StructSymbol>(*node_ref, owner_type);
			return;
		}

		std::vector<gmt::Ref<StructSymbol>> arguments(owner_type->type_parameters.begin(), owner_type->type_parameters.end());
		gmt::Ref<GenericTypeSymbol> self_type = get_symbol_table().find_or_create_generic_type(owner_type, arguments);
		get_symbol_table().bind_symbol<StructSymbol>(*node_ref, self_type);
		return;
	}

	gmt::Ref<SyntaxSymbol> symbol = get_current_scope()->lookup(name);

	if (symbol.is_null())
	{
		if (is_ambiguous_name(name))
		{
			get_diagnostics().report_error(node->get_location(), L"Ambiguous reference: '" + std::wstring(name) + L"' is defined in multiple imported namespaces");
			return;
		}

		get_diagnostics().report_error(node->get_location(), L"Symbol wasn't found in current scope");
		return;
	}

	if (symbol->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<TypeParameterSymbol> type_parameter = symbol.as<TypeParameterSymbol>();
		get_symbol_table().bind_symbol<StructSymbol>(*node_ref, type_parameter);

		if (!is_symbol_accessible(type_parameter, get_symbol_table().lookup_node(type_parameter), *node_ref))
			get_diagnostics().report_error(node->get_location(), L"Symbol inaccessible");

		return;
	}

	if (!symbol->is_type())
	{
		get_diagnostics().report_error(node->get_location(), L"Symbol is not a type");
		return;
	}

	gmt::Ref<StructSymbol> type_symbol = symbol.as<StructSymbol>();
	get_symbol_table().bind_symbol<StructSymbol>(*node_ref, type_symbol);

	if (!is_symbol_accessible(type_symbol, get_symbol_table().lookup_node(type_symbol), *node_ref))
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
		gmt::Ref<NamespaceSymbol> parent_namespace = resolve_namespace_type(qualified->get_left());
		if (parent_namespace.is_null())
			return gmt::nullref;

		return parent_namespace->lookup(qualified->get_identifier().get_lexeme());
	}

	return gmt::nullref;
}

void TypeBinder::visit_qualified_name_type(const QualifiedNameTypeSyntax* node)
{
	std::optional<gmt::Ref<const QualifiedNameTypeSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<NamespaceSymbol> namespace_symbol = resolve_namespace_type(node->get_left());
	if (namespace_symbol.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Left side of qualified type name must be a namespace");
		return;
	}

	for (const gmt::Ref<SyntaxSymbol>& member : namespace_symbol->members)
	{
		if (member.is_null() || member->get_name() != node->get_identifier().get_lexeme())
			continue;

		if (!member->is_type())
			continue;

		get_symbol_table().bind_symbol<StructSymbol>(*node_ref, member.as<StructSymbol>());
		return;
	}

	get_diagnostics().report_error(node->get_location(), L"Type '" + std::wstring(node->get_identifier().get_lexeme()) + L"' not found in namespace");
}

void TypeBinder::visit_array_type(const ArrayTypeSyntax* node)
{
	std::optional<gmt::Ref<const ArrayTypeSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<StructSymbol> underlaying = resolve_type(node->get_underlaying_type());
	if (underlaying.is_null())
		return;

	// the registry makes every reoccurring 'int[]' share one symbol
	m_factory.create_array_type(*node_ref, underlaying);
}

void TypeBinder::visit_nullable_type(const NullableTypeSyntax* node)
{
	std::optional<gmt::Ref<const NullableTypeSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	// TODO : there is no nullable type symbol yet
	gmt::Ref<StructSymbol> underlaying = resolve_type(node->get_underlaying_type());
	if (!underlaying.is_null())
		get_symbol_table().bind_symbol<StructSymbol>(*node_ref, underlaying);
}

bool TypeBinder::type_implements_interface(gmt::Ref<StructSymbol> type_argument, gmt::Ref<InterfaceSymbol> interface_symbol)
{
	if (type_argument.is_null() || interface_symbol.is_null())
		return false;

	for (const gmt::Ref<StructSymbol>& interface_type : type_argument->interfaces)
	{
		if (interface_type.is_null())
			continue;

		if (interface_type == interface_symbol)
			return true;

		if (interface_type->get_kind() == SyntaxKind::GenericType &&
			interface_type.as<GenericTypeSymbol>()->get_underlaying_type() == interface_symbol)
		{
			return true;
		}
	}

	return false;
}

bool TypeBinder::type_satisfies_constraint(gmt::Ref<StructSymbol> type_argument, gmt::Ref<StructSymbol> constraint)
{
	if (type_argument.is_null() || constraint.is_null())
		return false;

	if (constraint->get_kind() == SyntaxKind::InterfaceDeclaration)
		return type_implements_interface(type_argument, constraint.as<InterfaceSymbol>());

	if (constraint->is_type())
		return type_argument == constraint;

	return false;
}

void TypeBinder::visit_generic_type(const GenericTypeSyntax* node)
{
	std::optional<gmt::Ref<const GenericTypeSyntax>> node_ref = ref_from(node);
	if (!node_ref.has_value())
		return;

	gmt::Ref<StructSymbol> underlaying = resolve_type(node->get_underlaying_type());
	if (underlaying.is_null())
		return;

	if (node->get_type_arguments().is_null() || node->get_type_arguments()->get_types().as_span().empty())
	{
		get_diagnostics().report_error(node->get_location(), L"Generic type requires type arguments");
		return;
	}

	const auto argument_nodes = node->get_type_arguments()->get_types().as_span();
	const std::size_t argument_count = argument_nodes.size();
	const std::size_t parameter_count = underlaying->type_parameters.size();

	if (argument_count != parameter_count)
	{
		get_diagnostics().report_error(node->get_location(), L"Type requires " + std::to_wstring(parameter_count) + L" type arguments, but got " + std::to_wstring(argument_count));
		return;
	}

	std::vector<gmt::Ref<StructSymbol>> arguments(argument_count, gmt::nullref);
	for (std::size_t i = 0; i < argument_count; ++i)
		arguments[i] = resolve_type(argument_nodes[i]);

	// Resolve fully before creation so the registry never sees partially substituted symbols.
	// Refs are shared, so mutating one would corrupt every occurrence of the same shape.
	gmt::Ref<GenericTypeSymbol> symbol = get_symbol_table().find_or_create_generic_type(underlaying, arguments);
	get_symbol_table().bind_symbol<StructSymbol>(*node_ref, symbol);

	for (std::size_t i = 0; i < argument_count; ++i)
	{
		gmt::Ref<StructSymbol> argument = arguments[i];
		if (argument.is_null())
		{
			get_diagnostics().report_error(node->get_location(), L"Type argument " + std::to_wstring(i + 1) + L" could not be resolved");
			continue;
		}

		gmt::Ref<TypeParameterSymbol> type_parameter = underlaying->type_parameters[i];
		for (const gmt::Ref<StructSymbol>& constraint : type_parameter->get_constraints())
		{
			if (constraint.is_null())
				continue;

			if (!type_satisfies_constraint(argument, constraint))
			{
				get_diagnostics().report_error(node->get_location(), L"Type argument '" + std::wstring(argument->get_name()) + L"' does not satisfy the constraint '" + std::wstring(constraint->get_name()) + L"' on type parameter '" + std::wstring(type_parameter->get_name()) + L"'");
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
