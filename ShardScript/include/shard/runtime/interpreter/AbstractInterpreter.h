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

namespace shard::runtime
{
	class AbstractInterpreter
	{
	private:
		std::stack<shard::runtime::CallStackFrame*> callStack;

		shard::parsing::lexical::SyntaxTree& syntaxTree;
		shard::parsing::semantic::SemanticModel& semanticModel;

	public:
		AbstractInterpreter(shard::parsing::lexical::SyntaxTree& syntaxTree, shard::parsing::semantic::SemanticModel& semanticModel)
			: syntaxTree(syntaxTree), semanticModel(semanticModel) { }

		~AbstractInterpreter() = default;

		void Execute();
		void RaiseException(ObjectInstance* exceptionReg);

		CallStackFrame* CurrentFrame();
		void PushFrame(shard::syntax::symbols::MethodSymbol* methodSymbol);
		void PopFrame();

		InboundVariablesContext* CurrentContext();
		void PushContext(InboundVariablesContext* context);
		void PopContext();

		ObjectInstance* ExecuteMethod(shard::syntax::symbols::MethodSymbol* method, InboundVariablesContext* argumentsContext);
		ObjectInstance* ExecuteBlock(const shard::syntax::nodes::StatementsBlockSyntax* block);

		ObjectInstance* ExecuteStatement(const shard::syntax::nodes::StatementSyntax* statement);
		ObjectInstance* ExecuteExpressionStatement(const shard::syntax::nodes::ExpressionStatementSyntax* statement);
		ObjectInstance* ExecuteVariableStatement(const shard::syntax::nodes::VariableStatementSyntax* statement);
		ObjectInstance* ExecuteReturnStatement(const shard::syntax::nodes::ReturnStatementSyntax* statement);
		ObjectInstance* ExecuteThrowStatement(const shard::syntax::nodes::ThrowStatementSyntax* statement);
		ObjectInstance* ExecuteBreakStatement(const shard::syntax::nodes::BreakStatementSyntax* statement);
		ObjectInstance* ExecuteContinueStatement(const shard::syntax::nodes::ContinueStatementSyntax* statement);

		ObjectInstance* ExecuteIfStatement(const shard::syntax::nodes::IfStatementSyntax* statement);
		ObjectInstance* ExecuteUnlessStatement(const shard::syntax::nodes::UnlessStatementSyntax* statement);
		ObjectInstance* ExecuteElseStatement(const shard::syntax::nodes::ElseStatementSyntax* statement);

		ObjectInstance* ExecuteForLoopStatement(const shard::syntax::nodes::ForStatementSyntax* statement);
		ObjectInstance* ExecuteWhileLoopStatement(const shard::syntax::nodes::WhileStatementSyntax* statement);
		ObjectInstance* ExecuteUntilLoopStatement(const shard::syntax::nodes::UntilStatementSyntax* statement);

		ObjectInstance* EvaluateExpression(const shard::syntax::nodes::ExpressionSyntax* expression);
		ObjectInstance* EvaluateLiteralExpression(const shard::syntax::nodes::LiteralExpressionSyntax* expression);
		ObjectInstance* EvaluateObjectExpression(const shard::syntax::nodes::ObjectExpressionSyntax* expression);
		ObjectInstance* EvaluateBinaryExpression(const shard::syntax::nodes::BinaryExpressionSyntax* expression);
		ObjectInstance* EvaluateUnaryExpression(const shard::syntax::nodes::UnaryExpressionSyntax* expression);
		//ObjectInstance* EvaluateAssignExpression(const shard::syntax::nodes::BinaryExpressionSyntax* expression);

		ObjectInstance* EvaluateLinkedExpression(const shard::syntax::nodes::LinkedExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance, bool trimLast);
		ObjectInstance* EvaluateLinkedExpression(const shard::syntax::nodes::LinkedExpressionNode* expression, shard::runtime::ObjectInstance* prevInstance);
		ObjectInstance* EvaluateMemberAccessExpression(const shard::syntax::nodes::MemberAccessExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance);
		ObjectInstance* EvaluateInvokationExpression(const shard::syntax::nodes::InvokationExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance);
		ObjectInstance* EvaluateIndexatorExpression(const shard::syntax::nodes::IndexatorExpressionSyntax* expression, shard::runtime::ObjectInstance* prevInstance);
		
		ObjectInstance* EvaluateArgument(const shard::syntax::nodes::ArgumentSyntax* argument);

	private:
		InboundVariablesContext* CreateArgumentsContext(const shard::syntax::nodes::InvokationExpressionSyntax* expression, shard::syntax::symbols::MethodSymbol* symbol, shard::runtime::ObjectInstance* instance);
	};
}
