#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class VirtualMachine;

	enum class FrameInterruptionReason
	{
		None,
		ValueReturned,
		ExceptionRaised,
	};

	class SHARD_API CallStackFrame : public std::enable_shared_from_this<CallStackFrame>
	{
	public:
		struct ExceptionHandlerFrame
		{
			std::size_t HandlerOffset;
			std::size_t DeferStackBase;
		};

		const VirtualMachine* Host;
		CallStackFrame* PreviousFrame;
		MethodSymbol* Method;

		std::vector<ObjectInstance*> EvalStack;
		std::vector<TypeSymbol*> TypeArguments;
		std::vector<ExceptionHandlerFrame> ExceptionHandlers;
		std::size_t PendingTaskCount = 0;

		FrameInterruptionReason InterruptionReason = FrameInterruptionReason::None;
		ObjectInstance* InterruptionRegister = nullptr;
		ObjectInstance* CurrentException = nullptr;

		std::vector<std::size_t> DeferStack;
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
