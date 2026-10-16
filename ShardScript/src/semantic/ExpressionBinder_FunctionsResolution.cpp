#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/lexical/SyntaxFacts.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Concat.hpp>

#include <optional>

using namespace shard;

namespace
{
	// lower is better; negative means no conversion
	int numeric_rank(gmt::Ref<const StructSymbol> type, const SymbolTable& table)
	{
		if (type == table.get_byte_primitive() || type == table.get_char_primitive() || type == table.get_nint_primitive())
			return 0;

		if (type == table.get_integer_primitive())
			return 1;

		if (type == table.get_double_primitive())
			return 2;

		return -1;
	}

	bool is_reference_like(gmt::Ref<const StructSymbol> type)
	{
		if (type.is_null())
			return false;

		switch (type->get_kind())
		{
			case SyntaxKind::ArrayType:
			case SyntaxKind::DelegateType:
			case SyntaxKind::InterfaceDeclaration:
			case SyntaxKind::NullableType:
				return true;

			case SyntaxKind::GenericType:
			{
				gmt::Ref<StructSymbol> underlaying = type.as<GenericTypeSymbol>()->get_underlaying_type();
				return underlaying->get_kind() == SyntaxKind::InterfaceDeclaration;
			}

			default:
				return false;
		}
	}

	gmt::Ref<DelegateTypeSymbol> get_delegate_type(gmt::Ref<const StructSymbol> type)
	{
		if (type.is_null())
			return gmt::nullref;

		if (type->get_kind() == SyntaxKind::DelegateType)
			return type.as<DelegateTypeSymbol>();

		if (type->get_kind() == SyntaxKind::GenericType)
		{
			gmt::Ref<StructSymbol> underlaying = type.as<GenericTypeSymbol>()->get_underlaying_type();
			if (!underlaying.is_null() && underlaying->get_kind() == SyntaxKind::DelegateType)
				return underlaying.as<DelegateTypeSymbol>();
		}

		return gmt::nullref;
	}

	std::wstring format_argument_list(std::span<const gmt::Ref<StructSymbol>> argTypes, SemanticModel& model)
	{
		switch (argTypes.size())
		{
			case 0:
				return L"no arguments";

			case 1:
				return gmt::concat(L"argument (", argTypes[0].is_null() ? L"<error>" : model.get_type_display_name(argTypes[0]), L")");

			default:
			{
				std::wstring result = gmt::concat(L"arguments (", model.get_type_display_name(argTypes[0]));
				for (std::size_t i = 1; i < argTypes.size(); i++)
					result += gmt::concat(L", ", model.get_type_display_name(argTypes[i]));

				result += L")";
				return result;
			}
		}
	}
}

int ExpressionBinder::conversion_score(gmt::Ref<const StructSymbol> parameter, gmt::Ref<const StructSymbol> argument)
{
	if (parameter.is_null())
		return -1;

	// null literal assignment
	if (argument.is_null())
		return is_reference_like(parameter) ? 1 : -1;

	if (SemanticModel::are_types_equal(parameter, argument))
		return 0;

	const SymbolTable& table = get_symbol_table();

	bool parameterNullable = parameter->get_kind() == SyntaxKind::NullableType;
	bool argumentNullable = argument->get_kind() == SyntaxKind::NullableType;

	if (parameterNullable && !argumentNullable)
	{
		gmt::Ref<StructSymbol> underlaying = parameter.as<NullableTypeSymbol>()->get_underlaying_type();
		if (SemanticModel::are_types_equal(underlaying, argument))
			return 2;
	}

	if (!parameterNullable && argumentNullable)
	{
		gmt::Ref<StructSymbol> underlaying = argument.as<NullableTypeSymbol>()->get_underlaying_type();
		if (SemanticModel::are_types_equal(parameter, underlaying))
			return 3;
	}

	int argumentRank = numeric_rank(argument, table);
	int parameterRank = numeric_rank(parameter, table);
	if (argumentRank >= 0 && parameterRank >= argumentRank)
		return 4 + (parameterRank - argumentRank);

	if (get_model().is_assignable_to(parameter, argument))
		return 10;

	return -1;
}

void ExpressionBinder::report_no_matching_method(std::wstring_view methodName, SyntaxToken blameToken)
{
	get_diagnostics().report_error(blameToken, gmt::concat(L"No method named '", methodName, L"' exists in the current context"));
}

void ExpressionBinder::report_no_matching_overload(std::wstring_view methodName, std::span<const gmt::Ref<StructSymbol>> argTypes, SyntaxToken blameToken)
{
	get_diagnostics().report_error(blameToken, gmt::concat(L"Method '", methodName, L"' does not have an overload that accepts the supplied arguments (", format_argument_list(argTypes, get_model()), L")"));
}

bool ExpressionBinder::collect_argument_types(const InvokationExpressionSyntax* node, std::vector<gmt::Ref<StructSymbol>>& outArgTypes)
{
	if (node->get_arguments().is_null())
		return false;

	for (const gmt::Ref<ArgumentSyntax>& argument : node->get_arguments()->get_arguments().as_span())
	{
		if (argument.is_null() || argument->get_expression().is_null())
			return false;

		visit_expression(argument->get_expression());
		outArgTypes.push_back(get_expression_type(argument->get_expression()));
	}

	return true;
}

bool ExpressionBinder::match_method_arguments(SyntaxToken blameToken, std::span<const gmt::Ref<ParameterSymbol>> parameters, gmt::Span<const gmt::Ref<ArgumentSyntax>> arguments, gmt::Ref<GenericTypeSymbol> genericType, std::size_t parameterOffset)
{
	if (parameterOffset > parameters.size())
		parameterOffset = parameters.size();

	std::size_t expectedArguments = parameters.size() - parameterOffset;
	if (expectedArguments != arguments.as_span().size())
	{
		get_diagnostics().report_error(blameToken, gmt::concat(L"Method expects ", std::to_wstring(expectedArguments), L" arguments but got ", std::to_wstring(arguments.as_span().size())));
		return false;
	}

	for (std::size_t i = parameterOffset; i < parameters.size(); i++)
	{
		gmt::Ref<ParameterSymbol> parameter = parameters[i];
		gmt::Ref<ArgumentSyntax> argument = arguments.as_span()[i - parameterOffset];

		if (parameter.is_null() || argument.is_null() || argument->get_expression().is_null())
		{
			get_diagnostics().report_error(blameToken, gmt::concat(L"Invalid argument at position ", std::to_wstring(i - parameterOffset)));
			return false;
		}

		gmt::Ref<StructSymbol> argumentType = get_expression_type(argument->get_expression());
		if (argumentType.is_null())
			return false;

		gmt::Ref<StructSymbol> parameterType = substitute_type_parameters(parameter->get_type(), genericType);
		if (parameterType.is_null())
			return false;

		if (conversion_score(parameterType, argumentType) < 0)
		{
			get_diagnostics().report_error(blameToken, gmt::concat(L"Argument type mismatch for parameter '", parameter->get_name(), L"': expected '", get_model().get_type_display_name(parameterType), L"' but got '", get_model().get_type_display_name(argumentType), L"'"));
			return false;
		}
	}

	return true;
}

bool ExpressionBinder::try_infer_type_argument(gmt::Ref<StructSymbol> pattern, gmt::Ref<StructSymbol> concrete, gmt::Ref<MethodSymbol> method, gmt::Ref<GenericTypeSymbol> genericType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs)
{
	if (pattern.is_null() || concrete.is_null())
		return true;

	if (!genericType.is_null() && pattern->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<StructSymbol> substituted = genericType->substitute_type_parameter(pattern.as<TypeParameterSymbol>());
		if (!substituted.is_null())
			pattern = substituted;
	}

	if (pattern->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<TypeParameterSymbol> typeParameter = pattern.as<TypeParameterSymbol>();
		if (typeParameter->get_parent() != method)
			return true;

		for (std::size_t i = 0; i < method->get_type_parameters().size(); i++)
		{
			if (method->get_type_parameters()[i] == typeParameter)
			{
				if (outMethodTypeArgs[i].is_null())
					outMethodTypeArgs[i] = concrete;
				else if (!SemanticModel::are_types_equal(outMethodTypeArgs[i], concrete))
					return false;

				return true;
			}
		}

		return true;
	}

	if (pattern->get_kind() == SyntaxKind::GenericType)
	{
		gmt::Ref<GenericTypeSymbol> patternGeneric = pattern.as<GenericTypeSymbol>();
		gmt::Ref<StructSymbol> patternUnderlaying = patternGeneric->get_underlaying_type();

		if (concrete->get_kind() == SyntaxKind::GenericType)
		{
			gmt::Ref<GenericTypeSymbol> concreteGeneric = concrete.as<GenericTypeSymbol>();
			if (concreteGeneric->get_underlaying_type() == patternUnderlaying)
			{
				for (gmt::Ref<TypeParameterSymbol> parameter : patternUnderlaying->type_parameters)
				{
					gmt::Ref<StructSymbol> patternArgument = patternGeneric->substitute_type_parameter(parameter);
					gmt::Ref<StructSymbol> concreteArgument = concreteGeneric->substitute_type_parameter(parameter);
					if (!try_infer_type_argument(patternArgument, concreteArgument, method, genericType, outMethodTypeArgs))
						return false;
				}
			}

			return true;
		}

		if (concrete->get_kind() == SyntaxKind::StructDeclaration || concrete->get_kind() == SyntaxKind::InterfaceDeclaration || concrete->get_kind() == SyntaxKind::EnumDeclaration)
		{
			for (gmt::Ref<StructSymbol> interfaceType : concrete->interfaces)
			{
				if (interfaceType.is_null() || interfaceType->get_kind() != SyntaxKind::GenericType)
					continue;

				gmt::Ref<GenericTypeSymbol> interfaceGeneric = interfaceType.as<GenericTypeSymbol>();
				if (interfaceGeneric->get_underlaying_type() != patternUnderlaying)
					continue;

				for (gmt::Ref<TypeParameterSymbol> parameter : patternUnderlaying->type_parameters)
				{
					gmt::Ref<StructSymbol> patternArgument = patternGeneric->substitute_type_parameter(parameter);
					gmt::Ref<StructSymbol> concreteArgument = interfaceGeneric->substitute_type_parameter(parameter);
					if (!try_infer_type_argument(patternArgument, concreteArgument, method, genericType, outMethodTypeArgs))
						return false;
				}

				return true;
			}
		}

		if (patternUnderlaying->get_kind() == SyntaxKind::DelegateType && concrete->get_kind() == SyntaxKind::DelegateType)
		{
			gmt::Ref<DelegateTypeSymbol> patternDelegate = patternUnderlaying.as<DelegateTypeSymbol>();
			gmt::Ref<DelegateTypeSymbol> concreteDelegate = concrete.as<DelegateTypeSymbol>();

			if (!patternDelegate->get_return_type().is_null() && !concreteDelegate->get_return_type().is_null())
			{
				gmt::Ref<StructSymbol> returnPattern = substitute_type_parameters(patternDelegate->get_return_type(), patternGeneric);
				if (!try_infer_type_argument(returnPattern, concreteDelegate->get_return_type(), method, genericType, outMethodTypeArgs))
					return false;
			}

			if (patternDelegate->get_parameters().size() == concreteDelegate->get_parameters().size())
			{
				for (std::size_t i = 0; i < patternDelegate->get_parameters().size(); ++i)
				{
					gmt::Ref<StructSymbol> parameterPattern = substitute_type_parameters(patternDelegate->get_parameters()[i]->get_type(), patternGeneric);
					if (!try_infer_type_argument(parameterPattern, concreteDelegate->get_parameters()[i]->get_type(), method, genericType, outMethodTypeArgs))
						return false;
				}
			}

			return true;
		}
	}

	if (pattern->get_kind() == SyntaxKind::ArrayType && concrete->get_kind() == SyntaxKind::ArrayType)
	{
		return try_infer_type_argument(pattern.as<ArrayTypeSymbol>()->get_underlaying_type(), concrete.as<ArrayTypeSymbol>()->get_underlaying_type(), method, genericType, outMethodTypeArgs);
	}

	return true;
}

bool ExpressionBinder::infer_method_type_arguments(gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, gmt::Ref<GenericTypeSymbol> genericType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs)
{
	outMethodTypeArgs.assign(method->get_type_parameters().size(), gmt::nullref);

	if (argTypes.size() != method->get_parameters().size())
		return false;

	for (std::size_t i = 0; i < method->get_parameters().size(); ++i)
	{
		gmt::Ref<StructSymbol> parameterType = method->get_parameters()[i]->get_type();
		if (!try_infer_type_argument(parameterType, argTypes[i], method, genericType, outMethodTypeArgs))
			return false;
	}

	for (gmt::Ref<StructSymbol> type : outMethodTypeArgs)
	{
		if (type.is_null())
			return false;
	}

	return true;
}

namespace
{
	bool try_infer_class_type_argument(gmt::Ref<StructSymbol> pattern, gmt::Ref<StructSymbol> concrete, gmt::Ref<StructSymbol> classDef, std::vector<gmt::Ref<StructSymbol>>& outArgs)
	{
		if (pattern.is_null() || concrete.is_null())
			return true;

		if (pattern->get_kind() == SyntaxKind::TypeParameter)
		{
			gmt::Ref<TypeParameterSymbol> typeParameter = pattern.as<TypeParameterSymbol>();
			if (typeParameter->get_type_argument_index() < outArgs.size() && classDef->type_parameters[typeParameter->get_type_argument_index()] == typeParameter)
			{
				gmt::Ref<StructSymbol>& slot = outArgs[typeParameter->get_type_argument_index()];
				if (slot.is_null())
					slot = concrete;
				else if (!SemanticModel::are_types_equal(slot, concrete))
					return false;
			}

			return true;
		}

		if (pattern->get_kind() == SyntaxKind::ArrayType)
		{
			if (concrete->get_kind() != SyntaxKind::ArrayType)
				return false;

			return try_infer_class_type_argument(pattern.as<ArrayTypeSymbol>()->get_underlaying_type(), concrete.as<ArrayTypeSymbol>()->get_underlaying_type(), classDef, outArgs);
		}

		if (pattern->get_kind() == SyntaxKind::GenericType)
		{
			if (concrete->get_kind() != SyntaxKind::GenericType)
				return false;

			gmt::Ref<GenericTypeSymbol> patternGeneric = pattern.as<GenericTypeSymbol>();
			gmt::Ref<GenericTypeSymbol> concreteGeneric = concrete.as<GenericTypeSymbol>();
			if (patternGeneric->get_underlaying_type() != concreteGeneric->get_underlaying_type())
				return false;

			for (gmt::Ref<TypeParameterSymbol> parameter : patternGeneric->get_underlaying_type()->type_parameters)
			{
				gmt::Ref<StructSymbol> patternArgument = patternGeneric->substitute_type_parameter(parameter);
				gmt::Ref<StructSymbol> concreteArgument = concreteGeneric->substitute_type_parameter(parameter);
				if (!try_infer_class_type_argument(patternArgument, concreteArgument, classDef, outArgs))
					return false;
			}

			return true;
		}

		return true;
	}
}

bool ExpressionBinder::infer_class_type_arguments(gmt::Ref<StructSymbol> classDef, gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, std::vector<gmt::Ref<StructSymbol>>& outClassTypeArgs)
{
	outClassTypeArgs.assign(classDef->type_parameters.size(), gmt::nullref);

	if (argTypes.size() != method->get_parameters().size())
		return false;

	for (std::size_t i = 0; i < method->get_parameters().size(); ++i)
	{
		if (!try_infer_class_type_argument(method->get_parameters()[i]->get_type(), argTypes[i], classDef, outClassTypeArgs))
			return false;
	}

	for (gmt::Ref<StructSymbol> type : outClassTypeArgs)
	{
		if (type.is_null())
			return false;
	}

	return true;
}


int ExpressionBinder::score_method_match(gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, gmt::Ref<GenericTypeSymbol> genericType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs)
{
	const std::vector<gmt::Ref<ParameterSymbol>>& parameters = method->get_parameters();
	bool generic = !method->get_type_parameters().empty();

	if (generic)
	{
		if (!infer_method_type_arguments(method, argTypes, genericType, outMethodTypeArgs))
			return -1;
	}
	else
	{
		outMethodTypeArgs.clear();
		if (argTypes.size() != parameters.size())
			return -1;
	}

	int total = 0;
	for (std::size_t i = 0; i < argTypes.size(); i++)
	{
		gmt::Ref<StructSymbol> parameterType = generic
			? substitute_type_parameters(parameters[i]->get_type(), genericType, method, outMethodTypeArgs)
			: substitute_type_parameters(parameters[i]->get_type(), genericType);

		int score = conversion_score(parameterType, argTypes[i]);
		if (score < 0)
			return -1;

		total += score;
	}

	return total;
}

gmt::Ref<DelegateTypeSymbol> ExpressionBinder::try_resolve_delegate_invocation(const InvokationExpressionSyntax* node, gmt::Ref<SyntaxSymbol> lookup)
{
	if (lookup.is_null())
		return gmt::nullref;

	gmt::Ref<StructSymbol> delegateHolderType = gmt::nullref;
	switch (lookup->get_kind())
	{
		case SyntaxKind::VariableStatement:
			delegateHolderType = lookup.as<VariableSymbol>()->get_type();
			break;

		case SyntaxKind::Parameter:
			delegateHolderType = lookup.as<ParameterSymbol>()->get_type();
			break;

		case SyntaxKind::FieldDeclaration:
			delegateHolderType = lookup.as<FieldSymbol>()->get_return_type();
			break;

		default:
			return gmt::nullref;
	}

	return get_delegate_type(delegateHolderType);
}

gmt::Ref<MethodSymbol> ExpressionBinder::resolve_method(const InvokationExpressionSyntax* node, gmt::Ref<StructSymbol> currentType, std::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs, gmt::Ref<GenericTypeSymbol>& outGenericType)
{
	std::wstring_view methodName = node->get_identifier().get_lexeme();

	if (node->get_arguments().is_null())
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' invocation has no arguments list"));
		return gmt::nullref;
	}

	std::vector<gmt::Ref<StructSymbol>> argTypes;
	if (!collect_argument_types(node, argTypes))
		return gmt::nullref;

	bool staticContext = get_is_static_context(node->get_previous());
	if (currentType.is_null() && is_ambiguous_name(methodName))
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Ambiguous reference: '", methodName, L"' is defined in multiple imported namespaces"));
		return gmt::nullref;
	}

	gmt::Ref<GenericTypeSymbol> genericType = gmt::nullref;
	if (!currentType.is_null() && currentType->get_kind() == SyntaxKind::GenericType)
		genericType = currentType.as<GenericTypeSymbol>();
	outGenericType = gmt::nullref;

	// extension shape carries the receiver type as the first argument
	std::vector<gmt::Ref<StructSymbol>> extensionArgs;
	if (!currentType.is_null() && !staticContext)
	{
		extensionArgs.reserve(argTypes.size() + 1);
		extensionArgs.push_back(currentType);
		extensionArgs.insert(extensionArgs.end(), argTypes.begin(), argTypes.end());
	}

	struct candidate
	{
		gmt::Ref<MethodSymbol> method;
		bool as_extension;
	};

	std::vector<candidate> candidates;
	auto addCandidate = [&candidates](gmt::Ref<MethodSymbol> method, bool asExtension)
	{
		for (const candidate& existing : candidates)
		{
			if (existing.method == method)
				return;
		}

		candidates.push_back({ method, asExtension });
	};

	// instance and static methods declared on the receiver type
	gmt::Ref<StructSymbol> receiverType = currentType;
	if (!receiverType.is_null() && receiverType->get_kind() == SyntaxKind::GenericType)
		receiverType = receiverType.as<GenericTypeSymbol>()->get_underlaying_type();

	if (!receiverType.is_null())
	{
		if (receiverType->get_kind() == SyntaxKind::TypeParameter)
		{
			for (gmt::Ref<StructSymbol> constraint : receiverType.as<TypeParameterSymbol>()->get_constraints())
			{
				if (constraint.is_null())
					continue;

				for (gmt::Ref<MethodSymbol> method : constraint->methods)
				{
					if (!method.is_null() && method->get_name() == methodName)
						addCandidate(method, false);
				}
			}
		}
		else
		{
			for (gmt::Ref<MethodSymbol> method : receiverType->methods)
			{
				if (!method.is_null() && method->get_name() == methodName)
					addCandidate(method, false);
			}
		}
	}

	// functions declared in the current scope: locals, top-level functions
	// and members of imported namespaces
	bool asExtension = !staticContext && !currentType.is_null();
	gmt::Ref<SyntaxSymbol> scopeLookup = get_current_scope()->lookup(methodName);
	if (!scopeLookup.is_null() && scopeLookup->get_kind() == SyntaxKind::FunctionDeclaration)
		addCandidate(scopeLookup.as<MethodSymbol>(), asExtension);

	for (const gmt::Ref<SyntaxSymbol>& overload : get_current_scope()->find_overloads(methodName))
	{
		if (!overload.is_null() && overload->get_kind() == SyntaxKind::FunctionDeclaration)
			addCandidate(overload.as<MethodSymbol>(), asExtension);
	}

	gmt::Ref<MethodSymbol> best = gmt::nullref;
	std::vector<gmt::Ref<StructSymbol>> bestTypeArgs;
	bool bestExtension = false;
	int bestScore = -1;
	bool ambiguous = false;

	for (const candidate& item : candidates)
	{
		std::span<const gmt::Ref<StructSymbol>> candidateArgs = item.as_extension
			? std::span<const gmt::Ref<StructSymbol>>(extensionArgs)
			: std::span<const gmt::Ref<StructSymbol>>(argTypes);

		std::vector<gmt::Ref<StructSymbol>> candidateTypeArgs;
		int score = score_method_match(item.method, candidateArgs, genericType, candidateTypeArgs);
		if (score < 0)
			continue;

		if (best.is_null() || score < bestScore)
		{
			best = item.method;
			bestScore = score;
			bestTypeArgs = std::move(candidateTypeArgs);
			bestExtension = item.as_extension;
			ambiguous = false;
		}
		else if (score == bestScore)
			ambiguous = true;
	}

	if (best.is_null())
	{
		// a delegate-backed variable, parameter or field is not a method;
		// the caller analyzes that shape separately, so stay silent here
		gmt::Ref<SyntaxSymbol> lookup = get_current_scope()->lookup(methodName);
		if (try_resolve_delegate_invocation(node, lookup).is_null())
		{
			if (candidates.empty())
				report_no_matching_method(methodName, node->get_identifier());
			else
				report_no_matching_overload(methodName, argTypes, node->get_identifier());
		}

		return gmt::nullref;
	}

	if (ambiguous)
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"The call to '", methodName, L"' is ambiguous between multiple overloads"));
		return gmt::nullref;
	}

	if (staticContext && best->get_linking() == LINK_INSTANCE)
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Cannot call instance method '", methodName, L"' from type context"));
		return gmt::nullref;
	}

	if (!staticContext && best->get_linking() == LINK_STATIC && !bestExtension)
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Cannot call static method '", methodName, L"' on instance reference"));
		return gmt::nullref;
	}

	outMethodTypeArgs = std::move(bestTypeArgs);
	outGenericType = genericType;
	return best;
}

gmt::Ref<ConstructorSymbol> ExpressionBinder::resolve_constructor(const ObjectExpressionSyntax* node)
{
	std::optional<gmt::Ref<const ObjectExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<StructSymbol> createdType = nodeRef.has_value() ? lookup_symbol<StructSymbol>(*nodeRef) : gmt::nullref;
	if (createdType.is_null())
		return gmt::nullref;

	gmt::Ref<StructSymbol> searchType = createdType;
	if (searchType->get_kind() == SyntaxKind::GenericType)
		searchType = searchType.as<GenericTypeSymbol>()->get_underlaying_type();

	if (node->get_arguments().is_null())
		return gmt::nullref;

	std::vector<gmt::Ref<StructSymbol>> argTypes;
	for (const gmt::Ref<ArgumentSyntax>& argument : node->get_arguments()->get_arguments().as_span())
	{
		if (argument.is_null() || argument->get_expression().is_null())
			return gmt::nullref;

		visit_expression(argument->get_expression());
		gmt::Ref<StructSymbol> argumentType = get_expression_type(argument->get_expression());
		if (argumentType.is_null())
			return gmt::nullref;

		argTypes.push_back(argumentType);
	}

	gmt::Ref<ConstructorSymbol> best = gmt::nullref;
	int bestScore = -1;

	for (gmt::Ref<ConstructorSymbol> constructor : searchType->constructors)
	{
		if (constructor.is_null())
			continue;

		const std::vector<gmt::Ref<ParameterSymbol>>& parameters = constructor->get_parameters();
		if (parameters.size() != argTypes.size())
			continue;

		int total = 0;
		bool matches = true;
		for (std::size_t i = 0; i < parameters.size(); i++)
		{
			int score = conversion_score(parameters[i]->get_type(), argTypes[i]);
			if (score < 0)
			{
				matches = false;
				break;
			}

			total += score;
		}

		if (!matches)
			continue;

		if (best.is_null() || total < bestScore)
		{
			best = constructor;
			bestScore = total;
		}
	}

	if (best.is_null())
	{
		get_diagnostics().report_error(node->get_new_token(), gmt::concat(L"No constructor of '", get_model().get_type_display_name(createdType), L"' accepts the supplied arguments (", format_argument_list(argTypes, get_model()), L")"));
		return gmt::nullref;
	}

	return best;
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_object_expression(const ObjectExpressionSyntax* node)
{
	std::optional<gmt::Ref<const ObjectExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<StructSymbol> createdType = nodeRef.has_value() ? lookup_symbol<StructSymbol>(*nodeRef) : gmt::nullref;
	if (createdType.is_null())
		return gmt::nullref;

	gmt::Ref<ConstructorSymbol> constructor = resolve_constructor(node);
	if (constructor.is_null())
		return createdType;

	if (!node->get_arguments().is_null())
	{
		gmt::Ref<GenericTypeSymbol> genericType = gmt::nullref;
		if (createdType->get_kind() == SyntaxKind::GenericType)
			genericType = createdType.as<GenericTypeSymbol>();

		match_method_arguments(node->get_new_token(), constructor->get_parameters(), node->get_arguments()->get_arguments(), genericType);
	}

	if (nodeRef.has_value())
		get_symbol_table().bind_symbol<ConstructorSymbol>(*nodeRef, constructor);

	return createdType;
}

void ExpressionBinder::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	std::optional<gmt::Ref<const ObjectExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	if (node->is_array_creation())
	{
		if (!node->get_array_size().is_null())
			visit_expression(node->get_array_size());

		gmt::Ref<StructSymbol> elementType = lookup_symbol<StructSymbol>(*nodeRef);
		if (elementType.is_null())
			elementType = get_symbol_table().get_void_primitive();

		if (!node->get_array_size().is_null())
		{
			gmt::Ref<StructSymbol> sizeType = get_expression_type(node->get_array_size());
			bool sizeIsInteger = !sizeType.is_null() && sizeType == get_symbol_table().get_integer_primitive();
			if (!sizeIsInteger)
				get_diagnostics().report_error(node->get_new_token(), L"Array size must be an integer");
		}

		gmt::Ref<ArrayTypeSymbol> arrayType = m_factory.create_array_type(elementType);
		get_symbol_table().bind_symbol<ArrayTypeSymbol>(*nodeRef, arrayType);
		set_expression_type(*nodeRef, arrayType);
		return;
	}

	gmt::Ref<StructSymbol> type = analyze_object_expression(node);
	set_expression_type(*nodeRef, type);
}

gmt::Ref<NamespaceSymbol> ExpressionBinder::resolve_namespace_qualifier(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null())
		return gmt::nullref;

	if (expression->get_kind() != SyntaxKind::MemberAccessExpression)
		return gmt::nullref;

	gmt::Ref<const MemberAccessExpressionSyntax> member = expression.as<MemberAccessExpressionSyntax>();
	if (member->get_previous().is_null())
	{
		gmt::Ref<SyntaxSymbol> symbol = get_current_scope()->lookup(member->get_identifier().get_lexeme());
		if (!symbol.is_null() && symbol->get_kind() == SyntaxKind::NamespaceDirective)
			return symbol.as<NamespaceSymbol>();

		return get_symbol_table().get_root_namespace()->lookup(member->get_identifier().get_lexeme());
	}

	gmt::Ref<NamespaceSymbol> parentNamespace = resolve_namespace_qualifier(member->get_previous());
	if (parentNamespace.is_null())
		return gmt::nullref;

	return parentNamespace->lookup(member->get_identifier().get_lexeme());
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_qualified_invokation_expression(const InvokationExpressionSyntax* node, gmt::Ref<NamespaceSymbol> namespaceSymbol)
{
	std::wstring_view methodName = node->get_identifier().get_lexeme();

	if (node->get_arguments().is_null())
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' invocation has no arguments list"));
		return gmt::nullref;
	}

	std::vector<gmt::Ref<StructSymbol>> argTypes;
	if (!collect_argument_types(node, argTypes))
		return gmt::nullref;

	gmt::Ref<MethodSymbol> best = gmt::nullref;
	std::vector<gmt::Ref<StructSymbol>> bestTypeArgs;
	int bestScore = -1;

	for (gmt::Ref<SyntaxSymbol> member : namespaceSymbol->members)
	{
		if (member.is_null() || member->get_kind() != SyntaxKind::FunctionDeclaration)
			continue;

		gmt::Ref<MethodSymbol> method = member.as<MethodSymbol>();
		if (method->get_name() != methodName)
			continue;

		if (method->get_linking() != LINK_STATIC)
			continue;

		std::vector<gmt::Ref<StructSymbol>> candidateTypeArgs;
		int score = score_method_match(method, argTypes, gmt::nullref, candidateTypeArgs);
		if (score < 0)
			continue;

		if (best.is_null() || score < bestScore)
		{
			best = method;
			bestScore = score;
			bestTypeArgs = std::move(candidateTypeArgs);
		}
	}

	if (best.is_null())
	{
		for (gmt::Ref<SyntaxSymbol> member : namespaceSymbol->members)
		{
			if (!member.is_null() && member->get_name() == methodName && member->is_type())
			{
				get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Use 'new ", methodName, L"()' instead of '", methodName, L"()' to construct a type"));
				return gmt::nullref;
			}
		}

		if (!namespaceSymbol->lookup(methodName).is_null())
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Cannot call namespace '", methodName, L"'; use '", methodName, L".Member()' to access its members"));
			return gmt::nullref;
		}

		report_no_matching_method(methodName, node->get_identifier());
		return gmt::nullref;
	}

	std::optional<gmt::Ref<const InvokationExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
	{
		gmt::Ref<const SyntaxNode> declaringNode = get_symbol_table().lookup_node(best);
		if (!is_symbol_accessible(best, declaringNode, *nodeRef))
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' is not accessible"));
			return gmt::nullref;
		}

		get_symbol_table().bind_symbol<MethodSymbol>(*nodeRef, best);
	}

	if (best->get_return_type().is_null())
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' return type not resolved"));
		return gmt::nullref;
	}

	gmt::Ref<StructSymbol> type = substitute_type_parameters(best->get_return_type(), gmt::nullref, best, bestTypeArgs);
	return wrap_async_return_type(best, type);
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_invokation_expression(const InvokationExpressionSyntax* node, gmt::Ref<StructSymbol> currentType)
{
	std::wstring_view methodName = node->get_identifier().get_lexeme();
	std::vector<gmt::Ref<StructSymbol>> methodTypeArgs;
	gmt::Ref<GenericTypeSymbol> genericType = gmt::nullref;
	gmt::Ref<MethodSymbol> method = resolve_method(node, currentType, methodTypeArgs, genericType);
	std::optional<gmt::Ref<const InvokationExpressionSyntax>> nodeRef = ref_from(node);

	if (method.is_null())
	{
		// the callee may be a delegate-backed variable, parameter or field
		gmt::Ref<SyntaxSymbol> lookup = get_current_scope()->lookup(methodName);
		gmt::Ref<DelegateTypeSymbol> delegate = try_resolve_delegate_invocation(node, lookup);
		if (delegate.is_null())
			return gmt::nullref;

		if (!node->get_arguments().is_null())
			match_method_arguments(node->get_delimeter_token(), delegate->get_parameters(), node->get_arguments()->get_arguments(), gmt::nullref);

		if (nodeRef.has_value())
		{
			get_symbol_table().bind_symbol<DelegateTypeSymbol>(*nodeRef, delegate);
			set_expression_type(*nodeRef, delegate->get_return_type());
		}

		return delegate->get_return_type();
	}

	if (nodeRef.has_value())
	{
		gmt::Ref<const SyntaxNode> declaringNode = get_symbol_table().lookup_node(method);
		if (!is_symbol_accessible(method, declaringNode, *nodeRef))
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' is not accessible"));
			return gmt::nullref;
		}

		get_symbol_table().bind_symbol<MethodSymbol>(*nodeRef, method);
	}

	if (method->get_return_type().is_null())
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' return type not resolved"));
		return gmt::nullref;
	}

	gmt::Ref<StructSymbol> type = substitute_type_parameters(method->get_return_type(), genericType, method, methodTypeArgs);
	return wrap_async_return_type(method, type);
}

gmt::Ref<StructSymbol> ExpressionBinder::wrap_async_return_type(gmt::Ref<MethodSymbol> method, gmt::Ref<StructSymbol> type)
{
	if (method.is_null() || !method->get_is_async() || type.is_null())
		return type;

	SymbolTable& table = get_symbol_table();

	// async methods declare their plain result type; the Task wrapper is
	// implied for invokations and materialized by the hoisting pass
	if (type == table.get_void_primitive())
	{
		gmt::Ref<SyntaxSymbol> task = table.get_standard_type(StandardTypes::Task);
		return task->get_kind() == SyntaxKind::StructDeclaration ? task.as<StructSymbol>() : type;
	}

	gmt::Ref<SyntaxSymbol> valueTask = table.get_standard_type(StandardTypes::ValueTask);
	if (valueTask->get_kind() != SyntaxKind::StructDeclaration)
		return type;

	return table.find_or_create_generic_type(valueTask.as<StructSymbol>(), { type });
}

void ExpressionBinder::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	std::optional<gmt::Ref<const InvokationExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<const ExpressionSyntax> previous = node->get_previous();

	// call of a direct callee expression: (callee)(arguments)
	if (node->get_identifier().get_type() == TokenType::Unknown && !previous.is_null())
	{
		visit_expression(previous);
		gmt::Ref<StructSymbol> calleeType = get_expression_type(previous);
		gmt::Ref<DelegateTypeSymbol> delegate = get_delegate_type(calleeType);

		if (delegate.is_null())
		{
			std::wstring calleeName = calleeType.is_null() ? L"<error>" : get_model().get_type_display_name(calleeType);
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Cannot invoke expression of type '", calleeName, L"'"));

			if (nodeRef.has_value())
				set_expression_type(*nodeRef, gmt::nullref);

			return;
		}

		if (!node->get_arguments().is_null())
			match_method_arguments(node->get_delimeter_token(), delegate->get_parameters(), node->get_arguments()->get_arguments(), gmt::nullref);

		if (nodeRef.has_value())
		{
			get_symbol_table().bind_symbol<DelegateTypeSymbol>(*nodeRef, delegate);
			set_expression_type(*nodeRef, delegate->get_return_type());
		}

		return;
	}

	// namespace-qualified invocation: Foo::Bar(); namespaces are not
	// expression values, so the qualifier chain resolves without visiting it
	gmt::Ref<NamespaceSymbol> namespaceQualifier = resolve_namespace_qualifier(previous);
	if (!namespaceQualifier.is_null())
	{
		gmt::Ref<StructSymbol> type = analyze_qualified_invokation_expression(node, namespaceQualifier);
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, type);

		return;
	}

	if (!previous.is_null())
		visit_expression(previous);

	gmt::Ref<StructSymbol> currentType = previous.is_null() ? get_owner_type() : get_expression_type(previous);
	gmt::Ref<StructSymbol> type = analyze_invokation_expression(node, currentType);

	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}
