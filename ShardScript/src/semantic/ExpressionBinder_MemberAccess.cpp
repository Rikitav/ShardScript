#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Concat.hpp>

#include <gmt/InlineVector.hpp>

using namespace shard;

bool ExpressionBinder::get_is_static_context(gmt::Ref<const ExpressionSyntax> expression) const
{
	if (expression.is_null())
		return true;

	// an identifier marks itself as static context when it resolves to a
	// type; value-resolving expressions stay in instance context
	if (expression->get_kind() == SyntaxKind::InvokationExpression)
		return false;

	return m_staticContextExpressions.count(expression) != 0;
}

static gmt::Ref<StructSymbol> unwrap_nullable(gmt::Ref<StructSymbol> type)
{
	// TODO: pragmatic analysis — full nullable member-access semantics
	while (!type.is_null() && type->get_kind() == SyntaxKind::NullableType)
		type = type.as<NullableTypeSymbol>()->get_underlaying_type();

	return type;
}

static gmt::Ref<StructSymbol> search_type_of(SymbolTable& table, gmt::Ref<StructSymbol> type)
{
	if (type.is_null())
		return type;

	if (type->get_kind() == SyntaxKind::GenericType)
		return type.as<GenericTypeSymbol>()->get_underlaying_type();

	// array instances expose the members of the array primitive (Length, ...)
	if (type->get_kind() == SyntaxKind::ArrayType)
		return table.get_array_primitive();

	return type;
}

static gmt::Ref<PropertySymbol> find_property_on(SymbolTable& table, gmt::Ref<StructSymbol> type, std::wstring_view name)
{
	for (gmt::Ref<PropertySymbol> property : search_type_of(table, type)->properties)
	{
		if (!property.is_null() && property->get_name() == name)
			return property;
	}

	return gmt::nullref;
}

static gmt::Ref<FieldSymbol> find_field_on(SymbolTable& table, gmt::Ref<StructSymbol> type, std::wstring_view name)
{
	for (gmt::Ref<FieldSymbol> field : search_type_of(table, type)->fields)
	{
		if (!field.is_null() && field->get_name() == name)
			return field;
	}

	return gmt::nullref;
}

static gmt::Ref<MethodSymbol> find_method_on(SymbolTable& table, gmt::Ref<StructSymbol> type, std::wstring_view name)
{
	for (gmt::Ref<MethodSymbol> method : search_type_of(table, type)->methods)
	{
		if (!method.is_null() && method->get_name() == name)
			return method;
	}

	return gmt::nullref;
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_member_access_expression(const MemberAccessExpressionSyntax* node, gmt::Ref<StructSymbol> currentType)
{
	TextLocation location = node->get_location();
	SymbolTable& table = get_symbol_table();
	DiagnosticsContext& diagnostics = get_diagnostics();
	gmt::Ref<const ExpressionSyntax> previous = node->get_previous();
	SyntaxToken identifier = node->get_identifier();
	std::optional<gmt::Ref<const MemberAccessExpressionSyntax>> nodeRef = ref_from(node);
	std::wstring_view memberName = identifier.get_lexeme();
	gmt::Ref<SyntaxSymbol> symbol = gmt::nullref;

	if (previous.is_null())
		return analyze_identifier_expression(identifier, node, nodeRef);

	if (currentType.is_null())
		return gmt::nullref;

	currentType = unwrap_nullable(currentType);

	if (!currentType->is_type())
	{
		diagnostics.report_error(location, gmt::concat(L"Cannot access member on non-type '", currentType->get_name(), L"'"));
		return gmt::nullref;
	}

	if (currentType->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<TypeParameterSymbol> typeParameter = currentType.as<TypeParameterSymbol>();
		for (gmt::Ref<StructSymbol> constraint : typeParameter->get_constraints())
		{
			if (constraint.is_null())
				continue;

			symbol = find_property_on(table, constraint, memberName);
			if (symbol.is_null())
				symbol = find_field_on(table, constraint, memberName);
			if (symbol.is_null())
				symbol = find_method_on(table, constraint, memberName);

			if (!symbol.is_null())
			{
				currentType = constraint;
				break;
			}
		}
	}

	if (symbol.is_null())
	{
		symbol = find_property_on(table, currentType, memberName);
		if (symbol.is_null())
			symbol = find_field_on(table, currentType, memberName);
		if (symbol.is_null())
			symbol = find_method_on(table, currentType, memberName);

		if (symbol.is_null())
		{
			gmt::Ref<StructSymbol> accessParameters[] = { table.get_string_primitive() };
			gmt::Ref<OperatorSymbol> accessOperator = resolve_operator_method(currentType, TokenType::Delimeter, accessParameters);
			if (!accessOperator.is_null())
			{
				if (nodeRef.has_value())
					table.bind_symbol<OperatorSymbol>(*nodeRef, accessOperator);

				return accessOperator->get_return_type();
			}

			diagnostics.report_error(location, gmt::concat(L"Member '", memberName, L"' not found in type '", currentType->get_name(), L"'"));
			return gmt::nullref;
		}
	}

	return resolve_symbol_type(symbol, identifier, previous, currentType, nodeRef, location);
}

gmt::Ref<StructSymbol> ExpressionBinder::resolve_symbol_type(gmt::Ref<SyntaxSymbol> symbol, SyntaxToken identifier, gmt::Ref<const ExpressionSyntax> previous, gmt::Ref<StructSymbol> currentType, std::optional<gmt::Ref<const SyntaxNode>> nodeRef, TextLocation location)
{
	SymbolTable& table = get_symbol_table();
	DiagnosticsContext& diagnostics = get_diagnostics();
	std::wstring_view memberName = identifier.get_lexeme();

	bool assignmentContext = false;
	if (nodeRef.has_value())
	{
		const SyntaxNode* syntaxNode = nodeRef->as_ptr();
		gmt::Ref<const SyntaxNode> parent = syntaxNode->get_parent();
		if (!parent.is_null() && parent->get_kind() == SyntaxKind::BinaryExpression)
		{
			gmt::Ref<const BinaryExpressionSyntax> binary = parent.as<BinaryExpressionSyntax>();
			assignmentContext = is_assignment_operator(binary->get_operator_token().get_type()) && binary->get_left().as_ptr() == syntaxNode;
		}
	}

	switch (symbol->get_kind())
	{
		case SyntaxKind::VariableStatement:
		{
			gmt::Ref<VariableSymbol> variable = symbol.as<VariableSymbol>();
			if (nodeRef.has_value())
				table.bind_symbol<VariableSymbol>(*nodeRef, variable);

			return variable->get_type();
		}

		case SyntaxKind::PropertyDeclaration:
		{
			gmt::Ref<PropertySymbol> property = symbol.as<PropertySymbol>();
			if (nodeRef.has_value())
				table.bind_symbol<PropertySymbol>(*nodeRef, property);

			return analyze_property_access_expression(identifier, assignmentContext, nodeRef.has_value() ? *nodeRef : gmt::Ref<const SyntaxNode>(gmt::nullref), previous, property, currentType);
		}

		case SyntaxKind::FieldDeclaration:
		{
			gmt::Ref<FieldSymbol> field = symbol.as<FieldSymbol>();
			gmt::Ref<StructSymbol> fieldType = field->get_return_type();

			if (fieldType.is_null())
			{
				diagnostics.report_error(location, gmt::concat(L"Field '", memberName, L"' type not resolved"));
				return gmt::nullref;
			}

			if (!currentType.is_null() && currentType->get_kind() == SyntaxKind::GenericType)
				fieldType = substitute_type_parameters(fieldType, currentType.as<GenericTypeSymbol>());

			if (nodeRef.has_value())
				table.bind_symbol<FieldSymbol>(*nodeRef, field);

			bool staticContext = get_is_static_context(previous);
			if (staticContext && field->get_linking() == LINK_INSTANCE)
				diagnostics.report_error(location, gmt::concat(L"Cannot access instance field '", field->get_full_name(), L"' from type context"));

			if (!staticContext && field->get_linking() == LINK_STATIC)
				diagnostics.report_error(location, gmt::concat(L"Cannot access static field '", field->get_full_name(), L"' from instance reference"));

			return fieldType;
		}

		case SyntaxKind::FunctionDeclaration:
		{
			gmt::Ref<MethodSymbol> method = symbol.as<MethodSymbol>();
			gmt::Ref<DelegateTypeSymbol> delegateType = m_factory.create_delegate(method, method);

			if (nodeRef.has_value())
				table.bind_symbol<DelegateTypeSymbol>(*nodeRef, delegateType);

			return delegateType;
		}

		case SyntaxKind::Parameter:
		{
			gmt::Ref<ParameterSymbol> parameter = symbol.as<ParameterSymbol>();
			if (nodeRef.has_value())
				table.bind_symbol<ParameterSymbol>(*nodeRef, parameter);

			return parameter->get_type();
		}

		default:
		{
			diagnostics.report_error(location, gmt::concat(L"Symbol '", memberName, L"' is not a variable, parameter or field"));
			return gmt::nullref;
		}
	}
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_identifier_expression(SyntaxToken identifier, const SyntaxNode* node, std::optional<gmt::Ref<const SyntaxNode>> nodeRef)
{
	TextLocation location = node->get_location();
	SymbolTable& table = get_symbol_table();
	DiagnosticsContext& diagnostics = get_diagnostics();
	std::wstring_view memberName = identifier.get_lexeme();
	gmt::Ref<StructSymbol> currentType = gmt::nullref;

	switch (identifier.get_type())
	{
		case TokenType::FieldKeyword:
			return analyze_field_keyword_expression(node);

		case TokenType::StringKeyword:
			return table.get_string_primitive();

		case TokenType::CharKeyword:
			return table.get_char_primitive();

		case TokenType::NativeIntegerKeyword:
			return table.get_nint_primitive();

		case TokenType::IntegerKeyword:
			return table.get_integer_primitive();

		case TokenType::BooleanKeyword:
			return table.get_boolean_primitive();

		case TokenType::ByteKeyword:
			return table.get_byte_primitive();

		case TokenType::DoubleKeyword:
			return table.get_double_primitive();

		default:
			break;
	}

	gmt::Ref<SyntaxSymbol> symbol = get_current_scope()->lookup(memberName);
	if (symbol.is_null())
	{
		if (is_ambiguous_name(memberName))
		{
			diagnostics.report_error(location, gmt::concat(L"Ambiguous reference: '", memberName, L"' is defined in multiple imported namespaces"));
			return gmt::nullref;
		}

		gmt::Ref<StructSymbol> expectedType = unwrap_nullable(resolve_left_denotation());
		if (!expectedType.is_null() && expectedType->get_kind() == SyntaxKind::EnumDeclaration)
		{
			gmt::Ref<EnumSymbol> enumType = expectedType.as<EnumSymbol>();
			for (gmt::Ref<FieldSymbol> field : enumType->fields)
			{
				if (!field.is_null())
				{
					bool matchingEnumValue = field->get_is_enum_value() && field->get_name() == memberName;
					if (matchingEnumValue)
					{
						symbol = field;
						currentType = expectedType;
						break;
					}
				}
			}
		}

		if (symbol.is_null())
		{
			diagnostics.report_error(location, gmt::concat(L"Symbol '", memberName, L"' not found in current scope"));
			return gmt::nullref;
		}
	}

	if (!is_symbol_accessible(symbol, table.lookup_node(symbol), nodeRef.has_value() ? *nodeRef : gmt::Ref<const SyntaxNode>(gmt::nullref)))
		return gmt::nullref;

	if (symbol->is_type())
	{
		if (nodeRef.has_value())
			m_staticContextExpressions.insert(nodeRef->as<const ExpressionSyntax>());

		return symbol.as<StructSymbol>();
	}

	return resolve_symbol_type(symbol, identifier, nodeRef.has_value() ? nodeRef->as<const ExpressionSyntax>() : gmt::Ref<const ExpressionSyntax>(gmt::nullref), currentType, nodeRef, location);
}

void ExpressionBinder::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	std::optional<gmt::Ref<const IdentifierNameTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// type-position identifiers were resolved and bound by the type binder;
	// identifiers the binder itself bound (variables, parameters, fields)
	// resolve through the scope instead
	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
	if (!bound.is_null() && bound->is_type())
	{
		m_staticContextExpressions.insert(*nodeRef);
		set_expression_type(*nodeRef, bound.as<StructSymbol>());
		return;
	}

	gmt::Ref<StructSymbol> type = analyze_identifier_expression(node->get_identifier(), node, nodeRef);
	set_expression_type(*nodeRef, type);
}

void ExpressionBinder::visit_qualified_name_type(const QualifiedNameTypeSyntax* node)
{
	std::optional<gmt::Ref<const QualifiedNameTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// value-position qualified names resolve at their use site; only
	// type-position names carry a binding from the type binder
	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
	if (bound.is_null() || !bound->is_type())
		return;

	m_staticContextExpressions.insert(*nodeRef);
	set_expression_type(*nodeRef, bound.as<StructSymbol>());
}

void ExpressionBinder::visit_generic_type(const GenericTypeSyntax* node)
{
	std::optional<gmt::Ref<const GenericTypeSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
	if (bound.is_null() || !bound->is_type())
		return;

	m_staticContextExpressions.insert(*nodeRef);
	set_expression_type(*nodeRef, bound.as<StructSymbol>());
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_property_access_expression(SyntaxToken identifier, bool assignmentContext, gmt::Ref<const SyntaxNode> callSite, gmt::Ref<const ExpressionSyntax> previous, gmt::Ref<PropertySymbol> property, gmt::Ref<StructSymbol> currentType)
{
	DiagnosticsContext& diagnostics = get_diagnostics();
	std::wstring_view memberName = identifier.get_lexeme();
	TextLocation location = identifier.get_location();

	if (property.is_null() || currentType.is_null())
		return gmt::nullref;

	if (property->get_return_type().is_null())
	{
		diagnostics.report_error(location, gmt::concat(L"Property '", memberName, L"' type not resolved"));
		return gmt::nullref;
	}

	bool requiresSetter = assignmentContext;
	gmt::Ref<AccessorSymbol> accessor = requiresSetter ? property->get_setter() : property->get_getter();

	if (accessor.is_null())
	{
		diagnostics.report_error(location, gmt::concat(L"Property '", memberName, L"' does not have a ", requiresSetter ? L"set" : L"get", L" accessor"));
		return gmt::nullref;
	}

	if (!is_symbol_accessible(accessor, get_symbol_table().lookup_node(accessor), callSite))
		return gmt::nullref;

	bool staticContext = get_is_static_context(previous);
	if (staticContext && accessor->get_linking() == LINK_INSTANCE)
	{
		diagnostics.report_error(location, gmt::concat(L"Cannot access instance property '", memberName, L"' from type context"));
		return gmt::nullref;
	}

	if (!staticContext && accessor->get_linking() == LINK_STATIC)
	{
		diagnostics.report_error(location, gmt::concat(L"Cannot access static property '", memberName, L"' from instance reference"));
		return gmt::nullref;
	}

	gmt::Ref<StructSymbol> propertyType = property->get_return_type();

	if (currentType->get_kind() == SyntaxKind::GenericType && !propertyType.is_null())
		propertyType = substitute_type_parameters(propertyType, currentType.as<GenericTypeSymbol>());

	return propertyType;
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_field_keyword_expression(const SyntaxNode* node)
{
	gmt::Ref<PropertySymbol> propertySymbol = gmt::nullref;
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		gmt::Ref<SyntaxSymbol> owner = scope->get_owner();
		if (owner.is_null())
			continue;

		if (owner->is_type())
			break;

		if (owner->get_kind() != SyntaxKind::PropertyDeclaration)
			continue;

		propertySymbol = owner.as<PropertySymbol>();
		break;
	}

	if (propertySymbol.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Keyword 'field' can only be used in property accessors");
		return gmt::nullref;
	}

	if (propertySymbol->get_backing_field().is_null())
		propertySymbol->set_backing_field(m_factory.create_backing_field(propertySymbol, propertySymbol));

	gmt::Ref<FieldSymbol> backingField = propertySymbol->get_backing_field();
	if (backingField->get_return_type().is_null())
	{
		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Backing field type not resolved for property '", propertySymbol->get_name(), L"'"));
		return gmt::nullref;
	}

	std::optional<gmt::Ref<const SyntaxNode>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		get_symbol_table().bind_symbol<FieldSymbol>(*nodeRef, backingField);

	if (propertySymbol->get_linking() == LINK_STATIC)
		m_staticContextExpressions.insert(nodeRef->as<const ExpressionSyntax>());

	return backingField->get_return_type();
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_indexator_expression(const IndexatorExpressionSyntax* node, gmt::Ref<StructSymbol> currentType)
{
	gmt::Ref<const ExpressionSyntax> previous = node->get_previous();
	gmt::Ref<const ArgumentsListSyntax> arguments = node->get_arguments();

	if (currentType.is_null() || previous.is_null())
		return gmt::nullref;

	currentType = unwrap_nullable(currentType);

	// arrays expose an implicit element indexator
	if (currentType->get_kind() == SyntaxKind::ArrayType)
	{
		gmt::Ref<StructSymbol> elementType = currentType.as<ArrayTypeSymbol>()->get_underlaying_type();

		if (!arguments.is_null())
		{
			for (const gmt::Ref<ArgumentSyntax>& argument : arguments->get_arguments().as_span())
			{
				visit_expression(argument->get_expression());
				gmt::Ref<StructSymbol> argumentType = get_expression_type(argument->get_expression());
				if (!argumentType.is_null() && argumentType != get_symbol_table().get_integer_primitive())
					get_diagnostics().report_error(node->get_location(), L"Array index must be an integer");
			}
		}

		return elementType;
	}

	gmt::Ref<IndexatorSymbol> indexator = resolve_indexator(node, currentType);
	if (indexator.is_null())
		return gmt::nullref;

	std::optional<gmt::Ref<const IndexatorExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<StructSymbol> resultType = analyze_property_access_expression(node->get_delimeter_token(), is_assignment_context(node), nodeRef.has_value() ? *nodeRef : gmt::Ref<const SyntaxNode>(gmt::nullref), previous, indexator, currentType);

	if (resultType.is_null())
		return gmt::nullref;

	gmt::Ref<AccessorSymbol> accessor = is_assignment_context(node) ? indexator->get_setter() : indexator->get_getter();
	if (!accessor.is_null() && !arguments.is_null())
	{
		gmt::Ref<GenericTypeSymbol> genericType = gmt::nullref;
		if (currentType->get_kind() == SyntaxKind::GenericType)
			genericType = currentType.as<GenericTypeSymbol>();

		if (!match_method_arguments(node->get_delimeter_token(), indexator->get_parameters(), arguments->get_arguments(), genericType))
			return gmt::nullref;
	}

	if (nodeRef.has_value())
		get_symbol_table().bind_symbol<IndexatorSymbol>(*nodeRef, indexator);

	return resultType;
}

gmt::Ref<IndexatorSymbol> ExpressionBinder::resolve_indexator(const IndexatorExpressionSyntax* node, gmt::Ref<StructSymbol> currentType)
{
	TextLocation location = node->get_location();
	DiagnosticsContext& diagnostics = get_diagnostics();

	gmt::inline_vector<gmt::Ref<StructSymbol>, 4> argumentTypes;
	if (!node->get_arguments().is_null())
	{
		for (const gmt::Ref<ArgumentSyntax>& argument : node->get_arguments()->get_arguments().as_span())
		{
			visit_expression(argument->get_expression());
			gmt::Ref<StructSymbol> argumentType = get_expression_type(argument->get_expression());
			if (argumentType.is_null())
				return gmt::nullref;

			argumentTypes.push_back(argumentType);
		}
	}

	gmt::Ref<StructSymbol> searchType = search_type_of(get_symbol_table(), currentType);
	gmt::Ref<IndexatorSymbol> selected = gmt::nullref;
	int selectedScore = 0;

	for (gmt::Ref<IndexatorSymbol> candidate : searchType->indexators)
	{
		if (candidate.is_null())
			continue;

		const std::vector<gmt::Ref<ParameterSymbol>>& parameters = candidate->get_parameters();
		if (parameters.size() != argumentTypes.get_vector().size())
			continue;

		gmt::Ref<GenericTypeSymbol> genericType = gmt::nullref;
		if (currentType->get_kind() == SyntaxKind::GenericType)
			genericType = currentType.as<GenericTypeSymbol>();

		bool convertible = true;
		int totalScore = 0;
		for (std::size_t i = 0; i < parameters.size(); i++)
		{
			gmt::Ref<StructSymbol> parameterType = substitute_type_parameters(parameters[i]->get_type(), genericType);
			int score = conversion_score(parameterType, argumentTypes.get_vector()[i]);
			if (score < 0)
			{
				convertible = false;
				break;
			}

			totalScore += score;
		}

		if (!convertible)
			continue;

		if (selected.is_null() || totalScore < selectedScore)
		{
			selected = candidate;
			selectedScore = totalScore;
		}
		else if (totalScore == selectedScore)
		{
			selected = gmt::nullref; // ambiguous
		}
	}

	if (selected.is_null())
	{
		diagnostics.report_error(location, gmt::concat(L"No indexators for type '", currentType->get_name(), L"' found that accept the supplied arguments"));
		return gmt::nullref;
	}

	bool staticContext = get_is_static_context(node->get_previous());
	if (staticContext && selected->get_linking() == LINK_INSTANCE)
	{
		diagnostics.report_error(location, L"Cannot call instance indexator from type context");
		return gmt::nullref;
	}

	if (!staticContext && selected->get_linking() == LINK_STATIC)
	{
		diagnostics.report_error(location, L"Cannot call static indexator on instance reference");
		return gmt::nullref;
	}

	return selected;
}

void ExpressionBinder::visit_member_access_expression(const MemberAccessExpressionSyntax* node)
{
	gmt::Ref<const ExpressionSyntax> previous = node->get_previous();
	if (!previous.is_null())
		visit_expression(previous);

	gmt::Ref<StructSymbol> currentType = previous.is_null() ? get_owner_type() : get_expression_type(previous);
	gmt::Ref<StructSymbol> type = analyze_member_access_expression(node, currentType);

	std::optional<gmt::Ref<const MemberAccessExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}

void ExpressionBinder::visit_indexator_expression(const IndexatorExpressionSyntax* node)
{
	gmt::Ref<const ExpressionSyntax> previous = node->get_previous();
	if (!previous.is_null())
		visit_expression(previous);

	gmt::Ref<StructSymbol> currentType = previous.is_null() ? get_owner_type() : get_expression_type(previous);
	gmt::Ref<StructSymbol> type = analyze_indexator_expression(node, currentType);

	std::optional<gmt::Ref<const IndexatorExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}
