#pragma once
#include <shard/runtime/Register.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

using namespace std;

namespace shard::runtime
{
	class CallStackFrame
	{
	public:
		shared_ptr<CallStackFrame> Previous;
		shared_ptr<MethodDeclarationSyntax> Declaration;
		unordered_map<string, shared_ptr<Register>> VariablesHeap;

		CallStackFrame(shared_ptr<CallStackFrame> previous, shared_ptr<MethodDeclarationSyntax> declaration)
			: Declaration(declaration), Previous(previous), VariablesHeap() {}
	};
}
