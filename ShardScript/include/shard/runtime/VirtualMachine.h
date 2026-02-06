#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/compilation/ProgramVirtualImage.h>
#include <shard/compilation/ByteCodeDecoder.h>
#include <shard/runtime/CallStackFrame.h>

namespace shard
{
	class SHARD_API VirtualMachine
	{
		ProgramVirtualImage& Program;
		std::stack<CallStackFrame*> CallStack;

		CallStackFrame* CurrentFrame();
		CallStackFrame* PushFrame(MethodSymbol* methodSymbol);
		void PopFrame();

		void InvokeMethod(CallStackFrame* frame);
		void ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode);
		void RaiseException(ObjectInstance* exceptionReg);

	public:
		inline VirtualMachine(ProgramVirtualImage& program)
			: Program(program) { }

		inline ~VirtualMachine() { }

		void Run();
	};
}