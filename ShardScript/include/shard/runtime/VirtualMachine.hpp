#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/PrimitiveMathModule.hpp>
#include <shard/runtime/RuntimeException.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
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
	struct CallState;

	class SHARD_API VirtualMachine
	{
		ApplicationDomain* domain;
		ProgramVirtualImage& program;
		GarbageCollector& garbageCollector;
		PrimitiveMathModule primitiveMath;

		std::vector<std::shared_ptr<CallStackFrame>> CallStack;
		std::atomic<bool> AbortFlag;
		std::vector<TypeSymbol*> PendingTypeArguments;

		ObjectInstance UnhandledException;
		std::wstring UnhandledExceptionMessage;
		std::wstring UnhandledExceptionStackTrace;

		friend struct CallState;

		void HaltFireAndForgetTasks();
		void ExecuteDeferExpression(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t target);
		bool DrainDefersTo(CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t targetSize);
		bool HandleExceptionInFrame(CallStackFrame* frame, ByteCodeDecoder& decoder);

		void ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode);
		void InvokeMethodInternal(const MethodSymbol* method, CallStackFrame* currentFrame);

	public:
		ObjectInstance InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor, bool inPlace = false);
		ObjectInstance InstantiateDelegate(DelegateTypeSymbol* type);

		ObjectInstance InvokeOperatorMethod(ObjectInstance leftInstance, TokenType opToken, ObjectInstance rightInstance);
		ObjectInstance InvokeOperatorMethod(ObjectInstance sourceInstance, TokenType opToken);

		ObjectInstance CreateRuntimeException(const std::exception& err);
		ObjectInstance CreateRuntimeException(TypeSymbol* type, const std::wstring& message, const std::wstring& stackTrace);

	public:
		VirtualMachine(ApplicationDomain* appDomain);
		~VirtualMachine() = default;

		VirtualMachine(const VirtualMachine&) = delete;
		VirtualMachine& operator=(const VirtualMachine&) = delete;

		ApplicationDomain* GetDomain() const { return domain; }
		ProgramVirtualImage& GetProgram() const { return program; }
		GarbageCollector& GetGarbageCollector() const { return garbageCollector; }

		CallStackFrame* CurrentFrame() const;
		CallStackFrame* PushFrame(const MethodSymbol* methodSymbol);
		void PopFrame();

		ObjectInstance InvokeMethod(const MethodSymbol* method) const;
		ObjectInstance InvokeMethod(const MethodSymbol* method, const std::span<ObjectInstance> args) const;
		ObjectInstance InvokeMethod(const MethodSymbol* method, const std::initializer_list<ObjectInstance> args) const;
		ObjectInstance InvokeMethod(const MethodSymbol* method, const ObjectInstance* args, std::size_t count) const;

		void SetPendingTypeArguments(std::span<TypeSymbol*> args) const;
		void SetPendingTypeArguments(std::initializer_list<TypeSymbol*> args) const;
		void SetPendingTypeArguments(const std::vector<TypeSymbol*>& args) const;

		void RaiseException(ObjectInstance exceptionReg) const;

		std::wstring GetStackTrace() const;
		std::wstring GetThrowablePropertyValue(ObjectInstance exception, AccessorSymbol* interfacePropertyAccessor) const;

		ObjectInstance GetUnhandledException() const { return UnhandledException; }
		const std::wstring& GetUnhandledExceptionMessage() const { return UnhandledExceptionMessage; }
		const std::wstring& GetUnhandledExceptionStackTrace() const { return UnhandledExceptionStackTrace; }

		void Run();
		void Abort() const;
		void TerminateCallStack();

		ObjectInstance RunInteractive(std::size_t& pointer);
	};
}