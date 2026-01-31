#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/parsing/SyntaxTree.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>

#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>

#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>

#include <stack>
#include <vector>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>

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

		static InboundVariablesContext* CreateArgumentsContext(std::vector<shard::ArgumentSyntax*> arguments, std::vector<shard::ParameterSymbol*> parameters, bool isStatic, shard::ObjectInstance* instance);
		static InboundVariablesContext* CurrentContext();
		static void PushContext(InboundVariablesContext* context);
		static void PopContext();

		static void TerminateCallStack();
		static void Execute(shard::SyntaxTree& syntaxTree, shard::SemanticModel& semanticModel);
		static void RaiseException(ObjectInstance* exceptionReg);

		static ObjectInstance* ExecuteMethod(const shard::MethodSymbol* method, const shard::TypeSymbol* withinType, InboundVariablesContext* argumentsContext);
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
