#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <stack>

namespace shard
{
	enum class SHARD_API FrameInterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised,
		LoopBreak,
		LoopContinue,
		GotoJump,
	};

	class SHARD_API CallStackFrame
	{
	public:
		const shard::TypeSymbol* WithinType;
		const shard::MethodSymbol* Method;
		const CallStackFrame* PreviousFrame;

		std::stack<shard::InboundVariablesContext*> VariablesStack;
		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;
		size_t GotoIndex = -1;

		inline CallStackFrame(const shard::MethodSymbol* method, const shard::TypeSymbol* withinType, const CallStackFrame* previousFrame)
			: WithinType(withinType), Method(method), PreviousFrame(previousFrame) { }

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
