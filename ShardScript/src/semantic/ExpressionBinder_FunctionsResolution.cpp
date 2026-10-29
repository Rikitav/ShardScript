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
	int numeric_rank(const StructSymbol* type, const SymbolTable& table)
	{
		if (type == table.get_byte_primitive() || type == table.get_char_primitive() || type == table.get_nint_primitive())
			return 0;

		if (type == table.get_integer_primitive())
			return 1;

		if (type == table.get_double_primitive())
			return 2;

		return -1;
	}

	bool is_reference_like(const StructSymbol* type)
	{
		if (type == nullptr)
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
				StructSymbol* underlaying = gmt::as<GenericTypeSymbol>(type)->get_underlaying_type();
				return underlaying->get_kind() == SyntaxKind::InterfaceDeclaration;
			}

			default:
				return false;
		}
	}

	DelegateTypeSymbol* get_delegate_type(const StructSymbol* type)
	{
		if (type == nullptr)
			return nullptr;

		if (type->get_kind() == SyntaxKind::DelegateType)
			return gmt::as<DelegateTypeSymbol>(type);

		if (type->get_kind() == SyntaxKind::GenericType)
		{
			StructSymbol* underlaying = gmt::as<GenericTypeSymbol>(type)->get_underlaying_type();
			if (!(underlaying == nullptr) && underlaying->get_kind() == SyntaxKind::DelegateType)
				return static_cast<DelegateTypeSymbol*>(underlaying);
		}

		return nullptr;
	}

	gmt::wstring format_argument_list(std::span<StructSymbol* const> argTypes, SemanticModel& model)
	{
		switch (argTypes.size())
		{
			case 0:
				return L"no arguments";

			case 1:
				return gmt::concat(L"argument (", (argTypes[0] == nullptr) ? L"<error>" : model.get_type_display_name(argTypes[0]), L")");

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

int ExpressionBinder::conversion_score(const StructSymbol* parameter, const StructSymbol* argument)
{
	if (parameter == nullptr)
		return -1;

	// null literal assignment
	if (argument == nullptr)
		return is_reference_like(parameter) ? 1 : -1;

	if (SemanticModel::are_types_equal(parameter, argument))
		return 0;

	const SymbolTable& table = get_symbol_table();

	bool parameterNullable = parameter->get_kind() == SyntaxKind::NullableType;
	bool argumentNullable = argument->get_kind() == SyntaxKind::NullableType;

	if (parameterNullable && !argumentNullable)
	{
		StructSymbol* underlaying = gmt::as<NullableTypeSymbol>(parameter)->get_underlaying_type();
		if (SemanticModel::are_types_equal(underlaying, argument))
			return 2;
	}

	if (!parameterNullable && argumentNullable)
	{
		StructSymbol* underlaying = gmt::as<NullableTypeSymbol>(argument)->get_underlaying_type();
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

void ExpressionBinder::report_no_matching_overload(std::wstring_view methodName, std::span<StructSymbol* const> argTypes, SyntaxToken blameToken)
{
	get_diagnostics().report_error(blameToken, gmt::concat(L"Method '", methodName, L"' does not have an overload that accepts the supplied arguments (", format_argument_list(argTypes, get_model()), L")"));
}

bool ExpressionBinder::collect_argument_types(const InvokationExpressionSyntax* node, std::pmr::vector<StructSymbol*>& outArgTypes)
{
	if ((node->get_arguments() == nullptr))
		return false;

	for (ArgumentSyntax* const& argument : node->get_arguments()->get_arguments())
	{
		if ((argument == nullptr) || (argument->get_expression() == nullptr))
			return false;

		visit_expression(argument->get_expression());
		outArgTypes.push_back(get_expression_type(argument->get_expression()));
	}

	return true;
}

void ExpressionBinder::bind_pending_lambda_arguments(const InvokationExpressionSyntax* node, MethodSymbol* method, bool asExtension, GenericTypeSymbol* genericType, std::span<StructSymbol* const> candidateArgs, std::span<StructSymbol* const> explicitTypeArgs)
{
	if (m_pendingLambdas.empty() || (node->get_arguments() == nullptr))
		return;

	std::span<ArgumentSyntax* const> arguments = node->get_arguments()->get_arguments();
	std::size_t parameterOffset = asExtension ? 1 : 0;

	bool hasPendingArgument = false;
	for (ArgumentSyntax* const& argument : arguments)
	{
		if (!(argument == nullptr) && is_pending_lambda(argument->get_expression()))
		{
			hasPendingArgument = true;
			break;
		}
	}

	if (!hasPendingArgument)
		return;

	const gmt::vector<ParameterSymbol*>& parameters = method->get_parameters();

	// partially infer method type arguments from the non-lambda arguments, so
	// delegate parameter types that mention them can resolve to concrete types
	bool generic = !method->get_type_parameters().empty();
	std::pmr::vector<StructSymbol*> partialTypeArgs;
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
			partialTypeArgs.assign(method->get_type_parameters().size(), nullptr);
			for (std::size_t i = 0; i < parameters.size() && i < candidateArgs.size(); i++)
			{
				// a position behind a pending lambda contributes nothing until
				// the lambda binds; an extension receiver has no syntax
				// argument and always contributes
				bool pendingPosition = i >= parameterOffset;
				pendingPosition = pendingPosition && (i - parameterOffset) < arguments.size();
				pendingPosition = pendingPosition && !(arguments[i - parameterOffset] == nullptr);
				pendingPosition = pendingPosition && is_pending_lambda(arguments[i - parameterOffset]->get_expression());

				if (pendingPosition || (candidateArgs[i] == nullptr))
					continue;

				try_infer_type_argument(parameters[i]->get_type(), candidateArgs[i], method, genericType, partialTypeArgs);
			}
		}
	}

	for (std::size_t i = 0; i < arguments.size() && i + parameterOffset < parameters.size(); i++)
	{
		ArgumentSyntax* const& argument = arguments[i];
		if (argument == nullptr)
			continue;

		const ExpressionSyntax* expression = argument->get_expression();
		if (!is_pending_lambda(expression))
			continue;

		StructSymbol* expectedType = parameters[i + parameterOffset]->get_type();
		expectedType = generic
			? substitute_type_parameters(expectedType, genericType, method, partialTypeArgs)
			: substitute_type_parameters(expectedType, genericType);

		try_bind_pending_lambda(static_cast<const LambdaExpressionSyntax*>(expression), expectedType);
	}
}

bool ExpressionBinder::match_method_arguments(SyntaxToken blameToken, std::span<ParameterSymbol* const> parameters, std::span<ArgumentSyntax* const> arguments, GenericTypeSymbol* genericType, std::size_t parameterOffset)
{
	if (parameterOffset > parameters.size())
		parameterOffset = parameters.size();

	std::size_t expectedArguments = parameters.size() - parameterOffset;
	if (expectedArguments != arguments.size())
	{
		get_diagnostics().report_error(blameToken, gmt::concat(L"Method expects ", std::to_wstring(expectedArguments), L" arguments but got ", std::to_wstring(arguments.size())));
		return false;
	}

	for (std::size_t i = parameterOffset; i < parameters.size(); i++)
	{
		ParameterSymbol* parameter = parameters[i];
		ArgumentSyntax* argument = arguments[i - parameterOffset];

		if ((parameter == nullptr) || (argument == nullptr) || (argument->get_expression() == nullptr))
		{
			get_diagnostics().report_error(blameToken, gmt::concat(L"Invalid argument at position ", std::to_wstring(i - parameterOffset)));
			return false;
		}

		StructSymbol* argumentType = get_expression_type(argument->get_expression());
		if (argumentType == nullptr)
			return false;

		StructSymbol* parameterType = substitute_type_parameters(parameter->get_type(), genericType);
		if (parameterType == nullptr)
			return false;

		if (conversion_score(parameterType, argumentType) < 0)
		{
			get_diagnostics().report_error(blameToken, gmt::concat(L"Argument type mismatch for parameter '", parameter->get_name(), L"': expected '", get_model().get_type_display_name(parameterType), L"' but got '", get_model().get_type_display_name(argumentType), L"'"));
			return false;
		}
	}

	return true;
}

bool ExpressionBinder::try_infer_type_argument(StructSymbol* pattern, StructSymbol* concrete, MethodSymbol* method, GenericTypeSymbol* genericType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs)
{
	if ((pattern == nullptr) || (concrete == nullptr))
		return true;

	if (!(genericType == nullptr) && pattern->get_kind() == SyntaxKind::TypeParameter)
	{
		StructSymbol* substituted = genericType->substitute_type_parameter(static_cast<TypeParameterSymbol*>(pattern));
		if (!(substituted == nullptr))
			pattern = substituted;
	}

	if (pattern->get_kind() == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* typeParameter = static_cast<TypeParameterSymbol*>(pattern);
		if (typeParameter->get_parent() != method)
			return true;

		for (std::size_t i = 0; i < method->get_type_parameters().size(); i++)
		{
			if (method->get_type_parameters()[i] == typeParameter)
			{
				if (outMethodTypeArgs[i] == nullptr)
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
		GenericTypeSymbol* patternGeneric = static_cast<GenericTypeSymbol*>(pattern);
		StructSymbol* patternUnderlaying = patternGeneric->get_underlaying_type();

		if (concrete->get_kind() == SyntaxKind::GenericType)
		{
			GenericTypeSymbol* concreteGeneric = static_cast<GenericTypeSymbol*>(concrete);
			if (concreteGeneric->get_underlaying_type() == patternUnderlaying)
			{
				for (TypeParameterSymbol* parameter : patternUnderlaying->type_parameters)
				{
					StructSymbol* patternArgument = patternGeneric->substitute_type_parameter(parameter);
					StructSymbol* concreteArgument = concreteGeneric->substitute_type_parameter(parameter);
					if (!try_infer_type_argument(patternArgument, concreteArgument, method, genericType, outMethodTypeArgs))
						return false;
				}

				return true;
			}

			// a constructed type that implements the pattern's generic
			// interface: the interface's arguments substitute through the
			// construction before pairwise inference
			for (StructSymbol* interfaceType : concreteGeneric->get_underlaying_type()->interfaces)
			{
				if ((interfaceType == nullptr) || interfaceType->get_kind() != SyntaxKind::GenericType)
					continue;

				GenericTypeSymbol* interfaceGeneric = static_cast<GenericTypeSymbol*>(interfaceType);
				if (interfaceGeneric->get_underlaying_type() != patternUnderlaying)
					continue;

				for (TypeParameterSymbol* parameter : patternUnderlaying->type_parameters)
				{
					StructSymbol* patternArgument = patternGeneric->substitute_type_parameter(parameter);
					StructSymbol* interfaceArgument = interfaceGeneric->substitute_type_parameter(parameter);
					StructSymbol* concreteArgument = substitute_type_parameters(interfaceArgument, concreteGeneric);
					if (!try_infer_type_argument(patternArgument, concreteArgument, method, genericType, outMethodTypeArgs))
						return false;
				}

				return true;
			}

			return true;
		}

		if (concrete->get_kind() == SyntaxKind::StructDeclaration || concrete->get_kind() == SyntaxKind::InterfaceDeclaration || concrete->get_kind() == SyntaxKind::EnumDeclaration)
		{
			for (StructSymbol* interfaceType : concrete->interfaces)
			{
				if ((interfaceType == nullptr) || interfaceType->get_kind() != SyntaxKind::GenericType)
					continue;

				GenericTypeSymbol* interfaceGeneric = static_cast<GenericTypeSymbol*>(interfaceType);
				if (interfaceGeneric->get_underlaying_type() != patternUnderlaying)
					continue;

				for (TypeParameterSymbol* parameter : patternUnderlaying->type_parameters)
				{
					StructSymbol* patternArgument = patternGeneric->substitute_type_parameter(parameter);
					StructSymbol* concreteArgument = interfaceGeneric->substitute_type_parameter(parameter);
					if (!try_infer_type_argument(patternArgument, concreteArgument, method, genericType, outMethodTypeArgs))
						return false;
				}

				return true;
			}
		}

		if (patternUnderlaying->get_kind() == SyntaxKind::DelegateType && concrete->get_kind() == SyntaxKind::DelegateType)
		{
			DelegateTypeSymbol* patternDelegate = static_cast<DelegateTypeSymbol*>(patternUnderlaying);
			DelegateTypeSymbol* concreteDelegate = static_cast<DelegateTypeSymbol*>(concrete);

			if (!(patternDelegate->get_return_type() == nullptr) && !(concreteDelegate->get_return_type() == nullptr))
			{
				StructSymbol* returnPattern = substitute_type_parameters(patternDelegate->get_return_type(), patternGeneric);
				if (!try_infer_type_argument(returnPattern, concreteDelegate->get_return_type(), method, genericType, outMethodTypeArgs))
					return false;
			}

			if (patternDelegate->get_parameters().size() == concreteDelegate->get_parameters().size())
			{
				for (std::size_t i = 0; i < patternDelegate->get_parameters().size(); ++i)
				{
					StructSymbol* parameterPattern = substitute_type_parameters(patternDelegate->get_parameters()[i]->get_type(), patternGeneric);
					if (!try_infer_type_argument(parameterPattern, concreteDelegate->get_parameters()[i]->get_type(), method, genericType, outMethodTypeArgs))
						return false;
				}
			}

			return true;
		}
	}

	if (pattern->get_kind() == SyntaxKind::ArrayType && concrete->get_kind() == SyntaxKind::ArrayType)
	{
		return try_infer_type_argument(static_cast<ArrayTypeSymbol*>(pattern)->get_underlaying_type(), static_cast<ArrayTypeSymbol*>(concrete)->get_underlaying_type(), method, genericType, outMethodTypeArgs);
	}

	return true;
}

bool ExpressionBinder::infer_method_type_arguments(MethodSymbol* method, std::span<StructSymbol* const> argTypes, GenericTypeSymbol* genericType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs)
{
	outMethodTypeArgs.assign(method->get_type_parameters().size(), nullptr);

	if (argTypes.size() != method->get_parameters().size())
		return false;

	for (std::size_t i = 0; i < method->get_parameters().size(); ++i)
	{
		StructSymbol* parameterType = method->get_parameters()[i]->get_type();
		if (!try_infer_type_argument(parameterType, argTypes[i], method, genericType, outMethodTypeArgs))
			return false;
	}

	for (StructSymbol* type : outMethodTypeArgs)
	{
		if (type == nullptr)
			return false;
	}

	return true;
}

namespace
{
	bool try_infer_class_type_argument(StructSymbol* pattern, StructSymbol* concrete, StructSymbol* classDef, gmt::vector<StructSymbol*>& outArgs)
	{
		if ((pattern == nullptr) || (concrete == nullptr))
			return true;

		if (pattern->get_kind() == SyntaxKind::TypeParameter)
		{
			TypeParameterSymbol* typeParameter = static_cast<TypeParameterSymbol*>(pattern);
			if (typeParameter->get_type_argument_index() < outArgs.size() && classDef->type_parameters[typeParameter->get_type_argument_index()] == typeParameter)
			{
				StructSymbol*& slot = outArgs[typeParameter->get_type_argument_index()];
				if (slot == nullptr)
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

			return try_infer_class_type_argument(static_cast<ArrayTypeSymbol*>(pattern)->get_underlaying_type(), static_cast<ArrayTypeSymbol*>(concrete)->get_underlaying_type(), classDef, outArgs);
		}

		if (pattern->get_kind() == SyntaxKind::GenericType)
		{
			if (concrete->get_kind() != SyntaxKind::GenericType)
				return false;

			GenericTypeSymbol* patternGeneric = static_cast<GenericTypeSymbol*>(pattern);
			GenericTypeSymbol* concreteGeneric = static_cast<GenericTypeSymbol*>(concrete);
			if (patternGeneric->get_underlaying_type() != concreteGeneric->get_underlaying_type())
				return false;

			for (TypeParameterSymbol* parameter : patternGeneric->get_underlaying_type()->type_parameters)
			{
				StructSymbol* patternArgument = patternGeneric->substitute_type_parameter(parameter);
				StructSymbol* concreteArgument = concreteGeneric->substitute_type_parameter(parameter);
				if (!try_infer_class_type_argument(patternArgument, concreteArgument, classDef, outArgs))
					return false;
			}

			return true;
		}

		return true;
	}
}

bool ExpressionBinder::infer_class_type_arguments(StructSymbol* classDef, MethodSymbol* method, std::span<StructSymbol* const> argTypes, gmt::vector<StructSymbol*>& outClassTypeArgs)
{
	outClassTypeArgs.assign(classDef->type_parameters.size(), nullptr);

	if (argTypes.size() != method->get_parameters().size())
		return false;

	for (std::size_t i = 0; i < method->get_parameters().size(); ++i)
	{
		if (!try_infer_class_type_argument(method->get_parameters()[i]->get_type(), argTypes[i], classDef, outClassTypeArgs))
			return false;
	}

	for (StructSymbol* type : outClassTypeArgs)
	{
		if (type == nullptr)
			return false;
	}

	return true;
}


int ExpressionBinder::score_method_match(MethodSymbol* method, std::span<StructSymbol* const> argTypes, GenericTypeSymbol* genericType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs, std::span<StructSymbol* const> explicitTypeArgs)
{
	const gmt::vector<ParameterSymbol*>& parameters = method->get_parameters();
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
		StructSymbol* parameterType = generic
			? substitute_type_parameters(parameters[i]->get_type(), genericType, method, outMethodTypeArgs)
			: substitute_type_parameters(parameters[i]->get_type(), genericType);

		int score = conversion_score(parameterType, argTypes[i]);
		if (score < 0)
			return -1;

		total += score;
	}

	return total;
}

bool ExpressionBinder::check_method_type_constraints(MethodSymbol* method, std::span<StructSymbol* const> methodTypeArgs, SyntaxToken blameToken)
{
	const gmt::vector<TypeParameterSymbol*>& typeParameters = method->get_type_parameters();
	bool satisfied = true;

	for (std::size_t i = 0; i < typeParameters.size() && i < methodTypeArgs.size(); ++i)
	{
		TypeParameterSymbol* typeParameter = typeParameters[i];
		StructSymbol* argument = methodTypeArgs[i];
		if (argument == nullptr)
			continue;

		for (StructSymbol* const& constraint : typeParameter->get_constraints())
		{
			if (constraint == nullptr)
				continue;

			if (get_model().type_satisfies_constraint(argument, constraint))
				continue;

			get_diagnostics().report_error(blameToken, gmt::concat(L"Type argument '", argument->get_name(), L"' does not satisfy the constraint '", constraint->get_name(), L"' on type parameter '", typeParameter->get_name(), L"'"));
			satisfied = false;
		}
	}

	return satisfied;
}

bool ExpressionBinder::collect_explicit_type_arguments(const InvokationExpressionSyntax* node, std::pmr::vector<StructSymbol*>& outTypeArgs)
{
	outTypeArgs.clear();
	const TypeArgumentsListSyntax* list = node->get_type_arguments();
	if (list == nullptr)
		return false;

	for (TypeSyntax* const& argumentNode : list->get_types())
	{
		StructSymbol* type = resolve_type_expression(argumentNode);
		if (!(type == nullptr))
			outTypeArgs.push_back(type);
	}

	return true;
}

void ExpressionBinder::bind_invocation_type_arguments(const InvokationExpressionSyntax* node, MethodSymbol* method, GenericTypeSymbol* genericType, std::span<StructSymbol* const> methodTypeArgs)
{
	bool genericReceiver = !(genericType == nullptr);
	bool genericMethod = !method->get_type_parameters().empty();
	if (!genericReceiver && !genericMethod)
		return;

	// combined index space: receiver type arguments first, then the method's
	// own, matching TypeParameterSymbol::get_type_argument_index
	gmt::inline_vector<StructSymbol*, 4> combined;
	if (genericReceiver)
	{
		for (TypeParameterSymbol* parameter : genericType->get_underlaying_type()->type_parameters)
			combined.push_back(genericType->substitute_type_parameter(parameter));
	}

	for (StructSymbol* argument : methodTypeArgs)
		combined.push_back(argument);

	get_symbol_table().bind_type_arguments(node, combined.get_vector());
}

DelegateTypeSymbol* ExpressionBinder::try_resolve_delegate_invocation(const InvokationExpressionSyntax* node, SyntaxSymbol* lookup, GenericTypeSymbol*& outInstantiation)
{
	outInstantiation = nullptr;
	if (lookup == nullptr)
		return nullptr;

	StructSymbol* delegateHolderType = nullptr;
	switch (lookup->get_kind())
	{
		case SyntaxKind::VariableStatement:
			delegateHolderType = static_cast<VariableSymbol*>(lookup)->get_type();
			break;

		case SyntaxKind::Parameter:
			delegateHolderType = static_cast<ParameterSymbol*>(lookup)->get_type();
			break;

		case SyntaxKind::FieldDeclaration:
			delegateHolderType = static_cast<FieldSymbol*>(lookup)->get_return_type();
			break;

		default:
			return nullptr;
	}

	// a generic delegate instantiation substitutes the delegate's own type
	// parameters with the holder's type arguments
	if (!(delegateHolderType == nullptr) && delegateHolderType->get_kind() == SyntaxKind::GenericType)
		outInstantiation = static_cast<GenericTypeSymbol*>(delegateHolderType);

	return get_delegate_type(delegateHolderType);
}

MethodSymbol* ExpressionBinder::resolve_method(const InvokationExpressionSyntax* node, StructSymbol* currentType, std::pmr::vector<StructSymbol*>& outMethodTypeArgs, GenericTypeSymbol*& outGenericType)
{
	std::wstring_view methodName = node->get_identifier().get_lexeme();

	if ((node->get_arguments() == nullptr))
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' invocation has no arguments list"));
		return nullptr;
	}

	gmt::inline_vector<StructSymbol*, 8> argTypes;
	if (!collect_argument_types(node, argTypes.get_vector()))
		return nullptr;

	gmt::inline_vector<StructSymbol*, 4> explicitTypeArgs;
	bool hasExplicitTypeArgs = collect_explicit_type_arguments(node, explicitTypeArgs.get_vector());

	bool staticContext = get_is_static_context(node->get_previous());
	if ((currentType == nullptr) && is_ambiguous_name(methodName))
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Ambiguous reference: '", methodName, L"' is defined in multiple imported namespaces"));
		return nullptr;
	}

	GenericTypeSymbol* genericType = nullptr;
	if (!(currentType == nullptr) && currentType->get_kind() == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(currentType);

	outGenericType = nullptr;

	// extension shape carries the receiver type as the first argument
	gmt::inline_vector<StructSymbol*, 8> extensionArgs;
	if (!(currentType == nullptr) && !staticContext)
	{
		extensionArgs.get_vector().reserve(argTypes.get_vector().size() + 1);
		extensionArgs.push_back(currentType);
		extensionArgs.get_vector().insert(extensionArgs.get_vector().end(), argTypes.get_vector().begin(), argTypes.get_vector().end());
	}

	struct candidate
	{
		MethodSymbol* method = nullptr;
		bool as_extension;
	};

	gmt::inline_vector<candidate, 8> candidates;
	auto addCandidate = [&candidates](MethodSymbol* method, bool asExtension)
	{
		for (const candidate& existing : candidates.get_vector())
		{
			if (existing.method == method)
				return;
		}

		candidates.push_back({ method, asExtension });
	};

	// instance and static methods declared on the receiver type
	StructSymbol* receiverType = currentType;
	if (!(receiverType == nullptr) && receiverType->get_kind() == SyntaxKind::GenericType)
		receiverType = static_cast<GenericTypeSymbol*>(receiverType)->get_underlaying_type();

	if (!(receiverType == nullptr))
	{
		if (receiverType->get_kind() == SyntaxKind::TypeParameter)
		{
			for (StructSymbol* constraint : static_cast<TypeParameterSymbol*>(receiverType)->get_constraints())
			{
				if (constraint == nullptr)
					continue;

				for (MethodSymbol* method : constraint->methods)
				{
					if (!(method == nullptr) && method->get_name() == methodName)
						addCandidate(method, false);
				}
			}
		}
		else
		{
			for (MethodSymbol* method : receiverType->methods)
			{
				if (!(method == nullptr) && method->get_name() == methodName)
					addCandidate(method, false);
			}
		}
	}

	// functions declared in the current scope: locals, top-level functions
	// and members of imported namespaces
	bool asExtension = !staticContext && !(currentType == nullptr);
	SyntaxSymbol* scopeLookup = get_current_scope()->lookup(methodName);
	if (!(scopeLookup == nullptr) && scopeLookup->get_kind() == SyntaxKind::FunctionDeclaration)
		addCandidate(static_cast<MethodSymbol*>(scopeLookup), asExtension);

	for (SyntaxSymbol* const& overload : get_current_scope()->find_overloads(methodName))
	{
		if (!(overload == nullptr) && overload->get_kind() == SyntaxKind::FunctionDeclaration)
			addCandidate(static_cast<MethodSymbol*>(overload), asExtension);
	}

	if (hasExplicitTypeArgs)
	{
		bool arityAccepted = false;
		for (const candidate& item : candidates.get_vector())
		{
			const gmt::vector<TypeParameterSymbol*>& typeParameters = item.method->get_type_parameters();
			if (!typeParameters.empty() && typeParameters.size() == explicitTypeArgs.get_vector().size())
			{
				arityAccepted = true;
				break;
			}
		}

		if (!arityAccepted)
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' does not accept ", std::to_wstring(explicitTypeArgs.get_vector().size()), L" type argument(s)"));
			return nullptr;
		}
	}

	MethodSymbol* best = nullptr;
	std::pmr::vector<StructSymbol*> bestTypeArgs;
	bool bestExtension = false;
	int bestScore = -1;
	bool ambiguous = false;

	for (const candidate& item : candidates.get_vector())
	{
		std::span<StructSymbol* const> candidateArgs = item.as_extension
			? std::span<StructSymbol* const>(extensionArgs.get_vector())
			: std::span<StructSymbol* const>(argTypes.get_vector());

		// bind untyped lambda arguments against this candidate's parameter
		// types first, so scoring and type inference see complete delegates
		bind_pending_lambda_arguments(node, item.method, item.as_extension, genericType, candidateArgs, explicitTypeArgs.get_vector());

		gmt::inline_vector<StructSymbol*, 4> candidateTypeArgs;
		int score = score_method_match(item.method, candidateArgs, genericType, candidateTypeArgs.get_vector(), explicitTypeArgs.get_vector());
		if (score < 0)
			continue;

		if ((best == nullptr) || score < bestScore)
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

	if (best  == nullptr)
	{
		// explicit type arguments never apply to delegate-backed callees
		if (hasExplicitTypeArgs)
		{
			report_no_matching_overload(methodName, argTypes.get_vector(), node->get_identifier());
			return nullptr;
		}

		// a delegate-backed variable, parameter or field is not a method;
		// the caller analyzes that shape separately, so stay silent here
		SyntaxSymbol* lookup = get_current_scope()->lookup(methodName);
		GenericTypeSymbol* delegateInstantiation = nullptr;
		if ((try_resolve_delegate_invocation(node, lookup, delegateInstantiation) == nullptr))
		{
			if (candidates.get_vector().empty())
				report_no_matching_method(methodName, node->get_identifier());
			else
				report_no_matching_overload(methodName, argTypes.get_vector(), node->get_identifier());
		}

		return nullptr;
	}

	if (ambiguous)
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"The call to '", methodName, L"' is ambiguous between multiple overloads"));
		return nullptr;
	}

	if (staticContext && best->get_linking() == LINK_INSTANCE)
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Cannot call instance method '", methodName, L"' from type context"));
		return nullptr;
	}

	if (!staticContext && best->get_linking() == LINK_STATIC && !bestExtension)
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Cannot call static method '", methodName, L"' on instance reference"));
		return nullptr;
	}

	if (!bestTypeArgs.empty() && !check_method_type_constraints(best, bestTypeArgs, node->get_identifier()))
		return nullptr;

	outMethodTypeArgs = std::move(bestTypeArgs);
	outGenericType = genericType;
	return best;
}

ConstructorSymbol* ExpressionBinder::resolve_constructor(const ObjectExpressionSyntax* node)
{
	std::optional<const ObjectExpressionSyntax*> nodeRef = ref_from(node);
	StructSymbol* createdType = nodeRef.has_value() ? lookup_symbol<StructSymbol>(*nodeRef) : nullptr;
	if (createdType == nullptr)
		return nullptr;

	StructSymbol* searchType = createdType;
	if (searchType->get_kind() == SyntaxKind::GenericType)
		searchType = static_cast<GenericTypeSymbol*>(searchType)->get_underlaying_type();

	if ((node->get_arguments() == nullptr))
		return nullptr;

	gmt::inline_vector<StructSymbol*, 8> argTypes;
	for (ArgumentSyntax* const& argument : node->get_arguments()->get_arguments())
	{
		if ((argument == nullptr) || (argument->get_expression() == nullptr))
			return nullptr;

		visit_expression(argument->get_expression());
		StructSymbol* argumentType = get_expression_type(argument->get_expression());
		if (argumentType == nullptr)
			return nullptr;

		argTypes.push_back(argumentType);
	}

	// a type without declared constructors value-initializes when no
	// arguments are supplied
	if (searchType->constructors.empty() && argTypes.get_vector().empty())
		return nullptr;

	ConstructorSymbol* best = nullptr;
	int bestScore = -1;

	GenericTypeSymbol* construction = createdType->get_kind() == SyntaxKind::GenericType
		? static_cast<GenericTypeSymbol*>(createdType)
		: nullptr;

	for (ConstructorSymbol* constructor : searchType->constructors)
	{
		if (constructor == nullptr)
			continue;

		const gmt::vector<ParameterSymbol*>& parameters = constructor->get_parameters();
		if (parameters.size() != argTypes.get_vector().size())
			continue;

		int total = 0;
		bool matches = true;
		for (std::size_t i = 0; i < parameters.size(); i++)
		{
			StructSymbol* parameterType = parameters[i]->get_type();
			if (!(construction == nullptr))
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

		if ((best == nullptr) || total < bestScore)
		{
			best = constructor;
			bestScore = total;
		}
	}

	if (best == nullptr)
	{
		get_diagnostics().report_error(node->get_new_token(), gmt::concat(L"No constructor of '", get_model().get_type_display_name(createdType), L"' accepts the supplied arguments (", format_argument_list(argTypes.get_vector(), get_model()), L")"));
		return nullptr;
	}

	return best;
}

StructSymbol* ExpressionBinder::analyze_object_expression(const ObjectExpressionSyntax* node)
{
	std::optional<const ObjectExpressionSyntax*> nodeRef = ref_from(node);

	// expression nodes are analyzed repeatedly (member chains, overload
	// resolution); once the constructor is bound the type is already resolved
	if (nodeRef.has_value())
	{
		SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*nodeRef);
		if (!(bound == nullptr) && bound->get_kind() == SyntaxKind::ConstructorDeclaration)
			return get_expression_type(*nodeRef);
	}

	StructSymbol* createdType = nodeRef.has_value() ? lookup_symbol<StructSymbol>(*nodeRef) : nullptr;
	if (createdType == nullptr)
		return nullptr;

	ConstructorSymbol* constructor = resolve_constructor(node);
	if (constructor == nullptr)
		return createdType;

	if (!(node->get_arguments() == nullptr))
	{
		GenericTypeSymbol* genericType = nullptr;
		if (createdType->get_kind() == SyntaxKind::GenericType)
			genericType = static_cast<GenericTypeSymbol*>(createdType);

		match_method_arguments(node->get_new_token(), constructor->get_parameters(), node->get_arguments()->get_arguments(), genericType);
	}

	if (nodeRef.has_value())
		get_symbol_table().bind_symbol<ConstructorSymbol>(*nodeRef, constructor);

	return createdType;
}

void ExpressionBinder::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	std::optional<const ObjectExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	if (node->is_array_creation())
	{
		if (!(node->get_array_size() == nullptr))
			visit_expression(node->get_array_size());

		StructSymbol* elementType = lookup_symbol<StructSymbol>(*nodeRef);
		if (elementType == nullptr)
			elementType = get_symbol_table().get_void_primitive();

		if (!(node->get_array_size() == nullptr))
		{
			StructSymbol* sizeType = get_expression_type(node->get_array_size());
			bool sizeIsInteger = !(sizeType == nullptr) && sizeType == get_symbol_table().get_integer_primitive();
			if (!sizeIsInteger)
				get_diagnostics().report_error(node->get_new_token(), L"Array size must be an integer");
		}

		ArrayTypeSymbol* arrayType = m_factory.create_array_type(elementType);
		get_symbol_table().bind_symbol<ArrayTypeSymbol>(*nodeRef, arrayType);
		set_expression_type(*nodeRef, arrayType);
		return;
	}

	StructSymbol* type = analyze_object_expression(node);
	set_expression_type(*nodeRef, type);
}

NamespaceSymbol* ExpressionBinder::resolve_namespace_qualifier(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return nullptr;

	if (expression->get_kind() != SyntaxKind::MemberAccessExpression)
		return nullptr;

	const MemberAccessExpressionSyntax* member = gmt::as<MemberAccessExpressionSyntax>(expression);
	if ((member->get_previous() == nullptr))
	{
		SyntaxSymbol* symbol = get_current_scope()->lookup(member->get_identifier().get_lexeme());
		if (!(symbol == nullptr) && symbol->get_kind() == SyntaxKind::NamespaceDirective)
			return static_cast<NamespaceSymbol*>(symbol);

		return get_symbol_table().get_root_namespace()->lookup(member->get_identifier().get_lexeme());
	}

	NamespaceSymbol* parentNamespace = resolve_namespace_qualifier(member->get_previous());
	if (parentNamespace == nullptr)
		return nullptr;

	return parentNamespace->lookup(member->get_identifier().get_lexeme());
}

StructSymbol* ExpressionBinder::analyze_qualified_invokation_expression(const InvokationExpressionSyntax* node, NamespaceSymbol* namespaceSymbol)
{
	std::wstring_view methodName = node->get_identifier().get_lexeme();

	if ((node->get_arguments() == nullptr))
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' invocation has no arguments list"));
		return nullptr;
	}

	gmt::inline_vector<StructSymbol*, 8> argTypes;
	if (!collect_argument_types(node, argTypes.get_vector()))
		return nullptr;

	gmt::inline_vector<StructSymbol*, 4> explicitTypeArgs;
	collect_explicit_type_arguments(node, explicitTypeArgs.get_vector());

	MethodSymbol* best = nullptr;
	std::pmr::vector<StructSymbol*> bestTypeArgs;
	int bestScore = -1;

	for (SyntaxSymbol* member : namespaceSymbol->members)
	{
		if ((member == nullptr) || member->get_kind() != SyntaxKind::FunctionDeclaration)
			continue;

		MethodSymbol* method = static_cast<MethodSymbol*>(member);
		if (method->get_name() != methodName)
			continue;

		if (method->get_linking() != LINK_STATIC)
			continue;

		gmt::inline_vector<StructSymbol*, 4> candidateTypeArgs;
		int score = score_method_match(method, argTypes.get_vector(), nullptr, candidateTypeArgs.get_vector(), explicitTypeArgs.get_vector());
		if (score < 0)
			continue;

		if ((best == nullptr) || score < bestScore)
		{
			best = method;
			bestScore = score;
			bestTypeArgs = std::move(candidateTypeArgs.get_vector());
		}
	}

	if (best == nullptr)
	{
		for (SyntaxSymbol* member : namespaceSymbol->members)
		{
			if (!(member == nullptr) && member->get_name() == methodName && member->is_type())
			{
				get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Use 'new ", methodName, L"()' instead of '", methodName, L"()' to construct a type"));
				return nullptr;
			}
		}

		if (!(namespaceSymbol->lookup(methodName) == nullptr))
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Cannot call namespace '", methodName, L"'; use '", methodName, L".Member()' to access its members"));
			return nullptr;
		}

		report_no_matching_method(methodName, node->get_identifier());
		return nullptr;
	}

	std::optional<const InvokationExpressionSyntax*> nodeRef = ref_from(node);
	if (nodeRef.has_value())
	{
		const SyntaxNode* declaringNode = get_symbol_table().lookup_node(best);
		if (!is_symbol_accessible(best, declaringNode, *nodeRef))
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' is not accessible"));
			return nullptr;
		}

		if (!bestTypeArgs.empty() && !check_method_type_constraints(best, bestTypeArgs, node->get_identifier()))
			return nullptr;

		get_symbol_table().bind_symbol<MethodSymbol>(*nodeRef, best);
		bind_invocation_type_arguments(*nodeRef, best, nullptr, bestTypeArgs);
	}

	if ((best->get_return_type() == nullptr))
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' return type not resolved"));
		return nullptr;
	}

	StructSymbol* type = substitute_type_parameters(best->get_return_type(), nullptr, best, bestTypeArgs);
	return wrap_async_return_type(best, type);
}

StructSymbol* ExpressionBinder::analyze_invokation_expression(const InvokationExpressionSyntax* node, StructSymbol* currentType)
{
	std::wstring_view methodName = node->get_identifier().get_lexeme();
	gmt::inline_vector<StructSymbol*, 4> methodTypeArgs;
	GenericTypeSymbol* genericType = nullptr;
	MethodSymbol* method = resolve_method(node, currentType, methodTypeArgs.get_vector(), genericType);
	std::optional<const InvokationExpressionSyntax*> nodeRef = ref_from(node);

	if (method == nullptr)
	{
		// the callee may be a delegate-backed variable, parameter or field
		SyntaxSymbol* lookup = get_current_scope()->lookup(methodName);
		GenericTypeSymbol* delegateInstantiation = nullptr;
		DelegateTypeSymbol* delegate = try_resolve_delegate_invocation(node, lookup, delegateInstantiation);
		if (delegate == nullptr)
			return nullptr;

		if (!(node->get_arguments() == nullptr))
			match_method_arguments(node->get_delimeter_token(), delegate->get_parameters(), node->get_arguments()->get_arguments(), delegateInstantiation);

		StructSymbol* delegateReturn = substitute_type_parameters(delegate->get_return_type(), delegateInstantiation);
		if (nodeRef.has_value())
		{
			get_symbol_table().bind_symbol(*nodeRef, lookup);
			set_expression_type(*nodeRef, delegateReturn);
		}

		return delegateReturn;
	}

	if (nodeRef.has_value())
	{
		const SyntaxNode* declaringNode = get_symbol_table().lookup_node(method);
		if (!is_symbol_accessible(method, declaringNode, *nodeRef))
		{
			get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' is not accessible"));
			return nullptr;
		}

		get_symbol_table().bind_symbol<MethodSymbol>(*nodeRef, method);
		bind_invocation_type_arguments(*nodeRef, method, genericType, methodTypeArgs.get_vector());
	}

	if ((method->get_return_type() == nullptr))
	{
		get_diagnostics().report_error(node->get_identifier(), gmt::concat(L"Method '", methodName, L"' return type not resolved"));
		return nullptr;
	}

	StructSymbol* type = substitute_type_parameters(method->get_return_type(), genericType, method, methodTypeArgs.get_vector());
	return wrap_async_return_type(method, type);
}

StructSymbol* ExpressionBinder::wrap_async_return_type(MethodSymbol* method, StructSymbol* type)
{
	if ((method == nullptr) || !method->get_is_async() || (type == nullptr))
		return type;

	SymbolTable& table = get_symbol_table();

	// async methods declare their plain result type; the Task wrapper is
	// implied for invokations and materialized by the hoisting pass
	if (type == table.get_void_primitive())
	{
		SyntaxSymbol* task = table.get_standard_type(StandardTypes::Task);
		return task->get_kind() == SyntaxKind::StructDeclaration ? static_cast<StructSymbol*>(task) : type;
	}

	SyntaxSymbol* valueTask = table.get_standard_type(StandardTypes::ValueTask);
	if (valueTask->get_kind() != SyntaxKind::StructDeclaration)
		return type;

	StructSymbol* const valueTaskArgs[] = { type };
	return table.find_or_create_generic_type(static_cast<StructSymbol*>(valueTask), std::span(valueTaskArgs));
}

void ExpressionBinder::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	std::optional<const InvokationExpressionSyntax*> nodeRef = ref_from(node);
	const ExpressionSyntax* previous = node->get_previous();

	// call of a direct callee expression: (callee)(arguments)
	if (node->get_identifier().get_type() == TokenType::Unknown && !(previous == nullptr))
	{
		visit_expression(previous);
		StructSymbol* calleeType = get_expression_type(previous);
		DelegateTypeSymbol* delegate = get_delegate_type(calleeType);

		if (delegate == nullptr)
		{
			gmt::wstring calleeName = (calleeType == nullptr) ? L"<error>" : get_model().get_type_display_name(calleeType);
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Cannot invoke expression of type '", calleeName, L"'"));

			if (nodeRef.has_value())
				set_expression_type(*nodeRef, nullptr);

			return;
		}

		GenericTypeSymbol* delegateInstantiation = !(calleeType == nullptr) && calleeType->get_kind() == SyntaxKind::GenericType
			? static_cast<GenericTypeSymbol*>(calleeType)
			: nullptr;

		if (!(node->get_arguments() == nullptr))
			match_method_arguments(node->get_delimeter_token(), delegate->get_parameters(), node->get_arguments()->get_arguments(), delegateInstantiation);

		StructSymbol* delegateReturn = substitute_type_parameters(delegate->get_return_type(), delegateInstantiation);
		if (nodeRef.has_value())
		{
			get_symbol_table().bind_symbol<DelegateTypeSymbol>(*nodeRef, delegate);
			set_expression_type(*nodeRef, delegateReturn);
		}

		return;
	}

	// namespace-qualified invocation: Foo::Bar(); namespaces are not
	// expression values, so the qualifier chain resolves without visiting it
	NamespaceSymbol* namespaceQualifier = resolve_namespace_qualifier(previous);
	if (!(namespaceQualifier == nullptr))
	{
		StructSymbol* type = analyze_qualified_invokation_expression(node, namespaceQualifier);
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, type);

		return;
	}

	if (!(previous == nullptr))
		visit_expression(previous);

	StructSymbol* currentType = (previous == nullptr) ? get_owner_type() : get_expression_type(previous);
	StructSymbol* type = analyze_invokation_expression(node, currentType);

	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}
