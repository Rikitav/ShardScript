#include <shard/semantic/ExpressionBinder.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

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

	gmt::wstring format_argument_list(std::span<const gmt::Ref<StructSymbol>> argTypes, SemanticModel& model)
	{
		switch (argTypes.size())
		{
			case 0:
				return L"no arguments";

			case 1:
				return gmt::concat(L"argument (", argTypes[0].is_null() ? L"<error>" : model.get_type_display_name(argTypes[0]), L")");

			default:
			{
				gmt::wstring result = gmt::concat(L"arguments (", model.get_type_display_name(argTypes[0]));
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

bool ExpressionBinder::collect_argument_types(const InvokationExpressionSyntax* node, std::pmr::vector<gmt::Ref<StructSymbol>>& outArgTypes)
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

void ExpressionBinder::bind_pending_lambda_arguments(const InvokationExpressionSyntax* node, gmt::Ref<MethodSymbol> method, bool asExtension, gmt::Ref<GenericTypeSymbol> genericType, std::span<const gmt::Ref<StructSymbol>> candidateArgs, std::span<const gmt::Ref<StructSymbol>> explicitTypeArgs)
{
	if (m_pendingLambdas.empty() || node->get_arguments().is_null())
		return;

	std::span<const gmt::Ref<ArgumentSyntax>> arguments = node->get_arguments()->get_arguments().as_span();
	std::size_t parameterOffset = asExtension ? 1 : 0;

	bool hasPendingArgument = false;
	for (const gmt::Ref<ArgumentSyntax>& argument : arguments)
	{
		if (!argument.is_null() && is_pending_lambda(argument->get_expression()))
		{
			hasPendingArgument = true;
			break;
		}
	}

	if (!hasPendingArgument)
		return;

	const gmt::vector<gmt::Ref<ParameterSymbol>>& parameters = method->get_parameters();

	// partially infer method type arguments from the non-lambda arguments, so
	// delegate parameter types that mention them can resolve to concrete types
	bool generic = !method->get_type_parameters().empty();
	std::pmr::vector<gmt::Ref<StructSymbol>> partialTypeArgs;
	if (generic)
	{
		if (!explicitTypeArgs.empty())
		{
			if (explicitTypeArgs.size() != method->get_type_parameters().size())
				return;

			partialTypeArgs.assign(explicitTypeArgs.begin(), explicitTypeArgs.end());
		}
		else
		{
			partialTypeArgs.assign(method->get_type_parameters().size(), gmt::nullref);
			for (std::size_t i = 0; i < parameters.size() && i < candidateArgs.size(); i++)
			{
				// a position behind a pending lambda contributes nothing until
				// the lambda binds; an extension receiver has no syntax
				// argument and always contributes
				bool pendingPosition = i >= parameterOffset;
				pendingPosition = pendingPosition && (i - parameterOffset) < arguments.size();
				pendingPosition = pendingPosition && !arguments[i - parameterOffset].is_null();
				pendingPosition = pendingPosition && is_pending_lambda(arguments[i - parameterOffset]->get_expression());

				if (pendingPosition || candidateArgs[i].is_null())
					continue;

				try_infer_type_argument(parameters[i]->get_type(), candidateArgs[i], method, genericType, partialTypeArgs);
			}
		}
	}

	for (std::size_t i = 0; i < arguments.size() && i + parameterOffset < parameters.size(); i++)
	{
		const gmt::Ref<ArgumentSyntax>& argument = arguments[i];
		if (argument.is_null())
			continue;

		gmt::Ref<const ExpressionSyntax> expression = argument->get_expression();
		if (!is_pending_lambda(expression))
			continue;

		gmt::Ref<StructSymbol> expectedType = parameters[i + parameterOffset]->get_type();
		expectedType = generic
			? substitute_type_parameters(expectedType, genericType, method, partialTypeArgs)
			: substitute_type_parameters(expectedType, genericType);

		try_bind_pending_lambda(expression.as<const LambdaExpressionSyntax>(), expectedType);
	}
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

bool ExpressionBinder::try_infer_type_argument(gmt::Ref<StructSymbol> pattern, gmt::Ref<StructSymbol> concrete, gmt::Ref<MethodSymbol> method, gmt::Ref<GenericTypeSymbol> genericType, std::pmr::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs)
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

				return true;
			}

			// a constructed type that implements the pattern's generic
			// interface: the interface's arguments substitute through the
			// construction before pairwise inference
			for (gmt::Ref<StructSymbol> interfaceType : concreteGeneric->get_underlaying_type()->interfaces)
			{
				if (interfaceType.is_null() || interfaceType->get_kind() != SyntaxKind::GenericType)
					continue;

				gmt::Ref<GenericTypeSymbol> interfaceGeneric = interfaceType.as<GenericTypeSymbol>();
				if (interfaceGeneric->get_underlaying_type() != patternUnderlaying)
					continue;

				for (gmt::Ref<TypeParameterSymbol> parameter : patternUnderlaying->type_parameters)
				{
					gmt::Ref<StructSymbol> patternArgument = patternGeneric->substitute_type_parameter(parameter);
					gmt::Ref<StructSymbol> interfaceArgument = interfaceGeneric->substitute_type_parameter(parameter);
					gmt::Ref<StructSymbol> concreteArgument = substitute_type_parameters(interfaceArgument, concreteGeneric);
					if (!try_infer_type_argument(patternArgument, concreteArgument, method, genericType, outMethodTypeArgs))
						return false;
				}

				return true;
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

bool ExpressionBinder::infer_method_type_arguments(gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, gmt::Ref<GenericTypeSymbol> genericType, std::pmr::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs)
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
	bool try_infer_class_type_argument(gmt::Ref<StructSymbol> pattern, gmt::Ref<StructSymbol> concrete, gmt::Ref<StructSymbol> classDef, gmt::vector<gmt::Ref<StructSymbol>>& outArgs)
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

bool ExpressionBinder::infer_class_type_arguments(gmt::Ref<StructSymbol> classDef, gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, gmt::vector<gmt::Ref<StructSymbol>>& outClassTypeArgs)
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


int ExpressionBinder::score_method_match(gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> argTypes, gmt::Ref<GenericTypeSymbol> genericType, std::pmr::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs, std::span<const gmt::Ref<StructSymbol>> explicitTypeArgs)
{
	const gmt::vector<gmt::Ref<ParameterSymbol>>& parameters = method->get_parameters();
	bool generic = !method->get_type_parameters().empty();

	if (generic)
	{
		if (!explicitTypeArgs.empty())
		{
			if (explicitTypeArgs.size() != method->get_type_parameters().size())
				return -1;

			outMethodTypeArgs.assign(explicitTypeArgs.begin(), explicitTypeArgs.end());
			if (argTypes.size() != parameters.size())
				return -1;
		}
		else if (!infer_method_type_arguments(method, argTypes, genericType, outMethodTypeArgs))
		{
			return -1;
		}
	}
	else
	{
		if (!explicitTypeArgs.empty())
			return -1;

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

bool ExpressionBinder::check_method_type_constraints(gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> methodTypeArgs, SyntaxToken blameToken)
{
	const gmt::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = method->get_type_parameters();
	bool satisfied = true;

	for (std::size_t i = 0; i < typeParameters.size() && i < methodTypeArgs.size(); ++i)
	{
		gmt::Ref<TypeParameterSymbol> typeParameter = typeParameters[i];
		gmt::Ref<StructSymbol> argument = methodTypeArgs[i];
		if (argument.is_null())
			continue;

		for (const gmt::Ref<StructSymbol>& constraint : typeParameter->get_constraints())
		{
			if (constraint.is_null())
				continue;

			if (get_model().type_satisfies_constraint(argument, constraint))
				continue;

			get_diagnostics().report_error(blameToken, gmt::concat(L"Type argument '", argument->get_name(), L"' does not satisfy the constraint '", constraint->get_name(), L"' on type parameter '", typeParameter->get_name(), L"'"));
			satisfied = false;
		}
	}

	return satisfied;
}

bool ExpressionBinder::collect_explicit_type_arguments(const InvokationExpressionSyntax* node, std::pmr::vector<gmt::Ref<StructSymbol>>& outTypeArgs)
{
	outTypeArgs.clear();
	gmt::Ref<const TypeArgumentsListSyntax> list = node->get_type_arguments();
	if (list.is_null())
		return false;

	for (const gmt::Ref<TypeSyntax>& argumentNode : list->get_types().as_span())
	{
		gmt::Ref<StructSymbol> type = resolve_type_expression(argumentNode);
		if (!type.is_null())
			outTypeArgs.push_back(type);
	}

	return true;
}

void ExpressionBinder::bind_invocation_type_arguments(gmt::Ref<const InvokationExpressionSyntax> node, gmt::Ref<MethodSymbol> method, gmt::Ref<GenericTypeSymbol> genericType, std::span<const gmt::Ref<StructSymbol>> methodTypeArgs)
{
	bool genericReceiver = !genericType.is_null();
	bool genericMethod = !method->get_type_parameters().empty();
	if (!genericReceiver && !genericMethod)
		return;

	// combined index space: receiver type arguments first, then the method's
	// own, matching TypeParameterSymbol::get_type_argument_index
	gmt::inline_vector<gmt::Ref<StructSymbol>, 4> combined;
	if (genericReceiver)
	{
		for (gmt::Ref<TypeParameterSymbol> parameter : genericType->get_underlaying_type()->type_parameters)
			combined.push_back(genericType->substitute_type_parameter(parameter));
	}

	for (gmt::Ref<StructSymbol> argument : methodTypeArgs)
		combined.push_back(argument);

	get_symbol_table().bind_type_arguments(node, combined.get_vector());
}

gmt::Ref<DelegateTypeSymbol> ExpressionBinder::try_resolve_delegate_invocation(const InvokationExpressionSyntax* node, gmt::Ref<SyntaxSymbol> lookup, gmt::Ref<GenericTypeSymbol>& outInstantiation)
{
	outInstantiation = gmt::nullref;
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

	// a generic delegate instantiation substitutes the delegate's own type
	// parameters with the holder's type arguments
	if (!delegateHolderType.is_null() && delegateHolderType->get_kind() == SyntaxKind::GenericType)
		outInstantiation = delegateHolderType.as<GenericTypeSymbol>();

	return get_delegate_type(delegateHolderType);
}

gmt::Ref<MethodSymbol> ExpressionBinder::resolve_method(const InvokationExpressionSyntax* node, gmt::Ref<StructSymbol> currentType, std::pmr::vector<gmt::Ref<StructSymbol>>& outMethodTypeArgs, gmt::Ref<GenericTypeSymbol>& outGenericType)
{
	std::wstring_view methodName = node->get_identifier().get_lexeme();

	if (node->get_arguments().is_null())
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' invocation has no arguments list"));
		return gmt::nullref;
	}

	gmt::inline_vector<gmt::Ref<StructSymbol>, 8> argTypes;
	if (!collect_argument_types(node, argTypes.get_vector()))
		return gmt::nullref;

	gmt::inline_vector<gmt::Ref<StructSymbol>, 4> explicitTypeArgs;
	bool hasExplicitTypeArgs = collect_explicit_type_arguments(node, explicitTypeArgs.get_vector());

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
	gmt::inline_vector<gmt::Ref<StructSymbol>, 8> extensionArgs;
	if (!currentType.is_null() && !staticContext)
	{
		extensionArgs.get_vector().reserve(argTypes.get_vector().size() + 1);
		extensionArgs.push_back(currentType);
		extensionArgs.get_vector().insert(extensionArgs.get_vector().end(), argTypes.get_vector().begin(), argTypes.get_vector().end());
	}

	struct candidate
	{
		gmt::Ref<MethodSymbol> method;
		bool as_extension;
	};

	gmt::inline_vector<candidate, 8> candidates;
	auto addCandidate = [&candidates](gmt::Ref<MethodSymbol> method, bool asExtension)
	{
		for (const candidate& existing : candidates.get_vector())
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

	if (hasExplicitTypeArgs)
	{
		bool arityAccepted = false;
		for (const candidate& item : candidates.get_vector())
		{
			const gmt::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = item.method->get_type_parameters();
			if (!typeParameters.empty() && typeParameters.size() == explicitTypeArgs.get_vector().size())
			{
				arityAccepted = true;
				break;
			}
		}

		if (!arityAccepted)
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' does not accept ", std::to_wstring(explicitTypeArgs.get_vector().size()), L" type argument(s)"));
			return gmt::nullref;
		}
	}

	gmt::Ref<MethodSymbol> best = gmt::nullref;
	std::pmr::vector<gmt::Ref<StructSymbol>> bestTypeArgs;
	bool bestExtension = false;
	int bestScore = -1;
	bool ambiguous = false;

	for (const candidate& item : candidates.get_vector())
	{
		std::span<const gmt::Ref<StructSymbol>> candidateArgs = item.as_extension
			? std::span<const gmt::Ref<StructSymbol>>(extensionArgs.get_vector())
			: std::span<const gmt::Ref<StructSymbol>>(argTypes.get_vector());

		// bind untyped lambda arguments against this candidate's parameter
		// types first, so scoring and type inference see complete delegates
		bind_pending_lambda_arguments(node, item.method, item.as_extension, genericType, candidateArgs, explicitTypeArgs.get_vector());

		gmt::inline_vector<gmt::Ref<StructSymbol>, 4> candidateTypeArgs;
		int score = score_method_match(item.method, candidateArgs, genericType, candidateTypeArgs.get_vector(), explicitTypeArgs.get_vector());
		if (score < 0)
			continue;

		if (best.is_null() || score < bestScore)
		{
			best = item.method;
			bestScore = score;
			bestTypeArgs = std::move(candidateTypeArgs.get_vector());
			bestExtension = item.as_extension;
			ambiguous = false;
		}
		else if (score == bestScore)
			ambiguous = true;
	}

	if (best.is_null())
	{
		// explicit type arguments never apply to delegate-backed callees
		if (hasExplicitTypeArgs)
		{
			report_no_matching_overload(methodName, argTypes.get_vector(), node->get_identifier());
			return gmt::nullref;
		}

		// a delegate-backed variable, parameter or field is not a method;
		// the caller analyzes that shape separately, so stay silent here
		gmt::Ref<SyntaxSymbol> lookup = get_current_scope()->lookup(methodName);
		gmt::Ref<GenericTypeSymbol> delegateInstantiation = gmt::nullref;
		if (try_resolve_delegate_invocation(node, lookup, delegateInstantiation).is_null())
		{
			if (candidates.get_vector().empty())
				report_no_matching_method(methodName, node->get_identifier());
			else
				report_no_matching_overload(methodName, argTypes.get_vector(), node->get_identifier());
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

	if (!bestTypeArgs.empty() && !check_method_type_constraints(best, bestTypeArgs, node->get_identifier()))
		return gmt::nullref;

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

	gmt::inline_vector<gmt::Ref<StructSymbol>, 8> argTypes;
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

	// a type without declared constructors value-initializes when no
	// arguments are supplied
	if (searchType->constructors.empty() && argTypes.get_vector().empty())
		return gmt::nullref;

	gmt::Ref<ConstructorSymbol> best = gmt::nullref;
	int bestScore = -1;

	gmt::Ref<GenericTypeSymbol> construction = createdType->get_kind() == SyntaxKind::GenericType
		? createdType.as<GenericTypeSymbol>()
		: gmt::nullref;

	for (gmt::Ref<ConstructorSymbol> constructor : searchType->constructors)
	{
		if (constructor.is_null())
			continue;

		const gmt::vector<gmt::Ref<ParameterSymbol>>& parameters = constructor->get_parameters();
		if (parameters.size() != argTypes.get_vector().size())
			continue;

		int total = 0;
		bool matches = true;
		for (std::size_t i = 0; i < parameters.size(); i++)
		{
			gmt::Ref<StructSymbol> parameterType = parameters[i]->get_type();
			if (!construction.is_null())
				parameterType = substitute_type_parameters(parameterType, construction);

			int score = conversion_score(parameterType, argTypes.get_vector()[i]);
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
		get_diagnostics().report_error(node->get_new_token(), gmt::concat(L"No constructor of '", get_model().get_type_display_name(createdType), L"' accepts the supplied arguments (", format_argument_list(argTypes.get_vector(), get_model()), L")"));
		return gmt::nullref;
	}

	return best;
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_object_expression(const ObjectExpressionSyntax* node)
{
	std::optional<gmt::Ref<const ObjectExpressionSyntax>> nodeRef = ref_from(node);

	// expression nodes are analyzed repeatedly (member chains, overload
	// resolution); once the constructor is bound the type is already resolved
	if (nodeRef.has_value())
	{
		gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
		if (!bound.is_null() && bound->get_kind() == SyntaxKind::ConstructorDeclaration)
			return get_expression_type(*nodeRef);
	}

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

	gmt::inline_vector<gmt::Ref<StructSymbol>, 8> argTypes;
	if (!collect_argument_types(node, argTypes.get_vector()))
		return gmt::nullref;

	gmt::inline_vector<gmt::Ref<StructSymbol>, 4> explicitTypeArgs;
	collect_explicit_type_arguments(node, explicitTypeArgs.get_vector());

	gmt::Ref<MethodSymbol> best = gmt::nullref;
	std::pmr::vector<gmt::Ref<StructSymbol>> bestTypeArgs;
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

		gmt::inline_vector<gmt::Ref<StructSymbol>, 4> candidateTypeArgs;
		int score = score_method_match(method, argTypes.get_vector(), gmt::nullref, candidateTypeArgs.get_vector(), explicitTypeArgs.get_vector());
		if (score < 0)
			continue;

		if (best.is_null() || score < bestScore)
		{
			best = method;
			bestScore = score;
			bestTypeArgs = std::move(candidateTypeArgs.get_vector());
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

		if (!bestTypeArgs.empty() && !check_method_type_constraints(best, bestTypeArgs, node->get_identifier()))
			return gmt::nullref;

		get_symbol_table().bind_symbol<MethodSymbol>(*nodeRef, best);
		bind_invocation_type_arguments(*nodeRef, best, gmt::nullref, bestTypeArgs);
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
	gmt::inline_vector<gmt::Ref<StructSymbol>, 4> methodTypeArgs;
	gmt::Ref<GenericTypeSymbol> genericType = gmt::nullref;
	gmt::Ref<MethodSymbol> method = resolve_method(node, currentType, methodTypeArgs.get_vector(), genericType);
	std::optional<gmt::Ref<const InvokationExpressionSyntax>> nodeRef = ref_from(node);

	if (method.is_null())
	{
		// the callee may be a delegate-backed variable, parameter or field
		gmt::Ref<SyntaxSymbol> lookup = get_current_scope()->lookup(methodName);
		gmt::Ref<GenericTypeSymbol> delegateInstantiation = gmt::nullref;
		gmt::Ref<DelegateTypeSymbol> delegate = try_resolve_delegate_invocation(node, lookup, delegateInstantiation);
		if (delegate.is_null())
			return gmt::nullref;

		if (!node->get_arguments().is_null())
			match_method_arguments(node->get_delimeter_token(), delegate->get_parameters(), node->get_arguments()->get_arguments(), delegateInstantiation);

		gmt::Ref<StructSymbol> delegateReturn = substitute_type_parameters(delegate->get_return_type(), delegateInstantiation);
		if (nodeRef.has_value())
		{
			get_symbol_table().bind_symbol(*nodeRef, lookup);
			set_expression_type(*nodeRef, delegateReturn);
		}

		return delegateReturn;
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
		bind_invocation_type_arguments(*nodeRef, method, genericType, methodTypeArgs.get_vector());
	}

	if (method->get_return_type().is_null())
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' return type not resolved"));
		return gmt::nullref;
	}

	gmt::Ref<StructSymbol> type = substitute_type_parameters(method->get_return_type(), genericType, method, methodTypeArgs.get_vector());
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

	const gmt::Ref<StructSymbol> valueTaskArgs[] = { type };
	return table.find_or_create_generic_type(valueTask.as<StructSymbol>(), std::span(valueTaskArgs));
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
			gmt::wstring calleeName = calleeType.is_null() ? L"<error>" : get_model().get_type_display_name(calleeType);
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Cannot invoke expression of type '", calleeName, L"'"));

			if (nodeRef.has_value())
				set_expression_type(*nodeRef, gmt::nullref);

			return;
		}

		gmt::Ref<GenericTypeSymbol> delegateInstantiation = !calleeType.is_null() && calleeType->get_kind() == SyntaxKind::GenericType
			? calleeType.as<GenericTypeSymbol>()
			: gmt::nullref;

		if (!node->get_arguments().is_null())
			match_method_arguments(node->get_delimeter_token(), delegate->get_parameters(), node->get_arguments()->get_arguments(), delegateInstantiation);

		gmt::Ref<StructSymbol> delegateReturn = substitute_type_parameters(delegate->get_return_type(), delegateInstantiation);
		if (nodeRef.has_value())
		{
			get_symbol_table().bind_symbol<DelegateTypeSymbol>(*nodeRef, delegate);
			set_expression_type(*nodeRef, delegateReturn);
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
