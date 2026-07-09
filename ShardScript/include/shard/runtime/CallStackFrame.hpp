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

		std::vector<std::size_t> ExceptionHandlers;

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
