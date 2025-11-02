#pragma once
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

namespace shard::runtime
{
	enum class FrameInterruptionReason
	{
		None,
		ValueReturned,
		ExcpetionRaised,
	};

	class CallStackFrame
	{
	public:
		const shard::syntax::symbols::TypeSymbol* OwnerObject;
		const shard::syntax::symbols::MethodSymbol* Method;
		const CallStackFrame* PreviousFrame;
		const InboundVariablesContext* VariablesContext;
		
		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;

		CallStackFrame(const shard::syntax::symbols::TypeSymbol* ownerObject, const shard::syntax::symbols::MethodSymbol* method, const CallStackFrame* previousFrame, const InboundVariablesContext* prevVarCcontext)
			: OwnerObject(ownerObject), Method(method), PreviousFrame(previousFrame), VariablesContext(new InboundVariablesContext(prevVarCcontext)) { }
	};
}
