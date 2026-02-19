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

const int ReserveMultiplier = 25;

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
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
			return true;
	}
}

static void EmitUnaryOperation(TokenType type, ByteCodeEncoder& encoder, std::vector<std::byte>& code, bool isRightDetermined)
{
	switch (type)
	{
		case TokenType::SubOperator:
		{
			encoder.EmitMathNegative(code);
			break;
		}

		case TokenType::AddOperator:
		{
			encoder.EmitMathPositive(code);
			break;
		}

		case TokenType::IncrementOperator:
		{
			if (isRightDetermined)
			{
				encoder.EmitLoadConstInt64(code, 1);
				encoder.EmitMathAdd(code);
				encoder.EmitDuplicate(code);
				break;
			}
			else
			{
				encoder.EmitDuplicate(code);
				encoder.EmitLoadConstInt64(code, 1);
				encoder.EmitMathAdd(code);
				break;
			}

			break;
		}

		case TokenType::DecrementOperator:
		{
			if (isRightDetermined)
			{
				encoder.EmitLoadConstInt64(code, 1);
				encoder.EmitMathSub(code);
				encoder.EmitDuplicate(code);
				break;
			}
			else
			{
				encoder.EmitDuplicate(code);
				encoder.EmitLoadConstInt64(code, 1);
				encoder.EmitMathSub(code);
				break;
			}
		}

		default:
			throw std::runtime_error("unknown operator");
	}
}

static void EmitBinaryOperation(TokenType type, ByteCodeEncoder& encoder, std::vector<std::byte>& code)
{
	switch (type)
	{
		case TokenType::AssignOperator:
			break;

		case TokenType::AddOperator:
		case TokenType::AddAssignOperator:
		{
			encoder.EmitMathAdd(code);
			break;
		}

		case TokenType::SubOperator:
		case TokenType::SubAssignOperator:
		{
			encoder.EmitMathSub(code);
			break;
		}

		case TokenType::MultOperator:
		case TokenType::MultAssignOperator:
		{
			encoder.EmitMathMult(code);
			break;
		}

		case TokenType::DivOperator:
		case TokenType::DivAssignOperator:
		{
			encoder.EmitMathDiv(code);
			break;
		}

		case TokenType::ModOperator:
		case TokenType::ModAssignOperator:
		{
			encoder.EmitMathMod(code);
			break;
		}

		case TokenType::PowOperator:
		case TokenType::PowAssignOperator:
		{
			encoder.EmitMathPow(code);
			break;
		}

		case TokenType::EqualsOperator:
		{
			encoder.EmitCompareEqual(code);
			break;
		}

		case TokenType::NotEqualsOperator:
		{
			encoder.EmitCompareNotEqual(code);
			break;
		}

		case TokenType::GreaterOperator:
		{
			encoder.EmitCompareGreater(code);
			break;
		}

		case TokenType::GreaterOrEqualsOperator:
		{
			encoder.EmitCompareGreaterOrEqual(code);
			break;
		}

		case TokenType::LessOperator:
		{
			encoder.EmitCompareLess(code);
			break;
		}

		case TokenType::LessOrEqualsOperator:
		{
			encoder.EmitCompareLessOrEqual(code);
			break;
		}

		case TokenType::OrOperator:
		case TokenType::OrAssignOperator:
		{
			encoder.EmitLogicalOr(code);
			break;
		}

		case TokenType::AndOperator:
		case TokenType::AndAssignOperator:
		{
			encoder.EmitLogicalAnd(code);
			break;
		}

		case TokenType::NotOperator:
		{
			encoder.EmitLogicalNot(code);
			break;
		}

		default:
			throw std::runtime_error("unknown operator");
	}
}

void AbstractEmiter::SetEntryPoint()
{
	if (EntryPointCandidates.empty())
	{
		Diagnostics.ReportError(SyntaxToken(), L"Entry point for script not found");
		return;
	}

	if (EntryPointCandidates.size() > 1)
	{
		for (MethodSymbol* entry : EntryPointCandidates)
		{
			MethodDeclarationSyntax* decl = static_cast<MethodDeclarationSyntax*>(Table->GetSyntaxNode(entry));
			Diagnostics.ReportError(decl->IdentifierToken, L"Script cannot have multiple entry points");
		}

		return;
	}

	Program.EntryPoint = EntryPointCandidates.front();
	EntryPointCandidates.clear();
	return;
}

void AbstractEmiter::VisitSyntaxTree(SyntaxTree& tree)
{
	for (CompilationUnitSyntax* unit : tree.CompilationUnits)
		VisitCompilationUnit(unit);
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

	size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
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

	size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
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

	size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitExpressionStatement(ExpressionStatementSyntax* const node)
{
	VisitExpression(node->Expression);
	Encoder.EmitPop(GeneratingFor->ExecutableByteCode);
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
	bool isFirst = !IsConditionalClause(node->Parent->Kind);
	if (isFirst)
	{
		Clauses.emplace();
	}

	ClauseScope& scope = Clauses.top();
	VisitStatement(node->ConditionExpression);

	size_t jumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
	scope.ClauseEndBacktracks.push_back(jumpAddress);

	VisitStatementsBlock(node->StatementsBlock);

	if (node->NextStatement != nullptr)
	{
		jumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(jumpAddress);

		VisitConditionalClause(node->NextStatement);
	}

	if (isFirst)
	{
		scope.ClauseEnd = GeneratingFor->ExecutableByteCode.size();
		for (size_t backtrack : scope.ClauseEndBacktracks)
			ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.ClauseEnd, sizeof(size_t));

		Clauses.pop();
	}
}

void AbstractEmiter::VisitUnlessStatement(UnlessStatementSyntax* const node)
{
	bool isFirst = !IsConditionalClause(node->Parent->Kind);
	if (isFirst)
	{
		Clauses.emplace();
	}

	ClauseScope& scope = Clauses.top();
	VisitStatement(node->ConditionExpression);

	size_t jumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpTrue(GeneratingFor->ExecutableByteCode, 0);
	scope.ClauseEndBacktracks.push_back(jumpAddress);

	VisitStatementsBlock(node->StatementsBlock);

	if (node->NextStatement != nullptr)
	{
		jumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(jumpAddress);

		VisitConditionalClause(node->NextStatement);
	}

	if (isFirst)
	{
		scope.ClauseEnd = GeneratingFor->ExecutableByteCode.size();
		for (size_t backtrack : scope.ClauseEndBacktracks)
			ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.ClauseEnd, sizeof(size_t));

		Clauses.pop();
	}
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

void AbstractEmiter::VisitObjectCreationExpression(ObjectExpressionSyntax* const node)
{
	VisitArgumentsList(node->ArgumentsList);
	Encoder.EmitNewObject(GeneratingFor->ExecutableByteCode, node->TypeSymbol);
}

void AbstractEmiter::VisitCollectionExpression(CollectionExpressionSyntax* const node)
{
	for (auto riter = node->ValuesExpressions.rbegin(); riter != node->ValuesExpressions.rend(); riter++)
		VisitExpression(*riter);

	Encoder.EmitNewArray(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitLambdaExpression(LambdaExpressionSyntax* const node)
{
	MethodSymbol* previous = GeneratingFor;
	GeneratingFor = node->Symbol->AnonymousSymbol;
	
	size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body);

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = previous;
}

void AbstractEmiter::VisitTernaryExpression(TernaryExpressionSyntax* const node)
{
	VisitExpression(node->Condition);

	size_t jumpFalseAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	VisitExpression(node->Left);
	size_t jumpEndAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);

	size_t rightAddress = GeneratingFor->ExecutableByteCode.size();
	VisitExpression(node->Right);
	size_t ternaryEndAddress = GeneratingFor->ExecutableByteCode.size();

	ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, jumpFalseAddress + sizeof(OpCode), &rightAddress, sizeof(size_t));
	ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, jumpEndAddress + sizeof(OpCode), &ternaryEndAddress, sizeof(size_t));
}

void AbstractEmiter::VisitUnaryExpression(UnaryExpressionSyntax* const node)
{
	if (IsAssignExpression(node->OperatorToken.Type))
	{
		VisitUnaryAssignExpression(node);
		return;
	}

	VisitExpression(node->Expression);
	EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
}

void AbstractEmiter::VisitUnaryAssignExpression(UnaryExpressionSyntax* const node)
{
	MemberAccessExpressionSyntax* memberExpression = static_cast<MemberAccessExpressionSyntax*>(node->Expression);
	VisitExpression(memberExpression);
	EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);

	if (memberExpression->ToParameter)
	{
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		return;
	}

	if (memberExpression->ToVariable != nullptr)
	{
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		return;
	}

	if (memberExpression->ToProperty != nullptr)
	{
		Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, memberExpression->ToProperty->Setter);
		return;
	}

	if (memberExpression->ToField != nullptr)
	{
		if (memberExpression->ToField->IsStatic)
		{
			Encoder.EmitStoreStaticField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
			return;
		}

		Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
		return;
	}
}

void AbstractEmiter::VisitBinaryExpression(BinaryExpressionSyntax* const node)
{
	if (IsAssignExpression(node->OperatorToken.Type))
	{
		VisitBinaryAssignExpression(node);
		return;
	}

	VisitExpression(node->Left);
	VisitExpression(node->Right);
	EmitBinaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode);
}

void AbstractEmiter::VisitBinaryAssignExpression(BinaryExpressionSyntax* const node)
{
	MemberAccessExpressionSyntax* memberExpression = static_cast<MemberAccessExpressionSyntax*>(node->Left);
	VisitExpression(memberExpression);
	EmitBinaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode);

	if (memberExpression->ToParameter)
	{
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		return;
	}

	if (memberExpression->ToVariable != nullptr)
	{
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		return;
	}

	if (memberExpression->ToProperty != nullptr)
	{
		Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, memberExpression->ToProperty->Setter);
		return;
	}

	if (memberExpression->ToField != nullptr)
	{
		if (memberExpression->ToField->IsStatic)
		{
			Encoder.EmitStoreStaticField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
			return;
		}

		Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
		return;
	}
}

void AbstractEmiter::VisitInvocationExpression(InvokationExpressionSyntax* const node)
{
	VisitArgumentsList(node->ArgumentsList);
	if (!node->Symbol->IsStatic)
		VisitExpression(node->PreviousExpression);

	Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitIndexatorExpression(IndexatorExpressionSyntax* const node)
{
	VisitExpression(node->PreviousExpression);
	if (!node->ToProperty->IsStatic)
		VisitExpression(node->PreviousExpression);

	Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->ToProperty->Getter);
	return;
}

void AbstractEmiter::VisitMemberAccessExpression(MemberAccessExpressionSyntax* const node)
{
	VisitExpression(node->PreviousExpression);

	if (node->ToParameter != nullptr)
	{
		Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, node->ToParameter->SlotIndex);
		return;
	}

	if (node->ToVariable != nullptr)
	{
		Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, node->ToVariable->SlotIndex);
		return;
	}

	if (node->ToField != nullptr)
	{
		if (node->ToField->IsStatic)
		{
			Encoder.EmitLoadStaticField(GeneratingFor->ExecutableByteCode, node->ToField);
			return;
		}

		Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, node->ToField);
		return;
	}

	if (node->ToProperty != nullptr)
	{
		Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->ToProperty->Getter);
		return;
	}

	if (node->ToDelegate != nullptr)
	{
		Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->ToDelegate->AnonymousSymbol);
		return;
	}
}
