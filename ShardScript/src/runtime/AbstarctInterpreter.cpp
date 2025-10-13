#include <shard/runtime/AbstarctInterpreter.h>
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/Register.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/Expressions.h>
#include <shard/syntax/nodes/Statements.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

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

	for (const shared_ptr<StatementSyntax>& statement : method->Body->Statements)
		ExecuteStatement(statement, frame);

	CallStack.pop();
	return nullptr;
}

shared_ptr<Register> AbstarctInterpreter::ExecuteStatement(shared_ptr<StatementSyntax> statement, shared_ptr<CallStackFrame> frame)
{
	if (auto exprStatement = dynamic_pointer_cast<ExpressionStatementSyntax>(statement))
	{
		shared_ptr<Register> exprReg = EvaluateExpression(exprStatement->Expression, frame);
		return exprReg;
	}
	else if (auto varStatement = dynamic_pointer_cast<VariableStatementSyntax>(statement))
	{
		string varName = varStatement->Name.Word;
		if (frame->VariablesHeap.find(varName) != frame->VariablesHeap.end())
			throw runtime_error("variable already created");

		shared_ptr<Register> assignExprReg = EvaluateExpression(varStatement->Expression, frame);
		return frame->VariablesHeap[varName] = assignExprReg;
	}
	else
	{
		throw runtime_error("unknown statement type");
	}
}

shared_ptr<Register> AbstarctInterpreter::EvaluateExpression(shared_ptr<ExpressionSyntax> expression, shared_ptr<CallStackFrame> frame)
{
	if (auto constExpr = dynamic_pointer_cast<ConstValueExpressionSyntax>(expression))
	{
		SyntaxToken constToken = constExpr->Constant;
		return CreateRegisterFromConstToken(constToken);
	}
	else if (auto binaryExpr = dynamic_pointer_cast<BinaryExpressionSyntax>(expression))
	{
		shared_ptr<Register> leftReg = EvaluateExpression(binaryExpr->Left, frame);
		shared_ptr<Register> rightReg = EvaluateExpression(binaryExpr->Right, frame);
		return EvaluateBinaryExpressionValues(leftReg, binaryExpr->OperatorToken, rightReg);
	}
	else if (auto accessExpr = dynamic_pointer_cast<MemberAccessExpressionSyntax>(expression))
	{
		if (accessExpr->Path.size() == 1)
		{
			string varName = accessExpr->Path[0].Word;
			return frame->VariablesHeap[varName];
		}

		throw runtime_error("Unknown member access");
	}
	else if (auto invokeExpr = dynamic_pointer_cast<InvokationExpressionSyntax>(expression))
	{
		string accessName = invokeExpr->MemberAccess->Path[0].Word;
		if (accessName == "print")
		{
			vector<shared_ptr<ArgumentSyntax>> vArgs = invokeExpr->ArgumentsList->Arguments;
			if (vArgs.size() == 1)
			{
				shared_ptr<Register> exprReg = EvaluateExpression(vArgs[0]->Expression, frame);
				switch (exprReg->TypeCode)
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

				return exprReg;
			}
		}

		throw runtime_error("unknown invokation member");
	}
	else
	{
		throw runtime_error("unknown expression type");
	}
}

static shared_ptr<Register> CreateRegisterFromConstToken(SyntaxToken& constToken)
{
	switch (constToken.Type)
	{
		case TokenType::BooleanLiteral:
			return make_shared<Register>(TYPE_CODE_BOOLEAN, make_shared<bool>(constToken.Word == "true"));

		case TokenType::NumberLiteral:
			return make_shared<Register>(TYPE_CODE_INTEGER, make_shared<int>(stoi(constToken.Word)));

		case TokenType::StringLiteral:
			return make_shared<Register>(TYPE_CODE_STRING, make_shared<string>(constToken.Word));

		default:
			throw runtime_error("Unknown constant literal type");
	}
}

static shared_ptr<Register> EvaluateBinaryExpressionValues(shared_ptr<Register> leftReg, SyntaxToken& op, shared_ptr<Register> rightReg)
{
	if (leftReg->TypeCode != rightReg->TypeCode)
		throw runtime_error("cannot evaluate binary expression with different type codes");

	if (leftReg->TypeCode != TYPE_CODE_INTEGER)
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
			return make_shared<Register>(TYPE_CODE_INTEGER, make_shared<int>(left + right));
		}

		case TokenType::SubOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_INTEGER, make_shared<int>(left - right));
		}

		case TokenType::MultOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_INTEGER, make_shared<int>(left * right));
		}

		case TokenType::DivOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_INTEGER, make_shared<int>(left / right));
		}

		case TokenType::PowOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_INTEGER, make_shared<int>(pow(left, right)));
		}

		case TokenType::LessOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_BOOLEAN, make_shared<bool>(left < right));
		}

		case TokenType::LessOrEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_BOOLEAN, make_shared<bool>(left <= right));
		}

		case TokenType::GreaterOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_BOOLEAN, make_shared<bool>(left > right));
		}

		case TokenType::GreaterOrEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_BOOLEAN, make_shared<bool>(left <= right));
		}

		case TokenType::EqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_BOOLEAN, make_shared<bool>(left == right));
		}

		case TokenType::NotEqualsOperator:
		{
			int left = *static_pointer_cast<int>(leftReg->DataPtr);
			int right = *static_pointer_cast<int>(rightReg->DataPtr);
			return make_shared<Register>(TYPE_CODE_BOOLEAN, make_shared<bool>(left != right));
		}

		default:
		{
			throw runtime_error("Unknown binary operator");
		}
	}
}