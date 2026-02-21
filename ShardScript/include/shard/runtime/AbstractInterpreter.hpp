// TODO: rewrite
/*
#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ArgumentsSpan.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>

#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>

#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>

#include <stack>
#include <vector>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>

namespace shard
{
	class SHARD_API AbstractInterpreter
	{
	private:
		static std::stack<CallStackFrame*> callStack;
	
	public:
		static CallStackFrame* CurrentFrame();
		static void PushFrame(const shard::MethodSymbol* methodSymbol, const shard::TypeSymbol* withinType);
		static void PopFrame();

		static ArgumentsSpan& CreateArgumentsSpan(std::vector<shard::ArgumentSyntax*> arguments, std::vector<shard::ParameterSymbol*> parameters, bool isStatic, shard::ObjectInstance* instance);
		static ArgumentsSpan& CurrentContext();
		static void PushContext(ArgumentsSpan& context);
		static void PopContext();

		static void TerminateCallStack();
		static void Execute(shard::SyntaxTree& syntaxTree, shard::SemanticModel& semanticModel);
		static void RaiseException(ObjectInstance* exceptionReg);

		static ObjectInstance* ExecuteMethod(const shard::MethodSymbol* method, const shard::TypeSymbol* withinType, ArgumentsSpan& argumentsContext);
		static ObjectInstance* ExecuteBlock(const shard::StatementsBlockSyntax* block);

		static ObjectInstance* ExecuteStatement(const shard::StatementSyntax* statement);
		static ObjectInstance* ExecuteExpressionStatement(const shard::ExpressionStatementSyntax* statement);
		static ObjectInstance* ExecuteVariableStatement(const shard::VariableStatementSyntax* statement);
		static ObjectInstance* ExecuteReturnStatement(const shard::ReturnStatementSyntax* statement);
		static ObjectInstance* ExecuteThrowStatement(const shard::ThrowStatementSyntax* statement);
		static ObjectInstance* ExecuteBreakStatement(const shard::BreakStatementSyntax* statement);
		static ObjectInstance* ExecuteContinueStatement(const shard::ContinueStatementSyntax* statement);

		static ObjectInstance* ExecuteIfStatement(const shard::IfStatementSyntax* statement);
		static ObjectInstance* ExecuteUnlessStatement(const shard::UnlessStatementSyntax* statement);
		static ObjectInstance* ExecuteElseStatement(const shard::ElseStatementSyntax* statement);

		static ObjectInstance* ExecuteForLoopStatement(const shard::ForStatementSyntax* statement);
		static ObjectInstance* ExecuteWhileLoopStatement(const shard::WhileStatementSyntax* statement);
		static ObjectInstance* ExecuteUntilLoopStatement(const shard::UntilStatementSyntax* statement);

		static ObjectInstance* EvaluateExpression(const shard::ExpressionSyntax* expression);
		static ObjectInstance* EvaluateLiteralExpression(const shard::LiteralExpressionSyntax* expression);
		static ObjectInstance* EvaluateObjectExpression(const shard::ObjectExpressionSyntax* expression);
		static ObjectInstance* EvaluateBinaryExpression(const shard::BinaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateAssignExpression(const shard::BinaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateUnaryExpression(const shard::UnaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateCollectionExpression(const shard::CollectionExpressionSyntax* expression);
		static ObjectInstance* EvaluateLambdaExpression(const shard::LambdaExpressionSyntax* expression);
		static ObjectInstance* EvaluateTernaryExpression(const shard::TernaryExpressionSyntax* expression);
		//static ExpressionSyntax* ChooseTernaryExpression(const shard::TernaryExpressionSyntax* expression);

		static ObjectInstance* EvaluateMemberAccessExpression(const shard::MemberAccessExpressionSyntax* expression, shard::ObjectInstance* prevInstance);
		static ObjectInstance* EvaluateInvokationExpression(const shard::InvokationExpressionSyntax* expression, shard::ObjectInstance* prevInstance);
		static ObjectInstance* EvaluateIndexatorExpression(const shard::IndexatorExpressionSyntax* expression, shard::ObjectInstance* prevInstance);
		
	private:
		static ObjectInstance* EvaluateArgument(const shard::ArgumentSyntax* argument);
		static void ExecuteInstanceSetter(ObjectInstance* instance, const shard::MemberAccessExpressionSyntax* access, ObjectInstance* value);
	};
}
*/