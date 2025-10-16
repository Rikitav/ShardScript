#include <shard/runtime/AbstarctInterpreter.h>
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/VariableRegister.h>
#include <shard/runtime/TypeInfo.h>
#include <shard/runtime/InboundVariablesContext.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/Expressions.h>
#include <shard/syntax/nodes/Statements.h>
#include <shard/syntax/nodes/Loops.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <shard/syntax/nodes/Desides.h>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax::nodes;

shared_ptr<VariableRegister> CreateRegisterFromConstToken(SyntaxToken& constToken);
shared_ptr<VariableRegister> EvaluateBinaryExpressionValues(shared_ptr<VariableRegister> left, SyntaxToken& op, shared_ptr<VariableRegister> right);
shared_ptr<VariableRegister> EvaluateUnaryExpressionValues(SyntaxToken& op, shared_ptr<VariableRegister> exprReg, bool isRightDetermined);

void AbstarctInterpreter::Execute()
{
	shared_ptr<MethodDeclarationSyntax> entryPoint = Tree->EntryPoint;
	ExecuteMethod(entryPoint, nullptr);
}

shared_ptr<VariableRegister> AbstarctInterpreter::ExecuteMethod(shared_ptr<MethodDeclarationSyntax> method, shared_ptr<CallStackFrame> prevFrame)
{
	shared_ptr<CallStackFrame> frame = make_shared<CallStackFrame>(prevFrame, method);
	CallStack.push(frame);
	
	shared_ptr<VariableRegister> retReg = ExecuteBlock(method->Body, frame->Context, frame);
	CallStack.pop();
	
	return retReg;
}

shared_ptr<VariableRegister> AbstarctInterpreter::ExecuteBlock(shared_ptr<StatementsBlockSyntax> block, shared_ptr<InboundVariablesContext> prevContext, shared_ptr<CallStackFrame> frame)
{
	shared_ptr<InboundVariablesContext> context = make_shared<InboundVariablesContext>();
	context->Previous = prevContext;

	for (const shared_ptr<StatementSyntax>& statement : block->Statements)
	{
		shared_ptr<VariableRegister> retReg = ExecuteStatement(statement, context, frame);
		if (frame->IsInterrupted)
			return retReg;
	}

	return nullptr;
}

shared_ptr<VariableRegister> AbstarctInterpreter::ExecuteStatement(shared_ptr<StatementSyntax> statement, shared_ptr<InboundVariablesContext> context, shared_ptr<CallStackFrame> frame)
{
	switch (statement->Kind)
	{
		case SyntaxKind::ExpressionStatement:
		{
			auto exprStatement = dynamic_pointer_cast<ExpressionStatementSyntax>(statement);
			shared_ptr<VariableRegister> exprReg = EvaluateExpression(exprStatement->Expression, context);
			return exprReg;
		}

		case SyntaxKind::IfStatement:
		{
			auto ifStatement = dynamic_pointer_cast<IfStatementSyntax>(statement);
			shared_ptr<VariableRegister> conditionReg = ExecuteStatement(ifStatement->Condition, context, frame);
			if (conditionReg->Type.Id != TYPE_CODE_BOOLEAN)
				throw runtime_error("condition must return boolean");

			bool conditionMet = *static_pointer_cast<bool>(conditionReg->DataPtr);
			if (conditionMet)
			{
				return ExecuteBlock(ifStatement->Block, context, frame);
			}
			else
			{
				if (ifStatement->NextStatement == nullptr)
					return nullptr;

				return ExecuteStatement(ifStatement->NextStatement, context, frame);
			}
		}

		case SyntaxKind::UnlessStatement:
		{
			auto unlessStatement = dynamic_pointer_cast<UnlessStatementSyntax>(statement);
			shared_ptr<VariableRegister> conditionReg = ExecuteStatement(unlessStatement->Condition, context, frame);
			if (conditionReg->Type.Id != TYPE_CODE_BOOLEAN)
				throw runtime_error("condition must return boolean");

			bool conditionMet = *static_pointer_cast<bool>(conditionReg->DataPtr);
			if (!conditionMet)
			{
				return ExecuteBlock(unlessStatement->Block, context, frame);
			}
			else
			{
				if (unlessStatement->NextStatement == nullptr)
					return nullptr;

				return ExecuteStatement(unlessStatement->NextStatement, context, frame);
			}
		}
		
		case SyntaxKind::ElseStatement:
		{
			auto elseStatement = dynamic_pointer_cast<ElseSatetmentSyntax>(statement);
			return ExecuteBlock(elseStatement->Block, context, frame);
		}

		case SyntaxKind::ForStatement:
		{
			auto forStatement = dynamic_pointer_cast<ForStatementSyntax>(statement);

			shared_ptr<InboundVariablesContext> loopContext = make_shared<InboundVariablesContext>();
			loopContext->Previous = context;
			shared_ptr<VariableRegister> initReg = ExecuteStatement(forStatement->InitializerStatement, loopContext, frame);
			shared_ptr<VariableRegister> retReg = nullptr;

			while (true)
			{
				shared_ptr<VariableRegister> loopAgainReg = EvaluateExpression(forStatement->ConditionExpression, loopContext);
				bool loopAgain = *static_pointer_cast<bool>(loopAgainReg->DataPtr);

				if (!loopAgain)
					break;

				retReg = ExecuteBlock(forStatement->Block, loopContext, frame);
				ExecuteStatement(forStatement->AfterRepeatStatement, loopContext, frame);
			}

			return retReg;
		}

		case SyntaxKind::VariableStatement:
		{
			auto varStatement = dynamic_pointer_cast<VariableStatementSyntax>(statement);
			string varName = varStatement->Name.Word;
			
			if (context->TryFind(varName))
				throw runtime_error("variable already created");

			shared_ptr<VariableRegister> assignExprReg = EvaluateExpression(varStatement->Expression, context);
			return context->Heap[varName] = assignExprReg;
		}

		case SyntaxKind::ReturnStatement:
		{
			auto retStatement = dynamic_pointer_cast<ReturnStatementSyntax>(statement);
			shared_ptr<VariableRegister> retReg = retStatement->Expression == nullptr ? nullptr : EvaluateExpression(retStatement->Expression, context);
			frame->IsInterrupted = true;
			return retReg;
		}

		default:
		{
			throw runtime_error("unknown statement type");
		}
	}
}

shared_ptr<VariableRegister> AbstarctInterpreter::EvaluateExpression(shared_ptr<ExpressionSyntax> expression, shared_ptr<InboundVariablesContext> context)
{
	switch (expression->Kind)
	{
		case SyntaxKind::ConstExpression:
		{
			auto constExpr = dynamic_pointer_cast<ConstValueExpressionSyntax>(expression);
			SyntaxToken constToken = constExpr->Constant;
			return CreateRegisterFromConstToken(constToken);
		}

		case SyntaxKind::UnaryExpression:
		{
			auto unaryExpr = dynamic_pointer_cast<UnaryExpressionSyntax>(expression);
			shared_ptr<VariableRegister> exprReg = EvaluateExpression(unaryExpr->Expression, context);
			return EvaluateUnaryExpressionValues(unaryExpr->OperatorToken, exprReg, unaryExpr->IsRightDetermined);
		}

		case SyntaxKind::BinaryExpression:
		{
			auto binaryExpr = dynamic_pointer_cast<BinaryExpressionSyntax>(expression);
			shared_ptr<VariableRegister> leftReg = EvaluateExpression(binaryExpr->Left, context);
			shared_ptr<VariableRegister> rightReg = EvaluateExpression(binaryExpr->Right, context);
			return EvaluateBinaryExpressionValues(leftReg, binaryExpr->OperatorToken, rightReg);
		}

		case SyntaxKind::InvokationExpression:
		case SyntaxKind::IndexatorExpression:
		case SyntaxKind::FieldAccessExpression:
		{
			auto accessExpr = dynamic_pointer_cast<MemberAccessExpressionSyntax>(expression);
			return EvaluateMemberAccessChain(accessExpr, context, nullptr);
		}

		default:
		{
			throw runtime_error("unknown expression type");
		}
	}
}

shared_ptr<VariableRegister> AbstarctInterpreter::EvaluateMemberAccessChain(shared_ptr<MemberAccessExpressionSyntax> expression, shared_ptr<InboundVariablesContext> context, shared_ptr<VariableRegister> prevRegister)
{
	shared_ptr<VariableRegister> retRegister = EvaluateMemberAccessExpression(expression, context, prevRegister);
	if (expression->NextAccess != nullptr)
		return EvaluateMemberAccessChain(expression->NextAccess, context, retRegister);

	return retRegister;
}

shared_ptr<VariableRegister> AbstarctInterpreter::EvaluateMemberAccessExpression(shared_ptr<MemberAccessExpressionSyntax> expression, shared_ptr<InboundVariablesContext> context, shared_ptr<VariableRegister> prevRegister)
{
	switch (expression->Kind)
	{
		case SyntaxKind::FieldAccessExpression:
		{
			if (prevRegister == nullptr)
			{
				string varName = expression->IdentifierToken.Word;
				return context->TryFind(varName);
			}

			throw runtime_error("field access currently unsupported");
		}

		case SyntaxKind::InvokationExpression:
		{
			auto invokeExpr = dynamic_pointer_cast<InvokationExpressionSyntax>(expression);

			string methodName = expression->IdentifierToken.Word;
			vector<shared_ptr<ArgumentSyntax>> arguments = invokeExpr->ArgumentsList->Arguments;

			if (methodName == "print" && arguments.size() == 1)
				return EvaluatePrintInvokationExpression(arguments[0], context, false);

			if (methodName == "println" && arguments.size() == 1)
				return EvaluatePrintInvokationExpression(arguments[0], context, true);

			throw runtime_error("unknown invokation member");
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

	return nullptr;
}

shared_ptr<VariableRegister> AbstarctInterpreter::EvaluatePrintInvokationExpression(shared_ptr<ArgumentSyntax> argument, shared_ptr<InboundVariablesContext> context, bool line)
{
	shared_ptr<VariableRegister> exprReg = EvaluateExpression(argument->Expression, context);
	PrintRegister(exprReg);

	if (line)
		cout << endl;

	return nullptr;
}

void AbstarctInterpreter::PrintRegister(shared_ptr<VariableRegister> pRegister)
{
	switch (pRegister->Type.Id)
	{
		case TYPE_CODE_BOOLEAN:
		{
			bool data = *static_pointer_cast<bool>(pRegister->DataPtr);
			cout << data;
			break;
		}

		case TYPE_CODE_INTEGER:
		{
			int data = *static_pointer_cast<int>(pRegister->DataPtr);
			cout << data;
			break;
		}

		case TYPE_CODE_STRING:
		{
			string data = *static_pointer_cast<string, void>(pRegister->DataPtr);
			cout << data;
			break;
		}
	}
}

static shared_ptr<VariableRegister> CreateRegisterFromConstToken(SyntaxToken& constToken)
{
	switch (constToken.Type)
	{
		case TokenType::BooleanLiteral:
			return make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<bool>(constToken.Word == "true"));

		case TokenType::NumberLiteral:
			return make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(stoi(constToken.Word)));

		case TokenType::StringLiteral:
			return make_shared<VariableRegister>(TYPEINFO_STRING, make_shared<string>(constToken.Word));

		default:
			throw runtime_error("Unknown constant literal type");
	}
}

static shared_ptr<VariableRegister> EvaluateUnaryExpressionValues(SyntaxToken& op, shared_ptr<VariableRegister> exprReg, bool isRightDetermined)
{
	switch (op.Type)
	{
		case TokenType::IncrementOperator:
		{
			if (exprReg->Type.Id != TYPE_CODE_INTEGER)
				throw runtime_error("unsupported type");

			int value = *static_pointer_cast<int>(exprReg->DataPtr);
			shared_ptr<VariableRegister> newValueReg = make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(value + 1));

			shared_ptr<VariableRegister> retReg = isRightDetermined ? exprReg : newValueReg;
			*exprReg = *newValueReg;
			return retReg;
		}

		case TokenType::DecrementOperator:
		{
			if (exprReg->Type.Id != TYPE_CODE_INTEGER)
				throw runtime_error("unsupported type");

			int value = *static_pointer_cast<int>(exprReg->DataPtr);
			shared_ptr<VariableRegister> newValueReg = make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(value - 1));

			shared_ptr<VariableRegister> retReg = isRightDetermined ? exprReg : newValueReg;
			*exprReg = *newValueReg;
			return retReg;
		}

		case TokenType::SubOperator:
		{
			if (exprReg->Type.Id != TYPE_CODE_INTEGER)
				throw runtime_error("unsupported type");

			int value = *static_pointer_cast<int>(exprReg->DataPtr);
			shared_ptr<VariableRegister> retReg = make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<int>(value * -1));
			return retReg;
		}

		case TokenType::NotOperator:
		{
			if (exprReg->Type.Id != TYPE_CODE_BOOLEAN)
				throw runtime_error("unsupported type");

			bool value = *static_pointer_cast<bool>(exprReg->DataPtr);
			shared_ptr<VariableRegister> retReg = make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<int>(!value));
			return retReg;
		}
	}
}

static shared_ptr<VariableRegister> EvaluateBinaryExpressionValues(shared_ptr<VariableRegister> leftReg, SyntaxToken& op, shared_ptr<VariableRegister> rightReg)
{
	if (leftReg->Type.Id != rightReg->Type.Id)
		throw runtime_error("cannot evaluate binary expression with different type codes");

	if (leftReg->Type.Id != TYPE_CODE_INTEGER)
		throw runtime_error("unsupported type");
	
	switch (op.Type)
	{
		case TokenType::AssignOperator:
		{
			*leftReg = *rightReg;
			return leftReg;
		}

		case TokenType::AddOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(left + right));
		}

		case TokenType::SubOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(left - right));
		}

		case TokenType::MultOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(left * right));
		}

		case TokenType::DivOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(left / right));
		}

		case TokenType::PowOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_INTEGER, make_shared<int>(pow(left, right)));
		}

		case TokenType::LessOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<bool>(left < right));
		}

		case TokenType::LessOrEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<bool>(left <= right));
		}

		case TokenType::GreaterOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<bool>(left > right));
		}

		case TokenType::GreaterOrEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<bool>(left <= right));
		}

		case TokenType::EqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<bool>(left == right));
		}

		case TokenType::NotEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<VariableRegister>(TYPEINFO_BOOLEAN, make_shared<bool>(left != right));
		}

		default:
		{
			throw runtime_error("Unknown binary operator");
		}
	}
}