#include <shard/compilation/AbstractEmiter.h>
#include <shard/compilation/ProgramVirtualImage.h>
#include <shard/compilation/OperationCode.h>

#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/SyntaxTree.h>

#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/symbols/LiteralSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>

#include <stdexcept>
#include <vector>

using namespace shard;

static void SetEntryPoint(DiagnosticsContext& diagnostics, std::vector<MethodSymbol*>& entryPointCandidates, SymbolTable* table, ProgramVirtualImage& program)
{
	if (entryPointCandidates.empty())
	{
		diagnostics.ReportError(SyntaxToken(), L"Entry point for script not found");
		return;
	}

	if (entryPointCandidates.size() > 1)
	{
		for (MethodSymbol* entry : entryPointCandidates)
		{
			MethodDeclarationSyntax* decl = static_cast<MethodDeclarationSyntax*>(table->GetSyntaxNode(entry));
			diagnostics.ReportError(decl->IdentifierToken, L"Script cannot have multiple entry points");
		}

		return;
	}

	program.EntryPoint = entryPointCandidates.front();
	return;
}

void AbstractEmiter::VisitSyntaxTree(SyntaxTree& tree)
{
	for (CompilationUnitSyntax* unit : tree.CompilationUnits)
		VisitCompilationUnit(unit);

	SetEntryPoint(Diagnostics, EntryPointCandidates, Table, Program);
}

void AbstractEmiter::VisitArgumentsList(ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	// reverse itteration for method stack loading
	for (auto riter = node->Arguments.rbegin(); riter != node->Arguments.rend(); riter++)
		VisitArgument(*riter);
}

void AbstractEmiter::VisitMethodDeclaration(MethodDeclarationSyntax* const node)
{
	GeneratingFor = LookupSymbol<MethodSymbol>(node);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	if (GeneratingFor->Name == L"Main")
	{
		EntryPointCandidates.push_back(GeneratingFor);
		if (GeneratingFor->Accesibility != SymbolAccesibility::Public)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should be public");

		if (!GeneratingFor->IsStatic)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should be static");

		if (GeneratingFor->IsExtern)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point cannot be external");

		if (GeneratingFor->Parameters.size() != 0)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should have empty parameters list");

		if (GeneratingFor->ReturnType != SymbolTable::Primitives::Void)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should have 'void' return type");

		TypeSymbol* withinType = static_cast<TypeSymbol*>(GeneratingFor->Parent);
		if (withinType->TypeParameters.size() > 0)
			Diagnostics.ReportError(node->IdentifierToken, L"Type containing entry point should not have any type parameters");
	}

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitConstructorDeclaration(ConstructorDeclarationSyntax* const node)
{
	GeneratingFor = LookupSymbol<ConstructorSymbol>(node);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	// TODO: add field initialization

	size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitAccessorDeclaration(AccessorDeclarationSyntax* const node)
{
	GeneratingFor = LookupSymbol<AccessorSymbol>(node);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitExpressionStatement(ExpressionStatementSyntax* const node)
{
	VisitExpression(node->Expression);
}

void AbstractEmiter::VisitVariableStatement(VariableStatementSyntax* const node)
{
	VariableSymbol* var = LookupSymbol<VariableSymbol>(node);
	VisitExpression(node->Expression);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, var->SlotIndex);
}

void AbstractEmiter::VisitReturnStatement(ReturnStatementSyntax* const node)
{
	VisitExpression(node->Expression);
	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
}

void AbstractEmiter::VisitThrowStatement(ThrowStatementSyntax* const node)
{
	VisitExpression(node->Expression);
	Encoder.EmitThrow(GeneratingFor->ExecutableByteCode);
}

void AbstractEmiter::VisitBreakStatement(BreakStatementSyntax* const node)
{
	LoopScope& scope = Loops.top();
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
}

void AbstractEmiter::VisitContinueStatement(ContinueStatementSyntax* const node)
{
	LoopScope& scope = Loops.top();
	scope.BlockEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
}

void AbstractEmiter::VisitWhileStatement(WhileStatementSyntax* const node)
{
	// Entering loop scope
	Loops.emplace();
	LoopScope& scope = Loops.top();

	// Getting loop starting position, current cursor pos
	scope.LoopStart = GeneratingFor->ExecutableByteCode.size();

	// Emiting looping condition expression
	VisitExpression(node->ConditionExpression);

	// Emiting jump to loop end if condition is false
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	// Emiting loop body
	VisitStatementsBlock(node->StatementsBlock);

	// Getting loop block ending and miting looping jump
	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	// Getting loop ending
	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	// Backtracking uninitialized jumps
	for (size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(size_t));

	for (size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(size_t));

	// Exiting loop scope
	Loops.pop();
}

void AbstractEmiter::VisitUntilStatement(UntilStatementSyntax* const node)
{
	// Entering loop scope
	Loops.emplace();
	LoopScope& scope = Loops.top();

	// Getting loop starting position, current cursor pos
	scope.LoopStart = GeneratingFor->ExecutableByteCode.size();

	// Emiting looping condition expression
	VisitExpression(node->ConditionExpression);

	// Emiting jump to loop end if condition is false
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpTrue(GeneratingFor->ExecutableByteCode, 0);

	// Emiting loop body
	VisitStatementsBlock(node->StatementsBlock);

	// Getting loop block ending and miting looping jump
	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	// Getting loop ending
	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	// Backtracking uninitialized jumps
	for (size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(size_t));

	for (size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(size_t));

	// Exiting loop scope
	Loops.pop();
}

void AbstractEmiter::VisitForStatement(ForStatementSyntax* const node)
{
	// Entering loop scope
	Loops.emplace();
	LoopScope& scope = Loops.top();

	// Emiting initializer expression
	VisitStatement(node->InitializerStatement);

	// Getting loop starting position, current cursor pos
	scope.LoopStart = GeneratingFor->ExecutableByteCode.size();

	// Emiting looping condition expression
	VisitExpression(node->ConditionExpression);

	// Emiting jump to loop end if condition is false
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	// Emiting loop body
	VisitStatementsBlock(node->StatementsBlock);
	VisitStatement(node->AfterRepeatStatement);

	// Getting loop block ending and miting looping jump
	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	// Getting loop ending
	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	// Backtracking uninitialized jumps
	for (size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(size_t));

	for (size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(size_t));

	// Exiting loop scope
	Loops.pop();
}

static bool IsConditionalClause(SyntaxKind kind)
{
	return kind == SyntaxKind::IfStatement
		|| kind == SyntaxKind::UnlessStatement
		|| kind == SyntaxKind::ElseStatement;
}

void AbstractEmiter::VisitIfStatement(IfStatementSyntax* const node)
{
	Clauses.emplace();
	ClauseScope& scope = Clauses.top();

	VisitStatement(node->ConditionExpression);

	size_t jumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
	scope.ClauseEndBacktracks.push_back(jumpAddress);

	VisitStatementsBlock(node->StatementsBlock);
	size_t bodyEnd = GeneratingFor->ExecutableByteCode.size();

	if (node->NextStatement != nullptr)
	{
		jumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(jumpAddress);

		VisitConditionalClause(node->NextStatement);
	}

	scope.ClauseEnd = GeneratingFor->ExecutableByteCode.size();
	for (size_t backtrack : scope.ClauseEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.ClauseEnd, sizeof(size_t));

	Clauses.pop();
}

void AbstractEmiter::VisitUnlessStatement(UnlessStatementSyntax* const node)
{
	Clauses.emplace();
	ClauseScope& scope = Clauses.top();

	VisitStatement(node->ConditionExpression);

	size_t jumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpTrue(GeneratingFor->ExecutableByteCode, 0);
	scope.ClauseEndBacktracks.push_back(jumpAddress);

	VisitStatementsBlock(node->StatementsBlock);
	size_t bodyEnd = GeneratingFor->ExecutableByteCode.size();

	if (node->NextStatement != nullptr)
	{
		jumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(jumpAddress);

		VisitConditionalClause(node->NextStatement);
	}

	scope.ClauseEnd = GeneratingFor->ExecutableByteCode.size();
	for (size_t backtrack : scope.ClauseEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.ClauseEnd, sizeof(size_t));

	Clauses.pop();
}

void AbstractEmiter::VisitElseStatement(ElseStatementSyntax* const node)
{
	VisitStatementsBlock(node->StatementsBlock);
}

void AbstractEmiter::VisitLiteralExpression(LiteralExpressionSyntax* const node)
{
	LiteralSymbol* const symbol = static_cast<LiteralSymbol*>(Table->LookupSymbol(node));
	switch (symbol->LiteralType)
	{
		case TokenType::NullLiteral:
		{
			Encoder.EmitLoadConstNull(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::CharLiteral:
		{
			Encoder.EmitLoadConstChar16(GeneratingFor->ExecutableByteCode, node->LiteralToken.Word[0]);
			break;
		}

		case TokenType::StringLiteral:
		{
			Encoder.EmitLoadConstString(GeneratingFor->ExecutableByteCode, Program.DataSection, node->LiteralToken.Word.data());
			break;
		}

		case TokenType::NumberLiteral:
		{
			Encoder.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, symbol->AsIntegerValue);
			break;
		}

		case TokenType::DoubleLiteral:
		{
			Encoder.EmitLoadConstDouble64(GeneratingFor->ExecutableByteCode, symbol->AsDoubleValue);
			break;
		}

		case TokenType::BooleanLiteral:
		{
			Encoder.EmitLoadConstBool(GeneratingFor->ExecutableByteCode, symbol->AsBooleanValue);
			break;
		}

		default:
			throw std::runtime_error("unsupported literal type");
	}
}

void AbstractEmiter::VisitUnaryExpression(UnaryExpressionSyntax* const node)
{
}

void AbstractEmiter::VisitObjectCreationExpression(ObjectExpressionSyntax* const node)
{
	VisitArgumentsList(node->ArgumentsList);
	Encoder.EmitNewObject(GeneratingFor->ExecutableByteCode, node->TypeSymbol);
}

void AbstractEmiter::VisitCollectionExpression(CollectionExpressionSyntax* const node)
{
}

void AbstractEmiter::VisitLambdaExpression(LambdaExpressionSyntax* const node)
{
}

void AbstractEmiter::VisitTernaryExpression(TernaryExpressionSyntax* const node)
{
}

static bool IsAssignExpression(TokenType type)
{
	switch (type)
	{
		default:
			return false;

		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
		case TokenType::AndAssignOperator:
		case TokenType::OrAssignOperator:
		case TokenType::AssignOperator:
			return true;
	}
}

void AbstractEmiter::VisitBinaryExpression(BinaryExpressionSyntax* const node)
{
	if (IsAssignExpression(node->OperatorToken.Type))
	{
		VisitAssignExpression(node);
		return;
	}

	VisitExpression(node->Left);
	VisitExpression(node->Right);

	switch (node->OperatorToken.Type)
	{
		case TokenType::AssignOperator:
		{
			// TODO: fix
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, 0);
			break;
		}

		case TokenType::EqualsOperator:
		{
			Encoder.EmitCompareEqual(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::NotEqualsOperator:
		{
			Encoder.EmitCompareNotEqual(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::LessOperator:
		{
			Encoder.EmitCompareLess(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::LessOrEqualsOperator:
		{
			Encoder.EmitCompareLessOrEqual(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::GreaterOperator:
		{
			Encoder.EmitCompareGreater(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::GreaterOrEqualsOperator:
		{
			Encoder.EmitCompareGreaterOrEqual(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::AddOperator:
		{
			Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::SubOperator:
		{
			Encoder.EmitMathSub(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::MultOperator:
		{
			Encoder.EmitMathMult(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::DivOperator:
		{
			Encoder.EmitMathDiv(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::ModOperator:
		{
			Encoder.EmitMathMod(GeneratingFor->ExecutableByteCode);
			break;
		}

		case TokenType::PowOperator:
		{
			Encoder.EmitMathPow(GeneratingFor->ExecutableByteCode);
			break;
		}

		default:
			throw std::runtime_error("unsupported operator type");
	}
}

static bool IsMemberAccess(const ExpressionSyntax* expression, const MemberAccessExpressionSyntax*& memberExpression)
{
	if (expression->Kind != SyntaxKind::MemberAccessExpression)
		return false;

	memberExpression = static_cast<const MemberAccessExpressionSyntax*>(expression);
	return true;
}

static bool IsFieldAccess(const MemberAccessExpressionSyntax* memberExpression, const ExpressionSyntax*& instanceExpression)
{
	instanceExpression = memberExpression->PreviousExpression;
	return memberExpression->ToField != nullptr || memberExpression->ToProperty != nullptr;
}

void AbstractEmiter::VisitAssignExpression(BinaryExpressionSyntax* const node)
{
	const MemberAccessExpressionSyntax* memberExpression = static_cast<const MemberAccessExpressionSyntax*>(node->Left);
	const ExpressionSyntax* instanceExpression = memberExpression->PreviousExpression;

	if (memberExpression->ToVariable != nullptr)
	{
		VisitVariableAssignExpression(node);
		return;
	}

	if (memberExpression->ToProperty != nullptr)
	{
		VisitVariableAssignExpression(node);
		return;
	}

	if (memberExpression->ToField != nullptr)
	{
		if (memberExpression->ToField->IsStatic)
		{
			VisitVariableAssignExpression(node);
			return;
		}

		VisitVariableAssignExpression(node);
		return;
	}

	/*
	if (isFieldAccess)
	{
		if (access->Kind == SyntaxKind::IndexatorExpression)
		{
			const IndexatorExpressionSyntax* indexator = static_cast<const IndexatorExpressionSyntax*>(access);
			InboundVariablesContext* arguments = CreateArgumentsContext(indexator->IndexatorList->Arguments, indexator->IndexatorSymbol->Parameters, indexator->IndexatorSymbol->IsStatic, instance);
			arguments->AddVariable(L"value", value);
			ExecuteMethod(indexator->IndexatorSymbol->Setter, instance->Info, arguments);
			return;
		}

		// Check if this is a property or field
		FieldSymbol* field = access->ToField;
		if (access->ToProperty != nullptr)
		{
			PropertySymbol* property = access->ToProperty;
			field = property->BackingField;

			if (property->Setter != nullptr)
			{
				InboundVariablesContext* setterArgs = new InboundVariablesContext(nullptr);
				if (!property->IsStatic)
					setterArgs->AddVariable(L"this", instance);

				setterArgs->AddVariable(L"value", value);
				ExecuteMethod(property->Setter, instance->Info, setterArgs);
				return;
			}
		}

		// Check if this is a static field
		if (field->IsStatic)
		{
			GarbageCollector::SetStaticField(field, value);
			return;
		}

		// Instance field assignment
		instance->SetField(field, value);
	}
	else
	{
		InboundVariablesContext* current = CurrentContext();
		std::wstring varName = memberExpression->IdentifierToken.Word;

		ObjectInstance* instanceReg = current->TryFind(varName);
		ObjectInstance* rightReg = EvaluateExpression(expression->Right);
		current->SetVariable(varName, rightReg);

		GarbageCollector::CollectInstance(instanceReg);
		GarbageCollector::CollectInstance(rightReg);
		return rightReg;
	}
	*/
}

void AbstractEmiter::VisitParameterAssignExpression(BinaryExpressionSyntax* const node, ParameterSymbol* param)
{
	switch (node->OperatorToken.Type)
	{
		case TokenType::AssignOperator:
		{
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, param->SlotIndex);
			break;
		}

		case TokenType::AddAssignOperator:
		{
			Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, param->SlotIndex);
			break;
		}
	}
}

void AbstractEmiter::VisitVariableAssignExpression(BinaryExpressionSyntax* const node, VariableSymbol* var)
{
	switch (node->OperatorToken.Type)
	{
		case TokenType::AssignOperator:
		{
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, var->SlotIndex);
			break;
		}

		case TokenType::AddAssignOperator:
		{
			Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, var->SlotIndex);
			break;
		}
	}
}

void AbstractEmiter::VisitPropertyAssignExpression(BinaryExpressionSyntax* const node, PropertySymbol* prop)
{
}

void AbstractEmiter::VisitFieldAssignExpression(BinaryExpressionSyntax* const node, FieldSymbol* field)
{
	switch (node->OperatorToken.Type)
	{
		case TokenType::AssignOperator:
		{
			Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, field);
			break;
		}

		case TokenType::AddAssignOperator:
		{
			Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
			Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, field);
			break;
		}
	}
}

void AbstractEmiter::VisitStaticFieldAssignExpression(BinaryExpressionSyntax* const node, FieldSymbol* field)
{
	switch (node->OperatorToken.Type)
	{
		case TokenType::AssignOperator:
		{
			Encoder.EmitStoreStaticField(GeneratingFor->ExecutableByteCode, field);
			break;
		}

		case TokenType::AddAssignOperator:
		{
			Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
			Encoder.EmitStoreStaticField(GeneratingFor->ExecutableByteCode, field);
			break;
		}
	}
}

void AbstractEmiter::VisitInvocationExpression(InvokationExpressionSyntax* const node)
{
	VisitArgumentsList(node->ArgumentsList);
	if (!node->Symbol->IsStatic)
		VisitExpression(node->PreviousExpression);

	Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitMemberAccessExpression(MemberAccessExpressionSyntax* const node)
{
	VisitExpression(node->PreviousExpression);

	if (node->ToProperty != nullptr)
	{

	}

	// TODO: Expand
	if (node->ToParameter != nullptr)
	{
		ParameterSymbol* param = node->ToParameter;
		Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, param->SlotIndex);
		return;
	}

	if (node->ToVariable != nullptr)
	{
		VariableSymbol* var = node->ToVariable;
		Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, var->SlotIndex);
		return;
	}

	if (node->ToField != nullptr)
	{
		FieldSymbol* field = node->ToField;
		if (field->IsStatic)
		{
			Encoder.EmitLoadStaticField(GeneratingFor->ExecutableByteCode, field);
			return;
		}

		Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, field);
		return;
	}
}

void AbstractEmiter::VisitIndexatorExpression(IndexatorExpressionSyntax* const node)
{
}
