#include <shard/semantic/ControlFlowAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/BlockSyntax.hpp>
#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/statements/ElseStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>

#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>

#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>

#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Concat.hpp>
#include <gmt/Containers.hpp>

using namespace shard;

ControlFlowAnalyzer::ControlFlowAnalyzer(SemanticModel& model, DiagnosticsContext& diagnostics)
	: SemanticVisitor(model, diagnostics)
{
}

void ControlFlowAnalyzer::analyze(SyntaxTree& tree)
{
	visit_syntax_tree(tree);
	propagate_effects();
}

void ControlFlowAnalyzer::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	const BlockSyntax* body = node->get_body();
	if (body == nullptr)
		return;

	enter_member_body(symbol);
	push_scope(symbol);
	declare_parameters(symbol);

	analyze_member_body(symbol, body, node->get_identifier().get_location());

	pop_scope();
	leave_member_body(symbol);
}

void ControlFlowAnalyzer::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	MethodSymbol* symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	const BlockSyntax* body = node->get_body();
	if (body == nullptr)
		return;

	enter_member_body(symbol);
	push_scope(symbol);
	declare_parameters(symbol);

	analyze_member_body(symbol, body, node->get_operator_token().get_location());

	pop_scope();
	leave_member_body(symbol);
}

void ControlFlowAnalyzer::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	auto nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return;

	ConstructorSymbol* symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol == nullptr)
		return;

	const BlockSyntax* body = node->get_body();
	if (body == nullptr)
		return;

	enter_member_body(symbol);
	push_scope(symbol);
	declare_parameters(symbol);

	// constructors are not subject to the missing-return check
	analyze_body(body);

	pop_scope();
	leave_member_body(symbol);
}

void ControlFlowAnalyzer::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// the anonymous method symbol is bound to the lambda's parameters list node
	MethodSymbol* symbol = nullptr;
	if (!(node->get_parameters() == nullptr))
		symbol = lookup_symbol<MethodSymbol>(node->get_parameters());

	if (symbol == nullptr)
		return;

	const BlockSyntax* body = node->get_body();
	if (body == nullptr)
		return;

	enter_member_body(symbol);
	push_scope(symbol);
	declare_parameters(symbol);

	analyze_member_body(symbol, body, node->get_location());

	pop_scope();
	leave_member_body(symbol);
}

void ControlFlowAnalyzer::declare_parameters(MethodSymbol* method)
{
	for (TypeParameterSymbol* const& typeParameter : method->get_type_parameters())
		get_current_scope()->declare_symbol(typeParameter);

	for (ParameterSymbol* const& parameter : method->get_parameters())
	{
		get_current_scope()->declare_symbol(parameter);
		mark_assigned(parameter);
	}
}

// =====================================================================
//  Body analysis
// =====================================================================

void ControlFlowAnalyzer::analyze_member_body(MethodSymbol* method, const BlockSyntax* body, const TextLocation& errorLocation)
{
	if (body == nullptr)
		return;

	bool wasAsync = m_currentMethodIsAsync;
	m_currentMethodIsAsync = method->get_is_async();

	FlowState bodyState = analyze_body(body);

	m_currentMethodIsAsync = wasAsync;

	// arrow bodies always yield their expression as the result
	bool isArrowBody = body->get_kind() == SyntaxKind::ArrowClause;
	if (isArrowBody)
		return;

	if (method->get_is_async())
		return;

	StructSymbol* returnType = method->get_return_type();
	if (returnType == nullptr)
		return;

	bool returnsVoid = returnType == get_symbol_table().get_void_primitive();
	if (returnsVoid)
		return;

	bool alwaysTerminates = !has_flag(bodyState, FlowState::Normal);
	bool onlyReturnOrThrow = !has_flag(bodyState, FlowState::Breaks);
	onlyReturnOrThrow = onlyReturnOrThrow && !has_flag(bodyState, FlowState::Continues);

	if (alwaysTerminates && onlyReturnOrThrow)
		return;

	// point the diagnostic at the end of the body (closing brace) rather than the member name
	TextLocation location = errorLocation;
	if (body->get_kind() == SyntaxKind::StatementsBlock)
	{
		const StatementsBlockSyntax* block = static_cast<const StatementsBlockSyntax*>(body);
		if (block->get_close_bracket().get_type() == TokenType::CloseBrace)
			location = block->get_close_bracket().get_location();
	}

	report_missing_return(location, returnType);
}

void ControlFlowAnalyzer::report_missing_return(const TextLocation& location, StructSymbol* returnType)
{
	if (returnType == nullptr)
	{
		get_diagnostics().report_error(location, L"Method must return a value");
		return;
	}

	get_diagnostics().report_error(
		location,
		gmt::concat(L"Method must return a value of type '", returnType->get_name(), L"'"));
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::merge_branches(FlowState a, FlowState b)
{
	return a | b;
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::without_breaks_and_continues(FlowState state)
{
	const FlowState consumed = ~(FlowState::Breaks | FlowState::Continues);
	FlowState result = state & consumed;

	// a break/continue consumed by its enclosing loop leaves normal flow behind
	// unless the body also returns or throws
	bool keepsNormalFlow = has_flag(result, FlowState::Normal);
	keepsNormalFlow = keepsNormalFlow || has_flag(result, FlowState::Returns);
	keepsNormalFlow = keepsNormalFlow || has_flag(result, FlowState::Throws);

	if (!keepsNormalFlow)
		result = FlowState::Normal;

	return result;
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::without_throws(FlowState state)
{
	FlowState result = state & ~FlowState::Throws;

	// a caught throw no longer aborts the method; if nothing else can escape,
	// the try/catch falls through normally
	bool keepsEscapingFlow = has_flag(result, FlowState::Normal);
	keepsEscapingFlow = keepsEscapingFlow || has_flag(result, FlowState::Returns);
	keepsEscapingFlow = keepsEscapingFlow || has_flag(result, FlowState::Breaks);
	keepsEscapingFlow = keepsEscapingFlow || has_flag(result, FlowState::Continues);

	if (!keepsEscapingFlow)
		result = FlowState::Normal;

	return result;
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_statement(const StatementSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	switch (node->get_kind())
	{
		case SyntaxKind::ExpressionStatement:
		{
			const ExpressionStatementSyntax* statement = static_cast<const ExpressionStatementSyntax*>(node);
			const ExpressionSyntax* expression = statement->get_expression();
			if (!(expression == nullptr))
			{
				analyze_expression_for_assignment(expression);
				analyze_expression_for_use(expression);
				scan_expression(expression);
			}

			return FlowState::Normal;
		}

		case SyntaxKind::VariableStatement:
		{
			const VariableStatementSyntax* statement = static_cast<const VariableStatementSyntax*>(node);
			if (!(statement->get_expression() == nullptr))
				scan_expression(statement->get_expression());

			analyze_variable_statement(statement);
			return FlowState::Normal;
		}

		case SyntaxKind::BreakStatement:
		{
			const BreakStatementSyntax* statement = static_cast<const BreakStatementSyntax*>(node);
			if (m_loopDepth == 0)
				get_diagnostics().report_error(statement->get_break_keyword(), L"Break statement must be inside a loop");

			return FlowState::Breaks;
		}

		case SyntaxKind::ContinueStatement:
		{
			const ContinueStatementSyntax* statement = static_cast<const ContinueStatementSyntax*>(node);
			if (m_loopDepth == 0)
				get_diagnostics().report_error(statement->get_continue_keyword(), L"Continue statement must be inside a loop");

			return FlowState::Continues;
		}

		case SyntaxKind::DeferStatement:
		{
			const DeferStatementSyntax* statement = static_cast<const DeferStatementSyntax*>(node);
			const ExpressionSyntax* expression = statement->get_expression();
			if (!(expression == nullptr))
			{
				analyze_expression_for_assignment(expression);
				analyze_expression_for_use(expression);
				scan_expression(expression);
			}

			return FlowState::Normal;
		}

		case SyntaxKind::DeferBlock:
		{
			const DeferBlockSyntax* statement = static_cast<const DeferBlockSyntax*>(node);
			analyze_statements_block(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::DeferVariable:
		{
			const DeferVariableSyntax* statement = static_cast<const DeferVariableSyntax*>(node);
			const VariableStatementSyntax* variable = statement->get_variable();
			if (!(variable == nullptr))
			{
				if (!(variable->get_expression() == nullptr))
					scan_expression(variable->get_expression());

				analyze_variable_statement(variable);
			}

			return FlowState::Normal;
		}

		case SyntaxKind::WhileStatement:
		{
			const WhileStatementSyntax* statement = static_cast<const WhileStatementSyntax*>(node);
			if (!(statement->get_condition() == nullptr))
				scan_expression(statement->get_condition());

			analyze_loop_body(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::UntilStatement:
		{
			const UntilStatementSyntax* statement = static_cast<const UntilStatementSyntax*>(node);
			if (!(statement->get_condition() == nullptr))
				scan_expression(statement->get_condition());

			analyze_loop_body(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::ForInStatement:
		{
			const ForInStatementSyntax* statement = static_cast<const ForInStatementSyntax*>(node);
			if (!(statement->get_range_expression() == nullptr))
				scan_expression(statement->get_range_expression());

			// the loop variable is assigned by the enumeration itself
			mark_assigned(lookup_symbol<VariableSymbol>(node));

			analyze_loop_body(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::IfStatement:
		case SyntaxKind::UnlessStatement:
		case SyntaxKind::ElseStatement:
			return analyze_conditional_chain(static_cast<const ConditionalClauseBaseSyntax*>(node));

		case SyntaxKind::TryStatement:
			return analyze_try_statement(static_cast<const TryStatementSyntax*>(node));

		case SyntaxKind::ReturnStatement:
		{
			const ReturnStatementSyntax* statement = static_cast<const ReturnStatementSyntax*>(node);
			if (!(statement->get_expression() == nullptr))
				scan_expression(statement->get_expression());

			return FlowState::Returns;
		}

		case SyntaxKind::ThrowStatement:
			return analyze_throw_statement(static_cast<const ThrowStatementSyntax*>(node));

		default:
			return FlowState::Normal;
	}
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	FlowState combined = FlowState::Normal;

	for (StatementSyntax* const& statement : node->get_statements())
	{
		if (is_terminal(combined))
		{
			if (!m_currentMethodIsAsync)
				get_diagnostics().report_error(statement->get_location(), L"Unreachable code detected");

			break;
		}

		combined = analyze_statement(statement);
	}

	return combined;
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_body(const BlockSyntax* body)
{
	if (body == nullptr)
		return FlowState::Normal;

	if (body->get_kind() == SyntaxKind::ArrowClause)
	{
		const ArrowClauseSyntax* arrow = static_cast<const ArrowClauseSyntax*>(body);
		scan_expression(arrow->get_expression());
		return FlowState::Normal;
	}

	return analyze_statements_block(static_cast<const StatementsBlockSyntax*>(body));
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_conditional_chain(const ConditionalClauseBaseSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	if (node->get_kind() == SyntaxKind::ElseStatement)
	{
		const ElseStatementSyntax* elseClause = static_cast<const ElseStatementSyntax*>(node);

		FlowState elseState = (elseClause->get_block() == nullptr)
			? FlowState::Normal
			: analyze_statements_block(elseClause->get_block());

		if (!(elseClause->get_next() == nullptr))
			elseState = merge_branches(elseState, analyze_conditional_chain(elseClause->get_next()));

		return elseState;
	}

	const ConditionalClauseSyntax* clause = static_cast<const ConditionalClauseSyntax*>(node);
	if (!(clause->get_condition() == nullptr))
		scan_expression(clause->get_condition());

	FlowState thenState = (clause->get_block() == nullptr)
		? FlowState::Normal
		: analyze_statements_block(clause->get_block());

	FlowState elseState = FlowState::Normal;
	if (!(clause->get_next() == nullptr))
		elseState = analyze_conditional_chain(clause->get_next());

	return merge_branches(thenState, elseState);
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	std::span<CatchClauseSyntax* const> catchClauses = node->get_catch_clauses();
	const StatementsBlockSyntax* tryBlock = node->get_try_block();

	bool hasCatch = catchClauses.size() > 0;
	if (hasCatch && !(tryBlock == nullptr))
		push_try_catch(catchClauses);

	FlowState tryState = (tryBlock == nullptr)
		? FlowState::Normal
		: analyze_statements_block(tryBlock);

	if (hasCatch && !(tryBlock == nullptr))
		pop_try_catch();

	if (!hasCatch)
		return tryState;

	// break/continue inside try/catch propagate to the enclosing loop,
	// but throws from the try block are caught by the catch clauses
	tryState = without_throws(tryState);

	FlowState catchState = FlowState::Normal;
	bool firstCatch = true;

	for (CatchClauseSyntax* const& clause : catchClauses)
	{
		if ((clause == nullptr) || (clause->get_body() == nullptr))
			continue;

		// the catch variable is assigned by the runtime before the body runs
		mark_assigned(lookup_symbol<VariableSymbol>(clause));

		push_catch_exception(lookup_catch_type(clause));

		FlowState clauseState = analyze_statements_block(clause->get_body());
		pop_catch_exception();

		if (firstCatch)
		{
			catchState = clauseState;
			firstCatch = false;
		}
		else
		{
			catchState = merge_branches(catchState, clauseState);
		}
	}

	return merge_branches(tryState, catchState);
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_throw_statement(const ThrowStatementSyntax* statement)
{
	const ExpressionSyntax* expression = statement->get_expression();
	if (!(expression == nullptr))
		scan_expression(expression);

	if (m_currentMethod == nullptr)
		return FlowState::Throws;

	MethodEffectSummary& effects = m_currentMethod->get_effect_summary();
	effects.mayThrow = true;

	StructSymbol* thrownType = nullptr;
	if (!(expression == nullptr))
	{
		thrownType = get_model().get_expression_type(expression);
	}
	else
	{
		// bare 'throw;' rethrows the exception currently being handled
		thrownType = current_catch_exception_type();
	}

	// thrown type is unknown; only the may-throw flag is recorded
	if (thrownType == nullptr)
		return FlowState::Throws;

	if (is_caught_by_active_try(thrownType))
		return FlowState::Throws;

	if (m_currentMethodIsAsync)
		add_thrown_type(effects.asyncThrownTypes, thrownType);
	else
		add_thrown_type(effects.thrownTypes, thrownType);

	return FlowState::Throws;
}

void ControlFlowAnalyzer::push_try_catch(std::span<CatchClauseSyntax* const> clauses)
{
	TryCatchFrame frame;

	for (CatchClauseSyntax* const& clause : clauses)
	{
		if (clause == nullptr)
			continue;

		if ((clause->get_exception_type() == nullptr))
		{
			frame.catchesAll = true;
			break;
		}

		StructSymbol* catchType = lookup_catch_type(clause);
		if (catchType == nullptr)
		{
			// unresolved exception type; treat the clause as a catch-all
			frame.catchesAll = true;
			break;
		}

		frame.caughtTypes.insert(catchType);
	}

	m_tryCatchStack.push_back(std::move(frame));
	rebuild_effective_caught_types();
}

void ControlFlowAnalyzer::pop_try_catch()
{
	if (!m_tryCatchStack.empty())
	{
		m_tryCatchStack.pop_back();
		rebuild_effective_caught_types();
	}
}

void ControlFlowAnalyzer::rebuild_effective_caught_types()
{
	m_effectiveCaughtTypes.clear();
	m_catchAllDepth = 0;

	for (const TryCatchFrame& frame : m_tryCatchStack)
	{
		if (frame.catchesAll)
		{
			++m_catchAllDepth;
		}
		else
		{
			for (StructSymbol* type : frame.caughtTypes)
				m_effectiveCaughtTypes.insert(type);
		}
	}
}

bool ControlFlowAnalyzer::is_caught_by_active_try(StructSymbol* thrownType)
{
	if (thrownType == nullptr)
		return false;

	if (m_catchAllDepth > 0)
		return true;

	for (StructSymbol* catchType : m_effectiveCaughtTypes)
	{
		if (get_model().is_assignable_to(catchType, thrownType))
			return true;
	}

	return false;
}

void ControlFlowAnalyzer::push_catch_exception(StructSymbol* type)
{
	m_catchExceptionStack.push_back(type);
}

void ControlFlowAnalyzer::pop_catch_exception()
{
	if (!m_catchExceptionStack.empty())
		m_catchExceptionStack.pop_back();
}

StructSymbol* ControlFlowAnalyzer::current_catch_exception_type() const
{
	if (m_catchExceptionStack.empty())
		return nullptr;

	return m_catchExceptionStack.back();
}

StructSymbol* ControlFlowAnalyzer::lookup_catch_type(const CatchClauseSyntax* clause)
{
	const TypeSyntax* exceptionType = clause->get_exception_type();
	if (exceptionType == nullptr)
		return nullptr;

	return lookup_symbol<StructSymbol>(exceptionType);
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_loop_body(const StatementsBlockSyntax* body)
{
	if (body == nullptr)
		return FlowState::Normal;

	++m_loopDepth;
	AssignmentSet savedAssignments = m_assignedVariables;
	analyze_statements_block(body);
	m_assignedVariables = std::move(savedAssignments);
	--m_loopDepth;

	// break/continue are consumed by the loop, and the loop itself may or may not execute, so it always yields normal flow to the caller
	return FlowState::Normal;
}

void ControlFlowAnalyzer::mark_assigned(SyntaxSymbol* variable)
{
	if (!(variable == nullptr))
		m_assignedVariables.insert(variable);
}

bool ControlFlowAnalyzer::is_assigned(SyntaxSymbol* variable) const
{
	if (variable == nullptr)
		return false;

	return m_assignedVariables.find(variable) != m_assignedVariables.end();
}

void ControlFlowAnalyzer::analyze_expression_for_assignment(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	if (expression->get_kind() != SyntaxKind::BinaryExpression)
		return;

	const BinaryExpressionSyntax* binary = static_cast<const BinaryExpressionSyntax*>(expression);
	if (binary->get_operator_token().get_type() != TokenType::AssignOperator)
		return;

	const ExpressionSyntax* left = binary->get_left();

	// plain identifier targets are plain identifier type syntaxes
	if ((left == nullptr) || left->get_kind() != SyntaxKind::IdentifierNameType)
		return;

	SyntaxSymbol* symbol = get_symbol_table().lookup_symbol(left);
	if (symbol == nullptr)
		return;

	bool isLocal = !(static_cast<VariableSymbol*>(symbol) == nullptr);
	bool isParameter = !(static_cast<ParameterSymbol*>(symbol) == nullptr);
	if (isLocal || isParameter)
		mark_assigned(symbol);
}

void ControlFlowAnalyzer::analyze_expression_for_use(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	switch (expression->get_kind())
	{
		case SyntaxKind::IdentifierNameType:
		{
			const IdentifierNameTypeSyntax* identifier = static_cast<const IdentifierNameTypeSyntax*>(expression);

			SyntaxSymbol* symbol = get_symbol_table().lookup_symbol(identifier);
			if (symbol == nullptr)
				break;

			bool isLocal = !(static_cast<VariableSymbol*>(symbol) == nullptr);
			bool isParameter = !(static_cast<ParameterSymbol*>(symbol) == nullptr);
			if (!isLocal && !isParameter)
				break;

			if (!is_assigned(symbol))
			{
				get_diagnostics().report_error(
					identifier->get_identifier(),
					gmt::concat(L"Use of unassigned local variable '", symbol->get_name(), L"'"));
			}

			break;
		}

		case SyntaxKind::BinaryExpression:
		{
			const BinaryExpressionSyntax* binary = static_cast<const BinaryExpressionSyntax*>(expression);
			analyze_expression_for_use(binary->get_left());
			analyze_expression_for_use(binary->get_right());
			break;
		}

		default:
			break;
	}
}

void ControlFlowAnalyzer::analyze_variable_statement(const VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	VariableSymbol* variable = lookup_symbol<VariableSymbol>(node);
	if (variable == nullptr)
		return;

	if ((node->get_expression() == nullptr))
		return;

	analyze_expression_for_use(node->get_expression());
	mark_assigned(variable);
}

void ControlFlowAnalyzer::enter_member_body(MethodSymbol* method)
{
	m_methodStack.push_back(m_currentMethod);
	m_currentMethod = method;

	if (!(method == nullptr))
		method->set_effects_computed(false);
}

void ControlFlowAnalyzer::leave_member_body(MethodSymbol* method)
{
	if (!(method == nullptr))
		method->set_effects_computed(true);

	m_currentMethod = m_methodStack.back();
	m_methodStack.pop_back();
}

void ControlFlowAnalyzer::scan_expression(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	expression->accept(*this);
}

bool ControlFlowAnalyzer::is_assignment_operator(TokenType type)
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

bool ControlFlowAnalyzer::is_increment_decrement_operator(TokenType type)
{
	return type == TokenType::IncrementOperator || type == TokenType::DecrementOperator;
}

bool ControlFlowAnalyzer::is_this_expression(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return false;

	SyntaxKind kind = expression->get_kind();
	if (kind == SyntaxKind::MemberAccessExpression)
	{
		const MemberAccessExpressionSyntax* memberAccess = static_cast<const MemberAccessExpressionSyntax*>(expression);
		if (!(memberAccess->get_previous() == nullptr))
			return false;
	}
	else if (kind != SyntaxKind::IdentifierNameType)
	{
		return false;
	}

	if (expression == nullptr)
		return false;

	ParameterSymbol* parameter = lookup_symbol<ParameterSymbol>(expression);
	return !(parameter == nullptr) && parameter->get_name() == L"this";
}

void ControlFlowAnalyzer::record_field_or_property_write(const MemberAccessExpressionSyntax* node)
{
	if ((node == nullptr) || (m_currentMethod == nullptr))
		return;

	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node);
	if (bound == nullptr)
		return;

	FieldSymbol* field = static_cast<FieldSymbol*>(bound);
	PropertySymbol* property = static_cast<PropertySymbol*>(bound);

	bool isStatic = false;
	if (!(field == nullptr))
		isStatic = field->get_linking() == LINK_STATIC;
	else if (!(property == nullptr))
		isStatic = property->get_linking() == LINK_STATIC;

	bool isThisReceiver = !isStatic;
	isThisReceiver = isThisReceiver && ((node->get_previous() == nullptr) || is_this_expression(node->get_previous()));

	MethodEffectSummary& effects = m_currentMethod->get_effect_summary();

	if (!(field == nullptr))
	{
		if (isStatic)
		{
			effects.mutatesStatic = true;
			effects.mayAssignStaticFields.insert(field);
		}
		else if (isThisReceiver)
		{
			effects.mutatesInstance = true;
			effects.mayAssignInstanceFields.insert(field);
		}
	}
	else if (!(property == nullptr))
	{
		// a property write is a call to the setter accessor; record its effects
		if (isStatic)
			effects.mutatesStatic = true;
		else if (isThisReceiver)
			effects.mutatesInstance = true;

		AccessorSymbol* setter = property->get_setter();
		if (!(setter == nullptr))
			record_callee_effects(setter, node->get_identifier());
	}
}

void ControlFlowAnalyzer::record_indexator_write(const IndexatorExpressionSyntax* node)
{
	if ((node == nullptr) || (m_currentMethod == nullptr))
		return;

	SyntaxSymbol* bound = get_symbol_table().lookup_symbol(node);
	PropertySymbol* property = static_cast<PropertySymbol*>(bound);

	bool isStatic = !(property == nullptr);
	isStatic = isStatic && property->get_linking() == LINK_STATIC;

	bool isThisReceiver = !isStatic;
	isThisReceiver = isThisReceiver && ((node->get_previous() == nullptr) || is_this_expression(node->get_previous()));

	MethodEffectSummary& effects = m_currentMethod->get_effect_summary();

	if (isStatic)
		effects.mutatesStatic = true;
	else if (isThisReceiver)
		effects.mutatesInstance = true;

	// merge setter effects if the indexer symbol is known
	if (!(property == nullptr))
	{
		AccessorSymbol* setter = property->get_setter();
		if (!(setter == nullptr))
			record_callee_effects(setter, node->get_delimeter_token());
	}
}

void ControlFlowAnalyzer::record_callee_effects(MethodSymbol* callee, const SyntaxToken& callSite, bool isAwaited, bool isTryWrapped)
{
	// a null callee records an unknown call site (delegate or unresolved call)
	// that still requires an explicit 'try' or an enclosing try-catch
	if (m_currentMethod == nullptr)
		return;

	CallEdge edge;
	edge.callee = callee;
	edge.callSite = callSite;
	edge.caughtTypes = m_effectiveCaughtTypes;
	edge.catchesAll = m_catchAllDepth > 0;
	edge.isAwaited = isAwaited;
	edge.isTryWrapped = isTryWrapped;

	m_callGraph[m_currentMethod].push_back(std::move(edge));
}

bool ControlFlowAnalyzer::is_try_wrapped(const ExpressionSyntax* expression) const
{
	return m_tryWrappedExpressions.find(expression) != m_tryWrappedExpressions.end();
}

void ControlFlowAnalyzer::add_thrown_type(gmt::unordered_set<StructSymbol*>& thrownTypes, StructSymbol* type)
{
	if (type == nullptr)
		return;

	for (StructSymbol* existing : thrownTypes)
	{
		bool sameType = existing == type;

		bool mutuallyAssignable = get_model().is_assignable_to(existing, type);
		mutuallyAssignable = mutuallyAssignable && get_model().is_assignable_to(type, existing);

		if (sameType || mutuallyAssignable)
			return;
	}

	thrownTypes.insert(type);
}

bool ControlFlowAnalyzer::call_edge_catches(const CallEdge& edge, StructSymbol* thrownType)
{
	if (thrownType == nullptr)
		return false;

	if (edge.catchesAll)
		return true;

	for (StructSymbol* catchType : edge.caughtTypes)
	{
		if (get_model().is_assignable_to(catchType, thrownType))
			return true;
	}

	return false;
}

void ControlFlowAnalyzer::propagate_effects()
{
	bool changed = true;
	while (changed)
	{
		changed = false;

		for (auto& pair : m_callGraph)
		{
			MethodSymbol* caller = pair.first;
			if (caller == nullptr)
				continue;

			MethodEffectSummary previous = caller->get_effect_summary();

			for (const CallEdge& edge : pair.second)
			{
				MethodSymbol* callee = edge.callee;
				if (callee == nullptr)
					continue;

				MethodEffectSummary& callerEffects = caller->get_effect_summary();
				const MethodEffectSummary& calleeEffects = callee->get_effect_summary();

				// mutations always propagate regardless of catch context
				callerEffects.mutatesInstance = callerEffects.mutatesInstance || calleeEffects.mutatesInstance;
				callerEffects.mutatesStatic = callerEffects.mutatesStatic || calleeEffects.mutatesStatic;
				callerEffects.mutatesArguments = callerEffects.mutatesArguments || calleeEffects.mutatesArguments;
				callerEffects.hasUnknownSideEffects = callerEffects.hasUnknownSideEffects || calleeEffects.hasUnknownSideEffects;

				gmt::unordered_set<StructSymbol*>* targetThrownSet = caller->get_is_async()
					? &callerEffects.asyncThrownTypes
					: &callerEffects.thrownTypes;

				if (callee->get_is_async())
				{
					// awaiting an async method surfaces the exceptions captured in its task
					if (edge.isAwaited)
					{
						for (StructSymbol* thrownType : calleeEffects.asyncThrownTypes)
						{
							if (!call_edge_catches(edge, thrownType))
							{
								add_thrown_type(*targetThrownSet, thrownType);
								callerEffects.mayThrow = true;
							}
						}
					}
					// non-awaited async calls return a task; exceptions are deferred
				}
				else
				{
					// only exception types that are not caught at the call site propagate;
					// 'try'-wrapped calls inherit the callee's exceptions as their own effect
					for (StructSymbol* thrownType : calleeEffects.thrownTypes)
					{
						if (!call_edge_catches(edge, thrownType))
						{
							add_thrown_type(*targetThrownSet, thrownType);
							callerEffects.mayThrow = true;
						}
					}
				}
			}

			if (previous != caller->get_effect_summary())
				changed = true;
		}
	}

	for (auto& pair : m_callGraph)
	{
		if (!(pair.first == nullptr))
			pair.first->set_effects_computed(true);
	}

	report_effect_diagnostics();
}

void ControlFlowAnalyzer::report_effect_diagnostics()
{
	for (const auto& pair : m_callGraph)
	{
		for (const CallEdge& edge : pair.second)
		{
			// the callee is unknown (delegate or unresolved call); such a call
			// may throw as well, so it requires an explicit 'try' as well
			if ((edge.callee == nullptr))
			{
				if (!edge.isTryWrapped)
					get_diagnostics().report_error(edge.callSite, L"Call may throw an exception");

				continue;
			}

			report_edge_diagnostics(edge);
		}
	}
}

void ControlFlowAnalyzer::report_edge_diagnostics(const CallEdge& edge)
{
	MethodSymbol* callee = edge.callee;
	const MethodEffectSummary& calleeEffects = callee->get_effect_summary();
	gmt::unordered_set<StructSymbol*> reportedTypes;

	// the 'try' operator explicitly allows the callee's exceptions to unwind
	// further; they are inherited as the caller's own effects, so no diagnostic
	if (edge.callee->get_is_async())
	{
		if (edge.isAwaited)
		{
			for (StructSymbol* thrownType : calleeEffects.asyncThrownTypes)
			{
				bool alreadyReported = reportedTypes.find(thrownType) != reportedTypes.end();
				if (!edge.isTryWrapped && !call_edge_catches(edge, thrownType) && !alreadyReported)
				{
					reportedTypes.insert(thrownType);

					std::wstring_view typeName = (thrownType == nullptr) ? L"unknown" : thrownType->get_name();
					get_diagnostics().report_error(
						edge.callSite,
						gmt::concat(L"Await on '", callee->get_name(), L"' may throw '", typeName, L"'"));
				}
			}
		}
	}
	else
	{
		for (StructSymbol* thrownType : calleeEffects.thrownTypes)
		{
			bool alreadyReported = reportedTypes.find(thrownType) != reportedTypes.end();
			if (!edge.isTryWrapped && !call_edge_catches(edge, thrownType) && !alreadyReported)
			{
				reportedTypes.insert(thrownType);

				std::wstring_view typeName = (thrownType == nullptr) ? L"unknown" : thrownType->get_name();
				get_diagnostics().report_error(
					edge.callSite,
					gmt::concat(L"Call to '", callee->get_name(), L"' may throw '", typeName, L"'"));
			}
		}
	}

	bool hasKnownThrownTypes = callee->get_is_async()
		? !calleeEffects.asyncThrownTypes.empty()
		: !calleeEffects.thrownTypes.empty();

	bool throwsUnknown = calleeEffects.mayThrow && !hasKnownThrownTypes;
	if (!edge.isTryWrapped && throwsUnknown)
	{
		gmt::wstring message = callee->get_is_async() && edge.isAwaited
			? gmt::concat(L"Await on '", callee->get_name(), L"' may throw an exception")
			: gmt::concat(L"Call to '", callee->get_name(), L"' may throw an exception");

		get_diagnostics().report_error(edge.callSite, message);
	}
}

void ControlFlowAnalyzer::visit_binary_expression(const BinaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	bool isAssignment = is_assignment_operator(node->get_operator_token().get_type());
	if (!(m_currentMethod == nullptr) && isAssignment && !(node->get_left() == nullptr))
	{
		const ExpressionSyntax* left = node->get_left();
		if (left->get_kind() == SyntaxKind::MemberAccessExpression)
			record_field_or_property_write(static_cast<const MemberAccessExpressionSyntax*>(left));
		else if (left->get_kind() == SyntaxKind::IndexatorExpression)
			record_indexator_write(static_cast<const IndexatorExpressionSyntax*>(left));
	}

	SyntaxVisitor::visit_binary_expression(node);
}

void ControlFlowAnalyzer::visit_unary_expression(const UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// 'try <invokation>' explicitly allows the callee's exceptions to unwind
	// further; mark the operand so the invokation visit can recognize it
	if (node->get_operator_token().get_type() == TokenType::TryKeyword && !(node->get_operand() == nullptr))
		m_tryWrappedExpressions.insert(node->get_operand());

	bool isIncrement = is_increment_decrement_operator(node->get_operator_token().get_type());
	if (!(m_currentMethod == nullptr) && isIncrement && !(node->get_operand() == nullptr))
	{
		const ExpressionSyntax* operand = node->get_operand();
		if (operand->get_kind() == SyntaxKind::MemberAccessExpression)
			record_field_or_property_write(static_cast<const MemberAccessExpressionSyntax*>(operand));
		else if (operand->get_kind() == SyntaxKind::IndexatorExpression)
			record_indexator_write(static_cast<const IndexatorExpressionSyntax*>(operand));
	}

	SyntaxVisitor::visit_unary_expression(node);
}

void ControlFlowAnalyzer::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(m_currentMethod == nullptr))
	{
		MethodSymbol* callee = nullptr;
		bool isTryWrapped = false;

		auto nodeRef = ref_from(node);
		if (nodeRef.has_value())
		{
			// delegate invokations bind the callee variable or field rather
			// than a method; those flow through the unknown-callee path below
			SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*nodeRef);
			if (!(bound == nullptr) && bound->is_method())
				callee = static_cast<MethodSymbol*>(bound);

			isTryWrapped = is_try_wrapped(*nodeRef);
		}

		if (!(callee == nullptr))
		{
			// async callees defer exceptions into the returned task; those are
			// surfaced only when the call is awaited (handled in visit_await_expression)
			if (!callee->get_is_async())
				record_callee_effects(callee, node->get_identifier(), false, isTryWrapped);
		}
		else
		{
			m_currentMethod->get_effect_summary().hasUnknownSideEffects = true;

			// 'try' on an unknown callee (delegate or unresolved call) explicitly
			// allows the unknown exception to unwind further
			if (isTryWrapped)
			{
				m_currentMethod->get_effect_summary().mayThrow = true;
			}
			else if (nodeRef.has_value())
			{
				record_callee_effects(nullptr, node->get_identifier());
			}
		}
	}

	SyntaxVisitor::visit_invokation_expression(node);
}

void ControlFlowAnalyzer::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(m_currentMethod == nullptr))
	{
		ConstructorSymbol* ctor = nullptr;
		auto nodeRef = ref_from(node);
		if (nodeRef.has_value())
		{
			// an unresolved constructor (value-initialization) leaves the created
			// type bound to the node; only real constructors record a call edge
			SyntaxSymbol* bound = get_symbol_table().lookup_symbol(*nodeRef);
			if (!(bound == nullptr) && bound->is_method())
				ctor = static_cast<ConstructorSymbol*>(bound);
		}

		if (!(ctor == nullptr))
			record_callee_effects(ctor, node->get_new_token());
	}

	SyntaxVisitor::visit_object_creation_expression(node);
}

void ControlFlowAnalyzer::visit_await_expression(const AwaitExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(m_currentMethod == nullptr) && !(node->get_expression() == nullptr))
	{
		const ExpressionSyntax* awaited = node->get_expression();

		// direct await of an async method call: record the await edge so the
		// async callee's captured exceptions can propagate to this method
		if (awaited->get_kind() == SyntaxKind::InvokationExpression)
		{
			const InvokationExpressionSyntax* invocation = static_cast<const InvokationExpressionSyntax*>(awaited);

			MethodSymbol* callee = lookup_symbol<MethodSymbol>(invocation);
			bool isTryWrapped = is_try_wrapped(invocation);

			bool isAsyncCallee = !(callee == nullptr) && callee->get_is_async();
			if (isAsyncCallee)
				record_callee_effects(callee, invocation->get_identifier(), true, isTryWrapped);
		}

		// awaiting an unknown or non-async expression that could hold a faulted task
		// is treated conservatively as a potential throw
		StructSymbol* awaitedType = get_model().get_expression_type(awaited);
		if (!(awaitedType == nullptr) && get_model().is_task_or_task_like(awaitedType))
		{
			if (awaited->get_kind() != SyntaxKind::InvokationExpression)
				m_currentMethod->get_effect_summary().mayThrow = true;
		}
	}

	SyntaxVisitor::visit_await_expression(node);
}
