#include <shard/pragmatic/EscapeAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>

#include <shard/semantic/SemanticModel.hpp>

using namespace shard;

void EscapeAnalyzer::visit_await_expression(const AwaitExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (in_frame())
	{
		Frame& frame = current_frame();
		frame.seenAwait = true;

		for (int loopId : frame.loopStack)
			frame.loopsWithAwait.insert(loopId);
	}

	SyntaxVisitor::visit_await_expression(node);
}

void EscapeAnalyzer::visit_while_statement(const WhileStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bool pushed = in_frame();
	if (pushed)
		current_frame().loopStack.push_back(m_nextLoopId++);

	SyntaxVisitor::visit_while_statement(node);

	if (pushed)
		current_frame().loopStack.pop_back();
}

void EscapeAnalyzer::visit_until_statement(const UntilStatementSyntax* node)
{
	if (node == nullptr)
		return;

	bool pushed = in_frame();
	if (pushed)
		current_frame().loopStack.push_back(m_nextLoopId++);

	SyntaxVisitor::visit_until_statement(node);

	if (pushed)
		current_frame().loopStack.pop_back();
}

void EscapeAnalyzer::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	if (node != nullptr && m_inDefer == 0)
	{
		gmt::Ref<const ExpressionSyntax> expression = node->get_expression();

		bool isInvokation = !expression.is_null();
		isInvokation = isInvokation && expression->get_kind() == SyntaxKind::InvokationExpression;

		if (isInvokation)
		{
			gmt::Ref<StructSymbol> expressionType = get_model().get_expression_type(expression);
			bool isTaskLike = get_model().is_task_or_task_like(expressionType);

			if (isTaskLike)
			{
				// a discarded task publishes its receiver and arguments past
				// the frame (E3 + E4 as one implicit channel)
				EscapeChannel publication = EscapeChannel::Capture | EscapeChannel::Suspension;

				gmt::Ref<const InvokationExpressionSyntax> invokation = expression.as<const InvokationExpressionSyntax>();

				gmt::Ref<const ExpressionSyntax> receiver = invokation->get_previous();
				if (!receiver.is_null())
					expose_expression(receiver, EscapeLevel::GlobalEscape, publication, true);

				gmt::Ref<const ArgumentsListSyntax> arguments = invokation->get_arguments();
				if (!arguments.is_null())
				{
					for (const gmt::Ref<ArgumentSyntax>& argument : arguments->get_arguments().as_span())
					{
						if (!argument.is_null())
							expose_expression(argument->get_expression(), EscapeLevel::GlobalEscape, publication);
					}
				}
			}
		}
	}

	SyntaxVisitor::visit_expression_statement(node);
}
