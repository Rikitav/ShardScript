#include <shard/compilation/AbstractEmiter.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/ByteCodeEncoder.hpp>

#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>

#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>

#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/lexical/TokenType.hpp>

#include <shard/semantic/symbols/LiteralSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <optional>
#include <stdexcept>
#include <vector>
#include <cstddef>

using namespace shard;

static bool IsInterfaceMember(MethodSymbol* method);

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

		case TokenType::NotOperator:
		{
			encoder.EmitLogicalNot(code);
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

		case TokenType::LeftShiftOperator:
		{
			encoder.EmitMathLeftShift(code);
			break;
		}

		case TokenType::RightShiftOperator:
		{
			encoder.EmitMathRightShift(code);
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

static void EmitIndexatorArguments(AbstractEmiter* emitter, IndexatorListSyntax* node)
{
	if (node == nullptr || emitter == nullptr)
		return;

	for (auto riter = node->Arguments.rbegin(); riter != node->Arguments.rend(); riter++)
		emitter->VisitArgument((*riter).get());
}

void AbstractEmiter::VisitArgumentsList(ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	// reverse itteration for method stack loading
	for (auto riter = node->Arguments.rbegin(); riter != node->Arguments.rend(); riter++)
		VisitArgument((*riter).get());
}

void AbstractEmiter::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	GeneratingFor = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	if (GeneratingFor->IsAsync && GeneratingFor->AsyncStateMachineClass != nullptr)
	{
		// The body of an async method is replaced by the AsyncStateMachineLowering pass.
		GeneratingFor->ExecutableByteCode.shrink_to_fit();
		GeneratingFor = nullptr;
		return;
	}

	if (node->Body != nullptr)
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

		if (GeneratingFor->Linking == LINK_INSTANCE)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should be static");

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

void AbstractEmiter::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
	GeneratingFor = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Emiting target not found");
		return;
	}

	if (node->Body != nullptr)
	{
		std::size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
		GeneratingFor->ExecutableByteCode.reserve(reserve);
		VisitStatementsBlock(node->Body.get());
	}

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	GeneratingFor = LookupSymbol<ConstructorSymbol>(node).value_or(nullptr);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	// TODO: add field initialization

	if (node->Body != nullptr)
	{
		std::size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
		GeneratingFor->ExecutableByteCode.reserve(reserve);
		VisitStatementsBlock(node->Body.get());
	}

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	GeneratingFor = LookupSymbol<AccessorSymbol>(node).value_or(nullptr);
	if (GeneratingFor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Emiting target not found");
		return;
	}

	if (node->Body != nullptr)
	{
		std::size_t reserve = node->Body->Statements.size() * ReserveMultiplier;
		GeneratingFor->ExecutableByteCode.reserve(reserve);
		VisitStatementsBlock(node->Body.get());
	}
	else if (GeneratingFor->Parent != nullptr && GeneratingFor->Parent->Kind == SyntaxKind::PropertyDeclaration)
	{
		PropertySymbol* property = static_cast<PropertySymbol*>(GeneratingFor->Parent);
		if (property->BackingField != nullptr)
		{
			if (node->KeywordToken.Type == TokenType::GetKeyword)
			{
				Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, 0);
				Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, property->BackingField->SlotIndex);
			}
			else if (node->KeywordToken.Type == TokenType::SetKeyword)
			{
				Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, 0);
				Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, 1);
				Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, property->BackingField->SlotIndex);
			}
		}
		else
		{
			throw std::runtime_error("property backing field not found");
		}
	}

	GeneratingFor->ExecutableByteCode.shrink_to_fit();
	GeneratingFor = nullptr;
}

void AbstractEmiter::VisitEnumDeclaration(EnumDeclarationSyntax* node)
{
	// Enum fields are constants; no runtime initialization is emitted here.
	// Enum member access is handled directly by VisitMemberAccessExpression.
}

void AbstractEmiter::VisitExpressionStatement(ExpressionStatementSyntax* node)
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

void AbstractEmiter::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* var = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
	VisitExpression(node->Expression.get());
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, var->SlotIndex);
}

void AbstractEmiter::VisitTryStatement(TryStatementSyntax* node)
{
	if (node->CatchClauses.empty())
	{
		if (node->TryBlock != nullptr)
			VisitStatementsBlock(node->TryBlock.get());
		return;
	}

	std::size_t enterTryBacktrack = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitEnterTry(GeneratingFor->ExecutableByteCode, 0);

	if (node->TryBlock != nullptr)
		VisitStatementsBlock(node->TryBlock.get());

	Encoder.EmitLeaveTry(GeneratingFor->ExecutableByteCode);
	std::size_t tryEndJumpBacktrack = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);

	std::size_t handlerStart = GeneratingFor->ExecutableByteCode.size();
	ByteCodeEncoder::PasteData(
		GeneratingFor->ExecutableByteCode,
		enterTryBacktrack + sizeof(OpCode),
		&handlerStart,
		sizeof(std::size_t));

	std::vector<std::size_t> clauseStarts;
	std::vector<std::size_t> bodyEndBacktracks;
	std::vector<std::optional<std::size_t>> filterFailBacktracks;

	for (const auto& clauseUnique : node->CatchClauses)
	{
		CatchClauseSyntax* clause = clauseUnique.get();
		clauseStarts.push_back(GeneratingFor->ExecutableByteCode.size());
		filterFailBacktracks.emplace_back(std::nullopt);

		TypeSymbol* catchType = (clause->ExceptionType != nullptr && clause->ExceptionType->Symbol != nullptr)
			? clause->ExceptionType->Symbol
			: SymbolTable::Primitives::Any;

		if (catchType != SymbolTable::Primitives::Any)
		{
			Encoder.EmitDuplicate(GeneratingFor->ExecutableByteCode);
			Encoder.EmitIsInstance(GeneratingFor->ExecutableByteCode, catchType);
			filterFailBacktracks.back() = GeneratingFor->ExecutableByteCode.size();
			Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
		}

		VariableSymbol* catchVariable = clause->Symbol;
		if (catchVariable != nullptr)
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, catchVariable->SlotIndex);
		else
			Encoder.EmitPop(GeneratingFor->ExecutableByteCode);

		if (clause->Body != nullptr)
			VisitStatementsBlock(clause->Body.get());

		bodyEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
	}

	std::size_t fallbackStart = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitThrow(GeneratingFor->ExecutableByteCode);

	std::size_t endLabel = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitEndCatch(GeneratingFor->ExecutableByteCode);

	for (std::size_t i = 0; i < node->CatchClauses.size(); ++i)
	{
		if (filterFailBacktracks[i].has_value())
		{
			std::size_t target = (i + 1 < clauseStarts.size())
				? clauseStarts[i + 1]
				: fallbackStart;
			ByteCodeEncoder::PasteData(
				GeneratingFor->ExecutableByteCode,
				filterFailBacktracks[i].value() + sizeof(OpCode),
				&target,
				sizeof(std::size_t));
		}
	}

	for (std::size_t backtrack : bodyEndBacktracks)
	{
		ByteCodeEncoder::PasteData(
			GeneratingFor->ExecutableByteCode,
			backtrack + sizeof(OpCode),
			&endLabel,
			sizeof(std::size_t));
	}

	ByteCodeEncoder::PasteData(
		GeneratingFor->ExecutableByteCode,
		tryEndJumpBacktrack + sizeof(OpCode),
		&endLabel,
		sizeof(std::size_t));
}

void AbstractEmiter::VisitContinueStatement(ContinueStatementSyntax* node)
{
	if (Loops.empty())
	{
		Diagnostics.ReportError(node->KeywordToken, L"'continue' must be inside a loop");
		return;
	}

	LoopScope& scope = Loops.top();
	scope.BlockEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
}

void AbstractEmiter::VisitWhileStatement(WhileStatementSyntax* node)
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

void AbstractEmiter::VisitUntilStatement(UntilStatementSyntax* node)
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

void AbstractEmiter::VisitForStatement(ForStatementSyntax* node)
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

void AbstractEmiter::VisitForEachStatement(ForEachStatementSyntax* node)
{
	Loops.emplace();
	LoopScope& scope = Loops.top();

	VariableSymbol* loopVariable = LookupSymbol<VariableSymbol>(node).value_or(nullptr);

	std::uint16_t base = GeneratingFor->GetEvalStackArgumentsCount();
	std::uint16_t enumeratorSlot = base + GeneratingFor->AddVariableCount();

	VisitExpression(node->RangeExpression.get());
	EmitMethodCall(TRAIT_ENUMERABLE_GETENUMERATOR);
	Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, enumeratorSlot);

	scope.LoopStart = GeneratingFor->ExecutableByteCode.size();

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, enumeratorSlot);
	EmitMethodCall(TRAIT_ENUMERATOR_MOVENEXT);

	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);

	Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, enumeratorSlot);
	EmitMethodCall(TRAIT_ENUMERATOR_CURRENT_GET);

	if (loopVariable != nullptr)
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, loopVariable->SlotIndex);
	else
		Encoder.EmitPop(GeneratingFor->ExecutableByteCode);

	VisitStatementsBlock(node->StatementsBlock.get());

	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	for (std::size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(std::size_t));

	for (std::size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(std::size_t));

	Loops.pop();
}

void AbstractEmiter::VisitForInStatement(ForInStatementSyntax* node)
{
	Loops.emplace();
	LoopScope& scope = Loops.top();

	VariableSymbol* loopVariable = LookupSymbol<VariableSymbol>(node).value_or(nullptr);

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

void AbstractEmiter::VisitIfStatement(IfStatementSyntax* node)
{
	bool isFirst = !IsConditionalClause(node->Parent->Kind);
	if (isFirst)
	{
		Clauses.emplace();
	}

	ClauseScope& scope = Clauses.top();

	bool tmpPopExpressionStatement = PopExpressionStatement;
	SetPopExpressionStatement(false);
	VisitStatement(node->ConditionExpression.get());
	SetPopExpressionStatement(tmpPopExpressionStatement);

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

void AbstractEmiter::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	bool isFirst = !IsConditionalClause(node->Parent->Kind);
	if (isFirst)
	{
		Clauses.emplace();
	}

	ClauseScope& scope = Clauses.top();
	
	bool tmpPopExpressionStatement = PopExpressionStatement;
	SetPopExpressionStatement(false);
	VisitStatement(node->ConditionExpression.get());
	SetPopExpressionStatement(tmpPopExpressionStatement);

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

void AbstractEmiter::VisitElseStatement(ElseStatementSyntax* node)
{
	VisitStatementsBlock(node->StatementsBlock.get());
}

void AbstractEmiter::VisitLiteralExpression(LiteralExpressionSyntax* node)
{
	LiteralSymbol* symbol = static_cast<LiteralSymbol*>(Table->LookupSymbol(node).value_or(nullptr));
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
			if (symbol->BoundType != nullptr && symbol->BoundType != SymbolTable::Primitives::Integer)
				Encoder.EmitCastPrimitive(GeneratingFor->ExecutableByteCode, symbol->BoundType);

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

void AbstractEmiter::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	if (node->Symbol != nullptr && node->Symbol->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(node->Symbol);
		TypeSymbol* underlyingType = genericType->UnderlayingType;

		for (std::size_t i = 0; i < underlyingType->TypeParameters.size(); i++)
		{
			TypeSymbol* concreteType = genericType->SubstituteTypeParameters(underlyingType->TypeParameters[i]);
			if (concreteType != nullptr)
				Encoder.EmitLoadTypeArgument(GeneratingFor->ExecutableByteCode, static_cast<std::uint16_t>(i), concreteType);
		}
	}

	VisitArgumentsList(node->ArgumentsList.get());
	Encoder.EmitNewObject(GeneratingFor->ExecutableByteCode, node->Symbol, node->CtorSymbol);
}

void AbstractEmiter::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (auto riter = node->ValuesExpressions.rbegin(); riter != node->ValuesExpressions.rend(); riter++)
		VisitExpression((*riter).get());

	Encoder.EmitNewArray(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitRangeExpression(RangeExpressionSyntax* node)
{
	// Evaluate bounds in natural left-to-right order and let the VM build the array.
	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());
	Encoder.EmitLoadConstBool(GeneratingFor->ExecutableByteCode, node->IsInclusive);
	Encoder.EmitCreateRange(GeneratingFor->ExecutableByteCode, SymbolTable::Primitives::Integer);
}

void AbstractEmiter::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	MethodSymbol* previous = GeneratingFor;
	GeneratingFor = node->Symbol->AnonymousSymbol;

	std::vector<DeferScope> previousDefers = std::move(DeferScopes);
	DeferScopes = std::vector<DeferScope>();

	std::size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body.get());

	GeneratingFor->ExecutableByteCode.shrink_to_fit();

	DeferScopes = std::move(previousDefers);
	GeneratingFor = previous;

	Encoder.EmitNewDelegate(GeneratingFor->ExecutableByteCode, node->Symbol);
}

void AbstractEmiter::VisitTypeExpression(TypeExpressionSyntax* node)
{
	// Type expressions are used only as static receivers; member access / invocation
	// use the resolved ReceiverType directly, so no code needs to be emitted here.
}

void AbstractEmiter::VisitTernaryExpression(TernaryExpressionSyntax* node)
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

void AbstractEmiter::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	if (IsAssignExpression(node->OperatorToken.Type))
	{
		VisitUnaryAssignExpression(node);
		return;
	}

	if (node->ToOperator != nullptr)
	{
		VisitExpression(node->Expression.get());
		EmitMethodCall(node->ToOperator);
		return;
	}

	VisitExpression(node->Expression.get());
	EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
}

void AbstractEmiter::VisitUnaryAssignExpression(UnaryExpressionSyntax* node)
{
	MemberAccessExpressionSyntax* memberExpression = static_cast<MemberAccessExpressionSyntax*>(node->Expression.get());

	if (memberExpression->ToParameter != nullptr)
	{
		Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		return;
	}

	if (memberExpression->ToVariable != nullptr)
	{
		Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		return;
	}

	if (memberExpression->ToField != nullptr)
	{
		if (memberExpression->ToField->Linking == LINK_STATIC)
		{
			Encoder.EmitLoadStaticField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
			EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
			Encoder.EmitStoreStaticField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
			return;
		}

		VisitExpression(memberExpression->PreviousExpression.get());
		Encoder.EmitDuplicate(GeneratingFor->ExecutableByteCode);
		Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, memberExpression->ToField->SlotIndex);
		EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
		Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, memberExpression->ToField->SlotIndex);
		return;
	}

	if (memberExpression->ToProperty != nullptr)
	{
		PropertySymbol* property = memberExpression->ToProperty;
		AccessorSymbol* getter = property->Getter;
		AccessorSymbol* setter = property->Setter;

		bool isStatic = (getter != nullptr && getter->Linking == LINK_STATIC) ||
		                (setter != nullptr && setter->Linking == LINK_STATIC);

		if (isStatic)
		{
			EmitMethodCall(getter);
			EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
			EmitMethodCall(setter);
		}
		else
		{
			std::uint16_t base = GeneratingFor->GetEvalStackArgumentsCount();
			std::uint16_t tempThisSlot = base + GeneratingFor->AddVariableCount();
			std::uint16_t tempValueSlot = base + GeneratingFor->AddVariableCount();

			IndexatorExpressionSyntax* indexatorExpr = (memberExpression->Kind == SyntaxKind::IndexatorExpression)
				? static_cast<IndexatorExpressionSyntax*>(memberExpression)
				: nullptr;

			// Load receiver
			VisitExpression(memberExpression->PreviousExpression.get());
			Encoder.EmitDuplicate(GeneratingFor->ExecutableByteCode);

			// Load index arguments for indexers
			if (indexatorExpr != nullptr && indexatorExpr->IndexatorList != nullptr)
				EmitIndexatorArguments(this, indexatorExpr->IndexatorList.get());

			// Call getter: leaves [this, value] on the eval stack
			EmitMethodCall(getter);

			// Apply ++/--. For prefix this leaves [this, new, new];
			// for postfix this leaves [this, old, new].
			EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);

			// Preserve the new value and the receiver in temporary slots so we can
			// reorder the stack for the setter call ([args..., value, this]).
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, tempValueSlot);
			Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, tempThisSlot);

			// Push setter arguments
			if (indexatorExpr != nullptr && indexatorExpr->IndexatorList != nullptr)
				EmitIndexatorArguments(this, indexatorExpr->IndexatorList.get());

			Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, tempValueSlot);
			Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, tempThisSlot);

			EmitMethodCall(setter);
		}

		return;
	}
}

void AbstractEmiter::VisitBinaryExpression(BinaryExpressionSyntax* node)
{
	if (IsAssignExpression(node->OperatorToken.Type))
	{
		VisitBinaryAssignExpression(node);
		return;
	}

	if (node->ToOperator != nullptr)
	{
		VisitExpression(node->Right.get());
		VisitExpression(node->Left.get());
		EmitMethodCall(node->ToOperator);
		return;
	}

	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());
	EmitBinaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode);
}

void AbstractEmiter::VisitBinaryAssignExpression(BinaryExpressionSyntax* node)
{
	MemberAccessExpressionSyntax* memberExpression = static_cast<MemberAccessExpressionSyntax*>(node->Left.get());
	if (memberExpression->ToParameter)
	{
		VisitExpression(node->Right.get());
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		return;
	}

	if (memberExpression->ToVariable != nullptr)
	{
		VisitExpression(node->Right.get());
		Encoder.EmitStoreVarible(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		return;
	}

	if (memberExpression->ToProperty != nullptr)
	{
		PropertySymbol* property = memberExpression->ToProperty;
		AccessorSymbol* setter = property->Setter;

		IndexatorExpressionSyntax* indexatorExpr = (memberExpression->Kind == SyntaxKind::IndexatorExpression)
			? static_cast<IndexatorExpressionSyntax*>(memberExpression)
			: nullptr;

		// Push the value being assigned first; for instance setters the receiver
		// must end up on top of the evaluation stack.
		VisitExpression(node->Right.get());

		// Push index arguments for indexers (reverse order so the first argument
		// is at the lowest stack position).
		if (indexatorExpr != nullptr && indexatorExpr->IndexatorList != nullptr)
			EmitIndexatorArguments(this, indexatorExpr->IndexatorList.get());

		// Push the receiver for instance properties/indexers.
		if (setter == nullptr || setter->Linking == LINK_INSTANCE)
			VisitExpression(memberExpression->PreviousExpression.get());

		EmitMethodCall(setter);
		return;
	}

	if (memberExpression->ToField != nullptr)
	{
		if (memberExpression->ToField->Linking == LINK_STATIC)
		{
			Encoder.EmitStoreStaticField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
			return;
		}

		VisitExpression(memberExpression->PreviousExpression.get());
		VisitExpression(node->Right.get());
		Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, memberExpression->ToField->SlotIndex);
		return;
	}
}

void AbstractEmiter::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	if (node->Symbol != nullptr && IsInterfaceMember(node->Symbol))
	{
		VisitArgumentsList(node->ArgumentsList.get());
		if (node->PreviousExpression != nullptr)
			VisitExpression(node->PreviousExpression.get());

		EmitMethodCall(node->Symbol);
		return;
	}

	bool hasTypeArguments = false;
	std::size_t ownerParamCount = 0;

	if (node->ReceiverType != nullptr && node->ReceiverType->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(node->ReceiverType);
		TypeSymbol* underlyingType = genericType->UnderlayingType;
		ownerParamCount = underlyingType->TypeParameters.size();
		for (std::size_t i = 0; i < ownerParamCount; i++)
		{
			TypeSymbol* concreteType = genericType->SubstituteTypeParameters(underlyingType->TypeParameters[i]);
			if (concreteType != nullptr)
			{
				Encoder.EmitLoadTypeArgument(GeneratingFor->ExecutableByteCode, static_cast<std::uint16_t>(i), concreteType);
				hasTypeArguments = true;
			}
		}
	}
	else if (node->Symbol != nullptr && node->Symbol->Parent != nullptr && node->Symbol->Parent->IsType())
	{
		ownerParamCount = static_cast<TypeSymbol*>(node->Symbol->Parent)->TypeParameters.size();
	}

	if (!node->BoundTypeArguments.empty())
	{
		for (std::size_t i = 0; i < node->BoundTypeArguments.size(); ++i)
		{
			TypeSymbol* concreteType = node->BoundTypeArguments[i];
			if (concreteType != nullptr)
			{
				Encoder.EmitLoadTypeArgument(GeneratingFor->ExecutableByteCode, static_cast<std::uint16_t>(ownerParamCount + i), concreteType);
				hasTypeArguments = true;
			}
		}
	}

	VisitArgumentsList(node->ArgumentsList.get());
	if (node->PreviousExpression != nullptr)
		VisitExpression(node->PreviousExpression.get());

	if (node->IsDelegateInvocation)
		Encoder.EmitCallDelegate(GeneratingFor->ExecutableByteCode);
	else if (hasTypeArguments)
		Encoder.EmitCallGenericMethod(GeneratingFor->ExecutableByteCode, node->Symbol);
	else
		EmitMethodCall(node->Symbol);
}

void AbstractEmiter::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	if (node->ToProperty == nullptr)
		return;

	if (node->IndexatorList != nullptr)
		EmitIndexatorArguments(this, node->IndexatorList.get());

	VisitExpression(node->PreviousExpression.get());

	EmitMethodCall(node->ToProperty->Getter);
	return;
}

void AbstractEmiter::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
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
		if (node->ToField->IsEnumValue)
		{
			Encoder.EmitLoadEnumField(GeneratingFor->ExecutableByteCode, node->ToField);
			return;
		}

		if (node->ToField->Linking == LINK_STATIC)
		{
			Encoder.EmitLoadStaticField(GeneratingFor->ExecutableByteCode, node->ToField);
			return;
		}

		VisitExpression(node->PreviousExpression.get());
		Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, node->ToField->SlotIndex);
		return;
	}

	if (node->ToProperty != nullptr)
	{
		if (node->ToProperty->Getter == nullptr)
			return;

		VisitExpression(node->PreviousExpression.get());
		EmitMethodCall(node->ToProperty->Getter);
		return;
	}

	if (node->ToDelegate != nullptr)
	{
		Encoder.EmitNewDelegate(GeneratingFor->ExecutableByteCode, node->ToDelegate);
		return;
	}

	if (node->ToOperator != nullptr)
	{
		Encoder.EmitLoadConstString(GeneratingFor->ExecutableByteCode, Program.DataSection, node->IdentifierToken.Word.data());
		VisitExpression(node->PreviousExpression.get());
		EmitMethodCall(node->ToOperator);
		return;
	}
}

void AbstractEmiter::EmitDefer(DeferStatementSyntax* defer)
{
	if (defer == nullptr)
		return;

	if (defer->IsResourceDefer)
	{
		if (defer->Variable != nullptr)
			Encoder.EmitLoadVarible(GeneratingFor->ExecutableByteCode, defer->Variable->SlotIndex);

		if (defer->DisposeMethod != nullptr)
			EmitMethodCall(defer->DisposeMethod);
	}
	else if (defer->Statement != nullptr)
	{
		VisitStatement(defer->Statement.get());
	}
}

void AbstractEmiter::EmitCurrentScopeDefers()
{
	if (DeferScopes.empty())
		return;

	DeferScope& scope = DeferScopes.back();
	for (auto it = scope.Defers.rbegin(); it != scope.Defers.rend(); ++it)
		EmitDefer(*it);

	scope.Defers.clear();
}

void AbstractEmiter::EmitDefersUntilLoop()
{
	for (auto it = DeferScopes.rbegin(); it != DeferScopes.rend(); ++it)
	{
		for (auto deferIt = it->Defers.rbegin(); deferIt != it->Defers.rend(); ++deferIt)
			EmitDefer(*deferIt);

		it->Defers.clear();

		if (it->IsLoop)
			break;
	}
}

void AbstractEmiter::EmitAllDefers()
{
	for (auto it = DeferScopes.rbegin(); it != DeferScopes.rend(); ++it)
	{
		for (auto deferIt = it->Defers.rbegin(); deferIt != it->Defers.rend(); ++deferIt)
			EmitDefer(*deferIt);

		it->Defers.clear();
	}
}

void AbstractEmiter::VisitStatementsBlock(StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	bool isLoop = node->Parent != nullptr && (
		node->Parent->Kind == SyntaxKind::WhileStatement ||
		node->Parent->Kind == SyntaxKind::UntilStatement ||
		node->Parent->Kind == SyntaxKind::ForStatement ||
		node->Parent->Kind == SyntaxKind::ForEachStatement);

	DeferScopes.push_back({ {}, isLoop });

	for (const auto& statement : node->Statements)
		VisitStatement(statement.get());

	EmitCurrentScopeDefers();
	DeferScopes.pop_back();
}

void AbstractEmiter::VisitDeferStatement(DeferStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (DeferScopes.empty())
	{
		Diagnostics.ReportError(node->DeferToken, L"defer statement must be inside a block");
		return;
	}

	if (node->IsResourceDefer && node->Statement != nullptr)
		VisitStatement(node->Statement.get());

	DeferScopes.back().Defers.push_back(node);
}

void AbstractEmiter::VisitReturnStatement(ReturnStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	EmitAllDefers();
	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
}

void AbstractEmiter::VisitBreakStatement(BreakStatementSyntax* node)
{
	if (Loops.empty())
	{
		Diagnostics.ReportError(node->KeywordToken, L"'break' must be inside a loop");
		return;
	}

	EmitDefersUntilLoop();

	LoopScope& scope = Loops.top();
	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
}

void AbstractEmiter::VisitThrowStatement(ThrowStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	EmitAllDefers();

	if (node->Expression != nullptr)
		Encoder.EmitThrow(GeneratingFor->ExecutableByteCode);
	else
		Encoder.EmitRethrow(GeneratingFor->ExecutableByteCode);
}

static bool IsInterfaceMember(MethodSymbol* method)
{
    SyntaxSymbol* owner = method->Parent;
    while (owner != nullptr)
    {
        if (owner->Kind == SyntaxKind::InterfaceDeclaration)
            return true;

        if (owner->IsType())
            return false;

        owner = owner->Parent;
    }

    return false;
}

void AbstractEmiter::EmitMethodCall(MethodSymbol* method)
{
    if (method == nullptr)
        return;

    if (IsInterfaceMember(method))
        Encoder.EmitCallInterface(GeneratingFor->ExecutableByteCode, method);
    else
        Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, method);
}

void AbstractEmiter::VisitCastExpression(CastExpressionSyntax* node)
{
    if (node->Expression != nullptr)
        VisitExpression(node->Expression.get());

    if (node->TargetType == nullptr || node->TargetType->Symbol == nullptr)
        return;

    if (node->ToOperator != nullptr)
    {
        EmitMethodCall(node->ToOperator);
        return;
    }

    if (node->IsPrimitiveCast)
    {
        Encoder.EmitCastPrimitive(GeneratingFor->ExecutableByteCode, node->TargetType->Symbol);
        return;
    }

    Encoder.EmitCast(GeneratingFor->ExecutableByteCode, node->TargetType->Symbol);
}

void AbstractEmiter::VisitIsExpression(IsExpressionSyntax* node)
{
    if (node->Expression != nullptr)
        VisitExpression(node->Expression.get());

    if (node->TargetType != nullptr && node->TargetType->Symbol != nullptr)
        Encoder.EmitIsInstance(GeneratingFor->ExecutableByteCode, node->TargetType->Symbol);
}
