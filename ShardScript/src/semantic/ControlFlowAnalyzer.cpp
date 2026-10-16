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

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	gmt::Ref<const BlockSyntax> body = node->get_body();
	if (body.is_null())
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

	gmt::Ref<MethodSymbol> symbol = lookup_symbol<MethodSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	gmt::Ref<const BlockSyntax> body = node->get_body();
	if (body.is_null())
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

	gmt::Ref<ConstructorSymbol> symbol = lookup_symbol<ConstructorSymbol>(*nodeRef);
	if (symbol.is_null())
		return;

	gmt::Ref<const BlockSyntax> body = node->get_body();
	if (body.is_null())
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
	gmt::Ref<MethodSymbol> symbol;
	if (!node->get_parameters().is_null())
	{
		auto paramsRef = ref_from(node->get_parameters().as_ptr());
		if (paramsRef.has_value())
			symbol = lookup_symbol<MethodSymbol>(*paramsRef);
	}

	if (symbol.is_null())
		return;

	gmt::Ref<const BlockSyntax> body = node->get_body();
	if (body.is_null())
		return;

	enter_member_body(symbol);
	push_scope(symbol);
	declare_parameters(symbol);

	analyze_member_body(symbol, body, node->get_location());

	pop_scope();
	leave_member_body(symbol);
}

void ControlFlowAnalyzer::declare_parameters(gmt::Ref<MethodSymbol> method)
{
	for (const gmt::Ref<TypeParameterSymbol>& typeParameter : method->get_type_parameters())
		get_current_scope()->declare_symbol(typeParameter);

	for (const gmt::Ref<ParameterSymbol>& parameter : method->get_parameters())
	{
		get_current_scope()->declare_symbol(parameter);
		mark_assigned(parameter);
	}
}

// =====================================================================
//  Body analysis
// =====================================================================

void ControlFlowAnalyzer::analyze_member_body(gmt::Ref<MethodSymbol> method, gmt::Ref<const BlockSyntax> body, const TextLocation& errorLocation)
{
	if (body.is_null())
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

	gmt::Ref<StructSymbol> returnType = method->get_return_type();
	if (returnType.is_null())
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
		gmt::Ref<const StatementsBlockSyntax> block = body.as<const StatementsBlockSyntax>();
		if (block->get_close_bracket().get_type() == TokenType::CloseBrace)
			location = block->get_close_bracket().get_location();
	}

	report_missing_return(location, returnType);
}

void ControlFlowAnalyzer::report_missing_return(const TextLocation& location, gmt::Ref<StructSymbol> returnType)
{
	if (returnType.is_null())
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

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_statement(gmt::Ref<const StatementSyntax> node)
{
	if (node.is_null())
		return FlowState::Normal;

	switch (node->get_kind())
	{
		case SyntaxKind::ExpressionStatement:
		{
			gmt::Ref<const ExpressionStatementSyntax> statement = node.as<const ExpressionStatementSyntax>();
			gmt::Ref<const ExpressionSyntax> expression = statement->get_expression();
			if (!expression.is_null())
			{
				analyze_expression_for_assignment(expression);
				analyze_expression_for_use(expression);
				scan_expression(expression);
			}

			return FlowState::Normal;
		}

		case SyntaxKind::VariableStatement:
		{
			gmt::Ref<const VariableStatementSyntax> statement = node.as<const VariableStatementSyntax>();
			if (!statement->get_expression().is_null())
				scan_expression(statement->get_expression());

			analyze_variable_statement(statement);
			return FlowState::Normal;
		}

		case SyntaxKind::BreakStatement:
		{
			gmt::Ref<const BreakStatementSyntax> statement = node.as<const BreakStatementSyntax>();
			if (m_loopDepth == 0)
				get_diagnostics().report_error(statement->get_break_keyword(), L"Break statement must be inside a loop");

			return FlowState::Breaks;
		}

		case SyntaxKind::ContinueStatement:
		{
			gmt::Ref<const ContinueStatementSyntax> statement = node.as<const ContinueStatementSyntax>();
			if (m_loopDepth == 0)
				get_diagnostics().report_error(statement->get_continue_keyword(), L"Continue statement must be inside a loop");

			return FlowState::Continues;
		}

		case SyntaxKind::DeferStatement:
		{
			gmt::Ref<const DeferStatementSyntax> statement = node.as<const DeferStatementSyntax>();
			gmt::Ref<const ExpressionSyntax> expression = statement->get_expression();
			if (!expression.is_null())
			{
				analyze_expression_for_assignment(expression);
				analyze_expression_for_use(expression);
				scan_expression(expression);
			}

			return FlowState::Normal;
		}

		case SyntaxKind::DeferBlock:
		{
			gmt::Ref<const DeferBlockSyntax> statement = node.as<const DeferBlockSyntax>();
			analyze_statements_block(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::DeferVariable:
		{
			gmt::Ref<const DeferVariableSyntax> statement = node.as<const DeferVariableSyntax>();
			gmt::Ref<const VariableStatementSyntax> variable = statement->get_variable();
			if (!variable.is_null())
			{
				if (!variable->get_expression().is_null())
					scan_expression(variable->get_expression());

				analyze_variable_statement(variable);
			}

			return FlowState::Normal;
		}

		case SyntaxKind::WhileStatement:
		{
			gmt::Ref<const WhileStatementSyntax> statement = node.as<const WhileStatementSyntax>();
			if (!statement->get_condition().is_null())
				scan_expression(statement->get_condition());

			analyze_loop_body(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::UntilStatement:
		{
			gmt::Ref<const UntilStatementSyntax> statement = node.as<const UntilStatementSyntax>();
			if (!statement->get_condition().is_null())
				scan_expression(statement->get_condition());

			analyze_loop_body(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::ForInStatement:
		{
			gmt::Ref<const ForInStatementSyntax> statement = node.as<const ForInStatementSyntax>();
			if (!statement->get_range_expression().is_null())
				scan_expression(statement->get_range_expression());

			// the loop variable is assigned by the enumeration itself
			auto nodeRef = ref_from(node.as_ptr());
			if (nodeRef.has_value())
				mark_assigned(lookup_symbol<VariableSymbol>(*nodeRef));

			analyze_loop_body(statement->get_block());
			return FlowState::Normal;
		}

		case SyntaxKind::IfStatement:
		case SyntaxKind::UnlessStatement:
		case SyntaxKind::ElseStatement:
			return analyze_conditional_chain(node.as<const ConditionalClauseBaseSyntax>());

		case SyntaxKind::TryStatement:
			return analyze_try_statement(node.as<const TryStatementSyntax>());

		case SyntaxKind::ReturnStatement:
		{
			gmt::Ref<const ReturnStatementSyntax> statement = node.as<const ReturnStatementSyntax>();
			if (!statement->get_expression().is_null())
				scan_expression(statement->get_expression());

			return FlowState::Returns;
		}

		case SyntaxKind::ThrowStatement:
			return analyze_throw_statement(node.as<const ThrowStatementSyntax>());

		default:
			return FlowState::Normal;
	}
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_statements_block(gmt::Ref<const StatementsBlockSyntax> node)
{
	if (node.is_null())
		return FlowState::Normal;

	FlowState combined = FlowState::Normal;

	for (const gmt::Ref<StatementSyntax>& statement : node->get_statements().as_span())
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

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_body(gmt::Ref<const BlockSyntax> body)
{
	if (body.is_null())
		return FlowState::Normal;

	if (body->get_kind() == SyntaxKind::ArrowClause)
	{
		gmt::Ref<const ArrowClauseSyntax> arrow = body.as<const ArrowClauseSyntax>();
		scan_expression(arrow->get_expression());
		return FlowState::Normal;
	}

	return analyze_statements_block(body.as<const StatementsBlockSyntax>());
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_conditional_chain(gmt::Ref<const ConditionalClauseBaseSyntax> node)
{
	if (node.is_null())
		return FlowState::Normal;

	if (node->get_kind() == SyntaxKind::ElseStatement)
	{
		gmt::Ref<const ElseStatementSyntax> elseClause = node.as<const ElseStatementSyntax>();

		FlowState elseState = elseClause->get_block().is_null()
			? FlowState::Normal
			: analyze_statements_block(elseClause->get_block());

		if (!elseClause->get_next().is_null())
			elseState = merge_branches(elseState, analyze_conditional_chain(elseClause->get_next()));

		return elseState;
	}

	gmt::Ref<const ConditionalClauseSyntax> clause = node.as<const ConditionalClauseSyntax>();
	if (!clause->get_condition().is_null())
		scan_expression(clause->get_condition());

	FlowState thenState = clause->get_block().is_null()
		? FlowState::Normal
		: analyze_statements_block(clause->get_block());

	FlowState elseState = FlowState::Normal;
	if (!clause->get_next().is_null())
		elseState = analyze_conditional_chain(clause->get_next());

	return merge_branches(thenState, elseState);
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_try_statement(gmt::Ref<const TryStatementSyntax> node)
{
	if (node.is_null())
		return FlowState::Normal;

	gmt::Span<const gmt::Ref<CatchClauseSyntax>> catchClauses = node->get_catch_clauses();
	gmt::Ref<const StatementsBlockSyntax> tryBlock = node->get_try_block();

	bool hasCatch = catchClauses.length() > 0;
	if (hasCatch && !tryBlock.is_null())
		push_try_catch(catchClauses);

	FlowState tryState = tryBlock.is_null()
		? FlowState::Normal
		: analyze_statements_block(tryBlock);

	if (hasCatch && !tryBlock.is_null())
		pop_try_catch();

	if (!hasCatch)
		return tryState;

	// break/continue inside try/catch propagate to the enclosing loop,
	// but throws from the try block are caught by the catch clauses
	tryState = without_throws(tryState);

	FlowState catchState = FlowState::Normal;
	bool firstCatch = true;

	for (const gmt::Ref<CatchClauseSyntax>& clause : catchClauses.as_span())
	{
		if (clause.is_null() || clause->get_body().is_null())
			continue;

		// the catch variable is assigned by the runtime before the body runs
		auto clauseRef = ref_from(clause.as_ptr());
		if (clauseRef.has_value())
			mark_assigned(lookup_symbol<VariableSymbol>(*clauseRef));

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

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_throw_statement(gmt::Ref<const ThrowStatementSyntax> statement)
{
	gmt::Ref<const ExpressionSyntax> expression = statement->get_expression();
	if (!expression.is_null())
		scan_expression(expression);

	if (m_currentMethod.is_null())
		return FlowState::Throws;

	MethodEffectSummary& effects = m_currentMethod->get_effect_summary();
	effects.mayThrow = true;

	gmt::Ref<StructSymbol> thrownType;
	if (!expression.is_null())
	{
		thrownType = get_model().get_expression_type(expression);
	}
	else
	{
		// bare 'throw;' rethrows the exception currently being handled
		thrownType = current_catch_exception_type();
	}

	// thrown type is unknown; only the may-throw flag is recorded
	if (thrownType.is_null())
		return FlowState::Throws;

	if (is_caught_by_active_try(thrownType))
		return FlowState::Throws;

	if (m_currentMethodIsAsync)
		add_thrown_type(effects.asyncThrownTypes, thrownType);
	else
		add_thrown_type(effects.thrownTypes, thrownType);

	return FlowState::Throws;
}

void ControlFlowAnalyzer::push_try_catch(gmt::Span<const gmt::Ref<CatchClauseSyntax>> clauses)
{
	TryCatchFrame frame;

	for (const gmt::Ref<CatchClauseSyntax>& clause : clauses.as_span())
	{
		if (clause.is_null())
			continue;

		if (clause->get_exception_type().is_null())
		{
			frame.catchesAll = true;
			break;
		}

		gmt::Ref<StructSymbol> catchType = lookup_catch_type(clause);
		if (catchType.is_null())
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
			for (gmt::Ref<StructSymbol> type : frame.caughtTypes)
				m_effectiveCaughtTypes.insert(type);
		}
	}
}

bool ControlFlowAnalyzer::is_caught_by_active_try(gmt::Ref<StructSymbol> thrownType)
{
	if (thrownType.is_null())
		return false;

	if (m_catchAllDepth > 0)
		return true;

	for (gmt::Ref<StructSymbol> catchType : m_effectiveCaughtTypes)
	{
		if (get_model().is_assignable_to(catchType, thrownType))
			return true;
	}

	return false;
}

void ControlFlowAnalyzer::push_catch_exception(gmt::Ref<StructSymbol> type)
{
	m_catchExceptionStack.push_back(type);
}

void ControlFlowAnalyzer::pop_catch_exception()
{
	if (!m_catchExceptionStack.empty())
		m_catchExceptionStack.pop_back();
}

gmt::Ref<StructSymbol> ControlFlowAnalyzer::current_catch_exception_type() const
{
	if (m_catchExceptionStack.empty())
		return gmt::Ref<StructSymbol>();

	return m_catchExceptionStack.back();
}

gmt::Ref<StructSymbol> ControlFlowAnalyzer::lookup_catch_type(gmt::Ref<const CatchClauseSyntax> clause)
{
	gmt::Ref<const TypeSyntax> exceptionType = clause->get_exception_type();
	if (exceptionType.is_null())
		return gmt::Ref<StructSymbol>();

	auto typeRef = ref_from(exceptionType.as_ptr());
	if (!typeRef.has_value())
		return gmt::Ref<StructSymbol>();

	return lookup_symbol<StructSymbol>(*typeRef);
}

ControlFlowAnalyzer::FlowState ControlFlowAnalyzer::analyze_loop_body(gmt::Ref<const StatementsBlockSyntax> body)
{
	if (body.is_null())
		return FlowState::Normal;

	++m_loopDepth;
	AssignmentSet savedAssignments = m_assignedVariables;
	analyze_statements_block(body);
	m_assignedVariables = std::move(savedAssignments);
	--m_loopDepth;

	// break/continue are consumed by the loop, and the loop itself may or may not execute, so it always yields normal flow to the caller
	return FlowState::Normal;
}

void ControlFlowAnalyzer::mark_assigned(gmt::Ref<SyntaxSymbol> variable)
{
	if (!variable.is_null())
		m_assignedVariables.insert(variable);
}

bool ControlFlowAnalyzer::is_assigned(gmt::Ref<SyntaxSymbol> variable) const
{
	if (variable.is_null())
		return false;

	return m_assignedVariables.find(variable) != m_assignedVariables.end();
}

void ControlFlowAnalyzer::analyze_expression_for_assignment(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null())
		return;

	if (expression->get_kind() != SyntaxKind::BinaryExpression)
		return;

	gmt::Ref<const BinaryExpressionSyntax> binary = expression.as<const BinaryExpressionSyntax>();
	if (binary->get_operator_token().get_type() != TokenType::AssignOperator)
		return;

	gmt::Ref<const ExpressionSyntax> left = binary->get_left();

	// plain identifier targets are plain identifier type syntaxes
	if (left.is_null() || left->get_kind() != SyntaxKind::IdentifierNameType)
		return;

	auto leftRef = ref_from(left.as_ptr());
	if (!leftRef.has_value())
		return;

	gmt::Ref<SyntaxSymbol> symbol = get_symbol_table().lookup_symbol(*leftRef);
	if (symbol.is_null())
		return;

	bool isLocal = !symbol.as<VariableSymbol>().is_null();
	bool isParameter = !symbol.as<ParameterSymbol>().is_null();
	if (isLocal || isParameter)
		mark_assigned(symbol);
}

void ControlFlowAnalyzer::analyze_expression_for_use(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null())
		return;

	switch (expression->get_kind())
	{
		case SyntaxKind::IdentifierNameType:
		{
			gmt::Ref<const IdentifierNameTypeSyntax> identifier = expression.as<const IdentifierNameTypeSyntax>();

			auto identifierRef = ref_from(identifier.as_ptr());
			if (!identifierRef.has_value())
				break;

			gmt::Ref<SyntaxSymbol> symbol = get_symbol_table().lookup_symbol(*identifierRef);
			if (symbol.is_null())
				break;

			bool isLocal = !symbol.as<VariableSymbol>().is_null();
			bool isParameter = !symbol.as<ParameterSymbol>().is_null();
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
			gmt::Ref<const BinaryExpressionSyntax> binary = expression.as<const BinaryExpressionSyntax>();
			analyze_expression_for_use(binary->get_left());
			analyze_expression_for_use(binary->get_right());
			break;
		}

		default:
			break;
	}
}

void ControlFlowAnalyzer::analyze_variable_statement(gmt::Ref<const VariableStatementSyntax> node)
{
	if (node.is_null())
		return;

	auto nodeRef = ref_from(node.as_ptr());
	if (!nodeRef.has_value())
		return;

	gmt::Ref<VariableSymbol> variable = lookup_symbol<VariableSymbol>(*nodeRef);
	if (variable.is_null())
		return;

	if (node->get_expression().is_null())
		return;

	analyze_expression_for_use(node->get_expression());
	mark_assigned(variable);
}

void ControlFlowAnalyzer::enter_member_body(gmt::Ref<MethodSymbol> method)
{
	m_methodStack.push_back(m_currentMethod);
	m_currentMethod = method;

	if (!method.is_null())
		method->set_effects_computed(false);
}

void ControlFlowAnalyzer::leave_member_body(gmt::Ref<MethodSymbol> method)
{
	if (!method.is_null())
		method->set_effects_computed(true);

	m_currentMethod = m_methodStack.back();
	m_methodStack.pop_back();
}

void ControlFlowAnalyzer::scan_expression(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null())
		return;

	expression.as_ptr()->accept(*this);
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

bool ControlFlowAnalyzer::is_this_expression(gmt::Ref<const ExpressionSyntax> expression)
{
	if (expression.is_null())
		return false;

	SyntaxKind kind = expression->get_kind();
	if (kind == SyntaxKind::MemberAccessExpression)
	{
		gmt::Ref<const MemberAccessExpressionSyntax> memberAccess = expression.as<const MemberAccessExpressionSyntax>();
		if (!memberAccess->get_previous().is_null())
			return false;
	}
	else if (kind != SyntaxKind::IdentifierNameType)
	{
		return false;
	}

	auto expressionRef = ref_from(expression.as_ptr());
	if (!expressionRef.has_value())
		return false;

	gmt::Ref<ParameterSymbol> parameter = lookup_symbol<ParameterSymbol>(*expressionRef);
	return !parameter.is_null() && parameter->get_name() == L"this";
}

void ControlFlowAnalyzer::record_field_or_property_write(gmt::Ref<const MemberAccessExpressionSyntax> node)
{
	if (node.is_null() || m_currentMethod.is_null())
		return;

	auto nodeRef = ref_from(node.as_ptr());
	if (!nodeRef.has_value())
		return;

	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
	if (bound.is_null())
		return;

	gmt::Ref<FieldSymbol> field = bound.as<FieldSymbol>();
	gmt::Ref<PropertySymbol> property = bound.as<PropertySymbol>();

	bool isStatic = false;
	if (!field.is_null())
		isStatic = field->get_linking() == LINK_STATIC;
	else if (!property.is_null())
		isStatic = property->get_linking() == LINK_STATIC;

	bool isThisReceiver = !isStatic;
	isThisReceiver = isThisReceiver && (node->get_previous().is_null() || is_this_expression(node->get_previous()));

	MethodEffectSummary& effects = m_currentMethod->get_effect_summary();

	if (!field.is_null())
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
	else if (!property.is_null())
	{
		// a property write is a call to the setter accessor; record its effects
		if (isStatic)
			effects.mutatesStatic = true;
		else if (isThisReceiver)
			effects.mutatesInstance = true;

		gmt::Ref<AccessorSymbol> setter = property->get_setter();
		if (!setter.is_null())
			record_callee_effects(setter, node->get_identifier());
	}
}

void ControlFlowAnalyzer::record_indexator_write(gmt::Ref<const IndexatorExpressionSyntax> node)
{
	if (node.is_null() || m_currentMethod.is_null())
		return;

	auto nodeRef = ref_from(node.as_ptr());
	if (!nodeRef.has_value())
		return;

	gmt::Ref<SyntaxSymbol> bound = get_symbol_table().lookup_symbol(*nodeRef);
	gmt::Ref<PropertySymbol> property = bound.as<PropertySymbol>();

	bool isStatic = !property.is_null();
	isStatic = isStatic && property->get_linking() == LINK_STATIC;

	bool isThisReceiver = !isStatic;
	isThisReceiver = isThisReceiver && (node->get_previous().is_null() || is_this_expression(node->get_previous()));

	MethodEffectSummary& effects = m_currentMethod->get_effect_summary();

	if (isStatic)
		effects.mutatesStatic = true;
	else if (isThisReceiver)
		effects.mutatesInstance = true;

	// merge setter effects if the indexer symbol is known
	if (!property.is_null())
	{
		gmt::Ref<AccessorSymbol> setter = property->get_setter();
		if (!setter.is_null())
			record_callee_effects(setter, node->get_delimeter_token());
	}
}

void ControlFlowAnalyzer::record_callee_effects(gmt::Ref<MethodSymbol> callee, const SyntaxToken& callSite, bool isAwaited, bool isTryWrapped)
{
	// a null callee records an unknown call site (delegate or unresolved call)
	// that still requires an explicit 'try' or an enclosing try-catch
	if (m_currentMethod.is_null())
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

bool ControlFlowAnalyzer::is_try_wrapped(gmt::Ref<const ExpressionSyntax> expression) const
{
	return m_tryWrappedExpressions.find(expression) != m_tryWrappedExpressions.end();
}

void ControlFlowAnalyzer::add_thrown_type(std::unordered_set<gmt::Ref<StructSymbol>>& thrownTypes, gmt::Ref<StructSymbol> type)
{
	if (type.is_null())
		return;

	for (gmt::Ref<StructSymbol> existing : thrownTypes)
	{
		bool sameType = existing == type;

		bool mutuallyAssignable = get_model().is_assignable_to(existing, type);
		mutuallyAssignable = mutuallyAssignable && get_model().is_assignable_to(type, existing);

		if (sameType || mutuallyAssignable)
			return;
	}

	thrownTypes.insert(type);
}

bool ControlFlowAnalyzer::call_edge_catches(const CallEdge& edge, gmt::Ref<StructSymbol> thrownType)
{
	if (thrownType.is_null())
		return false;

	if (edge.catchesAll)
		return true;

	for (gmt::Ref<StructSymbol> catchType : edge.caughtTypes)
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
			gmt::Ref<MethodSymbol> caller = pair.first;
			if (caller.is_null())
				continue;

			MethodEffectSummary previous = caller->get_effect_summary();

			for (const CallEdge& edge : pair.second)
			{
				gmt::Ref<MethodSymbol> callee = edge.callee;
				if (callee.is_null())
					continue;

				MethodEffectSummary& callerEffects = caller->get_effect_summary();
				const MethodEffectSummary& calleeEffects = callee->get_effect_summary();

				// mutations always propagate regardless of catch context
				callerEffects.mutatesInstance = callerEffects.mutatesInstance || calleeEffects.mutatesInstance;
				callerEffects.mutatesStatic = callerEffects.mutatesStatic || calleeEffects.mutatesStatic;
				callerEffects.mutatesArguments = callerEffects.mutatesArguments || calleeEffects.mutatesArguments;
				callerEffects.hasUnknownSideEffects = callerEffects.hasUnknownSideEffects || calleeEffects.hasUnknownSideEffects;

				std::unordered_set<gmt::Ref<StructSymbol>>* targetThrownSet = caller->get_is_async()
					? &callerEffects.asyncThrownTypes
					: &callerEffects.thrownTypes;

				if (callee->get_is_async())
				{
					// awaiting an async method surfaces the exceptions captured in its task
					if (edge.isAwaited)
					{
						for (gmt::Ref<StructSymbol> thrownType : calleeEffects.asyncThrownTypes)
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
					for (gmt::Ref<StructSymbol> thrownType : calleeEffects.thrownTypes)
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
		if (!pair.first.is_null())
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
			if (edge.callee.is_null())
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
	gmt::Ref<MethodSymbol> callee = edge.callee;
	const MethodEffectSummary& calleeEffects = callee->get_effect_summary();
	std::unordered_set<gmt::Ref<StructSymbol>> reportedTypes;

	// the 'try' operator explicitly allows the callee's exceptions to unwind
	// further; they are inherited as the caller's own effects, so no diagnostic
	if (edge.callee->get_is_async())
	{
		if (edge.isAwaited)
		{
			for (gmt::Ref<StructSymbol> thrownType : calleeEffects.asyncThrownTypes)
			{
				bool alreadyReported = reportedTypes.find(thrownType) != reportedTypes.end();
				if (!edge.isTryWrapped && !call_edge_catches(edge, thrownType) && !alreadyReported)
				{
					reportedTypes.insert(thrownType);

					std::wstring_view typeName = thrownType.is_null() ? L"unknown" : thrownType->get_name();
					get_diagnostics().report_error(
						edge.callSite,
						gmt::concat(L"Await on '", callee->get_name(), L"' may throw '", typeName, L"'"));
				}
			}
		}
	}
	else
	{
		for (gmt::Ref<StructSymbol> thrownType : calleeEffects.thrownTypes)
		{
			bool alreadyReported = reportedTypes.find(thrownType) != reportedTypes.end();
			if (!edge.isTryWrapped && !call_edge_catches(edge, thrownType) && !alreadyReported)
			{
				reportedTypes.insert(thrownType);

				std::wstring_view typeName = thrownType.is_null() ? L"unknown" : thrownType->get_name();
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
		std::wstring message = callee->get_is_async() && edge.isAwaited
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
	if (!m_currentMethod.is_null() && isAssignment && !node->get_left().is_null())
	{
		gmt::Ref<const ExpressionSyntax> left = node->get_left();
		if (left->get_kind() == SyntaxKind::MemberAccessExpression)
			record_field_or_property_write(left.as<const MemberAccessExpressionSyntax>());
		else if (left->get_kind() == SyntaxKind::IndexatorExpression)
			record_indexator_write(left.as<const IndexatorExpressionSyntax>());
	}

	SyntaxVisitor::visit_binary_expression(node);
}

void ControlFlowAnalyzer::visit_unary_expression(const UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// 'try <invokation>' explicitly allows the callee's exceptions to unwind
	// further; mark the operand so the invokation visit can recognize it
	if (node->get_operator_token().get_type() == TokenType::TryKeyword && !node->get_operand().is_null())
	{
		auto operandRef = ref_from(node->get_operand().as_ptr());
		if (operandRef.has_value())
			m_tryWrappedExpressions.insert(*operandRef);
	}

	bool isIncrement = is_increment_decrement_operator(node->get_operator_token().get_type());
	if (!m_currentMethod.is_null() && isIncrement && !node->get_operand().is_null())
	{
		gmt::Ref<const ExpressionSyntax> operand = node->get_operand();
		if (operand->get_kind() == SyntaxKind::MemberAccessExpression)
			record_field_or_property_write(operand.as<const MemberAccessExpressionSyntax>());
		else if (operand->get_kind() == SyntaxKind::IndexatorExpression)
			record_indexator_write(operand.as<const IndexatorExpressionSyntax>());
	}

	SyntaxVisitor::visit_unary_expression(node);
}

void ControlFlowAnalyzer::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!m_currentMethod.is_null())
	{
		gmt::Ref<MethodSymbol> callee;
		bool isTryWrapped = false;

		auto nodeRef = ref_from(node);
		if (nodeRef.has_value())
		{
			callee = lookup_symbol<MethodSymbol>(*nodeRef);
			isTryWrapped = is_try_wrapped(*nodeRef);
		}

		if (!callee.is_null())
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
				record_callee_effects(gmt::Ref<MethodSymbol>(), node->get_identifier());
			}
		}
	}

	SyntaxVisitor::visit_invokation_expression(node);
}

void ControlFlowAnalyzer::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!m_currentMethod.is_null())
	{
		gmt::Ref<ConstructorSymbol> ctor;
		auto nodeRef = ref_from(node);
		if (nodeRef.has_value())
			ctor = lookup_symbol<ConstructorSymbol>(*nodeRef);

		if (!ctor.is_null())
			record_callee_effects(ctor, node->get_new_token());
	}

	SyntaxVisitor::visit_object_creation_expression(node);
}

void ControlFlowAnalyzer::visit_await_expression(const AwaitExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!m_currentMethod.is_null() && !node->get_expression().is_null())
	{
		gmt::Ref<const ExpressionSyntax> awaited = node->get_expression();

		// direct await of an async method call: record the await edge so the
		// async callee's captured exceptions can propagate to this method
		if (awaited->get_kind() == SyntaxKind::InvokationExpression)
		{
			gmt::Ref<const InvokationExpressionSyntax> invocation = awaited.as<const InvokationExpressionSyntax>();

			gmt::Ref<MethodSymbol> callee;
			bool isTryWrapped = false;

			auto invocationRef = ref_from(invocation.as_ptr());
			if (invocationRef.has_value())
			{
				callee = lookup_symbol<MethodSymbol>(*invocationRef);
				isTryWrapped = is_try_wrapped(*invocationRef);
			}

			bool isAsyncCallee = !callee.is_null() && callee->get_is_async();
			if (isAsyncCallee)
				record_callee_effects(callee, invocation->get_identifier(), true, isTryWrapped);
		}

		// awaiting an unknown or non-async expression that could hold a faulted task
		// is treated conservatively as a potential throw
		gmt::Ref<StructSymbol> awaitedType = get_model().get_expression_type(awaited);
		if (!awaitedType.is_null() && SemanticModel::is_task_or_task_like(awaitedType))
		{
			if (awaited->get_kind() != SyntaxKind::InvokationExpression)
				m_currentMethod->get_effect_summary().mayThrow = true;
		}
	}

	SyntaxVisitor::visit_await_expression(node);
}
