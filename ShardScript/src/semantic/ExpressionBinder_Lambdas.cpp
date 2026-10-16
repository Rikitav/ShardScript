#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <vector>

using namespace shard;

namespace
{
	// walks the bound body without side effects, so the lambda return type
	// can be inferred from the value-returning statements
	class return_expression_collector final : public SyntaxVisitor
	{
		std::vector<gmt::Ref<const ExpressionSyntax>>& m_expressions;

	public:
		explicit return_expression_collector(std::vector<gmt::Ref<const ExpressionSyntax>>& expressions)
			: m_expressions(expressions) { }

		void visit_return_statement(const ReturnStatementSyntax* node) override
		{
			if (!node->get_expression().is_null())
				m_expressions.push_back(node->get_expression());

			SyntaxVisitor::visit_return_statement(node);
		}
	};
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
	{
		std::vector<gmt::Ref<const ExpressionSyntax>> returns;
		return_expression_collector collector(returns);
		body.as_ptr()->accept(collector);

		for (gmt::Ref<const ExpressionSyntax> expression : returns)
		{
			gmt::Ref<StructSymbol> expressionType = get_expression_type(expression);
			if (!expressionType.is_null())
			{
				returnType = expressionType;
				break;
			}
		}
	}

	if (returnType.is_null())
		returnType = get_symbol_table().get_void_primitive();

	anonymousMethod->set_return_type(returnType);
	delegateType->set_return_type(returnType);

	pop_scope();

	set_expression_type(*nodeRef, delegateType);
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
