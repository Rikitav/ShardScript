#include <shard/semantic/FlowAnalyzer.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/SwitchStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/SwitchCaseClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForInStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticScope.hpp>

using namespace shard;


// =====================================================================
//  Construction / entry point
// =====================================================================

FlowAnalyzer::FlowAnalyzer(SemanticModel& model, DiagnosticsContext& diagnostics)
	: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table.get()), Factory(model.Table.get())
{
}

void FlowAnalyzer::Analyze(SyntaxTree& syntaxTree)
{
	VisitSyntaxTree(syntaxTree);
	PropagateEffects();
}

// =====================================================================
//  Member body entry points
// =====================================================================

void FlowAnalyzer::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	if (node == nullptr || node->Body == nullptr)
		return;

	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return;

	EnterMemberBody(symbol);
	PushScope(symbol);

	for (TypeParameterSymbol* typeParam : symbol->TypeParameters)
		CurrentScope()->DeclareSymbol(typeParam);

	if (symbol->Linking == LINK_INSTANCE)
	{
		TypeSymbol* ownerType = symbol->Parent != nullptr && symbol->Parent->IsType()
			? static_cast<TypeSymbol*>(symbol->Parent)
			: nullptr;
		if (ownerType != nullptr)
			CurrentScope()->DeclareSymbol(Factory.Parameter(L"this", ownerType));
	}

	for (ParameterSymbol* parameter : symbol->Parameters)
	{
		CurrentScope()->DeclareSymbol(parameter);
		if (VariableSymbol* var = dynamic_cast<VariableSymbol*>(parameter))
			MarkAssigned(var);
	}

	AnalyzeMemberBody(symbol, node->Body.get(), node->IdentifierToken);

	PopScope();
	LeaveMemberBody(symbol);
}

void FlowAnalyzer::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
	if (node == nullptr || node->Body == nullptr)
		return;

	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return;

	EnterMemberBody(symbol);
	PushScope(symbol);

	if (symbol->Linking == LINK_INSTANCE)
	{
		TypeSymbol* ownerType = symbol->Parent != nullptr && symbol->Parent->IsType()
			? static_cast<TypeSymbol*>(symbol->Parent) : nullptr;

		if (ownerType != nullptr)
			CurrentScope()->DeclareSymbol(Factory.Parameter(L"this", ownerType));
	}

	for (ParameterSymbol* parameter : symbol->Parameters)
	{
		CurrentScope()->DeclareSymbol(parameter);
		if (VariableSymbol* var = dynamic_cast<VariableSymbol*>(parameter))
			MarkAssigned(var);
	}

	AnalyzeMemberBody(symbol, node->Body.get(), node->OperatorToken);
	PopScope();
	LeaveMemberBody(symbol);
}

void FlowAnalyzer::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	if (node == nullptr || node->Body == nullptr)
		return;

	AccessorSymbol* symbol = LookupSymbol<AccessorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return;

	EnterMemberBody(symbol);
	PushScope(symbol);

	PropertySymbol* propertySymbol = symbol->Parent != nullptr && symbol->Parent->Kind == SyntaxKind::PropertyDeclaration
		? static_cast<PropertySymbol*>(symbol->Parent) : nullptr;

	IndexatorSymbol* indexatorSymbol = symbol->Parent != nullptr && symbol->Parent->Kind == SyntaxKind::IndexatorDeclaration
		? static_cast<IndexatorSymbol*>(symbol->Parent) : nullptr;

	PropertySymbol* ownerProperty = propertySymbol;
	if (propertySymbol == nullptr && indexatorSymbol != nullptr)
		ownerProperty = static_cast<PropertySymbol*>(indexatorSymbol);

	if (ownerProperty != nullptr && symbol->Linking == LINK_INSTANCE)
	{
		TypeSymbol* ownerType = ownerProperty->Parent != nullptr && ownerProperty->Parent->IsType()
			? static_cast<TypeSymbol*>(ownerProperty->Parent) : nullptr;

		if (ownerType != nullptr)
			CurrentScope()->DeclareSymbol(Factory.Parameter(L"this", ownerType));
	}

	for (ParameterSymbol* parameter : symbol->Parameters)
	{
		CurrentScope()->DeclareSymbol(parameter);
		if (VariableSymbol* var = dynamic_cast<VariableSymbol*>(parameter))
			MarkAssigned(var);
	}

	if (node->KeywordToken.Type == TokenType::GetKeyword)
	{
		AnalyzeMemberBody(symbol, node->Body.get(), node->KeywordToken);
	}
	else
	{
		AnalyzeStatementsBlock(node->Body.get());
	}

	PopScope();
	LeaveMemberBody(symbol);
}

void FlowAnalyzer::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	if (node == nullptr || node->Body == nullptr)
		return;

	ConstructorSymbol* symbol = LookupSymbol<ConstructorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return;

	EnterMemberBody(symbol);
	PushScope(symbol);

	TypeSymbol* ownerType = symbol->Parent != nullptr && symbol->Parent->IsType()
		? static_cast<TypeSymbol*>(symbol->Parent)
		: nullptr;
	if (ownerType != nullptr)
		CurrentScope()->DeclareSymbol(Factory.Parameter(L"this", ownerType));

	for (ParameterSymbol* parameter : symbol->Parameters)
	{
		CurrentScope()->DeclareSymbol(parameter);
		if (VariableSymbol* var = dynamic_cast<VariableSymbol*>(parameter))
			MarkAssigned(var);
	}

	AnalyzeStatementsBlock(node->Body.get());

	PopScope();
	LeaveMemberBody(symbol);
}

void FlowAnalyzer::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	if (node == nullptr || node->Body == nullptr)
		return;

	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return;

	EnterMemberBody(symbol);
	PushScope(symbol);

	for (ParameterSymbol* parameter : symbol->Parameters)
	{
		CurrentScope()->DeclareSymbol(parameter);
		if (VariableSymbol* var = dynamic_cast<VariableSymbol*>(parameter))
			MarkAssigned(var);
	}

	AnalyzeMemberBody(symbol, node->Body.get(), node->LambdaToken);

	PopScope();
	LeaveMemberBody(symbol);
}

// =====================================================================
//  Body analysis
// =====================================================================

void FlowAnalyzer::AnalyzeMemberBody(SyntaxSymbol* symbol, StatementsBlockSyntax* body, SyntaxToken errorToken)
{
	if (body == nullptr)
		return;

	MethodSymbol* method = dynamic_cast<MethodSymbol*>(symbol);

	bool wasAsync = _currentMethodIsAsync;
	if (method != nullptr)
		_currentMethodIsAsync = method->IsAsync;

	FlowState bodyState = AnalyzeStatementsBlock(body);

	_currentMethodIsAsync = wasAsync;

	if (method == nullptr)
		return;

	if (method->IsAsync)
		return;

	TypeSymbol* returnType = method->ReturnType;
	if (returnType == nullptr || returnType == SymbolTable::Primitives::Void)
		return;

	bool alwaysTerminates = !HasFlag(bodyState, FlowState::Normal);
	bool onlyReturnOrThrow = !HasFlag(bodyState, FlowState::Breaks) && !HasFlag(bodyState, FlowState::Continues);

	if (!alwaysTerminates || !onlyReturnOrThrow)
	{
		// Point the diagnostic at the end of the body (closing brace or trailing semicolon) rather than the member name.
		SyntaxToken locationToken = errorToken;
		if (body->CloseBraceToken.Type == TokenType::CloseBrace)
			locationToken = body->CloseBraceToken;
		else if (!body->Statements.empty() && body->Statements.back()->SemicolonToken.Type == TokenType::Semicolon)
			locationToken = body->Statements.back()->SemicolonToken;

		ReportMissingReturn(locationToken, returnType);
	}
}

void FlowAnalyzer::ReportMissingReturn(SyntaxToken token, TypeSymbol* returnType)
{
	if (returnType == nullptr)
		Diagnostics.ReportError(token, L"Method must return a value");
	else
		Diagnostics.ReportError(token, L"Method must return a value of type '" + returnType->Name + L"'");
}

// =====================================================================
//  Flow state helpers
// =====================================================================

FlowAnalyzer::FlowState FlowAnalyzer::MergeBranches(FlowState a, FlowState b)
{
	return a | b;
}

FlowAnalyzer::FlowState FlowAnalyzer::WithoutBreaksAndContinues(FlowState state)
{
	FlowState result = state & ~(FlowState::Breaks | FlowState::Continues);
	// A break/continue consumed by its enclosing loop or switch leaves
	// Normal flow behind unless the body also returns or throws.
	if (!HasFlag(result, FlowState::Normal) &&
		!HasFlag(result, FlowState::Returns) &&
		!HasFlag(result, FlowState::Throws))
	{
		result = FlowState::Normal;
	}
	return result;
}

FlowAnalyzer::FlowState FlowAnalyzer::WithoutThrows(FlowState state)
{
	FlowState result = state & ~FlowState::Throws;

	// A caught throw no longer aborts the method;
	// if nothing else can escape, the try/catch falls through normally.
	if (!HasFlag(result, FlowState::Normal) &&
		!HasFlag(result, FlowState::Returns) &&
		!HasFlag(result, FlowState::Breaks) &&
		!HasFlag(result, FlowState::Continues))
	{
		result = FlowState::Normal;
	}

	return result;
}

// =====================================================================
//  Statements block analysis
// =====================================================================

void FlowAnalyzer::VisitStatementsBlock(StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	for (const auto& statement : node->Statements)
		AnalyzeStatement(statement.get());
}

void FlowAnalyzer::VisitStatement(StatementSyntax* node)
{
	// Top-level statement visits outside of a member body are no-ops.
	// Flow analysis is driven from member bodies via AnalyzeStatement.
}

FlowAnalyzer::FlowState FlowAnalyzer::AnalyzeStatement(StatementSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	switch (node->Kind)
	{
		case SyntaxKind::ExpressionStatement:
		{
			ExpressionStatementSyntax* statement = static_cast<ExpressionStatementSyntax*>(node);
			if (statement->Expression != nullptr)
			{
				AnalyzeExpressionForAssignment(statement->Expression.get());
				AnalyzeExpressionForUse(statement->Expression.get());
				SyntaxVisitor::VisitExpression(statement->Expression.get());
			}

			return FlowState::Normal;
		}

		case SyntaxKind::VariableStatement:
		{
			VariableStatementSyntax* statement = static_cast<VariableStatementSyntax*>(node);
			if (statement->Expression != nullptr)
				SyntaxVisitor::VisitExpression(statement->Expression.get());

			VisitVariableStatement(statement);
			return FlowState::Normal;
		}

		case SyntaxKind::BreakStatement:
		{
			BreakStatementSyntax* statement = static_cast<BreakStatementSyntax*>(node);
			if (_loopDepth == 0 && _switchDepth == 0)
				Diagnostics.ReportError(statement->KeywordToken, L"Break statement must be inside a loop or switch");

			return FlowState::Breaks;
		}

		case SyntaxKind::ContinueStatement:
		{
			ContinueStatementSyntax* statement = static_cast<ContinueStatementSyntax*>(node);
			if (_loopDepth == 0)
				Diagnostics.ReportError(statement->KeywordToken, L"Continue statement must be inside a loop");

			return FlowState::Continues;
		}

		case SyntaxKind::DeferStatement:
		{
			DeferStatementSyntax* statement = static_cast<DeferStatementSyntax*>(node);
			if (statement->Statement != nullptr)
				AnalyzeStatement(statement->Statement.get());

			return FlowState::Normal;
		}

		case SyntaxKind::WhileStatement:
		case SyntaxKind::UntilStatement:
		case SyntaxKind::ForStatement:
		case SyntaxKind::ForEachStatement:
		case SyntaxKind::ForInStatement:
		{
			SyntaxVisitor::VisitStatement(node);
			return FlowState::Normal;
		}

		case SyntaxKind::IfStatement:
		case SyntaxKind::UnlessStatement:
		case SyntaxKind::ElseStatement:
			return AnalyzeConditionalChain(static_cast<ConditionalClauseBaseSyntax*>(node));

		case SyntaxKind::SwitchStatement:
			return AnalyzeSwitchStatement(static_cast<SwitchStatementSyntax*>(node));

		case SyntaxKind::TryStatement:
			return AnalyzeTryStatement(static_cast<TryStatementSyntax*>(node));

		case SyntaxKind::ReturnStatement:
		{
			ReturnStatementSyntax* statement = static_cast<ReturnStatementSyntax*>(node);
			if (statement->Expression != nullptr)
				ScanExpression(statement->Expression.get());
			return FlowState::Returns;
		}

		case SyntaxKind::ThrowStatement:
		{
			ThrowStatementSyntax* statement = static_cast<ThrowStatementSyntax*>(node);
			if (statement->Expression != nullptr)
				ScanExpression(statement->Expression.get());

			if (_currentMethod != nullptr)
			{
				_currentMethod->EffectSummary.MayThrow = true;

				TypeSymbol* thrownType = SymbolTable::Primitives::Any;
				if (Model != nullptr && statement->Expression != nullptr)
					thrownType = Model->GetExpressionType(statement->Expression.get());

				if (thrownType == nullptr)
					thrownType = SymbolTable::Primitives::Any;

				if (!IsCaughtByActiveTry(thrownType))
					_currentMethod->EffectSummary.ThrownTypes.insert(thrownType);
			}

			return FlowState::Throws;
		}

		default:
			return FlowState::Normal;
	}
}

FlowAnalyzer::FlowState FlowAnalyzer::AnalyzeStatementsBlock(StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	FlowState combined = FlowState::Normal;

	for (const auto& statement : node->Statements)
	{
		if (IsTerminal(combined))
		{
			if (!_currentMethodIsAsync)
				Diagnostics.ReportError(statement->SemicolonToken, L"Unreachable code detected");
			break;
		}

		combined = AnalyzeStatement(statement.get());
	}

	return combined;
}

// =====================================================================
//  Conditional branches
// =====================================================================

void FlowAnalyzer::VisitIfStatement(IfStatementSyntax* node)
{
	// Flow analysis is driven through AnalyzeStatement / AnalyzeConditionalChain.
}

void FlowAnalyzer::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	// Flow analysis is driven through AnalyzeStatement / AnalyzeConditionalChain.
}

void FlowAnalyzer::VisitElseStatement(ElseStatementSyntax* node)
{
	// Flow analysis is driven through AnalyzeStatement / AnalyzeConditionalChain.
}

void FlowAnalyzer::VisitConditionalClause(ConditionalClauseBaseSyntax* node)
{
	// Flow analysis is driven through AnalyzeStatement / AnalyzeConditionalChain.
}

FlowAnalyzer::FlowState FlowAnalyzer::AnalyzeConditionalChain(ConditionalClauseBaseSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	if (node->Kind == SyntaxKind::ElseStatement)
	{
		ElseStatementSyntax* elseClause = static_cast<ElseStatementSyntax*>(node);
		FlowState elseState = elseClause->StatementsBlock != nullptr
			? AnalyzeStatementsBlock(elseClause->StatementsBlock.get())
			: FlowState::Normal;

		if (elseClause->NextStatement != nullptr)
			elseState = MergeBranches(elseState, AnalyzeConditionalChain(elseClause->NextStatement.get()));

		return elseState;
	}

	ConditionalClauseSyntax* clause = static_cast<ConditionalClauseSyntax*>(node);
	if (clause->ConditionExpression != nullptr)
		ScanStatement(clause->ConditionExpression.get());

	FlowState thenState = clause->StatementsBlock != nullptr
		? AnalyzeStatementsBlock(clause->StatementsBlock.get())
		: FlowState::Normal;

	FlowState elseState = FlowState::Normal;
	if (clause->NextStatement != nullptr)
		elseState = AnalyzeConditionalChain(clause->NextStatement.get());

	return MergeBranches(thenState, elseState);
}

// =====================================================================
//  Switch
// =====================================================================

void FlowAnalyzer::VisitSwitchStatement(SwitchStatementSyntax* node)
{
	// Flow analysis is driven through AnalyzeStatement.
}

FlowAnalyzer::FlowState FlowAnalyzer::AnalyzeSwitchStatement(SwitchStatementSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	if (node->Expression != nullptr)
		ScanExpression(node->Expression.get());

	++_switchDepth;
	AssignmentSet savedAssignments = _assignedVariables;

	FlowState combined = FlowState::Normal;
	bool hasCases = false;

	for (const auto& clause : node->Clauses)
	{
		if (clause == nullptr || clause->Body == nullptr)
			continue;

		if (clause->Pattern != nullptr)
			ScanExpression(clause->Pattern.get());

		FlowState caseState = WithoutBreaksAndContinues(AnalyzeStatementsBlock(clause->Body.get()));

		if (!hasCases)
		{
			combined = caseState;
			hasCases = true;
		}
		else
		{
			combined = MergeBranches(combined, caseState);
		}
	}

	_assignedVariables = std::move(savedAssignments);
	--_switchDepth;

	// A switch with no clauses can fall through.
	// Otherwise preserve all flow flags (including Normal) so callers see every possible outcome.
	if (!hasCases)
		return FlowState::Normal;

	return combined;
}

// =====================================================================
//  Try / catch
// =====================================================================

void FlowAnalyzer::VisitTryStatement(TryStatementSyntax* node)
{
	// Flow analysis is driven through AnalyzeStatement.
}

FlowAnalyzer::FlowState FlowAnalyzer::AnalyzeTryStatement(TryStatementSyntax* node)
{
	if (node == nullptr)
		return FlowState::Normal;

	bool hasCatch = !node->CatchClauses.empty();
	if (hasCatch && node->TryBlock != nullptr)
		PushTryCatch(node->CatchClauses);

	FlowState tryState = node->TryBlock != nullptr
		? AnalyzeStatementsBlock(node->TryBlock.get())
		: FlowState::Normal;

	if (hasCatch && node->TryBlock != nullptr)
		PopTryCatch();

	if (!hasCatch)
		return tryState;

	// Break/continue inside try/catch propagate to the enclosing loop/switch,
	// but throws from the try block are caught by the catch clauses.
	tryState = WithoutThrows(tryState);

	FlowState catchState = FlowState::Normal;
	bool firstCatch = true;

	for (const auto& catchClause : node->CatchClauses)
	{
		if (catchClause == nullptr || catchClause->Body == nullptr)
			continue;

		if (catchClause->Symbol != nullptr)
			MarkAssigned(catchClause->Symbol);

		if (firstCatch)
		{
			catchState = AnalyzeStatementsBlock(catchClause->Body.get());
			firstCatch = false;
		}
		else
		{
			catchState = MergeBranches(catchState, AnalyzeStatementsBlock(catchClause->Body.get()));
		}
	}

	return MergeBranches(tryState, catchState);
}

// =====================================================================
//  Try / catch context helpers
// =====================================================================

void FlowAnalyzer::PushTryCatch(const std::vector<std::unique_ptr<CatchClauseSyntax>>& clauses)
{
	TryCatchFrame frame;
	for (const auto& clause : clauses)
	{
		if (clause == nullptr)
			continue;

		if (clause->ExceptionType == nullptr)
		{
			frame.CatchesAll = true;
			break;
		}

		TypeSymbol* catchType = clause->ExceptionType->Symbol;
		if (catchType != nullptr)
			frame.CaughtTypes.insert(catchType);
	}

	_tryCatchStack.push_back(std::move(frame));
	RebuildEffectiveCaughtTypes();
}

void FlowAnalyzer::PopTryCatch()
{
	if (!_tryCatchStack.empty())
	{
		_tryCatchStack.pop_back();
		RebuildEffectiveCaughtTypes();
	}
}

void FlowAnalyzer::RebuildEffectiveCaughtTypes()
{
	_effectiveCaughtTypes.clear();
	_catchAllDepth = 0;

	for (const TryCatchFrame& frame : _tryCatchStack)
	{
		if (frame.CatchesAll)
		{
			++_catchAllDepth;
		}
		else
		{
			for (TypeSymbol* type : frame.CaughtTypes)
				_effectiveCaughtTypes.insert(type);
		}
	}
}

bool FlowAnalyzer::IsCaughtByActiveTry(TypeSymbol* thrownType) const
{
	if (thrownType == nullptr)
		return false;

	if (_catchAllDepth > 0)
		return true;

	for (TypeSymbol* catchType : _effectiveCaughtTypes)
	{
		if (SemanticModel::IsAssignableTo(catchType, thrownType))
			return true;
	}

	return false;
}

bool FlowAnalyzer::CallEdge::Catches(TypeSymbol* thrownType) const
{
	if (thrownType == nullptr)
		return false;

	if (CatchesAll)
		return true;

	for (TypeSymbol* catchType : CaughtTypes)
	{
		if (SemanticModel::IsAssignableTo(catchType, thrownType))
			return true;
	}

	return false;
}

// =====================================================================
//  Loops
// =====================================================================

FlowAnalyzer::FlowState FlowAnalyzer::AnalyzeLoopBody(StatementsBlockSyntax* body)
{
	if (body == nullptr)
		return FlowState::Normal;

	++_loopDepth;
	AssignmentSet savedAssignments = _assignedVariables;
	FlowState bodyState = AnalyzeStatementsBlock(body);
	_assignedVariables = std::move(savedAssignments);
	--_loopDepth;

	// Break/continue are consumed by the loop.
	// The loop itself may or may not execute, so it always yields Normal flow to the caller.
	return FlowState::Normal;
}

void FlowAnalyzer::VisitWhileStatement(WhileStatementSyntax* node)
{
	if (node == nullptr || node->StatementsBlock == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		ScanExpression(node->ConditionExpression.get());

	AnalyzeLoopBody(node->StatementsBlock.get());
}

void FlowAnalyzer::VisitUntilStatement(UntilStatementSyntax* node)
{
	if (node == nullptr || node->StatementsBlock == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		ScanExpression(node->ConditionExpression.get());

	AnalyzeLoopBody(node->StatementsBlock.get());
}

void FlowAnalyzer::VisitForStatement(ForStatementSyntax* node)
{
	if (node == nullptr || node->StatementsBlock == nullptr)
		return;

	if (node->InitializerStatement != nullptr)
		AnalyzeStatement(node->InitializerStatement.get());

	if (node->ConditionExpression != nullptr)
		ScanExpression(node->ConditionExpression.get());

	AnalyzeLoopBody(node->StatementsBlock.get());

	if (node->AfterRepeatStatement != nullptr)
		AnalyzeStatement(node->AfterRepeatStatement.get());
}

void FlowAnalyzer::VisitForEachStatement(ForEachStatementSyntax* node)
{
	if (node == nullptr || node->StatementsBlock == nullptr)
		return;

	if (node->RangeExpression != nullptr)
		ScanExpression(node->RangeExpression.get());

	AnalyzeLoopBody(node->StatementsBlock.get());
}

void FlowAnalyzer::VisitForInStatement(ForInStatementSyntax* node)
{
	if (node == nullptr || node->StatementsBlock == nullptr)
		return;

	if (node->RangeExpression != nullptr)
		ScanExpression(node->RangeExpression.get());

	AnalyzeLoopBody(node->StatementsBlock.get());
}

// =====================================================================
//  Definite assignment helpers
// =====================================================================

void FlowAnalyzer::MarkAssigned(VariableSymbol* variable)
{
	if (variable != nullptr)
		_assignedVariables.insert(variable);
}

bool FlowAnalyzer::IsAssigned(VariableSymbol* variable) const
{
	return variable != nullptr && _assignedVariables.find(variable) != _assignedVariables.end();
}

void FlowAnalyzer::AnalyzeExpressionForAssignment(ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	if (expression->Kind != SyntaxKind::BinaryExpression)
		return;

	BinaryExpressionSyntax* binary = static_cast<BinaryExpressionSyntax*>(expression);
	if (binary->OperatorToken.Type != TokenType::AssignOperator)
		return;

	if (binary->Left == nullptr || binary->Left->Kind != SyntaxKind::MemberAccessExpression)
		return;

	MemberAccessExpressionSyntax* left = static_cast<MemberAccessExpressionSyntax*>(binary->Left.get());
	if (left->PreviousExpression != nullptr)
		return;

	SyntaxSymbol* symbol = CurrentScope()->Lookup(left->IdentifierToken.Word).value_or(nullptr);
	if (symbol == nullptr)
		return;

	if (VariableSymbol* variable = dynamic_cast<VariableSymbol*>(symbol))
		MarkAssigned(variable);
}

void FlowAnalyzer::AnalyzeExpressionForUse(ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	switch (expression->Kind)
	{
		case SyntaxKind::MemberAccessExpression:
		{
			MemberAccessExpressionSyntax* memberAccess = static_cast<MemberAccessExpressionSyntax*>(expression);
			if (memberAccess->PreviousExpression == nullptr)
			{
				SyntaxSymbol* symbol = CurrentScope()->Lookup(memberAccess->IdentifierToken.Word).value_or(nullptr);
				if (symbol != nullptr)
				{
					if (VariableSymbol* variable = dynamic_cast<VariableSymbol*>(symbol))
					{
						if (!IsAssigned(variable))
							Diagnostics.ReportError(memberAccess->IdentifierToken, L"Use of unassigned local variable '" + variable->Name + L"'");
					}
				}
			}
		
			break;
		}

		case SyntaxKind::BinaryExpression:
		{
			BinaryExpressionSyntax* binary = static_cast<BinaryExpressionSyntax*>(expression);
			AnalyzeExpressionForUse(binary->Left.get());
			AnalyzeExpressionForUse(binary->Right.get());
			break;
		}
	}
}

void FlowAnalyzer::VisitVariableStatement(VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	SyntaxSymbol* symbol = Table->LookupSymbol(node).value_or(nullptr);
	if (symbol == nullptr)
		return;

	VariableSymbol* variable = dynamic_cast<VariableSymbol*>(symbol);
	if (variable == nullptr)
		return;

	if (node->Expression != nullptr)
	{
		AnalyzeExpressionForUse(node->Expression.get());
		MarkAssigned(variable);
	}
}

// =====================================================================
//  Side-effect / mutation scanning
// =====================================================================

void FlowAnalyzer::EnterMemberBody(MethodSymbol* method)
{
	_currentMethod = method;
	if (method != nullptr)
		method->EffectsComputed = false;
}

void FlowAnalyzer::LeaveMemberBody(MethodSymbol* method)
{
	if (method != nullptr)
		method->EffectsComputed = true;

	_currentMethod = nullptr;
}

void FlowAnalyzer::ScanExpression(ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return;

	SyntaxVisitor::VisitExpression(expression);
}

void FlowAnalyzer::ScanStatement(StatementSyntax* statement)
{
	if (statement == nullptr)
		return;

	SyntaxVisitor::VisitStatement(statement);
}

bool FlowAnalyzer::IsAssignmentOperator(TokenType type)
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
		case TokenType::OrAssignOperator:
		case TokenType::AndAssignOperator:
			return true;

		default:
			return false;
	}
}

bool FlowAnalyzer::IsIncrementDecrementOperator(TokenType type)
{
	return type == TokenType::IncrementOperator || type == TokenType::DecrementOperator;
}

bool FlowAnalyzer::IsThisExpression(ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return false;

	if (expression->Kind != SyntaxKind::MemberAccessExpression)
		return false;

	MemberAccessExpressionSyntax* memberAccess = static_cast<MemberAccessExpressionSyntax*>(expression);
	if (memberAccess->PreviousExpression != nullptr)
		return false;

	ParameterSymbol* param = memberAccess->ToParameter;
	return param != nullptr && param->Name == L"this";
}

void FlowAnalyzer::RecordFieldOrPropertyWrite(MemberAccessExpressionSyntax* node)
{
	if (node == nullptr || _currentMethod == nullptr)
		return;

	bool isStaticContext = node->IsStaticContext;
	bool isThisReceiver = !isStaticContext && (node->PreviousExpression == nullptr || IsThisExpression(node->PreviousExpression.get()));

	if (node->ToField != nullptr)
	{
		FieldSymbol* field = node->ToField;
		if (isStaticContext)
		{
			_currentMethod->EffectSummary.MutatesStatic = true;
			_currentMethod->EffectSummary.MayAssignStaticFields.insert(field);
		}
		else if (isThisReceiver)
		{
			_currentMethod->EffectSummary.MutatesInstance = true;
			_currentMethod->EffectSummary.MayAssignInstanceFields.insert(field);
		}
	}
	else if (node->ToProperty != nullptr)
	{
		// A property write is a call to the setter accessor; record its effects.
		PropertySymbol* property = node->ToProperty;
		if (isStaticContext)
		{
			_currentMethod->EffectSummary.MutatesStatic = true;
		}
		else if (isThisReceiver)
		{
			_currentMethod->EffectSummary.MutatesInstance = true;
		}

		if (property->Setter != nullptr)
			RecordCalleeEffects(property->Setter, node->IdentifierToken);
	}
}

void FlowAnalyzer::RecordIndexatorWrite(IndexatorExpressionSyntax* node)
{
	if (node == nullptr || _currentMethod == nullptr)
		return;

	bool isStaticContext = node->IsStaticContext;
	bool isThisReceiver = !isStaticContext && (node->PreviousExpression == nullptr || IsThisExpression(node->PreviousExpression.get()));

	if (isStaticContext)
		_currentMethod->EffectSummary.MutatesStatic = true;
	else if (isThisReceiver)
		_currentMethod->EffectSummary.MutatesInstance = true;

	// Merge setter effects if the indexer symbol is known.
	if (node->ToProperty != nullptr)
	{
		PropertySymbol* property = node->ToProperty;
		if (property->Setter != nullptr)
			RecordCalleeEffects(property->Setter, node->IdentifierToken);
	}
}

void FlowAnalyzer::RecordCalleeEffects(MethodSymbol* callee, const SyntaxToken& callSite)
{
	if (callee == nullptr || _currentMethod == nullptr)
		return;

	CallEdge edge;
	edge.Callee = callee;
	edge.CallSite = callSite;
	edge.CaughtTypes = _effectiveCaughtTypes;
	edge.CatchesAll = _catchAllDepth > 0;

	_callGraph[_currentMethod].push_back(std::move(edge));
}

void FlowAnalyzer::PropagateEffects()
{
	bool changed = true;
	while (changed)
	{
		changed = false;

		for (auto& pair : _callGraph)
		{
			MethodSymbol* caller = pair.first;
			if (caller == nullptr)
				continue;

			MethodEffectSummary previous = caller->EffectSummary;

			for (const CallEdge& edge : pair.second)
			{
				MethodSymbol* callee = edge.Callee;
				if (callee == nullptr)
					continue;

				// Mutations always propagate regardless of catch context.
				caller->EffectSummary.MayThrow = caller->EffectSummary.MayThrow || callee->EffectSummary.MayThrow;
				caller->EffectSummary.MutatesInstance = caller->EffectSummary.MutatesInstance || callee->EffectSummary.MutatesInstance;
				caller->EffectSummary.MutatesStatic = caller->EffectSummary.MutatesStatic || callee->EffectSummary.MutatesStatic;
				caller->EffectSummary.MutatesArguments = caller->EffectSummary.MutatesArguments || callee->EffectSummary.MutatesArguments;
				caller->EffectSummary.HasUnknownSideEffects = caller->EffectSummary.HasUnknownSideEffects || callee->EffectSummary.HasUnknownSideEffects;

				// Only exception types that are not caught at the call site propagate.
				for (TypeSymbol* thrownType : callee->EffectSummary.ThrownTypes)
				{
					if (!edge.Catches(thrownType))
						caller->EffectSummary.ThrownTypes.insert(thrownType);
				}
			}

			if (previous != caller->EffectSummary)
				changed = true;
		}
	}

	for (auto& pair : _callGraph)
	{
		if (pair.first != nullptr)
			pair.first->EffectsComputed = true;
	}

	ReportEffectDiagnostics();
}

void FlowAnalyzer::ReportEffectDiagnostics()
{
	for (const auto& pair : _callGraph)
	{
		MethodSymbol* caller = pair.first;
		if (caller == nullptr)
			continue;

		for (const CallEdge& edge : pair.second)
		{
			MethodSymbol* callee = edge.Callee;
			if (callee == nullptr)
				continue;

			for (TypeSymbol* thrownType : callee->EffectSummary.ThrownTypes)
			{
				if (!edge.Catches(thrownType))
				{
					std::wstring typeName = thrownType != nullptr ? thrownType->Name : L"unknown";
					Diagnostics.ReportWarning(
						edge.CallSite,
						L"Call to '" + callee->Name + L"' may throw '" + typeName + L"'");
				}
			}

			if (callee->EffectSummary.MayThrow && callee->EffectSummary.ThrownTypes.empty())
			{
				Diagnostics.ReportWarning(
					edge.CallSite,
					L"Call to '" + callee->Name + L"' may throw an exception");
			}
		}
	}
}

void FlowAnalyzer::VisitBinaryExpression(BinaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (_currentMethod != nullptr && IsAssignmentOperator(node->OperatorToken.Type) && node->Left != nullptr)
	{
		ExpressionSyntax* left = node->Left.get();
		if (left->Kind == SyntaxKind::MemberAccessExpression)
		{
			RecordFieldOrPropertyWrite(static_cast<MemberAccessExpressionSyntax*>(left));
		}
		else if (left->Kind == SyntaxKind::IndexatorExpression)
		{
			RecordIndexatorWrite(static_cast<IndexatorExpressionSyntax*>(left));
		}
	}

	SyntaxVisitor::VisitBinaryExpression(node);
}

void FlowAnalyzer::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (_currentMethod != nullptr && IsIncrementDecrementOperator(node->OperatorToken.Type) && node->Expression != nullptr)
	{
		ExpressionSyntax* operand = node->Expression.get();
		if (operand->Kind == SyntaxKind::MemberAccessExpression)
		{
			RecordFieldOrPropertyWrite(static_cast<MemberAccessExpressionSyntax*>(operand));
		}
		else if (operand->Kind == SyntaxKind::IndexatorExpression)
		{
			RecordIndexatorWrite(static_cast<IndexatorExpressionSyntax*>(operand));
		}
	}

	SyntaxVisitor::VisitUnaryExpression(node);
}

void FlowAnalyzer::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (_currentMethod != nullptr)
	{
		MethodSymbol* callee = node->Symbol;
		if (callee != nullptr && !node->IsDelegateInvocation)
		{
			RecordCalleeEffects(callee, node->IdentifierToken);
		}
		else
		{
			_currentMethod->EffectSummary.HasUnknownSideEffects = true;
		}
	}

	SyntaxVisitor::VisitInvocationExpression(node);
}

void FlowAnalyzer::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (_currentMethod != nullptr && node->CtorSymbol != nullptr)
	{
		RecordCalleeEffects(node->CtorSymbol, node->NewToken);
	}

	SyntaxVisitor::VisitObjectCreationExpression(node);
}

void FlowAnalyzer::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	// Reads through indexers are handled by the base visitor; writes are
	// detected from the parent assignment in VisitBinaryExpression.
	SyntaxVisitor::VisitIndexatorExpression(node);
}
