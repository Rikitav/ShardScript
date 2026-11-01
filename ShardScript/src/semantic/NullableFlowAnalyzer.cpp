#include <shard/semantic/NullableFlowAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>
#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/nodes/statements/ElseStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/IfStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/UnlessStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>

#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LiteralExpressionsSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>

#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Containers.hpp>

#include <utility>

using namespace shard;

namespace
{
	bool is_assignment_token(TokenType type)
	{
		switch (type)
		{
			case TokenType::AssignOperator:
			case TokenType::AddAssignOperator:
			case TokenType::SubAssignOperator:
			case TokenType::MultAssignOperator:
			case TokenType::DivAssignOperator:
			case TokenType::ModAssignOperator:
			case TokenType::PowAssignOperator:
				return true;

			default:
				return false;
		}
	}

	bool is_nullable(StructSymbol* type)
	{
		return !(type == nullptr) && type->get_kind() == SyntaxKind::NullableType;
	}
}

NullableFlowAnalyzer::NullableFlowAnalyzer(SemanticModel& model, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics)
{
}

void NullableFlowAnalyzer::analyze(SyntaxTree& tree)
{
	visit_syntax_tree(tree);
}

NullableFlowAnalyzer::NullState NullableFlowAnalyzer::join(NullState left, NullState right)
{
	if (left == right)
		return left;

	return NullState::MaybeNull;
}

NullableFlowAnalyzer::State NullableFlowAnalyzer::merge(const State& left, const State& right)
{
	State result;

	for (const auto& pair : left)
	{
		auto choice = right.find(pair.first);
		NullState other = choice != right.end() ? choice->second : NullState::MaybeNull;
		result[pair.first] = join(pair.second, other);
	}

	for (const auto& pair : right)
	{
		if (left.find(pair.first) == left.end())
			result[pair.first] = NullState::MaybeNull;
	}

	return result;
}

StructSymbol* NullableFlowAnalyzer::symbol_type(SyntaxSymbol* symbol) const
{
	if (symbol == nullptr)
		return nullptr;

	switch (symbol->get_kind())
	{
		case SyntaxKind::VariableStatement:
			return static_cast<VariableSymbol*>(symbol)->get_type();

		case SyntaxKind::Parameter:
			return static_cast<ParameterSymbol*>(symbol)->get_type();

		default:
			return nullptr;
	}
}

SyntaxSymbol* NullableFlowAnalyzer::tracked_root(const ExpressionSyntax* expression)
{
	if ((expression == nullptr) || expression->get_kind() != SyntaxKind::IdentifierNameType)
		return nullptr;

	SyntaxSymbol* symbol = get_symbol_table().lookup_symbol(expression);
	if ((symbol == nullptr) || !is_nullable(symbol_type(symbol)))
		return nullptr;

	return symbol;
}

NullableFlowAnalyzer::NullState NullableFlowAnalyzer::state_of_expression(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return NullState::MaybeNull;

	SemanticModel& model = get_model();
	StructSymbol* type = model.get_expression_type(expression);

	if (model.is_null_type(type))
		return NullState::Null;

	NullableConversion conversion = model.get_nullable_conversion(expression);
	if (conversion.kind == NullableConversionKind::Wrap)
		return NullState::NotNull;

	if (conversion.kind == NullableConversionKind::Default)
		return NullState::Null;

	switch (expression->get_kind())
	{
		case SyntaxKind::IdentifierNameType:
		{
			SyntaxSymbol* symbol = tracked_root(expression);
			if (symbol == nullptr)
				break;

			auto choice = m_state.find(symbol);
			return choice != m_state.end() ? choice->second : NullState::MaybeNull;
		}

		case SyntaxKind::MemberAccessExpression:
		{
			const MemberAccessExpressionSyntax* member = gmt::as<MemberAccessExpressionSyntax>(expression);
			if (member->get_identifier().get_lexeme() == L"Value")
				return NullState::NotNull;

			break;
		}

		case SyntaxKind::BinaryExpression:
		{
			const BinaryExpressionSyntax* binary = gmt::as<BinaryExpressionSyntax>(expression);
			if (binary->get_operator_token().get_type() != TokenType::NullCoalescingOperator)
				break;

			NullState leftState = state_of_expression(binary->get_left());
			if (leftState == NullState::NotNull)
				return NullState::NotNull;

			NullState rightState = state_of_expression(binary->get_right());
			if (leftState == NullState::Null)
				return rightState;

			return rightState == NullState::NotNull ? NullState::NotNull : NullState::MaybeNull;
		}

		case SyntaxKind::IfExpression:
		{
			const IfExpressionSyntax* conditional = gmt::as<IfExpressionSyntax>(expression);
			NullState thenState = state_of_expression(conditional->get_then_expression());
			NullState elseState = state_of_expression(conditional->get_else_expression());
			return join(thenState, elseState);
		}

		default:
			break;
	}

	if (type == nullptr)
		return NullState::MaybeNull;

	if (!is_nullable(type))
		return NullState::NotNull;

	return NullState::MaybeNull;
}

void NullableFlowAnalyzer::report_unwrap(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	if (!m_reported.insert(expression).second)
		return;

	switch (state_of_expression(expression))
	{
		case NullState::NotNull:
			return;

		case NullState::Null:
			get_diagnostics().report_error(expression->get_location(), L"nullable value is always null here");
			return;

		default:
			get_diagnostics().report_error(expression->get_location(), L"cannot prove that this nullable value is non-null; check for null, use ??, or use '.Value'");
			return;
	}
}

void NullableFlowAnalyzer::apply_assignment(const BinaryExpressionSyntax* node)
{
	const ExpressionSyntax* target = node->get_left();
	SyntaxSymbol* symbol = get_symbol_table().lookup_symbol(target);
	if ((symbol == nullptr) || !is_nullable(symbol_type(symbol)))
		return;

	m_state[symbol] = state_of_expression(node->get_right());
}

void NullableFlowAnalyzer::analyze_expression(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	if (get_model().get_nullable_conversion(expression).kind == NullableConversionKind::Unwrap)
		report_unwrap(expression);

	switch (expression->get_kind())
	{
		case SyntaxKind::BinaryExpression:
		{
			const BinaryExpressionSyntax* binary = gmt::as<BinaryExpressionSyntax>(expression);
			TokenType op = binary->get_operator_token().get_type();

			if (is_assignment_token(op))
			{
				analyze_expression(binary->get_right());
				analyze_expression(binary->get_left());

				if (op == TokenType::AssignOperator)
					apply_assignment(binary);

				break;
			}

			analyze_expression(binary->get_left());
			analyze_expression(binary->get_right());
			break;
		}

		case SyntaxKind::UnaryExpression:
		{
			analyze_expression(gmt::as<UnaryExpressionSyntax>(expression)->get_operand());
			break;
		}

		case SyntaxKind::MemberAccessExpression:
		{
			analyze_expression(gmt::as<MemberAccessExpressionSyntax>(expression)->get_previous());
			break;
		}

		case SyntaxKind::InvokationExpression:
		{
			const InvokationExpressionSyntax* invocation = gmt::as<InvokationExpressionSyntax>(expression);
			analyze_expression(invocation->get_previous());

			if (!(invocation->get_arguments() == nullptr))
			{
				for (ArgumentSyntax* const& argument : invocation->get_arguments()->get_arguments())
				{
					if (!(argument == nullptr))
						analyze_expression(argument->get_expression());
				}
			}

			break;
		}

		case SyntaxKind::IndexatorExpression:
		{
			const IndexatorExpressionSyntax* indexator = gmt::as<IndexatorExpressionSyntax>(expression);
			analyze_expression(indexator->get_previous());

			if (!(indexator->get_arguments() == nullptr))
			{
				for (ArgumentSyntax* const& argument : indexator->get_arguments()->get_arguments())
				{
					if (!(argument == nullptr))
						analyze_expression(argument->get_expression());
				}
			}

			break;
		}

		case SyntaxKind::IfExpression:
		{
			const IfExpressionSyntax* conditional = gmt::as<IfExpressionSyntax>(expression);
			ConditionalStates condition = analyze_condition(conditional->get_condition());

			m_state = condition.whenTrue;
			analyze_expression(conditional->get_then_expression());
			State thenState = m_state;

			m_state = condition.whenFalse;
			analyze_expression(conditional->get_else_expression());
			State elseState = m_state;

			m_state = merge(thenState, elseState);
			break;
		}

		case SyntaxKind::LambdaExpression:
			analyze_lambda(gmt::as<LambdaExpressionSyntax>(expression));
			break;

		case SyntaxKind::CollectionExpression:
		{
			for (ExpressionSyntax* const& value : gmt::as<CollectionExpressionSyntax>(expression)->get_values())
				analyze_expression(value);

			break;
		}

		case SyntaxKind::RangeExpression:
		{
			const RangeExpressionSyntax* range = gmt::as<RangeExpressionSyntax>(expression);
			analyze_expression(range->get_left());
			analyze_expression(range->get_right());
			break;
		}

		case SyntaxKind::ObjectExpression:
		{
			const ObjectExpressionSyntax* object = gmt::as<ObjectExpressionSyntax>(expression);
			analyze_expression(object->get_array_size());

			if (!(object->get_arguments() == nullptr))
			{
				for (ArgumentSyntax* const& argument : object->get_arguments()->get_arguments())
				{
					if (!(argument == nullptr))
						analyze_expression(argument->get_expression());
				}
			}

			break;
		}

		case SyntaxKind::AwaitExpression:
			analyze_expression(gmt::as<AwaitExpressionSyntax>(expression)->get_expression());
			break;

		case SyntaxKind::CastExpression:
			analyze_expression(gmt::as<CastExpressionSyntax>(expression)->get_expression());
			break;

		case SyntaxKind::IsExpression:
			analyze_expression(gmt::as<IsExpressionSyntax>(expression)->get_expression());
			break;

		default:
			break;
	}
}

NullableFlowAnalyzer::ConditionalStates NullableFlowAnalyzer::analyze_condition(const ExpressionSyntax* expression)
{
	State incoming = m_state;
	ConditionalStates result{ incoming, incoming };

	if (expression == nullptr)
		return result;

	SemanticModel& model = get_model();

	if (expression->get_kind() == SyntaxKind::BinaryExpression)
	{
		const BinaryExpressionSyntax* binary = gmt::as<BinaryExpressionSyntax>(expression);
		TokenType op = binary->get_operator_token().get_type();

		if (op == TokenType::EqualsOperator || op == TokenType::NotEqualsOperator)
		{
			const ExpressionSyntax* left = binary->get_left();
			const ExpressionSyntax* right = binary->get_right();
			StructSymbol* leftType = model.get_expression_type(left);
			StructSymbol* rightType = model.get_expression_type(right);

			const ExpressionSyntax* valueSide = nullptr;
			if (model.is_null_type(rightType) && !model.is_null_type(leftType))
				valueSide = left;
			else if (model.is_null_type(leftType) && !model.is_null_type(rightType))
				valueSide = right;

			SyntaxSymbol* local = tracked_root(valueSide);
			if (!(local == nullptr))
			{
				analyze_expression(valueSide);

				bool checkIsNull = op == TokenType::EqualsOperator;
				State nullState = m_state;
				nullState[local] = NullState::Null;

				State valueState = m_state;
				valueState[local] = NullState::NotNull;

				m_state = incoming;
				result.whenTrue = checkIsNull ? nullState : valueState;
				result.whenFalse = checkIsNull ? valueState : nullState;
				return result;
			}
		}

		if (op == TokenType::AndOperator || op == TokenType::OrOperator)
		{
			ConditionalStates left = analyze_condition(binary->get_left());

			m_state = op == TokenType::AndOperator ? left.whenTrue : left.whenFalse;
			ConditionalStates right = analyze_condition(binary->get_right());
			m_state = incoming;

			if (op == TokenType::AndOperator)
			{
				result.whenTrue = merge(left.whenTrue, right.whenTrue);
				result.whenFalse = merge(left.whenFalse, right.whenFalse);
			}
			else
			{
				result.whenTrue = merge(left.whenTrue, right.whenTrue);
				result.whenFalse = merge(left.whenFalse, right.whenFalse);
			}

			return result;
		}
	}

	if (expression->get_kind() == SyntaxKind::UnaryExpression)
	{
		const UnaryExpressionSyntax* unary = gmt::as<UnaryExpressionSyntax>(expression);
		if (unary->get_operator_token().get_type() == TokenType::NotOperator)
		{
			ConditionalStates inner = analyze_condition(unary->get_operand());
			result.whenTrue = inner.whenFalse;
			result.whenFalse = inner.whenTrue;
			return result;
		}
	}

	if (expression->get_kind() == SyntaxKind::MemberAccessExpression)
	{
		const MemberAccessExpressionSyntax* member = gmt::as<MemberAccessExpressionSyntax>(expression);
		if (member->get_identifier().get_lexeme() == L"HasValue")
		{
			SyntaxSymbol* local = tracked_root(member->get_previous());
			if (!(local == nullptr))
			{
				analyze_expression(member->get_previous());

				State nullState = m_state;
				nullState[local] = NullState::Null;

				State valueState = m_state;
				valueState[local] = NullState::NotNull;

				m_state = incoming;
				result.whenTrue = valueState;
				result.whenFalse = nullState;
				return result;
			}
		}
	}

	analyze_expression(expression);
	result.whenTrue = m_state;
	result.whenFalse = m_state;
	return result;
}

bool NullableFlowAnalyzer::analyze_conditional_clause(const ConditionalClauseSyntax* clause, bool negate)
{
	if (clause == nullptr)
		return true;

	ConditionalStates condition = analyze_condition(clause->get_condition());
	if (negate)
	{
		State swap = condition.whenTrue;
		condition.whenTrue = condition.whenFalse;
		condition.whenFalse = swap;
	}

	bool anyReachable = false;
	State result;

	m_state = condition.whenTrue;
	if (analyze_block(clause->get_block()))
	{
		result = m_state;
		anyReachable = true;
	}

	m_state = condition.whenFalse;
	if (analyze_clause(clause->get_next()))
	{
		result = anyReachable ? merge(result, m_state) : m_state;
		anyReachable = true;
	}

	m_state = result;
	return anyReachable;
}

bool NullableFlowAnalyzer::analyze_clause(const ConditionalClauseBaseSyntax* clause)
{
	if (clause == nullptr)
		return true;

	switch (clause->get_kind())
	{
		case SyntaxKind::IfStatement:
			return analyze_conditional_clause(gmt::as<ConditionalClauseSyntax>(clause), false);

		case SyntaxKind::UnlessStatement:
			return analyze_conditional_clause(gmt::as<ConditionalClauseSyntax>(clause), true);

		default:
			return analyze_block(clause->get_block());
	}
}

bool NullableFlowAnalyzer::analyze_statement(const StatementSyntax* statement)
{
	if (statement == nullptr)
		return true;

	switch (statement->get_kind())
	{
		case SyntaxKind::VariableStatement:
		{
			const VariableStatementSyntax* variable = gmt::as<VariableStatementSyntax>(statement);
			analyze_expression(variable->get_expression());

			SyntaxSymbol* symbol = get_symbol_table().lookup_symbol(statement);
			if ((!(symbol == nullptr)) && is_nullable(symbol_type(symbol)))
			{
				if (variable->get_expression() == nullptr)
					m_state[symbol] = NullState::Null;
				else
					m_state[symbol] = state_of_expression(variable->get_expression());
			}

			return true;
		}

		case SyntaxKind::ExpressionStatement:
			analyze_expression(gmt::as<ExpressionStatementSyntax>(statement)->get_expression());
			return true;

		case SyntaxKind::IfStatement:
			return analyze_conditional_clause(gmt::as<ConditionalClauseSyntax>(statement), false);

		case SyntaxKind::UnlessStatement:
			return analyze_conditional_clause(gmt::as<ConditionalClauseSyntax>(statement), true);

		case SyntaxKind::WhileStatement:
		case SyntaxKind::UntilStatement:
		{
			const ExpressionSyntax* conditionExpression = nullptr;
			const StatementsBlockSyntax* body = nullptr;
			bool negate = false;

			if (statement->get_kind() == SyntaxKind::WhileStatement)
			{
				const WhileStatementSyntax* loop = gmt::as<WhileStatementSyntax>(statement);
				conditionExpression = loop->get_condition();
				body = loop->get_block();
			}
			else
			{
				const UntilStatementSyntax* loop = gmt::as<UntilStatementSyntax>(statement);
				conditionExpression = loop->get_condition();
				body = loop->get_block();
				negate = true;
			}

			gmt::unordered_set<SyntaxSymbol*> assigned;
			collect_assigned(body, assigned);

			State loopHead = m_state;
			widen_assigned(assigned);

			ConditionalStates condition = analyze_condition(conditionExpression);
			if (negate)
			{
				State swap = condition.whenTrue;
				condition.whenTrue = condition.whenFalse;
				condition.whenFalse = swap;
			}

			m_state = condition.whenTrue;
			analyze_block(body);
			widen_assigned(assigned);

			m_state = merge(loopHead, m_state);
			m_state = merge(m_state, condition.whenFalse);
			return true;
		}

		case SyntaxKind::ForInStatement:
		{
			const ForInStatementSyntax* loop = gmt::as<ForInStatementSyntax>(statement);
			analyze_expression(loop->get_range_expression());

			gmt::unordered_set<SyntaxSymbol*> assigned;
			collect_assigned(loop->get_block(), assigned);
			widen_assigned(assigned);
			analyze_block(loop->get_block());
			widen_assigned(assigned);
			return true;
		}

		case SyntaxKind::ReturnStatement:
			analyze_expression(gmt::as<ReturnStatementSyntax>(statement)->get_expression());
			return false;

		case SyntaxKind::ThrowStatement:
			analyze_expression(gmt::as<ThrowStatementSyntax>(statement)->get_expression());
			return false;

		case SyntaxKind::BreakStatement:
		case SyntaxKind::ContinueStatement:
			return false;

		case SyntaxKind::TryStatement:
		{
			const TryStatementSyntax* tryStatement = gmt::as<TryStatementSyntax>(statement);
			gmt::unordered_set<SyntaxSymbol*> assigned;
			collect_assigned(tryStatement->get_try_block(), assigned);

			State incoming = m_state;
			bool anyReachable = analyze_block(tryStatement->get_try_block());
			State result = m_state;

			for (CatchClauseSyntax* const& clause : tryStatement->get_catch_clauses())
			{
				if (clause == nullptr)
					continue;

				m_state = incoming;
				widen_assigned(assigned);

				if (analyze_block(clause->get_body()))
				{
					result = anyReachable ? merge(result, m_state) : m_state;
					anyReachable = true;
				}
			}

			m_state = result;
			return anyReachable;
		}

		case SyntaxKind::DeferStatement:
		{
			State saved = m_state;
			analyze_expression(gmt::as<DeferStatementSyntax>(statement)->get_expression());
			m_state = saved;
			return true;
		}

		case SyntaxKind::DeferBlock:
		{
			State saved = m_state;
			analyze_block(gmt::as<DeferBlockSyntax>(statement)->get_block());
			m_state = saved;
			return true;
		}

		case SyntaxKind::DeferVariable:
		{
			State saved = m_state;
			const DeferVariableSyntax* defer = gmt::as<DeferVariableSyntax>(statement);
			if (!(defer->get_variable() == nullptr))
				analyze_statement(defer->get_variable());

			m_state = saved;
			return true;
		}

		default:
			return true;
	}
}

bool NullableFlowAnalyzer::analyze_block(const BlockSyntax* block)
{
	if (block == nullptr)
		return true;

	switch (block->get_kind())
	{
		case SyntaxKind::StatementsBlock:
		{
			for (StatementSyntax* const& statement : gmt::as<StatementsBlockSyntax>(block)->get_statements())
			{
				if (!analyze_statement(statement))
					return false;
			}

			return true;
		}

		case SyntaxKind::ArrowClause:
			analyze_expression(gmt::as<ArrowClauseSyntax>(block)->get_expression());
			return false;

		default:
			return true;
	}
}

void NullableFlowAnalyzer::collect_assigned_expression(const ExpressionSyntax* expression, gmt::unordered_set<SyntaxSymbol*>& assigned)
{
	if (expression == nullptr)
		return;

	switch (expression->get_kind())
	{
		case SyntaxKind::BinaryExpression:
		{
			const BinaryExpressionSyntax* binary = gmt::as<BinaryExpressionSyntax>(expression);
			if (is_assignment_token(binary->get_operator_token().get_type()))
			{
				SyntaxSymbol* symbol = get_symbol_table().lookup_symbol(binary->get_left());
				if ((!(symbol == nullptr)) && is_nullable(symbol_type(symbol)))
					assigned.insert(symbol);
			}

			collect_assigned_expression(binary->get_left(), assigned);
			collect_assigned_expression(binary->get_right(), assigned);
			break;
		}

		case SyntaxKind::UnaryExpression:
			collect_assigned_expression(gmt::as<UnaryExpressionSyntax>(expression)->get_operand(), assigned);
			break;

		case SyntaxKind::MemberAccessExpression:
			collect_assigned_expression(gmt::as<MemberAccessExpressionSyntax>(expression)->get_previous(), assigned);
			break;

		case SyntaxKind::InvokationExpression:
		{
			const InvokationExpressionSyntax* invocation = gmt::as<InvokationExpressionSyntax>(expression);
			collect_assigned_expression(invocation->get_previous(), assigned);
			if (!(invocation->get_arguments() == nullptr))
			{
				for (ArgumentSyntax* const& argument : invocation->get_arguments()->get_arguments())
				{
					if (!(argument == nullptr))
						collect_assigned_expression(argument->get_expression(), assigned);
				}
			}

			break;
		}

		case SyntaxKind::IndexatorExpression:
		{
			const IndexatorExpressionSyntax* indexator = gmt::as<IndexatorExpressionSyntax>(expression);
			collect_assigned_expression(indexator->get_previous(), assigned);
			if (!(indexator->get_arguments() == nullptr))
			{
				for (ArgumentSyntax* const& argument : indexator->get_arguments()->get_arguments())
				{
					if (!(argument == nullptr))
						collect_assigned_expression(argument->get_expression(), assigned);
				}
			}

			break;
		}

		case SyntaxKind::IfExpression:
		{
			const IfExpressionSyntax* conditional = gmt::as<IfExpressionSyntax>(expression);
			collect_assigned_expression(conditional->get_condition(), assigned);
			collect_assigned_expression(conditional->get_then_expression(), assigned);
			collect_assigned_expression(conditional->get_else_expression(), assigned);
			break;
		}

		case SyntaxKind::AwaitExpression:
			collect_assigned_expression(gmt::as<AwaitExpressionSyntax>(expression)->get_expression(), assigned);
			break;

		case SyntaxKind::CastExpression:
			collect_assigned_expression(gmt::as<CastExpressionSyntax>(expression)->get_expression(), assigned);
			break;

		case SyntaxKind::IsExpression:
			collect_assigned_expression(gmt::as<IsExpressionSyntax>(expression)->get_expression(), assigned);
			break;

		default:
			break;
	}
}

void NullableFlowAnalyzer::collect_assigned(const SyntaxNode* node, gmt::unordered_set<SyntaxSymbol*>& assigned)
{
	if (node == nullptr)
		return;

	switch (node->get_kind())
	{
		case SyntaxKind::StatementsBlock:
		{
			for (StatementSyntax* const& child : gmt::as<StatementsBlockSyntax>(node)->get_statements())
				collect_assigned(child, assigned);

			break;
		}

		case SyntaxKind::ArrowClause:
			collect_assigned_expression(gmt::as<ArrowClauseSyntax>(node)->get_expression(), assigned);
			break;

		case SyntaxKind::VariableStatement:
		{
			SyntaxSymbol* symbol = get_symbol_table().lookup_symbol(node);
			if ((!(symbol == nullptr)) && is_nullable(symbol_type(symbol)))
				assigned.insert(symbol);

			collect_assigned_expression(gmt::as<VariableStatementSyntax>(node)->get_expression(), assigned);
			break;
		}

		case SyntaxKind::ExpressionStatement:
			collect_assigned_expression(gmt::as<ExpressionStatementSyntax>(node)->get_expression(), assigned);
			break;

		case SyntaxKind::IfStatement:
		case SyntaxKind::UnlessStatement:
		{
			const ConditionalClauseSyntax* clause = gmt::as<ConditionalClauseSyntax>(node);
			collect_assigned_expression(clause->get_condition(), assigned);
			collect_assigned(clause->get_block(), assigned);
			collect_assigned(clause->get_next(), assigned);
			break;
		}

		case SyntaxKind::ElseStatement:
			collect_assigned(gmt::as<ElseStatementSyntax>(node)->get_block(), assigned);
			break;

		case SyntaxKind::WhileStatement:
		{
			const WhileStatementSyntax* loop = gmt::as<WhileStatementSyntax>(node);
			collect_assigned_expression(loop->get_condition(), assigned);
			collect_assigned(loop->get_block(), assigned);
			break;
		}

		case SyntaxKind::UntilStatement:
		{
			const UntilStatementSyntax* loop = gmt::as<UntilStatementSyntax>(node);
			collect_assigned_expression(loop->get_condition(), assigned);
			collect_assigned(loop->get_block(), assigned);
			break;
		}

		case SyntaxKind::ForInStatement:
		{
			const ForInStatementSyntax* loop = gmt::as<ForInStatementSyntax>(node);
			collect_assigned_expression(loop->get_range_expression(), assigned);
			collect_assigned(loop->get_block(), assigned);
			break;
		}

		case SyntaxKind::ReturnStatement:
			collect_assigned_expression(gmt::as<ReturnStatementSyntax>(node)->get_expression(), assigned);
			break;

		case SyntaxKind::ThrowStatement:
			collect_assigned_expression(gmt::as<ThrowStatementSyntax>(node)->get_expression(), assigned);
			break;

		case SyntaxKind::TryStatement:
		{
			const TryStatementSyntax* tryStatement = gmt::as<TryStatementSyntax>(node);
			collect_assigned(tryStatement->get_try_block(), assigned);

			for (CatchClauseSyntax* const& clause : tryStatement->get_catch_clauses())
			{
				if (!(clause == nullptr))
					collect_assigned(clause->get_body(), assigned);
			}

			break;
		}

		case SyntaxKind::DeferStatement:
			collect_assigned_expression(gmt::as<DeferStatementSyntax>(node)->get_expression(), assigned);
			break;

		case SyntaxKind::DeferBlock:
			collect_assigned(gmt::as<DeferBlockSyntax>(node)->get_block(), assigned);
			break;

		default:
			break;
	}
}

void NullableFlowAnalyzer::widen_assigned(const gmt::unordered_set<SyntaxSymbol*>& assigned)
{
	for (SyntaxSymbol* symbol : assigned)
	{
		auto choice = m_state.find(symbol);
		if (choice != m_state.end())
			choice->second = NullState::MaybeNull;
	}
}

void NullableFlowAnalyzer::declare_parameters(MethodSymbol* method)
{
	if (method == nullptr)
		return;

	for (ParameterSymbol* const& parameter : method->get_parameters())
	{
		if (!(parameter == nullptr) && is_nullable(parameter->get_type()))
			m_state[parameter] = NullState::MaybeNull;
	}
}

void NullableFlowAnalyzer::analyze_member_body(MethodSymbol* method, const BlockSyntax* body)
{
	if ((method == nullptr) || (body == nullptr))
		return;

	m_state.clear();
	declare_parameters(method);
	analyze_block(body);
}

void NullableFlowAnalyzer::analyze_lambda(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	MethodSymbol* method = nullptr;
	if (!(node->get_parameters() == nullptr))
	{
		SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node->get_parameters());
		if ((!(bound == nullptr)) && bound->is_method())
			method = static_cast<MethodSymbol*>(bound);
	}

	State saved = m_state;
	analyze_member_body(method, node->get_body());
	m_state = saved;
}

void NullableFlowAnalyzer::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	MethodSymbol* method = lookup_symbol<MethodSymbol>(node);
	analyze_member_body(method, node->get_body());
}

void NullableFlowAnalyzer::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	MethodSymbol* method = lookup_symbol<MethodSymbol>(node);
	analyze_member_body(method, node->get_body());
}

void NullableFlowAnalyzer::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	MethodSymbol* method = lookup_symbol<MethodSymbol>(node);
	analyze_member_body(method, node->get_body());
}

void NullableFlowAnalyzer::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	AccessorSymbol* accessor = lookup_symbol<AccessorSymbol>(node);
	analyze_member_body(accessor, node->get_body());
}

void NullableFlowAnalyzer::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	analyze_lambda(node);
}
