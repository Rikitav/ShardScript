#include <shard/compilation/AbstractEmiter.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/ByteCodeEncoder.hpp>

#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/TokenType.hpp>

#include <shard/syntax/symbols/LiteralSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>

#include <stdexcept>
#include <vector>
#include <cstddef>

using namespace shard;

const int ReserveMultiplier = 25;

static bool IsAssignExpression(shard::TokenType type)
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

static void EmitUnaryOperation(shard::TokenType type, ByteCodeEncoder& encoder, std::vector<std::byte>& code, bool isRightDetermined)
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

static void EmitBinaryOperation(shard::TokenType type, ByteCodeEncoder& encoder, std::vector<std::byte>& code)
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
			MethodDeclarationSyntax* decl = static_cast<MethodDeclarationSyntax*>(Table->LookupNode(entry).value_or(nullptr));
			Diagnostics.ReportError(decl->IdentifierToken, L"Script cannot have multiple entry points");
		}

		return;
	}

	Program.EntryPoint = EntryPointCandidates.front();
	EntryPointCandidates.clear();
	return;
}

void AbstractEmiter::SetGeneratingTarget(MethodSymbol* method)
{
	GeneratingFor = method;
}

void AbstractEmiter::VisitSyntaxTree(SyntaxTree& tree)
{
	for (const auto& unit : tree.CompilationUnits)
		VisitCompilationUnit(unit.get());
}

void AbstractEmiter::VisitArgumentsList(ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	// reverse itteration for method stack loading
	for (auto riter = node->Arguments.rbegin(); riter != node->Arguments.rend(); riter++)
		VisitArgument((*riter).get());
}

void AbstractEmiter::VisitMethodDeclaration(MethodDeclarationSyntax* const node)
{
	GeneratingFor = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	if (!GeneratingFor->IsExtern)
	{
		std::size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
		GeneratingFor->ExecutableByteCode.reserve(reserve);
		VisitStatementsBlock(node->Body.get());
	}

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

		SyntaxSymbol* parent = GeneratingFor->Parent;
		if (parent != nullptr && parent->IsType())
		{
			TypeSymbol* withinType = static_cast<TypeSymbol*>(parent);
			if (withinType != nullptr && withinType->TypeParameters.size() > 0)
				Diagnostics.ReportError(node->IdentifierToken, L"Type containing entry point should not have any type parameters");
		}
	}

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitConstructorDeclaration(ConstructorDeclarationSyntax* const node)
{
	GeneratingFor = LookupSymbol<ConstructorSymbol>(node).value_or(nullptr);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	// TODO: add field initialization

	if (!GeneratingFor->IsExtern)
	{
		std::size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
		GeneratingFor->ExecutableByteCode.reserve(reserve);
		VisitStatementsBlock(node->Body.get());
	}

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitAccessorDeclaration(AccessorDeclarationSyntax* const node)
{
	GeneratingFor = LookupSymbol<AccessorSymbol>(node).value_or(nullptr);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	if (!GeneratingFor->IsExtern)
	{
		if (node->Body != nullptr)
		{
			std::size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
			GeneratingFor->ExecutableByteCode.reserve(reserve);
			VisitStatementsBlock(node->Body.get());
		}
	}

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitExpressionStatement(ExpressionStatementSyntax* const node)
{
	VisitExpression(node->Expression.get());
	switch (node->Expression->Kind)
	{
		case SyntaxKind::InvokationExpression:
		{
			InvokationExpressionSyntax* invokation = static_cast<InvokationExpressionSyntax*>(node->Expression.get());
			if (invokation->Symbol->ReturnType != SymbolTable::Primitives::Void && PopExpressionStatement)
				Encoder.EmitPop(GeneratingFor->ExecutableByteCode);

			break;
		}
	}
}

void AbstractEmiter::VisitVariableStatement(VariableStatementSyntax* const node)
{
	VariableSymbol* var = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
	VisitExpression(node->Expression.get());
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, var->SlotIndex);
}

void AbstractEmiter::VisitReturnStatement(ReturnStatementSyntax* const node)
{
	VisitExpression(node->Expression.get());
	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
}

void AbstractEmiter::VisitThrowStatement(ThrowStatementSyntax* const node)
{
	VisitExpression(node->Expression.get());
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
	VisitExpression(node->ConditionExpression.get());

	// Emiting jump to loop end if condition is false
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	// Emiting loop body
	VisitStatementsBlock(node->StatementsBlock.get());

	// Getting loop block ending and miting looping jump
	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	// Getting loop ending
	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	// Backtracking uninitialized jumps
	for (std::size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(std::size_t));

	for (std::size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(std::size_t));

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
	VisitExpression(node->ConditionExpression.get());

	// Emiting jump to loop end if condition is false
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpTrue(GeneratingFor->ExecutableByteCode, 0);

	// Emiting loop body
	VisitStatementsBlock(node->StatementsBlock.get());

	// Getting loop block ending and miting looping jump
	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	// Getting loop ending
	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	// Backtracking uninitialized jumps
	for (std::size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(std::size_t));

	for (std::size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(std::size_t));

	// Exiting loop scope
	Loops.pop();
}

void AbstractEmiter::VisitForStatement(ForStatementSyntax* const node)
{
	// Entering loop scope
	Loops.emplace();
	LoopScope& scope = Loops.top();

	// Emiting initializer expression
	VisitStatement(node->InitializerStatement.get());

	// Getting loop starting position, current cursor pos
	scope.LoopStart = GeneratingFor->ExecutableByteCode.size();

	// Emiting looping condition expression
	VisitExpression(node->ConditionExpression.get());

	// Emiting jump to loop end if condition is false
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	// Emiting loop body
	VisitStatementsBlock(node->StatementsBlock.get());
	VisitStatement(node->AfterRepeatStatement.get());

	// Getting loop block ending and miting looping jump
	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	// Getting loop ending
	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	// Backtracking uninitialized jumps
	for (std::size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(std::size_t));

	for (std::size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(std::size_t));

	// Exiting loop scope
	Loops.pop();
}

void AbstractEmiter::VisitForEachStatement(ForEachStatementSyntax* const node)
{
	Loops.emplace();
	LoopScope& scope = Loops.top();

	std::uint16_t base = GeneratingFor->GetEvalStackArgumentsCount();
	std::uint16_t arraySlot = base + GeneratingFor->AddVariableCount();
	std::uint16_t indexSlot = base + GeneratingFor->AddVariableCount();
	std::uint16_t lengthSlot = base + GeneratingFor->AddVariableCount();

	VisitExpression(node->RangeExpression.get());
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, arraySlot);

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, arraySlot);
	Encoder.EmitDuplicate(GeneratingFor->ExecutableByteCode);
	Encoder.EmitArrayLength(GeneratingFor->ExecutableByteCode);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, lengthSlot);

	Encoder.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, 0);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, indexSlot);

	scope.LoopStart = GeneratingFor->ExecutableByteCode.size();

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, indexSlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, lengthSlot);
	Encoder.EmitCompareLess(GeneratingFor->ExecutableByteCode);

	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, arraySlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, indexSlot);
	Encoder.EmitLoadArrayElement(GeneratingFor->ExecutableByteCode);

	VariableSymbol* loopVariable = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
	if (loopVariable != nullptr)
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, loopVariable->SlotIndex);
	else
		Encoder.EmitPop(GeneratingFor->ExecutableByteCode);

	VisitStatementsBlock(node->StatementsBlock.get());

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, indexSlot);
	Encoder.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, 1);
	Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, indexSlot);

	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	for (std::size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(std::size_t));

	for (std::size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(std::size_t));

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
	VisitStatement(node->ConditionExpression.get());

	std::size_t jumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
	scope.ClauseEndBacktracks.push_back(jumpAddress);

	VisitStatementsBlock(node->StatementsBlock.get());

	if (node->NextStatement != nullptr)
	{
		jumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(jumpAddress);

		VisitConditionalClause(node->NextStatement.get());
	}

	if (isFirst)
	{
		scope.ClauseEnd = GeneratingFor->ExecutableByteCode.size();
		for (std::size_t backtrack : scope.ClauseEndBacktracks)
			ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.ClauseEnd, sizeof(std::size_t));

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
	VisitStatement(node->ConditionExpression.get());

	std::size_t jumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpTrue(GeneratingFor->ExecutableByteCode, 0);
	scope.ClauseEndBacktracks.push_back(jumpAddress);

	VisitStatementsBlock(node->StatementsBlock.get());

	if (node->NextStatement != nullptr)
	{
		jumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(jumpAddress);

		VisitConditionalClause(node->NextStatement.get());
	}

	if (isFirst)
	{
		scope.ClauseEnd = GeneratingFor->ExecutableByteCode.size();
		for (std::size_t backtrack : scope.ClauseEndBacktracks)
			ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.ClauseEnd, sizeof(std::size_t));

		Clauses.pop();
	}
}

void AbstractEmiter::VisitElseStatement(ElseStatementSyntax* const node)
{
	VisitStatementsBlock(node->StatementsBlock.get());
}

void AbstractEmiter::VisitLiteralExpression(LiteralExpressionSyntax* const node)
{
	LiteralSymbol* const symbol = static_cast<LiteralSymbol*>(Table->LookupSymbol(node).value_or(nullptr));
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
	if (node->TypeSymbol != nullptr && node->TypeSymbol->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(node->TypeSymbol);
		TypeSymbol* underlyingType = genericType->UnderlayingType;

		for (std::size_t i = 0; i < underlyingType->TypeParameters.size(); i++)
		{
			TypeSymbol* concreteType = genericType->SubstituteTypeParameters(underlyingType->TypeParameters[i]);
			if (concreteType != nullptr)
				Encoder.EmitLoadTypeArgument(GeneratingFor->ExecutableByteCode, static_cast<std::uint16_t>(i), concreteType);
		}
	}

	VisitArgumentsList(node->ArgumentsList.get());
	Encoder.EmitNewObject(GeneratingFor->ExecutableByteCode, node->TypeSymbol, node->CtorSymbol);
}

void AbstractEmiter::VisitCollectionExpression(CollectionExpressionSyntax* const node)
{
	for (auto riter = node->ValuesExpressions.rbegin(); riter != node->ValuesExpressions.rend(); riter++)
		VisitExpression((*riter).get());

	Encoder.EmitNewArray(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitRangeExpression(RangeExpressionSyntax* const node)
{
	std::uint16_t base = GeneratingFor->GetEvalStackArgumentsCount();
	std::uint16_t upperSlot = base + GeneratingFor->AddVariableCount();
	std::uint16_t lowerSlot = base + GeneratingFor->AddVariableCount();
	std::uint16_t lengthSlot = base + GeneratingFor->AddVariableCount();
	std::uint16_t arraySlot = base + GeneratingFor->AddVariableCount();
	std::uint16_t indexSlot = base + GeneratingFor->AddVariableCount();

	VisitExpression(node->Right.get());
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, upperSlot);

	VisitExpression(node->Left.get());
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, lowerSlot);

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, upperSlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, lowerSlot);
	Encoder.EmitMathSub(GeneratingFor->ExecutableByteCode);

	if (node->IsInclusive)
	{
		Encoder.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, 1);
		Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
	}

	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, lengthSlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, lengthSlot);
	Encoder.EmitNewDynamicArray(GeneratingFor->ExecutableByteCode, SymbolTable::Primitives::Integer);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, arraySlot);

	Encoder.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, 0);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, indexSlot);

	std::size_t fillStart = GeneratingFor->ExecutableByteCode.size();

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, arraySlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, indexSlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, lowerSlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, indexSlot);
	Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
	Encoder.EmitStoreArrayElement(GeneratingFor->ExecutableByteCode);

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, indexSlot);
	Encoder.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, 1);
	Encoder.EmitMathAdd(GeneratingFor->ExecutableByteCode);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, indexSlot);

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, indexSlot);
	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, lengthSlot);
	Encoder.EmitCompareLess(GeneratingFor->ExecutableByteCode);

	std::size_t jumpBacktrack = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpTrue(GeneratingFor->ExecutableByteCode, 0);
	ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, jumpBacktrack + sizeof(OpCode), &fillStart, sizeof(std::size_t));

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, arraySlot);
}

void AbstractEmiter::VisitLambdaExpression(LambdaExpressionSyntax* const node)
{
	MethodSymbol* previous = GeneratingFor;
	GeneratingFor = node->Symbol->AnonymousSymbol;
	
	std::size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body.get());

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = previous;
}

void AbstractEmiter::VisitTernaryExpression(TernaryExpressionSyntax* const node)
{
	VisitExpression(node->Condition.get());

	std::size_t jumpFalseAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	VisitExpression(node->Left.get());
	std::size_t jumpEndAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);

	std::size_t rightAddress = GeneratingFor->ExecutableByteCode.size();
	VisitExpression(node->Right.get());
	std::size_t ternaryEndAddress = GeneratingFor->ExecutableByteCode.size();

	ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, jumpFalseAddress + sizeof(OpCode), &rightAddress, sizeof(std::size_t));
	ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, jumpEndAddress + sizeof(OpCode), &ternaryEndAddress, sizeof(std::size_t));
}

void AbstractEmiter::VisitUnaryExpression(UnaryExpressionSyntax* const node)
{
	if (IsAssignExpression(node->OperatorToken.Type))
	{
		VisitUnaryAssignExpression(node);
		return;
	}

	VisitExpression(node->Expression.get());
	EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
}

void AbstractEmiter::VisitUnaryAssignExpression(UnaryExpressionSyntax* const node)
{
	MemberAccessExpressionSyntax* memberExpression = static_cast<MemberAccessExpressionSyntax*>(node->Expression.get());

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
		VisitExpression(memberExpression);
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

		VisitExpression(memberExpression);
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

	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());
	EmitBinaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode);
}

void AbstractEmiter::VisitBinaryAssignExpression(BinaryExpressionSyntax* const node)
{
	MemberAccessExpressionSyntax* memberExpression = static_cast<MemberAccessExpressionSyntax*>(node->Left.get());
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
		VisitExpression(memberExpression->PreviousExpression.get());
		VisitExpression(node->Right.get());
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

		VisitExpression(memberExpression->PreviousExpression.get());
		VisitExpression(node->Right.get());
		Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
		return;
	}
}

void AbstractEmiter::VisitInvocationExpression(InvokationExpressionSyntax* const node)
{
	if (node->ReceiverType != nullptr && node->ReceiverType->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(node->ReceiverType);
		TypeSymbol* underlyingType = genericType->UnderlayingType;
		for (std::size_t i = 0; i < underlyingType->TypeParameters.size(); i++)
		{
			TypeSymbol* concreteType = genericType->SubstituteTypeParameters(underlyingType->TypeParameters[i]);
			if (concreteType != nullptr)
				Encoder.EmitLoadTypeArgument(GeneratingFor->ExecutableByteCode, static_cast<std::uint16_t>(i), concreteType);
		}
	}

	VisitArgumentsList(node->ArgumentsList.get());
	if (node->PreviousExpression != nullptr)
		VisitExpression(node->PreviousExpression.get());

	Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitIndexatorExpression(IndexatorExpressionSyntax* const node)
{
	VisitExpression(node->PreviousExpression.get());
	if (!node->ToProperty->IsStatic)
		VisitExpression(node->PreviousExpression.get());

	Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->ToProperty->Getter);
	return;
}

void AbstractEmiter::VisitMemberAccessExpression(MemberAccessExpressionSyntax* const node)
{
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

		VisitExpression(node->PreviousExpression.get());
		Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, node->ToField);
		return;
	}

	if (node->ToProperty != nullptr)
	{
		VisitExpression(node->PreviousExpression.get());
		Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->ToProperty->Getter);
		return;
	}

	if (node->ToDelegate != nullptr)
	{
		Encoder.EmitNewDelegate(GeneratingFor->ExecutableByteCode, node->ToDelegate);
		return;
	}
}
