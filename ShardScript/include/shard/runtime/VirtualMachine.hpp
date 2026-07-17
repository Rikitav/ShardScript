#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/PrimitiveMathModule.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>

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
		ApplicationDomain* domain;
		ProgramVirtualImage& program;
		GarbageCollector& garbageCollector;
		PrimitiveMathModule primitiveMath;

		std::vector<std::unique_ptr<CallStackFrame>> CallStack;
		std::atomic<bool> AbortFlag;
		std::vector<TypeSymbol*> PendingTypeArguments;

		ObjectInstance* UnhandledException = nullptr;
		std::wstring UnhandledExceptionMessage;
		std::wstring UnhandledExceptionStackTrace;

		void InvokeMethodInternal(MethodSymbol* method, CallStackFrame* currentFrame);

	public:
		void ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode);
		ObjectInstance* InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor);
		ObjectInstance* InstantiateDelegate(DelegateTypeSymbol* type);
		ObjectInstance* CreateRuntimeException(const std::exception& err);

		ObjectInstance* InvokeOperatorMethod(ObjectInstance* leftInstance, TokenType opToken, ObjectInstance* rightInstance);
		ObjectInstance* InvokeOperatorMethod(ObjectInstance* sourceInstance, TokenType opToken);

	public:
		VirtualMachine(ApplicationDomain* appDomain);
		~VirtualMachine() = default;

		VirtualMachine(const VirtualMachine&) = delete;
		VirtualMachine& operator=(const VirtualMachine&) = delete;

		ApplicationDomain* GetDomain() const { return domain; }
		ProgramVirtualImage& GetProgram() const { return program; }
		GarbageCollector& GetGarbageCollector() const { return garbageCollector; }

		CallStackFrame* CurrentFrame() const;
		CallStackFrame* PushFrame(MethodSymbol* methodSymbol);
		void PopFrame();

		void InvokeMethod(MethodSymbol* method) const;
		void InvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance*> args) const;
		ObjectInstance* InvokeMethod(MethodSymbol* method, ObjectInstance** args, std::size_t count) const;
		void SetPendingTypeArguments(std::initializer_list<TypeSymbol*> args) const;
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