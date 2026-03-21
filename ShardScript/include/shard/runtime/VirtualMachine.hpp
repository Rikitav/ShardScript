#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/PrimitiveMathModule.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>

#include <stack>
#include <atomic>
#include <initializer_list>

namespace shard
{
	class ApplicationDomain;
	class GarbageCollector;

	class SHARD_API VirtualMachine
	{
		ApplicationDomain* const domain;
		ProgramVirtualImage& program;
		GarbageCollector& garbageCollector;
		PrimitiveMathModule math;

		std::stack<CallStackFrame*> CallStack;
		std::atomic<bool> AbortFlag;

		void ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode);
		void InvokeMethodInternal(MethodSymbol* method, CallStackFrame* currentFrame);
		ObjectInstance* InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor);
		ObjectInstance* InstantiateDelegate(DelegateTypeSymbol* type);

	public:
		VirtualMachine(ApplicationDomain* appDomain);

		CallStackFrame* CurrentFrame() const;
		CallStackFrame* PushFrame(MethodSymbol* methodSymbol);
		void PopFrame();

		void InvokeMethod(MethodSymbol* method) const;
		void InvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance*> args) const;
		void RaiseException(ObjectInstance* exceptionReg) const;

		void Run();
		void Abort() const;
		void TerminateCallStack();

		ObjectInstance* RunInteractive(size_t& pointer);
	};
}