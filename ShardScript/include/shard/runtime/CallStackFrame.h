#pragma once
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>

#include <memory>

using namespace std;

namespace shard::runtime
{
	class CallStackFrame
	{
	public:
		shared_ptr<CallStackFrame> PreviousFrame;
		shared_ptr<MethodDeclarationSyntax> Declaration;
		shared_ptr<InboundVariablesContext> Context;
		bool IsInterrupted = false;

		CallStackFrame(shared_ptr<CallStackFrame> previousFrame, shared_ptr<MethodDeclarationSyntax> declaration)
			: Declaration(declaration), PreviousFrame(previousFrame), Context(make_shared<InboundVariablesContext>()) {}
	};
}
