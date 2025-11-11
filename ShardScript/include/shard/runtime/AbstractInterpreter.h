#pragma once
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/parsing/lexical/SyntaxTree.h>
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

namespace shard::runtime
{
	class AbstractInterpreter
	{
	private:
		static std::stack<CallStackFrame*> callStack;
	
	public:
		static CallStackFrame* CurrentFrame();
		static void PushFrame(shard::syntax::symbols::MethodSymbol* methodSymbol);
		static void PopFrame();

		static InboundVariablesContext* CurrentContext();
		static void PushContext(InboundVariablesContext* context);
		static void PopContext();

		static void TerminateCallStack();
		static void Execute(shard::parsing::lexical::SyntaxTree& syntaxTree, shard::parsing::semantic::SemanticModel& semanticModel);
		static void RaiseException(ObjectInstance* exceptionReg);

		static ObjectInstance* ExecuteMethod(shard::syntax::symbols::MethodSymbol* method, InboundVariablesContext* argumentsContext);
		static ObjectInstance* ExecuteBlock(const shard::syntax::nodes::StatementsBlockSyntax* block);

		static ObjectInstance* ExecuteStatement(const shard::syntax::nodes::StatementSyntax* statement);
		static ObjectInstance* ExecuteExpressionStatement(const shard::syntax::nodes::ExpressionStatementSyntax* statement);
		static ObjectInstance* ExecuteVariableStatement(const shard::syntax::nodes::VariableStatementSyntax* statement);
		static ObjectInstance* ExecuteReturnStatement(const shard::syntax::nodes::ReturnStatementSyntax* statement);
		static ObjectInstance* ExecuteThrowStatement(const shard::syntax::nodes::ThrowStatementSyntax* statement);
		static ObjectInstance* ExecuteBreakStatement(const shard::syntax::nodes::BreakStatementSyntax* statement);
		static ObjectInstance* ExecuteContinueStatement(const shard::syntax::nodes::ContinueStatementSyntax* statement);

		static ObjectInstance* ExecuteIfStatement(const shard::syntax::nodes::IfStatementSyntax* statement);
		static ObjectInstance* ExecuteUnlessStatement(const shard::syntax::nodes::UnlessStatementSyntax* statement);
		static ObjectInstance* ExecuteElseStatement(const shard::syntax::nodes::ElseStatementSyntax* statement);

		static ObjectInstance* ExecuteForLoopStatement(const shard::syntax::nodes::ForStatementSyntax* statement);
		static ObjectInstance* ExecuteWhileLoopStatement(const shard::syntax::nodes::WhileStatementSyntax* statement);
		static ObjectInstance* ExecuteUntilLoopStatement(const shard::syntax::nodes::UntilStatementSyntax* statement);

		static ObjectInstance* EvaluateExpression(const shard::syntax::nodes::ExpressionSyntax* expression);
		static ObjectInstance* EvaluateLiteralExpression(const shard::syntax::nodes::LiteralExpressionSyntax* expression);
		static ObjectInstance* EvaluateObjectExpression(const shard::syntax::nodes::ObjectExpressionSyntax* expression);
		static ObjectInstance* EvaluateBinaryExpression(const shard::syntax::nodes::BinaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateUnaryExpression(const shard::syntax::nodes::UnaryExpressionSyntax* expression);
		static ObjectInstance* EvaluateCollectionExpression(const shard::syntax::nodes::CollectionExpressionSyntax* expression);
		//static ObjectInstance* EvaluateAssignExpression(const shard::syntax::nodes::BinaryExpressionSyntax* expression);

		static ObjectInstance* EvaluateLinkedExpression(const shard::syntax::nodes::LinkedExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance, bool trimLast);
		static ObjectInstance* EvaluateLinkedExpression(const shard::syntax::nodes::LinkedExpressionNode* expression, shard::runtime::ObjectInstance* prevInstance);
		static ObjectInstance* EvaluateMemberAccessExpression(const shard::syntax::nodes::MemberAccessExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance);
		static ObjectInstance* EvaluateInvokationExpression(const shard::syntax::nodes::InvokationExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance);
		static ObjectInstance* EvaluateIndexatorExpression(const shard::syntax::nodes::IndexatorExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance);
		
	private:
		static ObjectInstance* EvaluateArgument(const shard::syntax::nodes::ArgumentSyntax* argument);
		static InboundVariablesContext* CreateArgumentsContext(std::vector<shard::syntax::nodes::ArgumentSyntax*> expression, shard::syntax::symbols::MethodSymbol* symbol, shard::runtime::ObjectInstance* instance);
		static void ExecuteInstanceSetter(ObjectInstance* instance, const shard::syntax::nodes::MemberAccessExpressionSyntax* access, ObjectInstance* value);
	};
}
