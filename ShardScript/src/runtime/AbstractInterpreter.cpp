#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/PrimitiveMathModule.h>
#include <shard/runtime/AbstractInterpreter.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>

#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>

#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <iterator>
#include <stack>

using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;
using namespace shard::parsing;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;

std::stack<CallStackFrame*> AbstractInterpreter::callStack;

CallStackFrame* AbstractInterpreter::CurrentFrame()
{
	if (callStack.empty())
		return nullptr;

	return callStack.top();
}

void AbstractInterpreter::PushFrame(MethodSymbol* methodSymbol)
{
	callStack.push(new CallStackFrame(methodSymbol, CurrentFrame()));
}

void AbstractInterpreter::PopFrame()
{
	if (callStack.empty())
		return;

	CallStackFrame* current = CurrentFrame();
	callStack.pop();
	delete current;
}

InboundVariablesContext* AbstractInterpreter::CurrentContext()
{
	CallStackFrame* frame = CurrentFrame();
	if (frame == nullptr)
		return nullptr;

	if (frame->VariablesStack.empty())
		return nullptr;

	return frame->VariablesStack.top();
}

void AbstractInterpreter::PushContext(InboundVariablesContext* context)
{
	CurrentFrame()->VariablesStack.push(context);
}

void AbstractInterpreter::PopContext()
{
	InboundVariablesContext* current = CurrentContext();
	CurrentFrame()->VariablesStack.pop();
	delete current;
}

void AbstractInterpreter::TerminateCallStack()
{
	while (!callStack.empty())
		PopFrame();
}

void AbstractInterpreter::Execute(SyntaxTree& syntaxTree, SemanticModel& semanticModel)
{
	MethodSymbol* entryPoint = semanticModel.Table->EntryPointCandidates.at(0);
	ExecuteMethod(entryPoint, nullptr);
}

void AbstractInterpreter::RaiseException(ObjectInstance* exceptionReg)
{
	CallStackFrame* frame = CurrentFrame();
	if (frame->PreviousFrame != nullptr)
	{
		frame->PreviousFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
		frame->PreviousFrame->InterruptionRegister = exceptionReg;
		return;
	}

	// null previous frame => frame is main entry point
	TypeSymbol* exceptionType = const_cast<TypeSymbol*>(exceptionReg->Info);
	if (exceptionType == SymbolTable::Primitives::String)
	{
		ConsoleHelper::WriteLine(L"Critical error, could not invoke to string method, to get exception description. Exception type - " + exceptionType->Name);
		return;
	}

	std::wstring toStringMethodName = L"ToString";
	MethodSymbol* toStringMethod = exceptionType->FindMethod(toStringMethodName, std::vector<TypeSymbol*>());

	ObjectInstance* toStringMethodRet = ExecuteMethod(toStringMethod, nullptr);
	if (toStringMethodRet->Info != SymbolTable::Primitives::String)
	{
		ConsoleHelper::WriteLine(L"Critical error, could not invoke to string method, to get exception description. Exception type - " + exceptionType->Name);
		return;
	}

	std::wstring exceptionString = toStringMethodRet->ReadPrimitive<std::wstring>();
	ConsoleHelper::WriteLine(exceptionString);
}

ObjectInstance* AbstractInterpreter::ExecuteMethod(MethodSymbol* method, InboundVariablesContext* argumentsContext)
{
	PushFrame(method);
	PushContext(argumentsContext);
	CallStackFrame* frame = CurrentFrame();

	switch (method->HandleType)
	{
		case MethodHandleType::ObjectInstance:
		{
			ExecuteBlock(method->Body);
			break;
		}

		case MethodHandleType::FunctionPointer:
		{
			try
			{
				frame->InterruptionReason = FrameInterruptionReason::ValueReturned;
				frame->InterruptionRegister = method->FunctionPointer(argumentsContext);
			}
			catch (const std::runtime_error& err)
			{
				frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				frame->InterruptionRegister = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String);

				std::string description = err.what();
				std::wstring wdescription = std::wstring(description.begin(), description.end());
				frame->InterruptionRegister->WritePrimitive<std::wstring>(wdescription);
			}

			break;
		}

		case MethodHandleType::ForeignInterface:
		{
			throw std::runtime_error("FFI not implemented");
			break;
		}
	}

	ObjectInstance* retReg = nullptr;
	switch (frame->InterruptionReason)
	{
		case FrameInterruptionReason::ExceptionRaised:
		{
			RaiseException(frame->InterruptionRegister);
			break;
		}

		case FrameInterruptionReason::ValueReturned:
		{
			retReg = frame->InterruptionRegister;
			break;
		}
	}

	PopContext();
	PopFrame();
	return retReg;
}

ObjectInstance* AbstractInterpreter::ExecuteBlock(const StatementsBlockSyntax* block)
{
	PushContext(new InboundVariablesContext(CurrentContext()));
	CallStackFrame* frame = CurrentFrame();

	auto statement = block->Statements.begin();
	while (statement != block->Statements.end() && !frame->interrupted())
	{
		ExecuteStatement(*statement);
		statement = next(statement, 1);
	}

	PopContext();
	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteStatement(const StatementSyntax* statement)
{
	switch (statement->Kind)
	{
		case SyntaxKind::ExpressionStatement:
		{
			const ExpressionStatementSyntax* expressionStatement = static_cast<const ExpressionStatementSyntax*>(statement);
			return ExecuteExpressionStatement(expressionStatement);
		}

		case SyntaxKind::VariableStatement:
		{
			const VariableStatementSyntax* variableStatement = static_cast<const VariableStatementSyntax*>(statement);
			return ExecuteVariableStatement(variableStatement);
		}

		case SyntaxKind::ThrowStatement:
		{
			const ThrowStatementSyntax* throwStatement = static_cast<const ThrowStatementSyntax*>(statement);
			return ExecuteThrowStatement(throwStatement);
		}

		case SyntaxKind::BreakStatement:
		{
			const BreakStatementSyntax* throwStatement = static_cast<const BreakStatementSyntax*>(statement);
			return ExecuteBreakStatement(throwStatement);
		}

		case SyntaxKind::ContinueStatement:
		{
			const ContinueStatementSyntax* throwStatement = static_cast<const ContinueStatementSyntax*>(statement);
			return ExecuteContinueStatement(throwStatement);
		}

		case SyntaxKind::ReturnStatement:
		{
			const ReturnStatementSyntax* returnStatement = static_cast<const ReturnStatementSyntax*>(statement);
			return ExecuteReturnStatement(returnStatement);
		}

		case SyntaxKind::IfStatement:
		{
			const IfStatementSyntax* ifStatement = static_cast<const IfStatementSyntax*>(statement);
			return ExecuteIfStatement(ifStatement);
		}

		case SyntaxKind::UnlessStatement:
		{
			const UnlessStatementSyntax* unlessStatement = static_cast<const UnlessStatementSyntax*>(statement);
			return ExecuteUnlessStatement(unlessStatement);
		}
		
		case SyntaxKind::ElseStatement:
		{
			const ElseStatementSyntax* elseStatement = static_cast<const ElseStatementSyntax*>(statement);
			return ExecuteElseStatement(elseStatement);
		}

		case SyntaxKind::WhileStatement:
		{
			const WhileStatementSyntax* whileStatement = static_cast<const WhileStatementSyntax*>(statement);
			return ExecuteWhileLoopStatement(whileStatement);
		}

		case SyntaxKind::UntilStatement:
		{
			const UntilStatementSyntax* untilStatement = static_cast<const UntilStatementSyntax*>(statement);
			return ExecuteUntilLoopStatement(untilStatement);
		}

		case SyntaxKind::ForStatement:
		{
			const ForStatementSyntax* forLoopStatement = static_cast<const ForStatementSyntax*>(statement);
			return ExecuteForLoopStatement(forLoopStatement);
		}

		default:
		{
			throw std::runtime_error("unknown statement type");
		}
	}
}

ObjectInstance* AbstractInterpreter::ExecuteExpressionStatement(const ExpressionStatementSyntax* statement)
{
	ObjectInstance* exprReg = EvaluateExpression(statement->Expression);
	return exprReg;
}

ObjectInstance* AbstractInterpreter::ExecuteVariableStatement(const VariableStatementSyntax* statement)
{
	std::wstring varName = statement->IdentifierToken.Word;
	ObjectInstance* assignInstance = EvaluateExpression(statement->Expression);
	return CurrentContext()->AddVariable(varName, assignInstance);
}

ObjectInstance* AbstractInterpreter::ExecuteThrowStatement(const ThrowStatementSyntax* statement)
{
	CallStackFrame* callFrame = callStack.top();
	callFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;

	if (statement->Expression != nullptr)
		callFrame->InterruptionRegister = EvaluateExpression(statement->Expression);

	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteBreakStatement(const BreakStatementSyntax* statement)
{
	CallStackFrame* callFrame = callStack.top();
	callFrame->InterruptionReason = FrameInterruptionReason::LoopBreak;
	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteContinueStatement(const ContinueStatementSyntax* statement)
{
	CallStackFrame* callFrame = callStack.top();
	callFrame->InterruptionReason = FrameInterruptionReason::LoopContinue;
	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteReturnStatement(const ReturnStatementSyntax* statement)
{
	CallStackFrame* callFrame = callStack.top();
	callFrame->InterruptionReason = FrameInterruptionReason::ValueReturned;

	if (statement->Expression != nullptr)
		callFrame->InterruptionRegister = EvaluateExpression(statement->Expression);

	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteIfStatement(const IfStatementSyntax* statement)
{
	PushContext(new InboundVariablesContext(CurrentContext()));
	
	ObjectInstance* conditionReg = ExecuteStatement(statement->ConditionExpression);
	bool conditionMet = conditionReg->ReadPrimitive<bool>();
	GarbageCollector::DestroyInstance(conditionReg);

	if (conditionMet)
	{
		ExecuteBlock(statement->StatementsBlock);
		PopContext();
	}
	else if (statement->NextStatement != nullptr)
	{
		PopContext();
		ExecuteStatement(statement->NextStatement);
	}

	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteUnlessStatement(const UnlessStatementSyntax* statement)
{
	PushContext(new InboundVariablesContext(CurrentContext()));

	ObjectInstance* conditionReg = ExecuteStatement(statement->ConditionExpression);
	bool conditionMet = conditionReg->ReadPrimitive<bool>();
	GarbageCollector::DestroyInstance(conditionReg);

	if (!conditionMet)
	{
		ExecuteBlock(statement->StatementsBlock);
		PopContext();
	}
	else if (statement->NextStatement != nullptr)
	{
		PopContext();
		ExecuteStatement(statement->NextStatement);
	}

	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteElseStatement(const ElseStatementSyntax* statement)
{
	return ExecuteBlock(statement->StatementsBlock);
}

ObjectInstance* AbstractInterpreter::ExecuteForLoopStatement(const ForStatementSyntax* statement)
{
	PushContext(new InboundVariablesContext(CurrentContext()));
	CallStackFrame* frame = CurrentFrame();

	bool looping = true;
	ObjectInstance* initReg = ExecuteStatement(statement->InitializerStatement);

	while (looping)
	{
		ObjectInstance* conditionReg = EvaluateExpression(statement->ConditionExpression);
		bool conditionMet = conditionReg->ReadPrimitive<bool>();
		GarbageCollector::DestroyInstance(conditionReg);

		if (!conditionMet)
		{
			looping = false;
			break;
		}

		ExecuteBlock(statement->StatementsBlock);
		ExecuteStatement(statement->AfterRepeatStatement);

		switch (frame->InterruptionReason)
		{
			case FrameInterruptionReason::None:
				break;

			case FrameInterruptionReason::LoopBreak:
			{
				frame->InterruptionReason = FrameInterruptionReason::None;
				looping = false;
				break;
			}

			case FrameInterruptionReason::LoopContinue:
			{
				frame->InterruptionReason = FrameInterruptionReason::None;
				looping = true;
				break;
			}

			default:
			{
				looping = false;
				break;
			}
		}
	}

	PopContext();
	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteWhileLoopStatement(const WhileStatementSyntax* statement)
{
	PushContext(new InboundVariablesContext(CurrentContext()));
	CallStackFrame* frame = CurrentFrame();

	bool looping = true;
	while (looping)
	{
		ObjectInstance* conditionReg = EvaluateExpression(statement->ConditionExpression);
		bool conditionMet = conditionReg->ReadPrimitive<bool>();
		GarbageCollector::DestroyInstance(conditionReg);

		if (!conditionMet)
		{
			looping = false;
			break;
		}

		ExecuteBlock(statement->StatementsBlock);

		switch (frame->InterruptionReason)
		{
			case FrameInterruptionReason::None:
				break;

			case FrameInterruptionReason::LoopBreak:
			{
				frame->InterruptionReason = FrameInterruptionReason::None;
				looping = false;
				break;
			}

			case FrameInterruptionReason::LoopContinue:
			{
				frame->InterruptionReason = FrameInterruptionReason::None;
				looping = true;
				break;
			}

			default:
			{
				looping = false;
				break;
			}
		}
	}

	PopContext();
	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteUntilLoopStatement(const UntilStatementSyntax* statement)
{
	PushContext(new InboundVariablesContext(CurrentContext()));
	CallStackFrame* frame = CurrentFrame();

	bool looping = true;
	while (looping)
	{
		ObjectInstance* conditionReg = EvaluateExpression(statement->ConditionExpression);
		bool conditionMet = conditionReg->ReadPrimitive<bool>();
		GarbageCollector::DestroyInstance(conditionReg);

		if (conditionMet)
		{
			looping = false;
			break;
		}

		ExecuteBlock(statement->StatementsBlock);

		switch (frame->InterruptionReason)
		{
			case FrameInterruptionReason::None:
				break;

			case FrameInterruptionReason::LoopBreak:
			{
				frame->InterruptionReason = FrameInterruptionReason::None;
				looping = false;
				break;
			}

			case FrameInterruptionReason::LoopContinue:
			{
				frame->InterruptionReason = FrameInterruptionReason::None;
				looping = true;
				break;
			}

			default:
			{
				looping = false;
				break;
			}
		}
	}

	PopContext();
	return nullptr;
}

ObjectInstance* AbstractInterpreter::EvaluateExpression(const ExpressionSyntax* expression)
{
	switch (expression->Kind)
	{
		case SyntaxKind::LiteralExpression:
		{
			const LiteralExpressionSyntax* literalExpression = static_cast<const LiteralExpressionSyntax*>(expression);
			return EvaluateLiteralExpression(literalExpression);
		}

		case SyntaxKind::ObjectExpression:
		{
			const ObjectExpressionSyntax* objectExpression = static_cast<const ObjectExpressionSyntax*>(expression);
			return EvaluateObjectExpression(objectExpression);
		}

		case SyntaxKind::UnaryExpression:
		{
			const UnaryExpressionSyntax* unaryExpression = static_cast<const UnaryExpressionSyntax*>(expression);
			return EvaluateUnaryExpression(unaryExpression);
		}

		case SyntaxKind::BinaryExpression:
		{
			const BinaryExpressionSyntax* binaryExpr = static_cast<const BinaryExpressionSyntax*>(expression);
			return EvaluateBinaryExpression(binaryExpr);
		}

		case SyntaxKind::MemberAccessExpression:
		{
			const MemberAccessExpressionSyntax* accessExpression = static_cast<const MemberAccessExpressionSyntax*>(expression);
			return EvaluateMemberAccessExpression(accessExpression, nullptr);
		}

		case SyntaxKind::InvokationExpression:
		{
			const InvokationExpressionSyntax* invokeExpression = static_cast<const InvokationExpressionSyntax*>(expression);
			return EvaluateInvokationExpression(invokeExpression, nullptr);
		}

		case SyntaxKind::IndexatorExpression:
		{
			const IndexatorExpressionSyntax* indexExpression = static_cast<const IndexatorExpressionSyntax*>(expression);
			return EvaluateIndexatorExpression(indexExpression, nullptr);
		}

		case SyntaxKind::CollectionExpression:
		{
			const CollectionExpressionSyntax* collectionExpression = static_cast<const CollectionExpressionSyntax*>(expression);
			return EvaluateCollectionExpression(collectionExpression);
		}

		default:
		{
			throw std::runtime_error("unknown expression kind");
		}
	}
}

ObjectInstance* AbstractInterpreter::EvaluateLiteralExpression(const LiteralExpressionSyntax* expression)
{
	switch (expression->LiteralToken.Type)
	{
		case TokenType::BooleanLiteral:
			return ObjectInstance::FromValue(expression->LiteralToken.Word == L"true");

		case TokenType::NumberLiteral:
			return ObjectInstance::FromValue(stoi(expression->LiteralToken.Word));

		case TokenType::CharLiteral:
			return ObjectInstance::FromValue(expression->LiteralToken.Word[0]);

		case TokenType::StringLiteral:
			return ObjectInstance::FromValue(expression->LiteralToken.Word);

		default:
			throw std::runtime_error("Unknown constant literal type");
	}
}

ObjectInstance* AbstractInterpreter::EvaluateObjectExpression(const ObjectExpressionSyntax* expression)
{
	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(expression->Symbol);
	for (FieldSymbol* field : newInstance->Info->Fields)
	{
		ObjectInstance* assignInstance = nullptr;
		if (field->DefaultValueExpression != nullptr)
		{
			assignInstance = AbstractInterpreter::EvaluateExpression(field->DefaultValueExpression);
		}
		else
		{
			assignInstance = field->ReturnType->IsReferenceType
				? GarbageCollector::NullInstance
				: GarbageCollector::AllocateInstance(field->ReturnType);
		}

		newInstance->SetField(field, assignInstance);
	}

	return newInstance;
}

static bool IsMemberAccess(const ExpressionSyntax* expression, const MemberAccessExpressionSyntax*& memberExpression)
{
	if (expression->Kind != SyntaxKind::MemberAccessExpression)
		return false;

	memberExpression = dynamic_cast<const MemberAccessExpressionSyntax*>(expression);
	return true;
}

static bool IsFieldAccess(const MemberAccessExpressionSyntax* memberExpression, const ExpressionSyntax*& instanceExpression)
{
	instanceExpression = memberExpression->PreviousExpression;
	return memberExpression->FieldSymbol != nullptr || memberExpression->PropertySymbol != nullptr;
}

ObjectInstance* AbstractInterpreter::EvaluateBinaryExpression(const BinaryExpressionSyntax* expression)
{
	const ExpressionSyntax* instanceExpression = nullptr;
	const MemberAccessExpressionSyntax* memberExpression = nullptr;

	bool isMemberAccess = IsMemberAccess(expression->Left, memberExpression);
	bool isFieldAccess = isMemberAccess ? IsFieldAccess(memberExpression, instanceExpression) : false;
	bool assign = expression->OperatorToken.Type == TokenType::AssignOperator;

	ObjectInstance* instanceReg = CurrentContext()->TryFind(L"this");
	ObjectInstance* retReg = nullptr;

	if (!isMemberAccess)
	{
		ObjectInstance* leftReg = EvaluateExpression(expression->Left);
		ObjectInstance* rightReg = EvaluateExpression(expression->Right);
		retReg = PrimitiveMathModule::EvaluateBinaryOperator(leftReg, expression->OperatorToken, rightReg, assign);

		GarbageCollector::DestroyInstance(leftReg);
		GarbageCollector::DestroyInstance(rightReg);
	}
	else
	{
		ObjectInstance* leftReg = nullptr;
		if (isFieldAccess)
		{
			instanceReg = EvaluateExpression(instanceExpression);
			if (!assign)
				leftReg = EvaluateMemberAccessExpression(memberExpression, instanceReg);
		}
		else
		{
			instanceReg = CurrentContext()->Variables[memberExpression->IdentifierToken.Word];
			if (!assign)
				leftReg = instanceReg;
		}

		ObjectInstance* rightReg = EvaluateExpression(expression->Right);
		retReg = PrimitiveMathModule::EvaluateBinaryOperator(leftReg, expression->OperatorToken, rightReg, assign);

		GarbageCollector::DestroyInstance(leftReg);
		GarbageCollector::DestroyInstance(rightReg);
	}

	if (assign)
	{
		if (!isMemberAccess)
		{
			// error, not a member access
			throw std::runtime_error("binary expression tried to assign value to inaccesible register");
		}

		// Checking if is variable access
		if (!isFieldAccess)
		{
			GarbageCollector::CopyInstance(retReg, instanceReg);
			GarbageCollector::DestroyInstance(retReg);
			return instanceReg;
		}

		ExecuteInstanceSetter(instanceReg, memberExpression, retReg);
	}

	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateUnaryExpression(const UnaryExpressionSyntax* expression)
{
	const ExpressionSyntax* instanceExpression = nullptr;
	const MemberAccessExpressionSyntax* memberExpression = nullptr;

	bool isMemberAccess = IsMemberAccess(expression->Expression, memberExpression);
	bool isFieldAccess = isMemberAccess ? IsFieldAccess(memberExpression, instanceExpression) : false;

	ObjectInstance* instanceReg = CurrentContext()->TryFind(L"this");
	ObjectInstance* exprReg = nullptr;

	if (!isMemberAccess)
	{
		exprReg = EvaluateExpression(expression->Expression);
		return PrimitiveMathModule::EvaluateUnaryOperator(exprReg, expression->OperatorToken, expression->IsRightDetermined);
	}
	else
	{
		if (isFieldAccess)
		{
			instanceReg = EvaluateExpression(instanceExpression);
			exprReg = EvaluateMemberAccessExpression(memberExpression, instanceReg);
		}
		else
		{
			instanceReg = CurrentContext()->Variables[memberExpression->IdentifierToken.Word];
			exprReg = instanceReg;
		}
	}

	ObjectInstance* targetReg = GarbageCollector::CopyInstance(exprReg);
	ObjectInstance* retReg = PrimitiveMathModule::EvaluateUnaryOperator(targetReg, expression->OperatorToken, expression->IsRightDetermined);

	if (targetReg == exprReg)
	{
		// Not need to assign
		GarbageCollector::DestroyInstance(exprReg);
		GarbageCollector::DestroyInstance(targetReg);
		return retReg;
	}

	if (!isMemberAccess)
	{
		// error, not a member access
		//throw std::runtime_error("unary expression tried to assign value to inaccesible register");
		return retReg;
	}

	// Checking if is variable access
	if (!isFieldAccess)
	{
		GarbageCollector::CopyInstance(retReg, instanceReg);
		return instanceReg;
	}

	ExecuteInstanceSetter(instanceReg, memberExpression, exprReg);
	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateCollectionExpression(const CollectionExpressionSyntax* expression)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(expression->Symbol);
	int size = static_cast<int>(expression->ValuesExpressions.size());
	instance->WriteMemory(0, sizeof(int), &size);

	for (size_t i = 0; i < expression->ValuesExpressions.size(); i++)
	{
		ExpressionSyntax* valueExpression = expression->ValuesExpressions[i];
		ObjectInstance* valueInstance = EvaluateExpression(valueExpression);
		instance->SetElement(i, valueInstance);
	}

	return instance;
}

ObjectInstance* AbstractInterpreter::EvaluateMemberAccessExpression(const MemberAccessExpressionSyntax* expression, ObjectInstance* prevInstance)
{
	if (prevInstance == nullptr)
		prevInstance = expression->PreviousExpression == nullptr ? CurrentContext()->TryFind(L"this") : EvaluateExpression(expression->PreviousExpression);

	// Check if this is a property or field
	FieldSymbol* field = expression->FieldSymbol;
	if (expression->PropertySymbol != nullptr)
	{
		PropertySymbol* property = expression->PropertySymbol;
		field = property->BackingField;

		if (property->Getter->Method != nullptr)
		{
			InboundVariablesContext* getterArgs = new InboundVariablesContext(nullptr);
			if (!property->IsStatic)
				getterArgs->AddVariable(L"this", prevInstance);

			return ExecuteMethod(property->Getter->Method, getterArgs);
		}
	}

	if (field != nullptr)
	{
		// Check if this is a static field
		if (field->IsStatic)
		{
			return GarbageCollector::GetStaticField(field);
		}

		// Instance field access
		return prevInstance->GetField(field);
	}

	return CurrentContext()->TryFind(expression->IdentifierToken.Word);
}

ObjectInstance* AbstractInterpreter::EvaluateInvokationExpression(const InvokationExpressionSyntax* expression, ObjectInstance* prevInstance)
{
	if (prevInstance == nullptr)
		prevInstance = expression->PreviousExpression == nullptr ? CurrentContext()->TryFind(L"this") : EvaluateExpression(expression->PreviousExpression);

	MethodSymbol* method = expression->Symbol;
	InboundVariablesContext* arguments = CreateArgumentsContext(expression->ArgumentsList->Arguments, method, prevInstance);
	ObjectInstance* retReg = ExecuteMethod(method, arguments);
	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateIndexatorExpression(const IndexatorExpressionSyntax* expression, ObjectInstance* prevInstance)
{
	if (prevInstance == nullptr)
		prevInstance = expression->PreviousExpression == nullptr ? CurrentContext()->TryFind(L"this") : EvaluateExpression(expression->PreviousExpression);

	MethodSymbol* method = expression->Symbol;
	InboundVariablesContext* arguments = CreateArgumentsContext(expression->IndexatorList->Arguments, method, prevInstance);
	ObjectInstance* retReg = ExecuteMethod(method, arguments);
	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateArgument(const ArgumentSyntax* argument)
{
	ObjectInstance* argInstance = EvaluateExpression(argument->Expression);
	//bool isByReference = argument->IsByReference;
	//return isByReference ? argInstance : GarbageCollector::CopyInstance(argInstance);
	return argInstance;
}

InboundVariablesContext* AbstractInterpreter::CreateArgumentsContext(std::vector<ArgumentSyntax*> arguments, MethodSymbol* symbol, ObjectInstance* instance)
{
	InboundVariablesContext* argumentsContext = new InboundVariablesContext(nullptr);

	if (!symbol->IsStatic)
		argumentsContext->AddVariable(L"this", instance);

	size_t size = symbol->Parameters.size();
	for (size_t i = 0; i < size; i++)
	{
		ArgumentSyntax* argument = arguments.at(i);
		ObjectInstance* argInstance = EvaluateArgument(argument);

		std::wstring argName = symbol->Parameters.at(i)->Name;
		argumentsContext->AddVariable(argName, argInstance);
	}

	return argumentsContext;
}

void AbstractInterpreter::ExecuteInstanceSetter(ObjectInstance* instance, const MemberAccessExpressionSyntax* access, ObjectInstance* value)
{
	// Check if this is a property or field
	FieldSymbol* field = access->FieldSymbol;
	if (access->PropertySymbol != nullptr)
	{
		PropertySymbol* property = access->PropertySymbol;
		field = property->BackingField;

		if (property->Getter->Method != nullptr)
		{
			InboundVariablesContext* setterArgs = new InboundVariablesContext(nullptr);
			if (!property->IsStatic)
				setterArgs->AddVariable(L"this", instance);

			setterArgs->AddVariable(L"value", value);
			ExecuteMethod(property->Setter->Method, setterArgs);
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
