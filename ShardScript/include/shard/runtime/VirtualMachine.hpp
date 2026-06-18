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
#include <memory>
#include <initializer_list>
#include <string>

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

		std::vector<std::unique_ptr<CallStackFrame>> CallStack;
		std::atomic<bool> AbortFlag;
		std::vector<TypeSymbol*> PendingTypeArguments;

		ObjectInstance* UnhandledException = nullptr;
		std::wstring UnhandledExceptionMessage;
		std::wstring UnhandledExceptionStackTrace;

		void ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode);
		void InvokeMethodInternal(MethodSymbol* method, CallStackFrame* currentFrame);
		ObjectInstance* InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor);
		ObjectInstance* InstantiateDelegate(DelegateTypeSymbol* type);
		ObjectInstance* CreateRuntimeException(const std::exception& err);

	public:
		VirtualMachine(ApplicationDomain* appDomain);
		~VirtualMachine() = default;

		VirtualMachine(const VirtualMachine&) = delete;
		VirtualMachine& operator=(const VirtualMachine&) = delete;

		CallStackFrame* CurrentFrame() const;
		CallStackFrame* PushFrame(MethodSymbol* methodSymbol);
		CallStackFrame* PushFrame(MethodSymbol* methodSymbol, TypeSymbol* withinType);
		void PopFrame();

		void InvokeMethod(MethodSymbol* method) const;
		void InvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance*> args) const;
		ObjectInstance* InvokeMethod(MethodSymbol* method, ObjectInstance** args, std::size_t count) const;
		void RaiseException(ObjectInstance* exceptionReg) const;

		std::wstring GetStackTrace() const;

		void Run();
		void Abort() const;
		void TerminateCallStack();

		ObjectInstance* GetUnhandledException() const { return UnhandledException; }
		const std::wstring& GetUnhandledExceptionMessage() const { return UnhandledExceptionMessage; }
		const std::wstring& GetUnhandledExceptionStackTrace() const { return UnhandledExceptionStackTrace; }

		ObjectInstance* RunInteractive(std::size_t& pointer);
	};
}