#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <vector>

namespace shard
{
	class VirtualMachine;

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
		const VirtualMachine* Host;
		CallStackFrame* PreviousFrame;
		TypeSymbol* WithinType;
		MethodSymbol* Method;

		std::vector<ObjectInstance*> EvalStack;
		std::vector<TypeSymbol*> TypeArguments;

		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;

		inline CallStackFrame(const VirtualMachine* host, CallStackFrame* previousFrame, TypeSymbol* withinType, MethodSymbol* method)
			: Host(host), WithinType(withinType), Method(method), PreviousFrame(previousFrame) { }

		inline bool interrupted() const
		{
			return InterruptionReason != FrameInterruptionReason::None;
		}

		void PushStack(ObjectInstance* value);
		ObjectInstance* PopStack();
		ObjectInstance* PeekStack();

		inline ~CallStackFrame()
		{
			Method = nullptr;
			PreviousFrame = nullptr;
		}
	};
}
