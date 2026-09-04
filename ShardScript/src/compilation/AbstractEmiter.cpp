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
#include <shard/parsing/nodes/Statements/SwitchStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/lexical/TokenType.hpp>

#include <shard/semantic/symbols/LiteralSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
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

// Byte-size a primitive-typed eval entry occupies on the frame, derived from
// the primitive's symbol instead of hardcoded literals.
static std::size_t PrimitivePayload(const TypeSymbol* primitive)
{
	return FrameLayout::ResolveTypePayload(const_cast<TypeSymbol*>(primitive));
}

void AbstractEmiter::EmitUnaryOperation(shard::TokenType type, ByteCodeEncoder& encoder, std::vector<std::byte>& code, bool isRightDetermined)
{
	switch (type)
	{
		case TokenType::SubOperator:
		{
			encoder.EmitMathNegative(code);
			EvalPop();
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
			break;
		}

		case TokenType::AddOperator:
		{
			// Unary plus is a no-op.
			break;
		}

		case TokenType::NotOperator:
		{
			encoder.EmitLogicalNot(code);
			EvalPop();
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));
			break;
		}

		case TokenType::IncrementOperator:
		{
			if (isRightDetermined)
			{
				encoder.EmitLoadConstInt64(code, 1);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitMathAdd(code);
				EvalPop(2);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitDuplicate(code);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				break;
			}
			else
			{
				encoder.EmitDuplicate(code);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitLoadConstInt64(code, 1);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitMathAdd(code);
				EvalPop(2);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				break;
			}

			break;
		}

		case TokenType::DecrementOperator:
		{
			if (isRightDetermined)
			{
				encoder.EmitLoadConstInt64(code, 1);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitMathSub(code);
				EvalPop(2);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitDuplicate(code);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				break;
			}
			else
			{
				encoder.EmitDuplicate(code);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitLoadConstInt64(code, 1);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				encoder.EmitMathSub(code);
				EvalPop(2);
				EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
				break;
			}
		}

		default:
			throw std::runtime_error("unknown operator");
	}
}

static bool IsLogicalComparisonToken(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		case TokenType::OrOperator:
		case TokenType::AndOperator:
			return true;

		default:
			return false;
	}
}

void AbstractEmiter::EmitBinaryOperation(shard::TokenType type, ByteCodeEncoder& encoder, std::vector<std::byte>& code)
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
			encoder.EmitMathShl(code);
			break;
		}

		case TokenType::RightShiftOperator:
		{
			encoder.EmitMathShr(code);
			break;
		}

		default:
			throw std::runtime_error("unknown operator");
	}

	if (type != TokenType::AssignOperator)
	{
		if (type == TokenType::NotOperator)
		{
			EvalPop();
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));
		}
		else
		{
			EvalPop(2);
			EvalPush(IsLogicalComparisonToken(type)
				? PrimitivePayload(SymbolTable::Primitives::Boolean)
				: PrimitivePayload(SymbolTable::Primitives::Integer));
		}
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
	EvalTracker = EvalLayoutTracker();
}

void AbstractEmiter::EvalPush(EvalLayoutTracker& tracker, std::size_t payload)
{
	tracker.CurrentDepth += 1;
	if (tracker.CurrentDepth > tracker.MaxDepth)
		tracker.MaxDepth = tracker.CurrentDepth;
	if (payload > tracker.MaxPayload)
		tracker.MaxPayload = payload;
}

void AbstractEmiter::EvalPop(EvalLayoutTracker& tracker, std::size_t count)
{
	if (tracker.CurrentDepth < count)
	{
		// Tracking desynced (missed emission site or unusual bytecode shape):
		// poison the tracker so no layout is published for this method instead
		// of shipping a potentially under-sized frame.
		tracker.Poisoned = true;
		tracker.CurrentDepth = 0;
		return;
	}

	tracker.CurrentDepth -= count;
}

void AbstractEmiter::EvalDrainDefers(EvalLayoutTracker& tracker)
{
	// A drained defer body executes at the drain site's current depth, which
	// can differ from the depth it was emitted at (e.g. a return statement
	// drains with its value still on the stack). Overlay the deepest body
	// onto the deepest drain site.
	std::size_t overlay = tracker.CurrentDepth + tracker.DeferBodyMax;
	if (overlay > tracker.MaxDepth)
		tracker.MaxDepth = overlay;
}

void AbstractEmiter::PublishLayout(MethodSymbol* method, const EvalLayoutTracker& tracker)
{
	if (method == nullptr || tracker.Poisoned)
		return;

	method->Layout.MaxEvalDepth = static_cast<std::uint32_t>(tracker.MaxDepth);
	method->Layout.EvalSlotPayload = tracker.MaxDepth > 0
		? (tracker.MaxPayload > 1 ? tracker.MaxPayload : 1) : 0;
}

void AbstractEmiter::EvalPush(std::size_t payload)
{
	EvalPush(EvalTracker, payload);
}

void AbstractEmiter::EvalPop(std::size_t count)
{
	EvalPop(EvalTracker, count);
}

void AbstractEmiter::EvalDrainDefers()
{
	EvalDrainDefers(EvalTracker);
}

void AbstractEmiter::EvalFinalizeTarget()
{
	PublishLayout(GeneratingFor, EvalTracker);
	EvalTracker = EvalLayoutTracker(); // the next method starts from a clean tracker
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

	if (EvalTracker.CurrentDepth != 0)
		EvalTracker.Poisoned = true;

	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
	EvalFinalizeTarget();

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

	if (EvalTracker.CurrentDepth != 0)
		EvalTracker.Poisoned = true;

	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
	EvalFinalizeTarget();

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

	if (EvalTracker.CurrentDepth != 0)
		EvalTracker.Poisoned = true;

	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
	EvalFinalizeTarget();

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
				Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, 0);
				EvalPush(FrameLayout::SlotPayload(*GeneratingFor, 0));
				Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, property->BackingField->SlotIndex);
				EvalPop();
				EvalPush(FrameLayout::ResolveTypePayload(property->BackingField->ReturnType));
			}
			else if (node->KeywordToken.Type == TokenType::SetKeyword)
			{
				Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, 0);
				EvalPush(FrameLayout::SlotPayload(*GeneratingFor, 0));
				Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, 1);
				EvalPush(FrameLayout::SlotPayload(*GeneratingFor, 1));
				Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, property->BackingField->SlotIndex);
				EvalPop(2);
			}
		}
		else
		{
			throw std::runtime_error("property backing field not found");
		}
	}

	if (EvalTracker.CurrentDepth != 0)
		EvalTracker.Poisoned = true;

	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
	EvalFinalizeTarget();

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
			{
				Encoder.EmitPop(GeneratingFor->ExecutableByteCode);
				EvalPop();
			}

			break;
		}
	}
}

void AbstractEmiter::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* var = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
	VisitExpression(node->Expression.get());
	Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, var->SlotIndex);
	EvalPop();
}

void AbstractEmiter::VisitTryStatement(TryStatementSyntax* node)
{
	if (node->CatchClauses.empty())
	{
		if (node->TryBlock != nullptr)
			VisitStatementsBlock(node->TryBlock.get());
		return;
	}

	std::size_t tryBaseDepth = EvalTracker.CurrentDepth;

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

	// The VM enters a catch handler with the eval stack truncated to the locals
	// region and the exception pushed: handler-relative depth starts at 1.
	EvalTracker.CurrentDepth = 1;

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
			EvalPush();
			Encoder.EmitIsInstance(GeneratingFor->ExecutableByteCode, catchType);
			EvalPop();
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));
			filterFailBacktracks.back() = GeneratingFor->ExecutableByteCode.size();
			Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
			EvalPop();
		}

		VariableSymbol* catchVariable = clause->Symbol;
		if (catchVariable != nullptr)
		{
			Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, catchVariable->SlotIndex);
			EvalPop();
		}
		else
		{
			Encoder.EmitPop(GeneratingFor->ExecutableByteCode);
			EvalPop();
		}

		if (clause->Body != nullptr)
			VisitStatementsBlock(clause->Body.get());

		bodyEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
	}

	std::size_t fallbackStart = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitThrow(GeneratingFor->ExecutableByteCode);
	EvalPop();

	std::size_t endLabel = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitEndCatch(GeneratingFor->ExecutableByteCode);
	EvalTracker.CurrentDepth = tryBaseDepth;

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
	EvalPop();

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
	EvalPop();

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
	EvalPop();

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

void AbstractEmiter::EmitEnumerationLoop(ExpressionSyntax* range, VariableSymbol* loopVariable, StatementsBlockSyntax* body)
{
	Loops.emplace();
	LoopScope& scope = Loops.top();

	std::uint16_t base = GeneratingFor->GetEvalStackArgumentsCount();
	std::uint16_t enumeratorSlot = base + GeneratingFor->AddVariableCount(TRAIT_ENUMERABLE_GETENUMERATOR->ReturnType);

	VisitExpression(range);
	EmitMethodCall(TRAIT_ENUMERABLE_GETENUMERATOR);
	Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, enumeratorSlot);
	EvalPop();

	scope.LoopStart = GeneratingFor->ExecutableByteCode.size();

	Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, enumeratorSlot);
	EvalPush(FrameLayout::SlotPayload(*GeneratingFor, enumeratorSlot));
	EmitMethodCall(TRAIT_ENUMERATOR_MOVENEXT);

	scope.LoopEndBacktracks.push_back(GeneratingFor->ExecutableByteCode.size());
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
	EvalPop();

	Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, enumeratorSlot);
	EvalPush(FrameLayout::SlotPayload(*GeneratingFor, enumeratorSlot));
	EmitMethodCall(TRAIT_ENUMERATOR_CURRENT_GET);

	if (loopVariable != nullptr)
	{
		Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, loopVariable->SlotIndex);
		EvalPop();
	}
	else
	{
		Encoder.EmitPop(GeneratingFor->ExecutableByteCode);
		EvalPop();
	}

	VisitStatementsBlock(body);

	scope.BlockEnd = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, scope.LoopStart);

	scope.LoopEnd = GeneratingFor->ExecutableByteCode.size();

	for (std::size_t backtrack : scope.BlockEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.BlockEnd, sizeof(std::size_t));

	for (std::size_t backtrack : scope.LoopEndBacktracks)
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, backtrack + sizeof(OpCode), &scope.LoopEnd, sizeof(std::size_t));

	Loops.pop();
}

void AbstractEmiter::VisitForEachStatement(ForEachStatementSyntax* node)
{
	EmitEnumerationLoop(node->RangeExpression.get(),
		LookupSymbol<VariableSymbol>(node).value_or(nullptr),
		node->StatementsBlock.get());
}

void AbstractEmiter::VisitForInStatement(ForInStatementSyntax* node)
{
	EmitEnumerationLoop(node->RangeExpression.get(),
		LookupSymbol<VariableSymbol>(node).value_or(nullptr),
		node->StatementsBlock.get());
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

	std::size_t conditionalJumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
	EvalPop();

	VisitStatementsBlock(node->StatementsBlock.get());

	if (node->NextStatement != nullptr)
	{
		std::size_t endJumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(endJumpAddress);

		std::size_t nextClauseStart = GeneratingFor->ExecutableByteCode.size();
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, conditionalJumpAddress + sizeof(OpCode), &nextClauseStart, sizeof(std::size_t));

		VisitConditionalClause(node->NextStatement.get());
	}
	else
	{
		std::size_t clauseEnd = GeneratingFor->ExecutableByteCode.size();
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, conditionalJumpAddress + sizeof(OpCode), &clauseEnd, sizeof(std::size_t));
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

	std::size_t conditionalJumpAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJumpTrue(GeneratingFor->ExecutableByteCode, 0);
	EvalPop();

	VisitStatementsBlock(node->StatementsBlock.get());

	if (node->NextStatement != nullptr)
	{
		std::size_t endJumpAddress = GeneratingFor->ExecutableByteCode.size();
		Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
		scope.ClauseEndBacktracks.push_back(endJumpAddress);

		std::size_t nextClauseStart = GeneratingFor->ExecutableByteCode.size();
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, conditionalJumpAddress + sizeof(OpCode), &nextClauseStart, sizeof(std::size_t));

		VisitConditionalClause(node->NextStatement.get());
	}
	else
	{
		std::size_t clauseEnd = GeneratingFor->ExecutableByteCode.size();
		ByteCodeEncoder::PasteData(GeneratingFor->ExecutableByteCode, conditionalJumpAddress + sizeof(OpCode), &clauseEnd, sizeof(std::size_t));
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
			EvalPush();
			break;
		}

		case TokenType::CharLiteral:
		{
			Encoder.EmitLoadConstChar16(GeneratingFor->ExecutableByteCode, node->LiteralToken.Word[0]);
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Char));
			break;
		}

		case TokenType::StringLiteral:
		{
			Encoder.EmitLoadConstString(GeneratingFor->ExecutableByteCode, Program.DataSection, node->LiteralToken.Word.data());
			EvalPush();
			break;
		}

		case TokenType::NumberLiteral:
		{
			Encoder.EmitLoadConstInt64(GeneratingFor->ExecutableByteCode, symbol->AsIntegerValue);
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Integer));
			if (symbol->BoundType != nullptr && symbol->BoundType != SymbolTable::Primitives::Integer)
			{
				Encoder.EmitCastPrimitive(GeneratingFor->ExecutableByteCode, symbol->BoundType);
				EvalPop();
				EvalPush(FrameLayout::ResolveTypePayload(symbol->BoundType));
			}

			break;
		}

		case TokenType::DoubleLiteral:
		{
			Encoder.EmitLoadConstDouble64(GeneratingFor->ExecutableByteCode, symbol->AsDoubleValue);
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Double));
			break;
		}

		case TokenType::BooleanLiteral:
		{
			Encoder.EmitLoadConstBool(GeneratingFor->ExecutableByteCode, symbol->AsBooleanValue);
			EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));
			break;
		}

		default:
			throw std::runtime_error("unsupported literal type");
	}
}

void AbstractEmiter::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	if (node->IsArrayCreation)
	{
		if (node->ArraySize != nullptr)
			VisitExpression(node->ArraySize.get());

		TypeSymbol* elementType = node->Type != nullptr ? node->Type->Symbol : nullptr;
		Encoder.EmitNewArrayDynamic(GeneratingFor->ExecutableByteCode, elementType);
		EvalPop();
		EvalPush();
		return;
	}

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

	// InstantiateObject pushes the fresh instance for the ctor frame before the
	// frame pops it together with the arguments; the constructed instance is
	// then pushed as the result.
	EvalPush();
	if (node->CtorSymbol != nullptr)
		EvalPop(node->CtorSymbol->GetEvalStackArgumentsCount());
	else
		EvalPop();
	EvalPush(FrameLayout::ResolveTypePayload(node->Symbol));
}

void AbstractEmiter::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (auto riter = node->ValuesExpressions.rbegin(); riter != node->ValuesExpressions.rend(); riter++)
		VisitExpression((*riter).get());

	Encoder.EmitNewArray(GeneratingFor->ExecutableByteCode, node->Symbol);
	if (node->Symbol != nullptr)
		EvalPop(node->Symbol->Length);
	else
		EvalPop();
	EvalPush();
}

void AbstractEmiter::VisitRangeExpression(RangeExpressionSyntax* node)
{
	// Evaluate bounds in natural left-to-right order and let the VM build the array.
	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());
	Encoder.EmitLoadConstBool(GeneratingFor->ExecutableByteCode, node->IsInclusive);
	EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));
	Encoder.EmitCreateRange(GeneratingFor->ExecutableByteCode, SymbolTable::Primitives::Integer);
	EvalPop(3);
	EvalPush();
}

void AbstractEmiter::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	MethodSymbol* previous = GeneratingFor;
	MethodSymbol* bodyMethod = node->ClosureMethod != nullptr
		? node->ClosureMethod
		: node->Symbol->AnonymousSymbol;
	GeneratingFor = bodyMethod;

	std::vector<DeferScope> previousDefers = std::move(DeferScopes);
	DeferScopes = std::vector<DeferScope>();

	// The closure body is a separate method with its own frame layout; track it
	// with a fresh tracker and restore the outer one afterwards.
	EvalLayoutTracker outerTracker = EvalTracker;
	EvalTracker = EvalLayoutTracker();

	std::size_t reserve = node->Body->Statements.size() * 20;
	GeneratingFor->ExecutableByteCode.reserve(reserve);
	VisitStatementsBlock(node->Body.get());

	if (EvalTracker.CurrentDepth != 0)
		EvalTracker.Poisoned = true;

	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
	EvalFinalizeTarget();

	GeneratingFor->ExecutableByteCode.shrink_to_fit();

	EvalTracker = outerTracker;
	DeferScopes = std::move(previousDefers);
	GeneratingFor = previous;

	if (node->ClosureClass != nullptr)
	{
		// Allocate the closure box.
		Encoder.EmitNewObject(GeneratingFor->ExecutableByteCode, static_cast<TypeSymbol*>(node->ClosureClass), node->ClosureConstructor);
		EvalPush();
		if (node->ClosureConstructor != nullptr)
			EvalPop(node->ClosureConstructor->GetEvalStackArgumentsCount());
		else
			EvalPop();

		// Store each captured value into the matching field.
		for (const auto& pair : node->CaptureFields)
		{
			SyntaxSymbol* captured = pair.first;
			FieldSymbol* field = pair.second;

			Encoder.EmitDuplicate(GeneratingFor->ExecutableByteCode);
			EvalPush();
			if (captured->Kind == SyntaxKind::Parameter)
			{
				Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode,
					static_cast<ParameterSymbol*>(captured)->SlotIndex);
				EvalPush(FrameLayout::SlotPayload(*GeneratingFor,
					static_cast<ParameterSymbol*>(captured)->SlotIndex));
			}
			else if (captured->Kind == SyntaxKind::VariableStatement)
			{
				Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode,
					static_cast<VariableSymbol*>(captured)->SlotIndex);
				EvalPush(FrameLayout::SlotPayload(*GeneratingFor,
					static_cast<VariableSymbol*>(captured)->SlotIndex));
			}

			Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, field->SlotIndex);
			EvalPop(2);
		}

		// The closure object itself becomes the delegate instance.
		Encoder.EmitNewDelegate(GeneratingFor->ExecutableByteCode, node->Symbol);
		if (node->Symbol != nullptr && node->Symbol->AnonymousSymbol != nullptr &&
			node->Symbol->AnonymousSymbol->Linking == LINK_INSTANCE)
			EvalPop();
		EvalPush();
	}
	else
	{
		Encoder.EmitNewDelegate(GeneratingFor->ExecutableByteCode, node->Symbol);
		EvalPush();
	}
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
	EvalPop();

	std::size_t armBaseDepth = EvalTracker.CurrentDepth;
	VisitExpression(node->Left.get());
	std::size_t jumpEndAddress = GeneratingFor->ExecutableByteCode.size();
	Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);

	// The right arm starts from the pre-arm depth at runtime (control arrived
	// via the conditional jump, past the left arm's result).
	EvalTracker.CurrentDepth = armBaseDepth;

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
		Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		EvalPush(FrameLayout::SlotPayload(*GeneratingFor, memberExpression->ToParameter->SlotIndex));
		EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
		Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		EvalPop();
		return;
	}

	if (memberExpression->ToVariable != nullptr)
	{
		Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		EvalPush(FrameLayout::SlotPayload(*GeneratingFor, memberExpression->ToVariable->SlotIndex));
		EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
		Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		EvalPop();
		return;
	}

	if (memberExpression->ToField != nullptr)
	{
		if (memberExpression->ToField->Linking == LINK_STATIC)
		{
			Encoder.EmitLoadStaticField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
			EvalPush();
			EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
			Encoder.EmitStoreStaticField(GeneratingFor->ExecutableByteCode, memberExpression->ToField);
			EvalPop();
			return;
		}

		VisitExpression(memberExpression->PreviousExpression.get());
		Encoder.EmitDuplicate(GeneratingFor->ExecutableByteCode);
		EvalPush();
		Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, memberExpression->ToField->SlotIndex);
		EvalPop();
		EvalPush(FrameLayout::ResolveTypePayload(memberExpression->ToField->ReturnType));
		EmitUnaryOperation(node->OperatorToken.Type, Encoder, GeneratingFor->ExecutableByteCode, node->IsRightDetermined);
		Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, memberExpression->ToField->SlotIndex);
		EvalPop(2);
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
			EvalPush();

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
			Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, tempValueSlot);
			EvalPop();
			Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, tempThisSlot);
			EvalPop();

			// Push setter arguments
			if (indexatorExpr != nullptr && indexatorExpr->IndexatorList != nullptr)
				EmitIndexatorArguments(this, indexatorExpr->IndexatorList.get());

			Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, tempValueSlot);
			EvalPush(FrameLayout::SlotPayload(*GeneratingFor, tempValueSlot));
			Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, tempThisSlot);
			EvalPush(FrameLayout::SlotPayload(*GeneratingFor, tempThisSlot));

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
		Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, memberExpression->ToParameter->SlotIndex);
		EvalPop();
		return;
	}

	if (memberExpression->ToVariable != nullptr)
	{
		VisitExpression(node->Right.get());
		Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, memberExpression->ToVariable->SlotIndex);
		EvalPop();
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
			EvalPop();
			return;
		}

		VisitExpression(memberExpression->PreviousExpression.get());
		VisitExpression(node->Right.get());
		Encoder.EmitStoreField(GeneratingFor->ExecutableByteCode, memberExpression->ToField->SlotIndex);
		EvalPop(2);
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

	// Evaluate arguments and receiver first. This prevents nested generic calls
	// inside arguments or the receiver from clobbering the pending type arguments
	// for this invocation.
	VisitArgumentsList(node->ArgumentsList.get());
	if (node->PreviousExpression != nullptr)
		VisitExpression(node->PreviousExpression.get());

	bool hasTypeArguments = false;
	std::size_t ownerParamCount = 0;

	// Extension methods are static methods in another namespace; their type
	// arguments are just the method's own type parameters, not the receiver's
	// class type parameters.
	if (!node->IsExtensionMethodInvocation &&
	    node->ReceiverType != nullptr && node->ReceiverType->Kind == SyntaxKind::GenericType)
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

	if (node->IsDelegateInvocation)
	{
		DelegateTypeSymbol* delegateType = nullptr;
		if (node->Symbol != nullptr && node->Symbol->Parent != nullptr && node->Symbol->Parent->Kind == SyntaxKind::DelegateDeclaration)
			delegateType = static_cast<DelegateTypeSymbol*>(node->Symbol->Parent);

		Encoder.EmitCallDelegate(GeneratingFor->ExecutableByteCode, delegateType);

		// VirtualMachine pops the delegate instance; closure targets get it back
		// as 'this' before the target pops its arguments.
		EvalPop();
		MethodSymbol* anon = delegateType != nullptr ? delegateType->AnonymousSymbol : nullptr;
		if (anon != nullptr)
		{
			if (anon->Linking == LINK_INSTANCE)
				EvalPush(); // pushed back as 'this'
			EvalPop(anon->GetEvalStackArgumentsCount());
			if (anon->ReturnType != nullptr && anon->ReturnType != SymbolTable::Primitives::Void)
				EvalPush(FrameLayout::ResolveTypePayload(anon->ReturnType));
		}
		else
		{
			// Unresolvable target: under-model pops, reference-sized push.
			EvalPop();
			EvalPush();
		}
	}

	else if (hasTypeArguments)
	{
		Encoder.EmitCallMethodSymbol(GeneratingFor->ExecutableByteCode, node->Symbol);
		if (node->Symbol != nullptr)
		{
			EvalPop(node->Symbol->GetEvalStackArgumentsCount());
			if (node->Symbol->ReturnType != nullptr && node->Symbol->ReturnType != SymbolTable::Primitives::Void)
				EvalPush(FrameLayout::ResolveTypePayload(node->Symbol->ReturnType));
		}
		else
		{
			EvalPop();
			EvalPush();
		}
	}
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
		Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, node->ToParameter->SlotIndex);
		EvalPush(FrameLayout::SlotPayload(*GeneratingFor, node->ToParameter->SlotIndex));
		return;
	}

	if (node->ToVariable != nullptr)
	{
		Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, node->ToVariable->SlotIndex);
		EvalPush(FrameLayout::SlotPayload(*GeneratingFor, node->ToVariable->SlotIndex));
		return;
	}

	if (node->ToField != nullptr)
	{
		if (node->ToField->IsEnumValue)
		{
			Encoder.EmitLoadEnumField(GeneratingFor->ExecutableByteCode, node->ToField);
			EvalPush(FrameLayout::ResolveTypePayload(node->ToField->ReturnType));
			return;
		}

		if (node->ToField->Linking == LINK_STATIC)
		{
			Encoder.EmitLoadStaticField(GeneratingFor->ExecutableByteCode, node->ToField);
			EvalPush();
			return;
		}

		VisitExpression(node->PreviousExpression.get());
		Encoder.EmitLoadField(GeneratingFor->ExecutableByteCode, node->ToField->SlotIndex);
		EvalPop();
		EvalPush(FrameLayout::ResolveTypePayload(node->ToField->ReturnType));
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
		if (node->ToDelegate->AnonymousSymbol != nullptr && node->ToDelegate->AnonymousSymbol->Linking == LINK_INSTANCE)
			EvalPop();
		EvalPush();
		return;
	}

	if (node->ToOperator != nullptr)
	{
		Encoder.EmitLoadConstString(GeneratingFor->ExecutableByteCode, Program.DataSection, node->IdentifierToken.Word.data());
		EvalPush();
		VisitExpression(node->PreviousExpression.get());
		EmitMethodCall(node->ToOperator);
		return;
	}
}

void AbstractEmiter::EmitDefer(DeferStatementSyntax* defer)
{
	if (defer == nullptr)
		return;

	std::vector<std::byte>& code = GeneratingFor->ExecutableByteCode;

	std::size_t deferOffset = code.size();
	Encoder.EmitDefer(code, 0);

	std::size_t jumpOverBacktrack = code.size();
	Encoder.EmitJump(code, 0);

	// The deferred body runs later, at the depth of whatever DEFER_DRAIN site
	// drains it — not at the registration depth. Track it in isolation and
	// remember its internal peak for EvalDrainDefers.
	EvalLayoutTracker outerTracker = EvalTracker;
	EvalTracker = EvalLayoutTracker();

	std::size_t expressionStart = code.size();
	if (defer->IsResourceDefer)
	{
		if (defer->Variable != nullptr)
		{
			Encoder.EmitLoadLocal(code, defer->Variable->SlotIndex);
			EvalPush(FrameLayout::SlotPayload(*GeneratingFor, defer->Variable->SlotIndex));
		}

		if (defer->DisposeMethod != nullptr)
			EmitMethodCall(defer->DisposeMethod);
	}
	else if (defer->Statement != nullptr)
	{
		VisitStatement(defer->Statement.get());
	}

	Encoder.EmitDeferBreak(code);
	std::size_t afterExpression = code.size();

	if (!EvalTracker.Poisoned && EvalTracker.CurrentDepth == 0)
	{
		if (EvalTracker.MaxDepth > outerTracker.DeferBodyMax)
			outerTracker.DeferBodyMax = EvalTracker.MaxDepth;
	}
	else
	{
		outerTracker.Poisoned = true;
	}

	if (EvalTracker.MaxPayload > outerTracker.MaxPayload)
		outerTracker.MaxPayload = EvalTracker.MaxPayload;

	EvalTracker = outerTracker;

	ByteCodeEncoder::PasteData(code, deferOffset + sizeof(OpCode), &expressionStart, sizeof(std::size_t));
	ByteCodeEncoder::PasteData(code, jumpOverBacktrack + sizeof(OpCode), &afterExpression, sizeof(std::size_t));
}

void AbstractEmiter::EmitCurrentScopeDefers()
{
	if (DeferScopes.empty())
		return;

	DeferScope& scope = DeferScopes.back();
	if (scope.Count > 0)
	{
		Encoder.EmitDeferDrain(GeneratingFor->ExecutableByteCode, scope.Count);
		EvalDrainDefers();
	}

	scope.Count = 0;
}

void AbstractEmiter::EmitDefersUntilLoop()
{
	std::size_t drainCount = 0;
	for (auto it = DeferScopes.rbegin(); it != DeferScopes.rend(); ++it)
	{
		drainCount += it->Count;
		it->Count = 0;

		if (it->IsLoop)
			break;
	}

	if (drainCount > 0)
	{
		Encoder.EmitDeferDrain(GeneratingFor->ExecutableByteCode, drainCount);
		EvalDrainDefers();
	}
}

void AbstractEmiter::EmitAllDefers()
{
	std::size_t drainCount = 0;
	for (auto it = DeferScopes.rbegin(); it != DeferScopes.rend(); ++it)
	{
		drainCount += it->Count;
		it->Count = 0;
	}

	if (drainCount > 0)
	{
		Encoder.EmitDeferDrain(GeneratingFor->ExecutableByteCode, drainCount);
		EvalDrainDefers();
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
		node->Parent->Kind == SyntaxKind::ForEachStatement ||
		node->Parent->Kind == SyntaxKind::SwitchStatement);

	DeferScopes.push_back({ 0, isLoop });

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

	DeferScopes.back().Count++;
	EmitDefer(node);
}

void AbstractEmiter::VisitReturnStatement(ReturnStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	EmitAllDefers();

	std::size_t expectedDepth = node->Expression != nullptr ? 1 : 0;
	if (EvalTracker.CurrentDepth != expectedDepth)
		EvalTracker.Poisoned = true;

	Encoder.EmitReturn(GeneratingFor->ExecutableByteCode);
	EvalTracker.CurrentDepth = 0; // unreachable code after 'return' is tracked from a clean base
}

void AbstractEmiter::VisitBreakStatement(BreakStatementSyntax* node)
{
	if (Loops.empty())
	{
		Diagnostics.ReportError(node->KeywordToken, L"'break' must be inside a loop or switch");
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

	// Do not emit DEFER_DRAIN here. The VM's exception dispatch drains the
	// defers that belong to the scopes being unwound, and any remaining defers
	// are drained before the frame returns with an unhandled exception.

	if (node->Expression != nullptr)
	{
		Encoder.EmitThrow(GeneratingFor->ExecutableByteCode);
		EvalPop();
	}
	else
	{
		Encoder.EmitRethrow(GeneratingFor->ExecutableByteCode);
	}

	EvalTracker.CurrentDepth = 0; // unreachable code after 'throw' is tracked from a clean base
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

    EvalPop(method->GetEvalStackArgumentsCount());
    if (method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void)
        EvalPush(FrameLayout::ResolveTypePayload(method->ReturnType));
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
        EvalPop();
        EvalPush(FrameLayout::ResolveTypePayload(node->TargetType->Symbol));
        return;
    }

    Encoder.EmitCast(GeneratingFor->ExecutableByteCode, node->TargetType->Symbol);
    EvalPop();
    EvalPush();
}

void AbstractEmiter::VisitIsExpression(IsExpressionSyntax* node)
{
    if (node->Expression != nullptr)
        VisitExpression(node->Expression.get());

    if (node->TargetType != nullptr && node->TargetType->Symbol != nullptr)
    {
        Encoder.EmitIsInstance(GeneratingFor->ExecutableByteCode, node->TargetType->Symbol);
        EvalPop();
        EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));
    }
}

void AbstractEmiter::VisitIsPattern(IsPatternSyntax* node)
{
    // Patterns are emitted as part of VisitSwitchExpression; this node itself
    // produces no standalone bytecode.
}

static bool isDefaultPattern(ExpressionSyntax* pattern)
{
	if (pattern == nullptr || pattern->Kind != SyntaxKind::LiteralExpression)
		return false;

	LiteralExpressionSyntax* literal = static_cast<LiteralExpressionSyntax*>(pattern);
	return literal->LiteralToken.Type == TokenType::Identifier && literal->LiteralToken.Word == L"_";
}

void AbstractEmiter::VisitSwitchExpression(SwitchExpressionSyntax* node)
{
    if (node->Expression == nullptr || node->Arms.empty())
    {
        Encoder.EmitLoadConstNull(GeneratingFor->ExecutableByteCode);
        EvalPush();
        return;
    }

    VisitExpression(node->Expression.get());

    std::uint16_t base = GeneratingFor->GetEvalStackArgumentsCount();
    std::uint16_t switchSlot = base + GeneratingFor->AddVariableCount();
    Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, switchSlot);
    EvalPop();

    std::vector<std::size_t> endBacktracks;
    std::optional<std::size_t> pendingTestFailBacktrack;

    // Arms are reached via jump, always with the eval stack at the base depth.
    std::size_t armBaseDepth = EvalTracker.CurrentDepth;

    for (std::size_t i = 0; i < node->Arms.size(); ++i)
    {
        if (pendingTestFailBacktrack.has_value())
        {
            std::size_t nextArmStart = GeneratingFor->ExecutableByteCode.size();
            ByteCodeEncoder::PasteData(
                GeneratingFor->ExecutableByteCode,
                pendingTestFailBacktrack.value() + sizeof(OpCode),
                &nextArmStart,
                sizeof(std::size_t));
            pendingTestFailBacktrack.reset();
        }

        EvalTracker.CurrentDepth = armBaseDepth;

        SwitchArmSyntax* arm = node->Arms[i].get();
        if (arm == nullptr || arm->Pattern == nullptr || arm->Expression == nullptr)
            continue;

        ExpressionSyntax* pattern = arm->Pattern.get();

        if (isDefaultPattern(pattern))
        {
            VisitExpression(arm->Expression.get());

            std::size_t jumpEnd = GeneratingFor->ExecutableByteCode.size();
            endBacktracks.push_back(jumpEnd);
            Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
            continue;
        }

        if (pattern->Kind == SyntaxKind::IsPattern)
        {
            IsPatternSyntax* isPattern = static_cast<IsPatternSyntax*>(pattern);
            TypeSymbol* targetType = (isPattern->TargetType != nullptr)
                ? isPattern->TargetType->Symbol
                : nullptr;

            if (targetType == nullptr)
            {
                // No type to test; skip this arm.
                continue;
            }

            Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, switchSlot);
            EvalPush(FrameLayout::SlotPayload(*GeneratingFor, switchSlot));
            Encoder.EmitIsInstance(GeneratingFor->ExecutableByteCode, targetType);
            EvalPop();
            EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));

            std::size_t jumpFalse = GeneratingFor->ExecutableByteCode.size();
            pendingTestFailBacktrack = jumpFalse;
            Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
            EvalPop();

            // Pattern matched. Load the value again to bind or discard.
            Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, switchSlot);
            EvalPush(FrameLayout::SlotPayload(*GeneratingFor, switchSlot));
            if (isPattern->Symbol != nullptr)
            {
                std::uint16_t patternSlot = base + GeneratingFor->AddVariableCount(const_cast<TypeSymbol*>(isPattern->Symbol->Type));
                isPattern->Symbol->SlotIndex = patternSlot;
                Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, patternSlot);
                EvalPop();
            }
            else
            {
                Encoder.EmitPop(GeneratingFor->ExecutableByteCode);
                EvalPop();
            }

            VisitExpression(arm->Expression.get());

            std::size_t jumpEnd = GeneratingFor->ExecutableByteCode.size();
            endBacktracks.push_back(jumpEnd);
            Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);

            continue;
        }

        // Value patterns are not yet supported.
        Diagnostics.ReportError(arm->ArrowToken,
            L"Only 'is Type name' and '_' patterns are supported in switch expressions");
    }

    // If no arm matched, leave null on the stack as the result.
    EvalTracker.CurrentDepth = armBaseDepth;
    std::size_t nullLoadOffset = GeneratingFor->ExecutableByteCode.size();
    Encoder.EmitLoadConstNull(GeneratingFor->ExecutableByteCode);
    EvalPush();

    std::size_t endOffset = GeneratingFor->ExecutableByteCode.size();

    if (pendingTestFailBacktrack.has_value())
    {
        ByteCodeEncoder::PasteData(
            GeneratingFor->ExecutableByteCode,
            pendingTestFailBacktrack.value() + sizeof(OpCode),
            &nullLoadOffset,
            sizeof(std::size_t));
    }

    for (std::size_t backtrack : endBacktracks)
    {
        ByteCodeEncoder::PasteData(
            GeneratingFor->ExecutableByteCode,
            backtrack + sizeof(OpCode),
            &endOffset,
            sizeof(std::size_t));
    }
}

void AbstractEmiter::VisitSwitchStatement(SwitchStatementSyntax* node)
{
    if (node->Expression == nullptr || node->Clauses.empty())
        return;

    VisitExpression(node->Expression.get());

    std::uint16_t base = GeneratingFor->GetEvalStackArgumentsCount();
    std::uint16_t switchSlot = base + GeneratingFor->AddVariableCount();
    Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, switchSlot);
    EvalPop();

    const std::size_t clauseBaseDepth = EvalTracker.CurrentDepth;

    Loops.emplace();
    LoopScope& scope = Loops.top();

    std::optional<std::size_t> pendingTestFailBacktrack;
    std::vector<std::size_t> endBacktracks;

    for (std::size_t i = 0; i < node->Clauses.size(); ++i)
    {
        if (pendingTestFailBacktrack.has_value())
        {
            std::size_t nextClauseStart = GeneratingFor->ExecutableByteCode.size();
            ByteCodeEncoder::PasteData(
                GeneratingFor->ExecutableByteCode,
                pendingTestFailBacktrack.value() + sizeof(OpCode),
                &nextClauseStart,
                sizeof(std::size_t));
            pendingTestFailBacktrack.reset();
        }

        // Each clause starts from a clean eval depth; statement bodies are balanced,
        // but resetting here keeps a stray imbalance in one clause from poisoning the rest.
        EvalTracker.CurrentDepth = clauseBaseDepth;

        SwitchCaseClauseSyntax* clause = node->Clauses[i].get();
        if (clause == nullptr || clause->Body == nullptr)
            continue;

        ExpressionSyntax* pattern = clause->Pattern.get();

        if (pattern == nullptr)
        {
            // default clause
            VisitStatementsBlock(clause->Body.get());

            std::size_t jumpEnd = GeneratingFor->ExecutableByteCode.size();
            endBacktracks.push_back(jumpEnd);
            Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
            continue;
        }

        if (pattern->Kind == SyntaxKind::IsPattern)
        {
            IsPatternSyntax* isPattern = static_cast<IsPatternSyntax*>(pattern);
            TypeSymbol* targetType = (isPattern->TargetType != nullptr)
                ? isPattern->TargetType->Symbol
                : nullptr;

            if (targetType == nullptr)
                continue;

            Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, switchSlot);
            EvalPush(FrameLayout::SlotPayload(*GeneratingFor, switchSlot));
            Encoder.EmitIsInstance(GeneratingFor->ExecutableByteCode, targetType);
            EvalPop();
            EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));

            std::size_t jumpFalse = GeneratingFor->ExecutableByteCode.size();
            pendingTestFailBacktrack = jumpFalse;
            Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
            EvalPop();

            Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, switchSlot);
            EvalPush(FrameLayout::SlotPayload(*GeneratingFor, switchSlot));
            if (isPattern->Symbol != nullptr)
            {
                std::uint16_t patternSlot = base + GeneratingFor->AddVariableCount(const_cast<TypeSymbol*>(isPattern->Symbol->Type));
                isPattern->Symbol->SlotIndex = patternSlot;
                Encoder.EmitStoreLocal(GeneratingFor->ExecutableByteCode, patternSlot);
                EvalPop();
            }
            else
            {
                Encoder.EmitPop(GeneratingFor->ExecutableByteCode);
                EvalPop();
            }

            VisitStatementsBlock(clause->Body.get());

            std::size_t jumpEnd = GeneratingFor->ExecutableByteCode.size();
            endBacktracks.push_back(jumpEnd);
            Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);

            continue;
        }

        // Value pattern: compare switch value to pattern value.
        if (clause->EqualityOperator != nullptr)
        {
            VisitExpression(pattern);
            Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, switchSlot);
            EmitMethodCall(clause->EqualityOperator);
        }
        else
        {
            Encoder.EmitLoadLocal(GeneratingFor->ExecutableByteCode, switchSlot);
            EvalPush(FrameLayout::SlotPayload(*GeneratingFor, switchSlot));
            VisitExpression(pattern);
            Encoder.EmitCompareEqual(GeneratingFor->ExecutableByteCode);
            EvalPop(2);
            EvalPush(PrimitivePayload(SymbolTable::Primitives::Boolean));
        }

        std::size_t jumpFalse = GeneratingFor->ExecutableByteCode.size();
        pendingTestFailBacktrack = jumpFalse;
        Encoder.EmitJumpFalse(GeneratingFor->ExecutableByteCode, 0);
        EvalPop();

        VisitStatementsBlock(clause->Body.get());

        std::size_t jumpEnd = GeneratingFor->ExecutableByteCode.size();
        endBacktracks.push_back(jumpEnd);
        Encoder.EmitJump(GeneratingFor->ExecutableByteCode, 0);
    }

    // All clause paths converge at the saved base depth (fall-through, jumps and clause
    // bodies are balanced, so any residual depth here is an emission bug we want to surface).
    EvalTracker.CurrentDepth = clauseBaseDepth;

    std::size_t endOffset = GeneratingFor->ExecutableByteCode.size();

    if (pendingTestFailBacktrack.has_value())
    {
        ByteCodeEncoder::PasteData(
            GeneratingFor->ExecutableByteCode,
            pendingTestFailBacktrack.value() + sizeof(OpCode),
            &endOffset,
            sizeof(std::size_t));
    }

    for (std::size_t backtrack : endBacktracks)
    {
        ByteCodeEncoder::PasteData(
            GeneratingFor->ExecutableByteCode,
            backtrack + sizeof(OpCode),
            &endOffset,
            sizeof(std::size_t));
    }

    for (std::size_t backtrack : scope.LoopEndBacktracks)
    {
        ByteCodeEncoder::PasteData(
            GeneratingFor->ExecutableByteCode,
            backtrack + sizeof(OpCode),
            &endOffset,
            sizeof(std::size_t));
    }

    scope.LoopEnd = endOffset;
    Loops.pop();
}
