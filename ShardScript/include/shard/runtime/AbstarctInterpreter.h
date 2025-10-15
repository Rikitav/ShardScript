#pragma once
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/Register.h>

#include <shard/syntax/structures/SyntaxTree.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>

#include <stack>
#include <memory>

using namespace std;
using namespace shard::syntax::structures;

namespace shard::runtime
{
	class AbstarctInterpreter
	{
	private:
		stack<shared_ptr<CallStackFrame>> CallStack;
		shared_ptr<SyntaxTree> Tree;

	public:
		AbstarctInterpreter(shared_ptr<SyntaxTree> tree)
			: Tree(tree) { }

		void Execute();
		shared_ptr<Register> ExecuteMethod(shared_ptr<MethodDeclarationSyntax> method, shared_ptr<CallStackFrame> prevFrame);
		shared_ptr<Register> ExecuteBlock(shared_ptr<StatementsBlockSyntax> block, shared_ptr<CallStackFrame> frame);
		shared_ptr<Register> ExecuteStatement(shared_ptr<StatementSyntax> statement, shared_ptr<CallStackFrame> frame);
		shared_ptr<Register> EvaluateExpression(shared_ptr<ExpressionSyntax> expression, shared_ptr<CallStackFrame> frame);
	};
}