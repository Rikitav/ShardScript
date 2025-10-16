#pragma once
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/VariableRegister.h>
#include <shard/runtime/InboundVariablesContext.h>

#include <shard/syntax/structures/SyntaxTree.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/Expressions.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <stack>
#include <memory>

namespace shard::runtime
{
	class AbstarctInterpreter
	{
	private:
		stack<shared_ptr<shard::runtime::CallStackFrame>> CallStack;
		std::shared_ptr<shard::syntax::structures::SyntaxTree> Tree;

	public:
		AbstarctInterpreter(std::shared_ptr<shard::syntax::structures::SyntaxTree> tree)
			: Tree(tree) { }

		void Execute();
		std::shared_ptr<VariableRegister> ExecuteMethod(std::shared_ptr<shard::syntax::nodes::MethodDeclarationSyntax> method, std::shared_ptr<shard::runtime::CallStackFrame> prevFrame);
		std::shared_ptr<VariableRegister> ExecuteBlock(std::shared_ptr<shard::syntax::nodes::StatementsBlockSyntax> block, std::shared_ptr<shard::runtime::InboundVariablesContext> prevContext, std::shared_ptr<shard::runtime::CallStackFrame> frame);
		std::shared_ptr<VariableRegister> ExecuteStatement(std::shared_ptr<shard::syntax::nodes::StatementSyntax> statement, std::shared_ptr<shard::runtime::InboundVariablesContext> context, std::shared_ptr<shard::runtime::CallStackFrame> frame);
		std::shared_ptr<VariableRegister> EvaluateExpression(std::shared_ptr<shard::syntax::nodes::ExpressionSyntax> expression, std::shared_ptr<shard::runtime::InboundVariablesContext> context);
		std::shared_ptr<VariableRegister> EvaluateMemberAccessChain(std::shared_ptr<shard::syntax::nodes::MemberAccessExpressionSyntax> expression, std::shared_ptr<shard::runtime::InboundVariablesContext> context, std::shared_ptr<shard::runtime::VariableRegister> prevRegister);
		std::shared_ptr<VariableRegister> EvaluateMemberAccessExpression(std::shared_ptr<shard::syntax::nodes::MemberAccessExpressionSyntax> expression, std::shared_ptr<shard::runtime::InboundVariablesContext> context, std::shared_ptr<shard::runtime::VariableRegister> prevRegister);

		std::shared_ptr<VariableRegister> EvaluatePrintInvokationExpression(std::shared_ptr<shard::syntax::nodes::ArgumentSyntax> argument, std::shared_ptr<shard::runtime::InboundVariablesContext> context, bool line);
		void PrintRegister(std::shared_ptr<VariableRegister> pRegister);
	};
}