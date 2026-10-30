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
#include <gmt/Containers.hpp>

#include <gmt/InlineVector.hpp>

using namespace shard;

bool ExpressionBinder::get_is_static_context(const ExpressionSyntax* expression) const
{
	if (expression == nullptr)
		return true;

	// an identifier marks itself as static context when it resolves to a
	// type; value-resolving expressions stay in instance context
	if (expression->get_kind() == SyntaxKind::InvokationExpression)
		return false;

	return m_staticContextExpressions.count(expression) != 0;
}

static StructSymbol* unwrap_nullable(StructSymbol* type)
{
	// TODO: pragmatic analysis — full nullable member-access semantics
	while (!(type == nullptr) && type->get_kind() == SyntaxKind::NullableType)
		type = static_cast<NullableTypeSymbol*>(type)->get_underlaying_type();

	return type;
}

static StructSymbol* search_type_of(SymbolTable& table, StructSymbol* type)
{
	if (type == nullptr)
		return type;

	if (type->get_kind() == SyntaxKind::GenericType)
		return static_cast<GenericTypeSymbol*>(type)->get_underlaying_type();

	// array instances expose the members of the array primitive (Length, ...)
	if (type->get_kind() == SyntaxKind::ArrayType)
		return table.get_array_primitive();

	return type;
}

static PropertySymbol* find_property_on(SymbolTable& table, StructSymbol* type, std::wstring_view name)
{
	for (PropertySymbol* property : search_type_of(table, type)->properties)
	{
		if (!(property == nullptr) && property->get_name() == name)
			return property;
	}

	return nullptr;
}

static FieldSymbol* find_field_on(SymbolTable& table, StructSymbol* type, std::wstring_view name)
{
	for (FieldSymbol* field : search_type_of(table, type)->fields)
	{
		if (!(field == nullptr) && field->get_name() == name)
			return field;
	}

	return nullptr;
}

static MethodSymbol* find_method_on(SymbolTable& table, StructSymbol* type, std::wstring_view name)
{
	for (MethodSymbol* method : search_type_of(table, type)->methods)
	{
		if (!(method == nullptr) && method->get_name() == name)
			return method;
	}

	return nullptr;
}

StructSymbol* ExpressionBinder::analyze_member_access_expression(const MemberAccessExpressionSyntax* node, StructSymbol* currentType)
{
	TextLocation location = node->get_location();
	SymbolTable& table = get_symbol_table();
	DiagnosticsContext& diagnostics = get_diagnostics();
	const ExpressionSyntax* previous = node->get_previous();
	SyntaxToken identifier = node->get_identifier();
	std::optional<const MemberAccessExpressionSyntax*> nodeRef = ref_from(node);
	std::wstring_view memberName = identifier.get_lexeme();
	SyntaxSymbol* symbol = nullptr;

	if (previous == nullptr)
		return analyze_identifier_expression(identifier, node, nodeRef);

	if (currentType == nullptr)
		return nullptr;

	currentType = unwrap_nullable(currentType);

	if (!currentType->is_type())
	{
		diagnostics.report_error(location, gmt::concat(L"Cannot access member on non-type '", currentType->get_name(), L"'"));
		return nullptr;
	}

	if (currentType->get_kind() == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* typeParameter = static_cast<TypeParameterSymbol*>(currentType);
		for (StructSymbol* constraint : typeParameter->get_constraints())
		{
			if (constraint == nullptr)
				continue;

			symbol = find_property_on(table, constraint, memberName);
			if (symbol == nullptr)
				symbol = find_field_on(table, constraint, memberName);
			if (symbol == nullptr)
				symbol = find_method_on(table, constraint, memberName);

			if (!(symbol == nullptr))
			{
				currentType = constraint;
				break;
			}
		}
	}

	if (symbol == nullptr)
	{
		symbol = find_property_on(table, currentType, memberName);
		if (symbol == nullptr)
			symbol = find_field_on(table, currentType, memberName);
		if (symbol == nullptr)
			symbol = find_method_on(table, currentType, memberName);

		if (symbol == nullptr)
		{
			StructSymbol* accessParameters[] = { table.get_string_primitive() };
			OperatorSymbol* accessOperator = resolve_operator_method(currentType, TokenType::Delimeter, accessParameters);
			if (!(accessOperator == nullptr))
			{
				if (nodeRef.has_value())
					table.bind_symbol<OperatorSymbol>(*nodeRef, accessOperator);

				return accessOperator->get_return_type();
			}

			diagnostics.report_error(location, gmt::concat(L"Member '", memberName, L"' not found in type '", currentType->get_name(), L"'"));
			return nullptr;
		}
	}

	return resolve_symbol_type(symbol, identifier, previous, currentType, nodeRef, location);
}

StructSymbol* ExpressionBinder::resolve_symbol_type(SyntaxSymbol* symbol, SyntaxToken identifier, const ExpressionSyntax* previous, StructSymbol* currentType, std::optional<const SyntaxNode*> nodeRef, TextLocation location)
{
	SymbolTable& table = get_symbol_table();
	DiagnosticsContext& diagnostics = get_diagnostics();
	std::wstring_view memberName = identifier.get_lexeme();

	bool assignmentContext = false;
	if (nodeRef.has_value())
	{
		const SyntaxNode* syntaxNode = *nodeRef;
		const SyntaxNode* parent = syntaxNode->get_parent();
		if (!(parent == nullptr) && parent->get_kind() == SyntaxKind::BinaryExpression)
		{
			const BinaryExpressionSyntax* binary = gmt::as<BinaryExpressionSyntax>(parent);
			assignmentContext = is_assignment_operator(binary->get_operator_token().get_type()) && binary->get_left() == syntaxNode;
		}
	}

	switch (symbol->get_kind())
	{
		case SyntaxKind::VariableStatement:
		{
			VariableSymbol* variable = static_cast<VariableSymbol*>(symbol);
			if (nodeRef.has_value())
				table.bind_symbol<VariableSymbol>(*nodeRef, variable);

			return variable->get_type();
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* property = static_cast<PropertySymbol*>(symbol);
			if (nodeRef.has_value())
				table.bind_symbol<PropertySymbol>(*nodeRef, property);

			return analyze_property_access_expression(identifier, assignmentContext, nodeRef.has_value() ? *nodeRef : nullptr, previous, property, currentType);
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* field = static_cast<FieldSymbol*>(symbol);
			StructSymbol* fieldType = field->get_return_type();

			if (fieldType == nullptr)
			{
				diagnostics.report_error(location, gmt::concat(L"Field '", memberName, L"' type not resolved"));
				return nullptr;
			}

			if (!(currentType == nullptr) && currentType->get_kind() == SyntaxKind::GenericType)
				fieldType = substitute_type_parameters(fieldType, static_cast<GenericTypeSymbol*>(currentType));

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
			MethodSymbol* method = static_cast<MethodSymbol*>(symbol);
			DelegateTypeSymbol* delegateType = m_factory.create_delegate(method, method);

			if (nodeRef.has_value())
				table.bind_symbol<DelegateTypeSymbol>(*nodeRef, delegateType);

			return delegateType;
		}

		case SyntaxKind::Parameter:
		{
			ParameterSymbol* parameter = static_cast<ParameterSymbol*>(symbol);
			if (nodeRef.has_value())
				table.bind_symbol<ParameterSymbol>(*nodeRef, parameter);

			return parameter->get_type();
		}

		default:
		{
			diagnostics.report_error(location, gmt::concat(L"Symbol '", memberName, L"' is not a variable, parameter or field"));
			return nullptr;
		}
	}
}

StructSymbol* ExpressionBinder::analyze_identifier_expression(SyntaxToken identifier, const SyntaxNode* node, std::optional<const SyntaxNode*> nodeRef)
{
	TextLocation location = node->get_location();
	SymbolTable& table = get_symbol_table();
	DiagnosticsContext& diagnostics = get_diagnostics();
	std::wstring_view memberName = identifier.get_lexeme();
	StructSymbol* currentType = nullptr;

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

	SyntaxSymbol* symbol = get_current_scope()->lookup(memberName);
	if (symbol == nullptr)
	{
		// global standard types are visible from every scope, both for type
		// positions and for static context such as 'Task.Delay(...)'
		StructSymbol* genericCandidate = nullptr;
		for (SyntaxSymbol* member : table.get_root_namespace()->members)
		{
			if ((member == nullptr) || !member->is_type() || member->get_name() != memberName)
				continue;

			StructSymbol* type = static_cast<StructSymbol*>(member);
			if (type->type_parameters.empty())
			{
				symbol = member;
				break;
			}

			if (genericCandidate == nullptr)
				genericCandidate = type;
		}

		if ((symbol == nullptr) && !(genericCandidate == nullptr))
			symbol = genericCandidate;
	}

	if (symbol == nullptr)
	{
		if (is_ambiguous_name(memberName))
		{
			diagnostics.report_error(location, gmt::concat(L"Ambiguous reference: '", memberName, L"' is defined in multiple imported namespaces"));
			return nullptr;
		}

		StructSymbol* expectedType = unwrap_nullable(resolve_left_denotation());
		if (!(expectedType == nullptr) && expectedType->get_kind() == SyntaxKind::EnumDeclaration)
		{
			EnumSymbol* enumType = static_cast<EnumSymbol*>(expectedType);
			for (FieldSymbol* field : enumType->fields)
			{
				if (!(field == nullptr))
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

		if (symbol == nullptr)
		{
			diagnostics.report_error(location, gmt::concat(L"Symbol '", memberName, L"' not found in current scope"));
			return nullptr;
		}
	}

	if (!is_symbol_accessible(symbol, table.lookup_node(symbol), nodeRef.has_value() ? *nodeRef : nullptr))
		return nullptr;

	if (symbol->is_type())
	{
		if (nodeRef.has_value())
			m_staticContextExpressions.insert(gmt::as<const ExpressionSyntax>(*nodeRef));

		return static_cast<StructSymbol*>(symbol);
	}

	return resolve_symbol_type(symbol, identifier, nodeRef.has_value() ? gmt::as<const ExpressionSyntax>(*nodeRef) : nullptr, currentType, nodeRef, location);
}

void ExpressionBinder::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	std::optional<const IdentifierNameTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// type-position identifiers were resolved and bound by the type binder;
	// identifiers the binder itself bound (variables, parameters, fields)
	// resolve through the scope instead
	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*nodeRef);
	if (!(bound == nullptr) && bound->is_type())
	{
		m_staticContextExpressions.insert(*nodeRef);
		set_expression_type(*nodeRef, static_cast<StructSymbol*>(bound));
		return;
	}

	StructSymbol* type = analyze_identifier_expression(node->get_identifier(), node, nodeRef);
	set_expression_type(*nodeRef, type);
}

void ExpressionBinder::visit_qualified_name_type(const QualifiedNameTypeSyntax* node)
{
	std::optional<const QualifiedNameTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	// value-position qualified names resolve at their use site; only
	// type-position names carry a binding from the type binder
	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*nodeRef);
	if ((bound == nullptr) || !bound->is_type())
		return;

	m_staticContextExpressions.insert(*nodeRef);
	set_expression_type(*nodeRef, static_cast<StructSymbol*>(bound));
}

void ExpressionBinder::visit_generic_type(const GenericTypeSyntax* node)
{
	std::optional<const GenericTypeSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*nodeRef);
	if ((bound == nullptr) || !bound->is_type())
		return;

	m_staticContextExpressions.insert(*nodeRef);
	set_expression_type(*nodeRef, static_cast<StructSymbol*>(bound));
}

StructSymbol* ExpressionBinder::analyze_property_access_expression(SyntaxToken identifier, bool assignmentContext, const SyntaxNode* callSite, const ExpressionSyntax* previous, PropertySymbol* property, StructSymbol* currentType)
{
	DiagnosticsContext& diagnostics = get_diagnostics();
	std::wstring_view memberName = identifier.get_lexeme();
	TextLocation location = identifier.get_location();

	if ((property == nullptr) || (currentType == nullptr))
		return nullptr;

	if ((property->get_return_type() == nullptr))
	{
		diagnostics.report_error(location, gmt::concat(L"Property '", memberName, L"' type not resolved"));
		return nullptr;
	}

	bool requiresSetter = assignmentContext;
	AccessorSymbol* accessor = requiresSetter ? property->get_setter() : property->get_getter();

	if (accessor == nullptr)
	{
		diagnostics.report_error(location, gmt::concat(L"Property '", memberName, L"' does not have a ", requiresSetter ? L"set" : L"get", L" accessor"));
		return nullptr;
	}

	if (!is_symbol_accessible(accessor, get_symbol_table().lookup_node(accessor), callSite))
		return nullptr;

	bool staticContext = get_is_static_context(previous);
	if (staticContext && accessor->get_linking() == LINK_INSTANCE)
	{
		diagnostics.report_error(location, gmt::concat(L"Cannot access instance property '", memberName, L"' from type context"));
		return nullptr;
	}

	if (!staticContext && accessor->get_linking() == LINK_STATIC)
	{
		diagnostics.report_error(location, gmt::concat(L"Cannot access static property '", memberName, L"' from instance reference"));
		return nullptr;
	}

	StructSymbol* propertyType = property->get_return_type();

	if (currentType->get_kind() == SyntaxKind::GenericType && !(propertyType == nullptr))
		propertyType = substitute_type_parameters(propertyType, static_cast<GenericTypeSymbol*>(currentType));

	return propertyType;
}

StructSymbol* ExpressionBinder::analyze_field_keyword_expression(const SyntaxNode* node)
{
	PropertySymbol* propertySymbol = nullptr;
	for (SemanticScope* scope = get_current_scope(); scope != nullptr; scope = scope->get_parent())
	{
		SyntaxSymbol* owner = scope->get_owner();
		if (owner == nullptr)
			continue;

		if (owner->is_type())
			break;

		if (owner->get_kind() != SyntaxKind::PropertyDeclaration)
			continue;

		propertySymbol = static_cast<PropertySymbol*>(owner);
		break;
	}

	if (propertySymbol == nullptr)
	{
		get_diagnostics().report_error(node->get_location(), L"Keyword 'field' can only be used in property accessors");
		return nullptr;
	}

	if ((propertySymbol->get_backing_field() == nullptr))
		propertySymbol->set_backing_field(m_factory.create_backing_field(propertySymbol, propertySymbol));

	FieldSymbol* backingField = propertySymbol->get_backing_field();
	if ((backingField->get_return_type() == nullptr))
	{
		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Backing field type not resolved for property '", propertySymbol->get_name(), L"'"));
		return nullptr;
	}

	std::optional<const SyntaxNode*> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		get_symbol_table().bind_symbol<FieldSymbol>(*nodeRef, backingField);

	if (propertySymbol->get_linking() == LINK_STATIC)
		m_staticContextExpressions.insert(gmt::as<const ExpressionSyntax>(*nodeRef));

	return backingField->get_return_type();
}

StructSymbol* ExpressionBinder::analyze_indexator_expression(const IndexatorExpressionSyntax* node, StructSymbol* currentType)
{
	const ExpressionSyntax* previous = node->get_previous();
	const ArgumentsListSyntax* arguments = node->get_arguments();

	if ((currentType == nullptr) || (previous == nullptr))
		return nullptr;

	currentType = unwrap_nullable(currentType);

	// arrays expose an implicit element indexator
	if (currentType->get_kind() == SyntaxKind::ArrayType)
	{
		StructSymbol* elementType = static_cast<ArrayTypeSymbol*>(currentType)->get_underlaying_type();

		if (!(arguments == nullptr))
		{
			for (ArgumentSyntax* const& argument : arguments->get_arguments())
			{
				visit_expression(argument->get_expression());
				StructSymbol* argumentType = get_expression_type(argument->get_expression());
				if (!(argumentType == nullptr) && argumentType != get_symbol_table().get_integer_primitive())
					get_diagnostics().report_error(node->get_location(), L"Array index must be an integer");
			}
		}

		return elementType;
	}

	IndexatorSymbol* indexator = resolve_indexator(node, currentType);
	if (indexator == nullptr)
		return nullptr;

	std::optional<const IndexatorExpressionSyntax*> nodeRef = ref_from(node);
	StructSymbol* resultType = analyze_property_access_expression(node->get_delimeter_token(), is_assignment_context(node), nodeRef.has_value() ? *nodeRef : nullptr, previous, indexator, currentType);

	if (resultType == nullptr)
		return nullptr;

	AccessorSymbol* accessor = is_assignment_context(node) ? indexator->get_setter() : indexator->get_getter();
	if (!(accessor == nullptr) && !(arguments == nullptr))
	{
		GenericTypeSymbol* genericType = nullptr;
		if (currentType->get_kind() == SyntaxKind::GenericType)
			genericType = static_cast<GenericTypeSymbol*>(currentType);

		if (!match_method_arguments(node->get_delimeter_token(), indexator->get_parameters(), arguments->get_arguments(), genericType))
			return nullptr;
	}

	if (nodeRef.has_value())
		get_symbol_table().bind_symbol<IndexatorSymbol>(*nodeRef, indexator);

	return resultType;
}

IndexatorSymbol* ExpressionBinder::resolve_indexator(const IndexatorExpressionSyntax* node, StructSymbol* currentType)
{
	TextLocation location = node->get_location();
	DiagnosticsContext& diagnostics = get_diagnostics();

	gmt::inline_vector<StructSymbol*, 4> argumentTypes;
	if (!(node->get_arguments() == nullptr))
	{
		for (ArgumentSyntax* const& argument : node->get_arguments()->get_arguments())
		{
			visit_expression(argument->get_expression());
			StructSymbol* argumentType = get_expression_type(argument->get_expression());
			if (argumentType == nullptr)
				return nullptr;

			argumentTypes.push_back(argumentType);
		}
	}

	StructSymbol* searchType = search_type_of(get_symbol_table(), currentType);
	IndexatorSymbol* selected = nullptr;
	int selectedScore = 0;

	for (IndexatorSymbol* candidate : searchType->indexators)
	{
		if (candidate == nullptr)
			continue;

		const gmt::vector<ParameterSymbol*>& parameters = candidate->get_parameters();
		if (parameters.size() != argumentTypes.get_vector().size())
			continue;

		GenericTypeSymbol* genericType = nullptr;
		if (currentType->get_kind() == SyntaxKind::GenericType)
			genericType = static_cast<GenericTypeSymbol*>(currentType);

		bool convertible = true;
		int totalScore = 0;
		for (std::size_t i = 0; i < parameters.size(); i++)
		{
			StructSymbol* parameterType = substitute_type_parameters(parameters[i]->get_type(), genericType);
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

		if ((selected == nullptr) || totalScore < selectedScore)
		{
			selected = candidate;
			selectedScore = totalScore;
		}
		else if (totalScore == selectedScore)
		{
			selected = nullptr; // ambiguous
		}
	}

	if (selected == nullptr)
	{
		diagnostics.report_error(location, gmt::concat(L"No indexators for type '", currentType->get_name(), L"' found that accept the supplied arguments"));
		return nullptr;
	}

	bool staticContext = get_is_static_context(node->get_previous());
	if (staticContext && selected->get_linking() == LINK_INSTANCE)
	{
		diagnostics.report_error(location, L"Cannot call instance indexator from type context");
		return nullptr;
	}

	if (!staticContext && selected->get_linking() == LINK_STATIC)
	{
		diagnostics.report_error(location, L"Cannot call static indexator on instance reference");
		return nullptr;
	}

	return selected;
}

void ExpressionBinder::visit_member_access_expression(const MemberAccessExpressionSyntax* node)
{
	const ExpressionSyntax* previous = node->get_previous();
	if (!(previous == nullptr))
		visit_expression(previous);

	StructSymbol* currentType = (previous == nullptr) ? get_owner_type() : get_expression_type(previous);
	StructSymbol* type = analyze_member_access_expression(node, currentType);

	std::optional<const MemberAccessExpressionSyntax*> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}

void ExpressionBinder::visit_indexator_expression(const IndexatorExpressionSyntax* node)
{
	const ExpressionSyntax* previous = node->get_previous();
	if (!(previous == nullptr))
		visit_expression(previous);

	StructSymbol* currentType = (previous == nullptr) ? get_owner_type() : get_expression_type(previous);
	StructSymbol* type = analyze_indexator_expression(node, currentType);

	std::optional<const IndexatorExpressionSyntax*> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}
