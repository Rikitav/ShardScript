#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <stack>

namespace shard::runtime
{
	enum class SHARD_API FrameInterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised,
		LoopBreak,
		LoopContinue,
	};

	class SHARD_API CallStackFrame
	{
	public:
		const shard::syntax::symbols::MethodSymbol* Method;
		CallStackFrame* PreviousFrame;
		std::stack<shard::runtime::InboundVariablesContext*> VariablesStack;
		
		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;

		inline CallStackFrame(const shard::syntax::symbols::MethodSymbol* method, CallStackFrame* previousFrame)
			: Method(method), PreviousFrame(previousFrame) { }

		inline bool interrupted() const
		{
			return InterruptionReason != FrameInterruptionReason::None;
		}

		inline ~CallStackFrame()
		{
			Method = nullptr;
			PreviousFrame = nullptr;
		}
	};
}
