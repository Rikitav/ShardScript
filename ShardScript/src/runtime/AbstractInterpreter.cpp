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
#include <shard/syntax/SyntaxFacts.h>

#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>

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
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>

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
using namespace shard::parsing::analysis;
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
		PopFrame();
		frame = CurrentFrame();
		frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
		frame->InterruptionRegister = exceptionReg;
		RaiseException(exceptionReg);
		return;
	}

	// null previous frame => frame is main entry point
	TypeSymbol* exceptionType = const_cast<TypeSymbol*>(exceptionReg->Info);
	if (exceptionType == SymbolTable::Primitives::String)
	{
		std::wstring exceptionString = exceptionReg->ReadPrimitive<std::wstring>();
		ConsoleHelper::WriteLine(exceptionString);
		GarbageCollector::CollectInstance(exceptionReg);
		return;
	}

	std::wstring toStringMethodName = L"ToString";
	MethodSymbol* toStringMethod = exceptionType->FindMethod(toStringMethodName, std::vector<TypeSymbol*>());

	ObjectInstance* toStringMethodRet = ExecuteMethod(toStringMethod, nullptr);
	if (toStringMethodRet->Info != SymbolTable::Primitives::String)
	{
		ConsoleHelper::WriteLine(L"Critical error, could not invoke to string method, to get exception description. Exception type - " + exceptionType->Name);
		GarbageCollector::CollectInstance(exceptionReg);
		return;
	}

	std::wstring exceptionString = toStringMethodRet->ReadPrimitive<std::wstring>();
	ConsoleHelper::WriteLine(exceptionString);
	GarbageCollector::CollectInstance(toStringMethodRet);
	GarbageCollector::CollectInstance(exceptionReg);
	return;
}

ObjectInstance* AbstractInterpreter::ExecuteMethod(MethodSymbol* method, InboundVariablesContext* argumentsContext)
{
	PushFrame(method);
	PushContext(argumentsContext);
	CallStackFrame* frame = CurrentFrame();

	switch (method->HandleType)
	{
		case MethodHandleType::Lambda:
		case MethodHandleType::Body:
		{
			ExecuteBlock(method->Body);
			break;
		}

		case MethodHandleType::External:
		{
			try
			{
				ObjectInstance* retReg = method->FunctionPointer(method, argumentsContext);
				if (retReg != nullptr)
				{
					frame->InterruptionReason = FrameInterruptionReason::ValueReturned;
					frame->InterruptionRegister = retReg;
					frame->InterruptionRegister->IncrementReference();
				}
			}
			catch (const std::runtime_error& err)
			{
				std::string description = err.what();
				std::wstring wdescription = std::wstring(description.begin(), description.end());

				frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				frame->InterruptionRegister = ObjectInstance::FromValue(wdescription);
				frame->InterruptionRegister->IncrementReference();
			}

			break;
		}

		/*
		case MethodHandleType::ForeignInterface:
		{
			throw std::runtime_error("FFI not implemented");
			break;
		}
		*/
	}

	PopContext();
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
			retReg->DecrementReference();
			break;
		}
	}

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

	if (exprReg != nullptr)
		GarbageCollector::CollectInstance(exprReg);
	
	return exprReg;
}

ObjectInstance* AbstractInterpreter::ExecuteVariableStatement(const VariableStatementSyntax* statement)
{
	std::wstring varName = statement->IdentifierToken.Word;
	ObjectInstance* assignInstance = EvaluateExpression(statement->Expression);
	CurrentContext()->AddVariable(varName, assignInstance);

	GarbageCollector::CollectInstance(assignInstance);
	return assignInstance;
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
	{
		callFrame->InterruptionRegister = EvaluateExpression(statement->Expression);
		callFrame->InterruptionRegister->IncrementReference();
	}

	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteIfStatement(const IfStatementSyntax* statement)
{
	PushContext(new InboundVariablesContext(CurrentContext()));
	
	ObjectInstance* conditionReg = ExecuteStatement(statement->ConditionExpression);
	bool conditionMet = conditionReg->ReadPrimitive<bool>();
	GarbageCollector::CollectInstance(conditionReg);

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
	GarbageCollector::CollectInstance(conditionReg);

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

	ObjectInstance* initReg = ExecuteStatement(statement->InitializerStatement);

	bool looping = true;
	while (looping)
	{
		ObjectInstance* conditionReg = EvaluateExpression(statement->ConditionExpression);
		looping = conditionReg->ReadPrimitive<bool>();
		GarbageCollector::CollectInstance(conditionReg);

		if (!looping)
			break;

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
		looping = conditionReg->ReadPrimitive<bool>();
		GarbageCollector::CollectInstance(conditionReg);

		if (!looping)
			break;

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
		looping = conditionReg->ReadPrimitive<bool>();
		GarbageCollector::CollectInstance(conditionReg);

		if (looping)
			break;

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

		case SyntaxKind::LambdaExpression:
		{
			const LambdaExpressionSyntax* lambdaExpression = static_cast<const LambdaExpressionSyntax*>(expression);
			return EvaluateLambdaExpression(lambdaExpression);
		}

		case SyntaxKind::TernaryExpression:
		{
			const TernaryExpressionSyntax* ternaryExpression = static_cast<const TernaryExpressionSyntax*>(expression);
			return EvaluateTernaryExpression(ternaryExpression);
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
	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(expression->TypeSymbol);

	TypeSymbol* info = expression->TypeSymbol;
	GenericTypeSymbol* genericInfo = nullptr;

	if (info->Kind == SyntaxKind::GenericType)
	{
		genericInfo = static_cast<GenericTypeSymbol*>(info);
		info = genericInfo->UnderlayingType;
	}

	AbstractInterpreter::PushFrame(expression->CtorSymbol);
	for (FieldSymbol* field : info->Fields)
	{
		ObjectInstance* assignInstance = nullptr;
		TypeSymbol* fieldType = field->ReturnType;

		if (fieldType->Kind == SyntaxKind::TypeParameter)
			fieldType = genericInfo->SubstituteTypeParameters(fieldType);

		if (field->DefaultValueExpression != nullptr)
		{
			assignInstance = AbstractInterpreter::EvaluateExpression(field->DefaultValueExpression);
		}
		else
		{
			assignInstance = fieldType->IsReferenceType
				? GarbageCollector::NullInstance
				: GarbageCollector::AllocateInstance(fieldType);
		}

		newInstance->SetField(field, assignInstance);
	}

	if (expression->CtorSymbol != nullptr)
	{
		InboundVariablesContext* arguments = CreateArgumentsContext(expression->ArgumentsList->Arguments, expression->CtorSymbol->Parameters, expression->CtorSymbol->IsStatic, newInstance);
		ExecuteMethod(expression->CtorSymbol, arguments);
	}

	AbstractInterpreter::PopFrame();
	return newInstance;
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
	return memberExpression->FieldSymbol != nullptr || memberExpression->PropertySymbol != nullptr;
}

ObjectInstance* AbstractInterpreter::EvaluateBinaryExpression(const BinaryExpressionSyntax* expression)
{
	if (expression->OperatorToken.Type == TokenType::AssignOperator)
		return EvaluateAssignExpression(expression);
	
	const ExpressionSyntax* instanceExpression = nullptr;
	const MemberAccessExpressionSyntax* memberExpression = nullptr;

	bool isMemberAccess = IsMemberAccess(expression->Left, memberExpression);
	bool isFieldAccess = isMemberAccess ? IsFieldAccess(memberExpression, instanceExpression) : false;
	bool assign = false;

	ObjectInstance* instanceReg = isFieldAccess ? EvaluateExpression(instanceExpression) : nullptr;
	ObjectInstance* leftReg =
		isFieldAccess ? EvaluateMemberAccessExpression(memberExpression, instanceReg)
		: isMemberAccess ? CurrentContext()->Find(memberExpression->IdentifierToken.Word)
		: EvaluateExpression(expression->Left);

	ObjectInstance* rightReg = EvaluateExpression(expression->Right);
	ObjectInstance* retReg = PrimitiveMathModule::EvaluateBinaryOperator(leftReg, expression->OperatorToken, rightReg, assign);

	GarbageCollector::CollectInstance(leftReg);
	GarbageCollector::CollectInstance(rightReg);

	if (assign)
	{
		// Checking if is field access
		if (isFieldAccess)
		{
			ExecuteInstanceSetter(instanceReg, memberExpression, retReg);
			GarbageCollector::CollectInstance(instanceReg);
			return retReg;
		}

		// Checking if is variable access
		if (isMemberAccess)
		{
			std::wstring varName = memberExpression->IdentifierToken.Word;
			return CurrentContext()->SetVariable(varName, retReg);
		}

		// error, not a member access
		throw std::runtime_error("binary expression tried to assign value to inaccesible register");
	}

	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateAssignExpression(const BinaryExpressionSyntax* expression)
{
	const MemberAccessExpressionSyntax* memberExpression = static_cast<const MemberAccessExpressionSyntax*>(expression->Left);
	const ExpressionSyntax* instanceExpression = nullptr;
	bool isFieldAccess = IsFieldAccess(memberExpression, instanceExpression);

	if (isFieldAccess)
	{
		ObjectInstance* instanceReg = EvaluateExpression(instanceExpression);
		ObjectInstance* rightReg = EvaluateExpression(expression->Right);
		ExecuteInstanceSetter(instanceReg, memberExpression, rightReg);

		GarbageCollector::CollectInstance(instanceReg);
		return rightReg;
	}
	else
	{
		InboundVariablesContext* current = CurrentContext();
		std::wstring varName = memberExpression->IdentifierToken.Word;

		ObjectInstance* instanceReg = current->TryFind(varName);
		ObjectInstance* rightReg = EvaluateExpression(expression->Right);

		current->SetVariable(varName, rightReg);
		return rightReg;
	}
}

ObjectInstance* AbstractInterpreter::EvaluateUnaryExpression(const UnaryExpressionSyntax* expression)
{
	const ExpressionSyntax* instanceExpression = nullptr;
	const MemberAccessExpressionSyntax* memberExpression = nullptr;

	bool isMemberAccess = IsMemberAccess(expression->Expression, memberExpression);
	bool isFieldAccess = isMemberAccess ? IsFieldAccess(memberExpression, instanceExpression) : false;

	ObjectInstance* instanceReg = isFieldAccess ? EvaluateExpression(instanceExpression) : nullptr;
	ObjectInstance* exprReg =
		isFieldAccess ? EvaluateMemberAccessExpression(memberExpression, instanceReg)
		: isMemberAccess ? CurrentContext()->Find(memberExpression->IdentifierToken.Word)
		: EvaluateExpression(expression->Expression);

	ObjectInstance* retReg = PrimitiveMathModule::EvaluateUnaryOperator(exprReg, expression->OperatorToken, expression->IsRightDetermined);
	GarbageCollector::CollectInstance(exprReg);
	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateCollectionExpression(const CollectionExpressionSyntax* expression)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(expression->Symbol);
	size_t size = expression->ValuesExpressions.size();

	// setting 'Lenght' property value
	int length = static_cast<int>(size);
	instance->WriteMemory(0, sizeof(int), &length);

	for (size_t i = 0; i < size; i++)
	{
		ExpressionSyntax* valueExpression = expression->ValuesExpressions[i];
		ObjectInstance* valueInstance = EvaluateExpression(valueExpression);
		instance->SetElement(i, valueInstance);
		GarbageCollector::CollectInstance(valueInstance);
	}

	return instance;
}

ObjectInstance* AbstractInterpreter::EvaluateLambdaExpression(const LambdaExpressionSyntax* expression)
{
	ObjectInstance* retReg = GarbageCollector::AllocateInstance(expression->Symbol);
	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateTernaryExpression(const TernaryExpressionSyntax* expression)
{
	PushContext(new InboundVariablesContext(CurrentContext()));

	ObjectInstance* conditionReg = EvaluateExpression(expression->Condition);
	bool conditionMet = conditionReg->ReadPrimitive<bool>();
	GarbageCollector::CollectInstance(conditionReg);

	ExpressionSyntax* retExpr = conditionMet ? expression->Left : expression->Right;
	ObjectInstance* retReg = EvaluateExpression(retExpr);
	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateMemberAccessExpression(const MemberAccessExpressionSyntax* expression, ObjectInstance* prevInstance)
{
	// Check if this is a delegate
	if (expression->DelegateSymbol != nullptr)
	{
		ObjectInstance* instance = GarbageCollector::AllocateInstance(expression->DelegateSymbol);
		return instance;
	}

	if (!expression->IsStaticContext && prevInstance == nullptr)
	{
		if (expression->PreviousExpression == nullptr)
		{
			ObjectInstance* varReg = CurrentContext()->Find(expression->IdentifierToken.Word);
			return varReg;
		}

		prevInstance = EvaluateExpression(expression->PreviousExpression);
	}

	// Check if this is a property or field
	FieldSymbol* field = expression->FieldSymbol;
	if (expression->PropertySymbol != nullptr)
	{
		PropertySymbol* property = expression->PropertySymbol;
		field = property->BackingField;

		if (property->Getter != nullptr)
		{
			InboundVariablesContext* getterArgs = new InboundVariablesContext(nullptr);
			if (!property->IsStatic)
				getterArgs->AddVariable(L"this", prevInstance);

			ObjectInstance* retReg = ExecuteMethod(property->Getter, getterArgs);
			return retReg;
		}
	}

	if (field != nullptr)
	{
		// Check if this is a static field
		if (field->IsStatic)
		{
			ObjectInstance* retReg = GarbageCollector::GetStaticField(field);
			return retReg;
		}

		// Instance field access
		ObjectInstance* retReg = prevInstance->GetField(field);
		return retReg;
	}

	throw std::runtime_error("Failed to evaluate member access expression");
	return nullptr;
}

ObjectInstance* AbstractInterpreter::EvaluateInvokationExpression(const InvokationExpressionSyntax* expression, ObjectInstance* prevInstance)
{
	MethodSymbol* method = expression->Symbol;
	if (method->HandleType == MethodHandleType::Lambda)
	{
		ObjectInstance* delegateInstance = CurrentContext()->Find(expression->IdentifierToken.Word);
		const DelegateTypeSymbol* delegateType = static_cast<const DelegateTypeSymbol*>(delegateInstance->Info);
		method = delegateType->AnonymousSymbol;
	}

	if (!method->IsStatic && prevInstance == nullptr)
	{
		if (expression->PreviousExpression == nullptr)
		{
			prevInstance = CurrentContext()->Find(L"this");
		}
		else
		{
			prevInstance = EvaluateExpression(expression->PreviousExpression);
		}
	}

	InboundVariablesContext* arguments = CreateArgumentsContext(expression->ArgumentsList->Arguments, method->Parameters, method->IsStatic, prevInstance);
	ObjectInstance* retReg = ExecuteMethod(method, arguments);
	return retReg;
}

ObjectInstance* AbstractInterpreter::EvaluateIndexatorExpression(const IndexatorExpressionSyntax* expression, ObjectInstance* prevInstance)
{
	if (prevInstance == nullptr)
		prevInstance = EvaluateExpression(expression->PreviousExpression);

	IndexatorSymbol* indexator = expression->IndexatorSymbol;
	if (indexator == nullptr || indexator->Getter == nullptr)
		throw std::runtime_error("indexator getter is not resolved");

	MethodSymbol* method = indexator->Getter;
	InboundVariablesContext* arguments = CreateArgumentsContext(expression->IndexatorList->Arguments, method->Parameters, method->IsStatic, prevInstance);
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

InboundVariablesContext* AbstractInterpreter::CreateArgumentsContext(std::vector<ArgumentSyntax*> arguments, std::vector<ParameterSymbol*> parameters, bool isStatic, ObjectInstance* instance)
{
	InboundVariablesContext* argumentsContext = new InboundVariablesContext(nullptr);

	if (!isStatic)
		argumentsContext->AddVariable(L"this", instance);

	size_t size = parameters.size();
	for (size_t i = 0; i < size; i++)
	{
		ArgumentSyntax* argument = arguments.at(i);
		ObjectInstance* argInstance = EvaluateExpression(argument->Expression);

		std::wstring argName = parameters.at(i)->Name;
		argumentsContext->AddVariable(argName, argInstance);
		GarbageCollector::CollectInstance(argInstance);
	}

	return argumentsContext;
}

void AbstractInterpreter::ExecuteInstanceSetter(ObjectInstance* instance, const MemberAccessExpressionSyntax* access, ObjectInstance* value)
{
	if (access->Kind == SyntaxKind::IndexatorExpression)
	{
		const IndexatorExpressionSyntax* indexator = static_cast<const IndexatorExpressionSyntax*>(access);
		InboundVariablesContext* arguments = CreateArgumentsContext(indexator->IndexatorList->Arguments, indexator->IndexatorSymbol->Parameters, indexator->IndexatorSymbol->IsStatic, instance);
		arguments->AddVariable(L"value", value);
		ExecuteMethod(indexator->IndexatorSymbol->Setter, arguments);
		return;
	}

	// Check if this is a property or field
	FieldSymbol* field = access->FieldSymbol;
	if (access->PropertySymbol != nullptr)
	{
		PropertySymbol* property = access->PropertySymbol;
		field = property->BackingField;

		if (property->Setter != nullptr)
		{
			InboundVariablesContext* setterArgs = new InboundVariablesContext(nullptr);
			if (!property->IsStatic)
				setterArgs->AddVariable(L"this", instance);

			if (access->Kind == SyntaxKind::IndexatorExpression)
			{

			}

			setterArgs->AddVariable(L"value", value);
			ExecuteMethod(property->Setter, setterArgs);
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
