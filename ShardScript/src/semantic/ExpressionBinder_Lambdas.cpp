#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

using namespace shard;

namespace
{
	// unwraps an expected type into a delegate signature; a generic
	// instantiation keeps its substitution context for parameter types
	gmt::Ref<DelegateTypeSymbol> unwrap_expected_delegate(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol>& outInstantiation)
	{
		outInstantiation = gmt::nullref;
		if (type.is_null())
			return gmt::nullref;

		if (type->get_kind() == SyntaxKind::DelegateType)
			return type.as<DelegateTypeSymbol>();

		if (type->get_kind() == SyntaxKind::GenericType)
		{
			gmt::Ref<GenericTypeSymbol> generic = type.as<GenericTypeSymbol>();
			gmt::Ref<StructSymbol> underlaying = generic->get_underlaying_type();
			if (!underlaying.is_null() && underlaying->get_kind() == SyntaxKind::DelegateType)
			{
				outInstantiation = generic;
				return underlaying.as<DelegateTypeSymbol>();
			}
		}

		return gmt::nullref;
	}
}

bool ExpressionBinder::is_pending_lambda(gmt::Ref<const ExpressionSyntax> expression) const
{
	if (expression.is_null() || expression->get_kind() != SyntaxKind::LambdaExpression)
		return false;

	return m_pendingLambdas.contains(expression.as<const LambdaExpressionSyntax>());
}

void ExpressionBinder::bind_lambda_body(const LambdaExpressionSyntax* node, gmt::Ref<const LambdaExpressionSyntax> nodeRef, gmt::Ref<DelegateTypeSymbol> delegateType, gmt::Ref<MethodSymbol> anonymousMethod)
{
	push_scope(anonymousMethod);

	for (const gmt::Ref<ParameterSymbol>& parameter : anonymousMethod->get_parameters())
		get_current_scope()->declare_symbol(parameter);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);

	// infer the return type from the bound body: arrow clauses carry the
	// value directly, statement bodies return through return statements
	gmt::Ref<StructSymbol> returnType = gmt::nullref;
	gmt::Ref<const BlockSyntax> body = node->get_body();
	if (!body.is_null() && body->get_kind() == SyntaxKind::ArrowClause)
	{
		gmt::Ref<const ExpressionSyntax> arrowExpression = body.as<ArrowClauseSyntax>()->get_expression();
		if (!arrowExpression.is_null())
			returnType = get_expression_type(arrowExpression);
	}
	else if (!body.is_null())
		returnType = infer_return_type_from_body(body);

	if (returnType.is_null())
		returnType = get_symbol_table().get_void_primitive();

	anonymousMethod->set_return_type(returnType);
	delegateType->set_return_type(returnType);

	pop_scope();

	set_expression_type(nodeRef, delegateType);
}

bool ExpressionBinder::try_bind_pending_lambda(gmt::Ref<const LambdaExpressionSyntax> node, gmt::Ref<StructSymbol> expectedType)
{
	gmt::Ref<GenericTypeSymbol> instantiation = gmt::nullref;
	gmt::Ref<DelegateTypeSymbol> expected = unwrap_expected_delegate(expectedType, instantiation);
	if (expected.is_null())
		return false;

	gmt::Ref<DelegateTypeSymbol> delegateType = lookup_symbol<DelegateTypeSymbol>(node);
	if (delegateType.is_null())
		return false;

	const LambdaExpressionSyntax* nodePtr = node.as_ptr();
	gmt::Ref<MethodSymbol> anonymousMethod = gmt::nullref;
	if (!nodePtr->get_parameters().is_null())
		anonymousMethod = lookup_symbol<MethodSymbol>(nodePtr->get_parameters());

	if (anonymousMethod.is_null())
		return false;

	const std::vector<gmt::Ref<ParameterSymbol>>& parameters = anonymousMethod->get_parameters();
	const std::vector<gmt::Ref<ParameterSymbol>>& expectedParameters = expected->get_parameters();
	if (parameters.size() != expectedParameters.size())
		return false;

	// every expected parameter type must be concrete: a signature that still
	// mentions an unresolved type parameter gives the body nothing to bind
	// against
	for (const gmt::Ref<ParameterSymbol>& expectedParameter : expectedParameters)
	{
		gmt::Ref<StructSymbol> parameterType = expectedParameter->get_type();
		if (!instantiation.is_null())
			parameterType = substitute_type_parameters(parameterType, instantiation);

		if (parameterType.is_null() || parameterType->get_kind() == SyntaxKind::TypeParameter)
			return false;
	}

	for (std::size_t i = 0; i < parameters.size(); i++)
	{
		gmt::Ref<StructSymbol> parameterType = expectedParameters[i]->get_type();
		if (!instantiation.is_null())
			parameterType = substitute_type_parameters(parameterType, instantiation);

		parameters[i]->set_type(parameterType);
	}

	m_pendingLambdas.erase(node);
	bind_lambda_body(nodePtr, node, delegateType, anonymousMethod);
	return true;
}

void ExpressionBinder::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	std::optional<gmt::Ref<const LambdaExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	gmt::Ref<DelegateTypeSymbol> delegateType = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if (delegateType.is_null())
		return;

	// the anonymous method is bound to the parameters list node by the declaration collector
	gmt::Ref<MethodSymbol> anonymousMethod = gmt::nullref;
	if (!node->get_parameters().is_null())
		anonymousMethod = lookup_symbol<MethodSymbol>(node->get_parameters());

	if (anonymousMethod.is_null())
	{
		// fallback for a lambda that was not collected (should not happen)
		anonymousMethod = m_factory.create_method(L"Lambda", get_current_scope()->get_owner(), gmt::nullref, LINK_STATIC);
		if (!node->get_parameters().is_null())
			get_symbol_table().bind_symbol<MethodSymbol>(node->get_parameters(), anonymousMethod);
	}

	bool hasUntypedParameters = false;
	for (const gmt::Ref<ParameterSymbol>& parameter : anonymousMethod->get_parameters())
	{
		if (parameter->get_type().is_null())
		{
			hasUntypedParameters = true;
			break;
		}
	}

	if (hasUntypedParameters)
	{
		// a target type from the denotation channel (a variable or assignment
		// the lambda initializes directly) binds the signature immediately
		gmt::Ref<StructSymbol> expectedType = resolve_left_denotation();
		if (!expectedType.is_null() && try_bind_pending_lambda(*nodeRef, expectedType))
			return;

		// otherwise the body stays unbound until overload resolution picks a
		// method whose parameter supplies the delegate signature
		m_pendingLambdas.insert(*nodeRef);
		set_expression_type(*nodeRef, delegateType);
		return;
	}

	bind_lambda_body(node, *nodeRef, delegateType, anonymousMethod);
}

void ExpressionBinder::visit_await_expression(const AwaitExpressionSyntax* node)
{
	visit_expression(node->get_expression());

	std::optional<gmt::Ref<const AwaitExpressionSyntax>> nodeRef = ref_from(node);

	gmt::Ref<MethodSymbol> hostMethod = find_host_method_symbol();
	bool hostIsAsync = !hostMethod.is_null() && hostMethod->get_is_async();
	if (!hostIsAsync)
	{
		get_diagnostics().report_error(node->get_await_keyword(), L"await can only be used inside async methods");
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, gmt::nullref);

		return;
	}

	gmt::Ref<StructSymbol> awaitedType = get_expression_type(node->get_expression());
	if (awaitedType.is_null())
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, gmt::nullref);

		return;
	}

	// TODO: resolve the awaitable pattern (IAwaitable/IAwaiter) once the async library types exist
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, gmt::nullref);
}
