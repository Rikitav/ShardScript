#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

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
		MethodSymbol* Method;

		std::vector<ObjectInstance*> EvalStack;
		std::vector<TypeSymbol*> TypeArguments;

		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;
		ObjectInstance* CurrentException = nullptr;

		struct ExceptionHandlerFrame
		{
			std::size_t HandlerOffset;
			std::size_t DeferStackBase;
		};
		std::vector<ExceptionHandlerFrame> ExceptionHandlers;

		// Defer machinery. DEFER pushes target IPs here; DEFER_DRAIN jumps into them
		// and uses DeferDrainStack to return to the drain loop after DEFER_BREAK.
		std::vector<std::size_t> DeferStack;

		// Synchronous DEFER_DRAIN executes deferred expressions inline. This counter
		// lets DEFER_BREAK verify it is only reached while draining.
		std::size_t DeferDrainDepth = 0;

		inline CallStackFrame(const VirtualMachine* host, CallStackFrame* previousFrame, MethodSymbol* method)
			: Host(host), Method(method), PreviousFrame(previousFrame) { }

		inline bool interrupted() const
		{
			return InterruptionReason != FrameInterruptionReason::None;
		}

		TypeSymbol* ResolveType(TypeSymbol* type);

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
