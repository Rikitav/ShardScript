#pragma once
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>

#include <shard/syntax/nodes/ArgumentsListSyntax.h>
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

		void Execute();
		ObjectInstance* ExecuteMethod(CallStackFrame* prevCallFrame, shard::syntax::symbols::MethodSymbol* method, InboundVariablesContext* argumentsContext);
		ObjectInstance* ExecuteBlock(CallStackFrame* callFrame, shard::syntax::nodes::StatementsBlockSyntax* block, InboundVariablesContext* variablesContext);
		ObjectInstance* ExecuteStatement(CallStackFrame* callFrame, shard::syntax::nodes::StatementSyntax* statement, InboundVariablesContext* variablesContext);
		ObjectInstance* EvaluateExpression(CallStackFrame* callFrame, shard::syntax::nodes::ExpressionSyntax* expression, InboundVariablesContext* variablesContext);
		ObjectInstance* EvaluateArgument(CallStackFrame* callFrame, shard::syntax::nodes::ArgumentSyntax* argument, InboundVariablesContext* variablesContext);
		ObjectInstance* EvaluateLinkedExpression(CallStackFrame* callFrame, shard::syntax::nodes::LinkedExpressionSyntax* expression, InboundVariablesContext* variablesContext);
		ObjectInstance* EvaluateLinkedExpression(CallStackFrame* callFrame, shard::syntax::nodes::LinkedExpressionNode* expression, InboundVariablesContext* variablesContext, shard::runtime::ObjectInstance* objInstance);
	};
}