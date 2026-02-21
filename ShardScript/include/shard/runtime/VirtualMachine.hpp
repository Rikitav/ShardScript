#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <stack>
#include <atomic>
#include <initializer_list>

namespace shard
{
	class SHARD_API VirtualMachine
	{
		ProgramVirtualImage& Program;
		std::stack<CallStackFrame*> CallStack;
		std::atomic<bool> AbortFlag;

		void InvokeMethodInternal(MethodSymbol* method, CallStackFrame* currentFrame);
		ObjectInstance* InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor);
		void ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode);

	public:
		VirtualMachine(ProgramVirtualImage& program);

		CallStackFrame* CurrentFrame() const;
		CallStackFrame* PushFrame(MethodSymbol* methodSymbol);
		void PopFrame();

		void InvokeMethod(MethodSymbol* method) const;
		void InvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance*> args) const;
		void RaiseException(ObjectInstance* exceptionReg) const;

		void Run() const;
		void Abort() const;
		void TerminateCallStack();

		ObjectInstance* RunInteractive(size_t& pointer);
	};
}