#include <shard/runtime/AbstarctInterpreter.h>
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/Register.h>
#include <shard/runtime/TypeInfo.h>

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

#define GetStatement(type) std::static_pointer_cast<type>(statement);

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax::nodes;

shared_ptr<Register> CreateRegisterFromConstToken(SyntaxToken& constToken);
shared_ptr<Register> EvaluateBinaryExpressionValues(shared_ptr<Register> left, SyntaxToken& op, shared_ptr<Register> right);

void AbstarctInterpreter::Execute()
{
	shared_ptr<MethodDeclarationSyntax> entryPoint = Tree->EntryPoint;
	ExecuteMethod(entryPoint, nullptr);
}

shared_ptr<Register> AbstarctInterpreter::ExecuteMethod(shared_ptr<MethodDeclarationSyntax> method, shared_ptr<CallStackFrame> prevFrame)
{
	shared_ptr<CallStackFrame> frame = make_shared<CallStackFrame>(prevFrame, method);
	CallStack.push(frame);
	
	shared_ptr<Register> retReg = ExecuteBlock(method->Body, frame);
	CallStack.pop();
	
	return retReg;
}

shared_ptr<Register> AbstarctInterpreter::ExecuteBlock(shared_ptr<StatementsBlockSyntax> block, shared_ptr<CallStackFrame> frame)
{
	for (const shared_ptr<StatementSyntax>& statement : block->Statements)
	{
		shared_ptr<Register> retReg = ExecuteStatement(statement, frame);
		if (retReg != retReg)
			return retReg;
	}

	return nullptr;
}

shared_ptr<Register> AbstarctInterpreter::ExecuteStatement(shared_ptr<StatementSyntax> statement, shared_ptr<CallStackFrame> frame)
{
	switch (statement->Kind)
	{
		case SyntaxKind::ExpressionStatement:
		{
			auto exprStatement = dynamic_pointer_cast<ExpressionStatementSyntax>(statement);
			shared_ptr<Register> exprReg = EvaluateExpression(exprStatement->Expression, frame);
			return nullptr; //exprReg;
		}

		case SyntaxKind::ForStatement:
		{
			auto forStatement = dynamic_pointer_cast<ForStatementSyntax>(statement);
			shared_ptr<Register> initReg = ExecuteStatement(forStatement->InitializerStatement, frame);
			
			while (true)
			{
				shared_ptr<Register> loopAgainReg = EvaluateExpression(forStatement->ConditionExpression, frame);
				bool loopAgain = *static_pointer_cast<bool>(loopAgainReg->DataPtr);

				if (!loopAgain)
					break;

				ExecuteBlock(forStatement->Block, frame);
				ExecuteStatement(forStatement->AfterRepeatStatement, frame);
			}

			return nullptr;
		}

		case SyntaxKind::VariableStatement:
		{
			auto varStatement = dynamic_pointer_cast<VariableStatementSyntax>(statement);
			string varName = varStatement->Name.Word;
			
			if (frame->VariablesHeap.find(varName) != frame->VariablesHeap.end())
				throw runtime_error("variable already created");

			shared_ptr<Register> assignExprReg = EvaluateExpression(varStatement->Expression, frame);
			frame->VariablesHeap[varName] = assignExprReg;
			return nullptr;
		}

		default:
		{
			throw runtime_error("unknown statement type");
		}
	}
}

shared_ptr<Register> AbstarctInterpreter::EvaluateExpression(shared_ptr<ExpressionSyntax> expression, shared_ptr<CallStackFrame> frame)
{
	switch (expression->Kind)
	{
		case SyntaxKind::ConstExpression:
		{
			auto constExpr = dynamic_pointer_cast<ConstValueExpressionSyntax>(expression);
			SyntaxToken constToken = constExpr->Constant;
			return CreateRegisterFromConstToken(constToken);
		}

		case SyntaxKind::BinaryExpression:
		{
			auto binaryExpr = dynamic_pointer_cast<BinaryExpressionSyntax>(expression);
			shared_ptr<Register> leftReg = EvaluateExpression(binaryExpr->Left, frame);
			shared_ptr<Register> rightReg = EvaluateExpression(binaryExpr->Right, frame);
			return EvaluateBinaryExpressionValues(leftReg, binaryExpr->OperatorToken, rightReg);
		}

		case SyntaxKind::InvokationExpression:
		case SyntaxKind::IndexatorExpression:
		case SyntaxKind::FieldAccessExpression:
		{
			auto accessExpr = dynamic_pointer_cast<MemberAccessExpressionSyntax>(expression);
			return EvaluateMemberAccesExpression(accessExpr, frame);
		}

		default:
		{
			throw runtime_error("unknown expression type");
		}
	}
}

shared_ptr<Register> AbstarctInterpreter::EvaluateMemberAccesExpression(shared_ptr<MemberAccessExpressionSyntax> expression, shared_ptr<CallStackFrame> frame)
{
	switch (expression->Kind)
	{
		case SyntaxKind::FieldAccessExpression:
		{
			string varName = expression->IdentifierToken.Word;
			return frame->VariablesHeap[varName];
		}

		case SyntaxKind::InvokationExpression:
		{
			auto invokeExpr = dynamic_pointer_cast<InvokationExpressionSyntax>(expression);
			if (expression->NextAccess != nullptr)
				throw runtime_error("Recursive member acces paths are currently unsupported!");

			if (expression->IdentifierToken.Word == "print" && invokeExpr->ArgumentsList->Arguments.size() == 1)
				return EvaluatePrintInvokationExpression(invokeExpr->ArgumentsList->Arguments[0], frame);

			throw runtime_error("unknown invokation member");
		}

		case SyntaxKind::IndexatorExpression:
		{
			if (expression->NextAccess != nullptr)
				throw runtime_error("Recursive member acces paths are currently unsupported!");
		
			throw runtime_error("Indexation expression are currently unupported!");
		}

		default:
		{
			throw runtime_error("Unknown member access");
		}
	}

	return nullptr;
}

shared_ptr<Register> AbstarctInterpreter::EvaluatePrintInvokationExpression(shared_ptr<ArgumentSyntax> argument, shared_ptr<CallStackFrame> frame)
{
	shared_ptr<Register> exprReg = EvaluateExpression(argument->Expression, frame);
	switch (exprReg->Type.Id)
	{
		case TYPE_CODE_BOOLEAN:
		{
			bool data = *static_pointer_cast<bool>(exprReg->DataPtr);
			cout << data << endl;
			break;
		}

		case TYPE_CODE_INTEGER:
		{
			int data = *static_pointer_cast<int>(exprReg->DataPtr);
			cout << data << endl;
			break;
		}

		case TYPE_CODE_STRING:
		{
			string data = *static_pointer_cast<string, void>(exprReg->DataPtr);
			cout << data << endl;
			break;
		}
	}

	return nullptr;
}

static shared_ptr<Register> CreateRegisterFromConstToken(SyntaxToken& constToken)
{
	switch (constToken.Type)
	{
		case TokenType::BooleanLiteral:
			return make_shared<Register>(TYPEINFO_BOOLEAN, make_shared<bool>(constToken.Word == "true"));

		case TokenType::NumberLiteral:
			return make_shared<Register>(TYPEINFO_INTEGER, make_shared<int>(stoi(constToken.Word)));

		case TokenType::StringLiteral:
			return make_shared<Register>(TYPEINFO_STRING, make_shared<string>(constToken.Word));

		default:
			throw runtime_error("Unknown constant literal type");
	}
}

static shared_ptr<Register> EvaluateBinaryExpressionValues(shared_ptr<Register> leftReg, SyntaxToken& op, shared_ptr<Register> rightReg)
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
			return make_shared<Register>(TYPEINFO_INTEGER, make_shared<int>(left + right));
		}

		case TokenType::SubOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_INTEGER, make_shared<int>(left - right));
		}

		case TokenType::MultOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_INTEGER, make_shared<int>(left * right));
		}

		case TokenType::DivOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_INTEGER, make_shared<int>(left / right));
		}

		case TokenType::PowOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_INTEGER, make_shared<int>(pow(left, right)));
		}

		case TokenType::LessOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_BOOLEAN, make_shared<bool>(left < right));
		}

		case TokenType::LessOrEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_BOOLEAN, make_shared<bool>(left <= right));
		}

		case TokenType::GreaterOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_BOOLEAN, make_shared<bool>(left > right));
		}

		case TokenType::GreaterOrEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_BOOLEAN, make_shared<bool>(left <= right));
		}

		case TokenType::EqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_BOOLEAN, make_shared<bool>(left == right));
		}

		case TokenType::NotEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPEINFO_BOOLEAN, make_shared<bool>(left != right));
		}

		default:
		{
			throw runtime_error("Unknown binary operator");
		}
	}
}