#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <gmt/Containers.hpp>

using namespace shard;

namespace
{
	// unwraps an expected type into a delegate signature; a generic
	// instantiation keeps its substitution context for parameter types
	DelegateTypeSymbol* unwrap_expected_delegate(StructSymbol* type, GenericTypeSymbol*& outInstantiation)
	{
		outInstantiation = nullptr;
		if (type == nullptr)
			return nullptr;

		if (type->get_kind() == SyntaxKind::DelegateType)
			return static_cast<DelegateTypeSymbol*>(type);

		if (type->get_kind() == SyntaxKind::GenericType)
		{
			GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
			StructSymbol* underlaying = generic->get_underlaying_type();
			if (!(underlaying == nullptr) && underlaying->get_kind() == SyntaxKind::DelegateType)
			{
				outInstantiation = generic;
				return static_cast<DelegateTypeSymbol*>(underlaying);
			}
		}

		return nullptr;
	}
}

bool ExpressionBinder::is_pending_lambda(const ExpressionSyntax* expression) const
{
	if ((expression == nullptr) || expression->get_kind() != SyntaxKind::LambdaExpression)
		return false;

	return m_pendingLambdas.contains(static_cast<const LambdaExpressionSyntax*>(expression));
}

void ExpressionBinder::bind_lambda_body(const LambdaExpressionSyntax* node, const LambdaExpressionSyntax* nodeRef, DelegateTypeSymbol* delegateType, MethodSymbol* anonymousMethod)
{
	push_scope(anonymousMethod);

	for (ParameterSymbol* const& parameter : anonymousMethod->get_parameters())
		get_current_scope()->declare_symbol(parameter);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);

	// infer the return type from the bound body: arrow clauses carry the
	// value directly, statement bodies return through return statements
	StructSymbol* returnType = nullptr;
	const BlockSyntax* body = node->get_body();
	if (!(body == nullptr) && body->get_kind() == SyntaxKind::ArrowClause)
	{
		const ExpressionSyntax* arrowExpression = gmt::as<ArrowClauseSyntax>(body)->get_expression();
		if (!(arrowExpression == nullptr))
			returnType = get_expression_type(arrowExpression);
	}
	else if (!(body == nullptr))
		returnType = infer_return_type_from_body(body);

	if (returnType == nullptr)
		returnType = get_symbol_table().get_void_primitive();

	anonymousMethod->set_return_type(returnType);
	delegateType->set_return_type(returnType);

	pop_scope();

	set_expression_type(nodeRef, delegateType);
}

bool ExpressionBinder::try_bind_pending_lambda(const LambdaExpressionSyntax* node, StructSymbol* expectedType)
{
	GenericTypeSymbol* instantiation = nullptr;
	DelegateTypeSymbol* expected = unwrap_expected_delegate(expectedType, instantiation);
	if (expected == nullptr)
		return false;

	DelegateTypeSymbol* delegateType = lookup_symbol<DelegateTypeSymbol>(node);
	if (delegateType == nullptr)
		return false;

	const LambdaExpressionSyntax* nodePtr = node;
	MethodSymbol* anonymousMethod = nullptr;
	if (!(nodePtr->get_parameters() == nullptr))
		anonymousMethod = lookup_symbol<MethodSymbol>(nodePtr->get_parameters());

	if (anonymousMethod == nullptr)
		return false;

	const gmt::vector<ParameterSymbol*>& parameters = anonymousMethod->get_parameters();
	const gmt::vector<ParameterSymbol*>& expectedParameters = expected->get_parameters();
	if (parameters.size() != expectedParameters.size())
		return false;

	// every expected parameter type must be concrete: a signature that still
	// mentions an unresolved type parameter gives the body nothing to bind
	// against
	for (ParameterSymbol* const& expectedParameter : expectedParameters)
	{
		StructSymbol* parameterType = expectedParameter->get_type();
		if (!(instantiation == nullptr))
			parameterType = substitute_type_parameters(parameterType, instantiation);

		if ((parameterType == nullptr) || parameterType->get_kind() == SyntaxKind::TypeParameter)
			return false;
	}

	for (std::size_t i = 0; i < parameters.size(); i++)
	{
		StructSymbol* parameterType = expectedParameters[i]->get_type();
		if (!(instantiation == nullptr))
			parameterType = substitute_type_parameters(parameterType, instantiation);

		parameters[i]->set_type(parameterType);
	}

	m_pendingLambdas.erase(node);
	bind_lambda_body(nodePtr, node, delegateType, anonymousMethod);
	return true;
}

void ExpressionBinder::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	std::optional<const LambdaExpressionSyntax*> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	DelegateTypeSymbol* delegateType = lookup_symbol<DelegateTypeSymbol>(*nodeRef);
	if (delegateType == nullptr)
		return;

	// the anonymous method is bound to the parameters list node by the declaration collector
	MethodSymbol* anonymousMethod = nullptr;
	if (!(node->get_parameters() == nullptr))
		anonymousMethod = lookup_symbol<MethodSymbol>(node->get_parameters());

	if (anonymousMethod == nullptr)
	{
		// fallback for a lambda that was not collected (should not happen)
		anonymousMethod = m_factory.create_method(L"Lambda", get_current_scope()->get_owner(), nullptr, LINK_STATIC);
		if (!(node->get_parameters() == nullptr))
			get_symbol_table().bind_symbol<MethodSymbol>(node->get_parameters(), anonymousMethod);
	}

	bool hasUntypedParameters = false;
	for (ParameterSymbol* const& parameter : anonymousMethod->get_parameters())
	{
		if ((parameter->get_type() == nullptr))
		{
			hasUntypedParameters = true;
			break;
		}
	}

	if (hasUntypedParameters)
	{
		// a target type from the denotation channel (a variable or assignment
		// the lambda initializes directly) binds the signature immediately
		StructSymbol* expectedType = resolve_left_denotation();
		if (!(expectedType == nullptr) && try_bind_pending_lambda(*nodeRef, expectedType))
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

	std::optional<const AwaitExpressionSyntax*> nodeRef = ref_from(node);

	MethodSymbol* hostMethod = find_host_method_symbol();
	bool hostIsAsync = !(hostMethod == nullptr) && hostMethod->get_is_async();
	if (!hostIsAsync)
	{
		get_diagnostics().report_error(node->get_await_keyword(), L"await can only be used inside async methods");
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, nullptr);

		return;
	}

	StructSymbol* awaitedType = get_expression_type(node->get_expression());
	if (awaitedType == nullptr)
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, nullptr);

		return;
	}

	// TODO: resolve the awaitable pattern (IAwaitable/IAwaiter) once the async library types exist
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, nullptr);
}
