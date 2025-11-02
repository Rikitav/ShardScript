#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ConsoleHelper.h>

#include <shard/runtime/interpreter/PrimitiveMathModule.h>
#include <shard/runtime/interpreter/AbstractInterpreter.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
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

#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

using namespace shard::syntax::symbols;
using namespace std;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::parsing;
using namespace shard::parsing::semantic;

static ObjectInstance* CreateRegisterFromConstToken(SyntaxToken& constToken)
{
	switch (constToken.Type)
	{
		case TokenType::BooleanLiteral:
		{
			ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Boolean);
			instance->WritePrimitive(constToken.Word == L"true");
			return instance;
		}

		case TokenType::NumberLiteral:
		{
			ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
			instance->WritePrimitive(stoi(constToken.Word));
			return instance;
		}

		case TokenType::CharLiteral:
		{
			ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Char);
			instance->WritePrimitive(constToken.Word[0]);
			return instance;
		}

		case TokenType::StringLiteral:
		{
			ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String);
			instance->WritePrimitive(constToken.Word);
			return instance;
		}

		default:
			throw runtime_error("Unknown constant literal type");
	}
}

void AbstractInterpreter::Execute()
{
	MethodSymbol* entryPoint = semanticModel.Table->EntryPointCandidates.at(0);
	ExecuteMethod(nullptr, entryPoint, nullptr);
}

ObjectInstance* AbstractInterpreter::ExecuteMethod(CallStackFrame* prevCallFrame, MethodSymbol* method, InboundVariablesContext* argumentsContext)
{
	CallStackFrame* callFrame = new CallStackFrame(nullptr, method, prevCallFrame, argumentsContext);
	callStack.push(callFrame);
    ExecuteBlock(callFrame, method->Body, argumentsContext);
	
	ObjectInstance* retReg = callFrame->InterruptionRegister;
	callStack.pop();
	delete callFrame;
	
	return retReg;
}

ObjectInstance* AbstractInterpreter::ExecuteBlock(CallStackFrame* callFrame, StatementsBlockSyntax* block, InboundVariablesContext* variablesContext)
{
	InboundVariablesContext* currentVariablesContext = new InboundVariablesContext(variablesContext);
	ObjectInstance* retReg = nullptr;

	for (StatementSyntax* statement : block->Statements)
	{
		retReg = ExecuteStatement(callFrame, statement, currentVariablesContext);
		if (callFrame->InterruptionReason != FrameInterruptionReason::None)
			break;
	}

	delete currentVariablesContext;
	return nullptr;
}

ObjectInstance* AbstractInterpreter::ExecuteStatement(CallStackFrame* callFrame, StatementSyntax* statement, InboundVariablesContext* variablesContext)
{
	switch (statement->Kind)
	{
		case SyntaxKind::ExpressionStatement:
		{
			auto exprStatement = dynamic_cast<ExpressionStatementSyntax*>(statement);
			ObjectInstance* exprReg = EvaluateExpression(callFrame, exprStatement->Expression, variablesContext);
			return exprReg;
		}

		case SyntaxKind::IfStatement:
		{
			auto ifStatement = dynamic_cast<IfStatementSyntax*>(statement);
			InboundVariablesContext* clauseVariablesContext = new InboundVariablesContext(variablesContext);

			ObjectInstance* conditionReg = ExecuteStatement(callFrame, ifStatement->ConditionExpression, clauseVariablesContext);
			bool conditionMet = conditionReg->ReadPrimitive<bool>();
			GarbageCollector::DestroyInstance(conditionReg);

			if (conditionMet)
				return ExecuteBlock(callFrame, ifStatement->StatementsBlock, clauseVariablesContext);

			delete clauseVariablesContext;
			if (ifStatement->NextStatement == nullptr)
				return nullptr;

			return ExecuteStatement(callFrame, ifStatement->NextStatement, variablesContext);
		}

		case SyntaxKind::UnlessStatement:
		{
			auto unlessStatement = dynamic_cast<UnlessStatementSyntax*>(statement);
			InboundVariablesContext* clauseVariablesContext = new InboundVariablesContext(variablesContext);

			ObjectInstance* conditionReg = ExecuteStatement(callFrame, unlessStatement->ConditionExpression, clauseVariablesContext);
			bool conditionMet = conditionReg->ReadPrimitive<bool>();
			GarbageCollector::DestroyInstance(conditionReg);

			if (!conditionMet)
				return ExecuteBlock(callFrame, unlessStatement->StatementsBlock, clauseVariablesContext);

			delete clauseVariablesContext;
			if (unlessStatement->NextStatement == nullptr)
				return nullptr;

			return ExecuteStatement(callFrame, unlessStatement->NextStatement, variablesContext);
		}
		
		case SyntaxKind::ElseStatement:
		{
			auto elseStatement = dynamic_cast<ElseSatetmentSyntax*>(statement);
			return ExecuteBlock(callFrame, elseStatement->StatementsBlock, variablesContext);
		}

		case SyntaxKind::WhileStatement:
		{
			auto whileStatement = dynamic_cast<WhileStatementSyntax*>(statement);
			InboundVariablesContext* loopVariablesContext = new InboundVariablesContext(callFrame->VariablesContext);

			while (callFrame->InterruptionReason == FrameInterruptionReason::None)
			{
				ObjectInstance* conditionReg = EvaluateExpression(callFrame, whileStatement->ConditionExpression, variablesContext);
				bool conditionMet = conditionReg->ReadPrimitive<bool>();
				GarbageCollector::DestroyInstance(conditionReg);

				if (!conditionMet)
					break;

				ExecuteBlock(callFrame, whileStatement->StatementsBlock, variablesContext);
			}

			delete loopVariablesContext;
			return nullptr;
		}

		case SyntaxKind::UntilStatement:
		{
			auto untilStatement = dynamic_cast<UntilStatementSyntax*>(statement);
			InboundVariablesContext* loopVariablesContext = new InboundVariablesContext(callFrame->VariablesContext);

			while (callFrame->InterruptionReason == FrameInterruptionReason::None)
			{
				ObjectInstance* conditionReg = EvaluateExpression(callFrame, untilStatement->ConditionExpression, variablesContext);
				bool conditionMet = conditionReg->ReadPrimitive<bool>();
				GarbageCollector::DestroyInstance(conditionReg);

				if (!conditionMet)
					break;

				ExecuteBlock(callFrame, untilStatement->StatementsBlock, variablesContext);
			}

			delete loopVariablesContext;
			return nullptr;
		}

		case SyntaxKind::ForStatement:
		{
			auto forStatement = dynamic_cast<ForStatementSyntax*>(statement);
			InboundVariablesContext* loopVariablesContext = new InboundVariablesContext(callFrame->VariablesContext);
			ObjectInstance* initReg = ExecuteStatement(callFrame, forStatement->InitializerStatement, loopVariablesContext);

			while (callFrame->InterruptionReason == FrameInterruptionReason::None)
			{
				ObjectInstance* conditionReg = EvaluateExpression(callFrame, forStatement->ConditionExpression, loopVariablesContext);
				bool conditionMet = conditionReg->ReadPrimitive<bool>();
				GarbageCollector::DestroyInstance(conditionReg);

				if (!conditionMet)
					break;

				ExecuteBlock(callFrame, forStatement->StatementsBlock, loopVariablesContext);
				ExecuteStatement(callFrame, forStatement->AfterRepeatStatement, loopVariablesContext);
			}

			delete loopVariablesContext;
			return nullptr;
		}

		case SyntaxKind::VariableStatement:
		{
			auto varStatement = dynamic_cast<VariableStatementSyntax*>(statement);
			wstring varName = varStatement->IdentifierToken.Word;
			ObjectInstance* assignInstance = EvaluateExpression(callFrame, varStatement->Expression, variablesContext);
			return variablesContext->AddVariable(varName, assignInstance);
		}

		case SyntaxKind::ReturnStatement:
		{
			auto retStatement = dynamic_cast<ReturnStatementSyntax*>(statement);
			callFrame->InterruptionReason = FrameInterruptionReason::ValueReturned;
			
			if (retStatement->Expression != nullptr)
				callFrame->InterruptionRegister = EvaluateExpression(callFrame, retStatement->Expression, variablesContext);

			return nullptr;
		}

		default:
		{
			throw runtime_error("unknown statement type");
		}
	}
}

ObjectInstance* AbstractInterpreter::EvaluateExpression(CallStackFrame* callFrame, ExpressionSyntax* expression, InboundVariablesContext* variablesContext)
{
	switch (expression->Kind)
	{
		case SyntaxKind::LiteralExpression:
		{
			auto constExpr = dynamic_cast<LiteralExpressionSyntax*>(expression);
			SyntaxToken constToken = constExpr->LiteralToken;
			return CreateRegisterFromConstToken(constToken);
		}

		case SyntaxKind::ObjectExpression:
		{
			auto objExpr = dynamic_cast<ObjectExpressionSyntax*>(expression);
			ObjectInstance* newInstance = GarbageCollector::AllocateInstance(objExpr->Symbol);
			return newInstance;
		}

		case SyntaxKind::UnaryExpression:
		{
			auto unaryExpr = dynamic_cast<UnaryExpressionSyntax*>(expression);
			ObjectInstance* exprReg = EvaluateExpression(callFrame, (ExpressionSyntax*)unaryExpr->Expression, variablesContext);
			return PrimitiveMathModule::EvaluateUnaryOperator(exprReg, unaryExpr->OperatorToken, unaryExpr->IsRightDetermined);
		}

		case SyntaxKind::BinaryExpression:
		{
			auto binaryExpr = dynamic_cast<BinaryExpressionSyntax*>(expression);
			ObjectInstance* leftReg = EvaluateExpression(callFrame, (ExpressionSyntax*)binaryExpr->Left, variablesContext);
			ObjectInstance* rightReg = EvaluateExpression(callFrame, (ExpressionSyntax*)binaryExpr->Right, variablesContext);

			switch (binaryExpr->OperatorToken.Type)
			{
				case TokenType::AssignOperator:
				{
					ObjectInstance* valueSrc = rightReg->Copy();
					valueSrc->CopyTo(leftReg);
					return leftReg;
				}

				default:
				{
					bool assign = false;
					ObjectInstance* retReg = PrimitiveMathModule::EvaluateBinaryOperator(leftReg, binaryExpr->OperatorToken, rightReg, assign);

					if (assign)
					{
						ObjectInstance* valueSrc = rightReg->Copy();
						valueSrc->CopyTo(leftReg);
					}

					return retReg;
				}
			}
		}

		case SyntaxKind::LinkedExpression:
		{
			auto linkedExpr = dynamic_cast<LinkedExpressionSyntax*>(expression);
			return EvaluateLinkedExpression(callFrame, linkedExpr, variablesContext);
		}

		default:
		{
			throw runtime_error("unknown expression type");
		}
	}
}

ObjectInstance* AbstractInterpreter::EvaluateLinkedExpression(CallStackFrame* callFrame, LinkedExpressionSyntax* expression, InboundVariablesContext* variablesContext)
{
	LinkedExpressionNode* exprNode = expression->First;
	switch (exprNode->Kind)
	{
		case SyntaxKind::MemberAccessExpression:
		{
			auto varAccess = dynamic_cast<MemberAccessExpressionSyntax*>(exprNode);
			ObjectInstance* objInstance = variablesContext->TryFind(varAccess->IdentifierToken.Word);

			if (objInstance != nullptr)
			{
				for (size_t i = 1; i < expression->Nodes.size(); i++)
				{
					exprNode = expression->Nodes.at(i);
					objInstance = EvaluateLinkedExpression(callFrame, exprNode, variablesContext, objInstance);
				}

				return objInstance;
			}

			break;
		}

		case SyntaxKind::InvokationExpression:
		{
			auto invokeExpr = dynamic_cast<InvokationExpressionSyntax*>(exprNode);
			wstring methodName = invokeExpr->IdentifierToken.Word;
			vector<ArgumentSyntax*> arguments = invokeExpr->ArgumentsList->Arguments;

			if (methodName == L"gc_info" && arguments.size() == 0)
			{
				wcout << "Garbage collector info dump" << endl;
				for (ObjectInstance* reg : GarbageCollector::Heap)
				{
					wcout
						<< L" * " << reg->Ptr
						<< L" : " << reg->Info->Name
						<< L" : " << reg->GetReferencesCount() << endl;
				}

				return nullptr;
			}

			if (methodName == L"print" && arguments.size() == 1)
			{
				ObjectInstance* instance = EvaluateExpression(callFrame, (ExpressionSyntax*)arguments[0]->Expression, variablesContext);
				ConsoleHelper::Write(instance);
				return nullptr;
			}

			if (methodName == L"println" && arguments.size() == 1)
			{
				ObjectInstance* instance = EvaluateExpression(callFrame, (ExpressionSyntax*)arguments[0]->Expression, variablesContext);
				ConsoleHelper::WriteLine(instance);
				return nullptr;
			}

			//throw runtime_error("unknown invokation member");
		}
	}

	ObjectInstance* objInstance = nullptr;
	while (exprNode != nullptr)
	{
		objInstance = EvaluateLinkedExpression(callFrame, exprNode, variablesContext, objInstance);
		exprNode = exprNode->NextNode;
	}

	return objInstance;
}

ObjectInstance* AbstractInterpreter::EvaluateArgument(CallStackFrame* callFrame, ArgumentSyntax* argument, InboundVariablesContext* variablesContext)
{
	ExpressionSyntax* argExpression = (ExpressionSyntax*)argument->Expression;
	ObjectInstance* argInstance = EvaluateExpression(callFrame, argExpression, variablesContext);

	bool isByReference = argument->IsByReference;
	return isByReference ? argInstance : argInstance->Copy();
}

ObjectInstance* AbstractInterpreter::EvaluateLinkedExpression(CallStackFrame* callFrame, LinkedExpressionNode* expression, InboundVariablesContext* variablesContext, ObjectInstance* objInstance)
{
	switch (expression->Kind)
	{
		case SyntaxKind::MemberAccessExpression:
		{
			auto accessExpr = dynamic_cast<MemberAccessExpressionSyntax*>(expression);
			wstring memberName = accessExpr->IdentifierToken.Word;

			TypeSymbol* type = (TypeSymbol*)objInstance->Info;
			FieldSymbol* field = type->FindField(memberName);
			return objInstance->GetField(field);
		}

		case SyntaxKind::InvokationExpression:
		{
			auto invokeExpr = dynamic_cast<InvokationExpressionSyntax*>(expression);
			wstring methodName = invokeExpr->IdentifierToken.Word;
			MethodSymbol* method = objInstance->Info->Methods.at(invokeExpr->FoundIndex);

			InboundVariablesContext* arguments = new InboundVariablesContext(variablesContext);
			arguments->AddVariable(L"this", objInstance->Copy());

			size_t size = method->Parameters.size();
			for (size_t i = 0; i < size; i++)
			{
				ArgumentSyntax* argument = invokeExpr->ArgumentsList->Arguments.at(i);
				ObjectInstance* argInstance = EvaluateArgument(callFrame, argument, variablesContext);
				
				wstring argName = method->Parameters.at(i)->Name;
				arguments->AddVariable(argName, argInstance);
			}
			
			return ExecuteMethod(callFrame, method, arguments);
		}

		case SyntaxKind::IndexatorExpression:
		{
			throw runtime_error("Indexation expression are currently unupported!");
		}

		default:
		{
			throw runtime_error("Unknown member access");
		}
	}
}
