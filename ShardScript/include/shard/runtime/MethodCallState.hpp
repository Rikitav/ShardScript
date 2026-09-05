#pragma once
#include <span>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/RuntimeException.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>

#include <shard/ApplicationDomain.hpp>
#include <shard/ShardScriptAPI.hpp>

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace shard
{
	namespace detail
	{
		inline bool               UnwrapArg(ObjectInstance value, bool*) { return value.AsBoolean(); }
		inline std::int64_t       UnwrapArg(ObjectInstance value, std::int64_t*) { return value.AsInteger(); }
		inline double             UnwrapArg(ObjectInstance value, double*) { return value.AsDouble(); }
		inline wchar_t            UnwrapArg(ObjectInstance value, wchar_t*) { return value.AsCharacter(); }
		inline std::uint8_t       UnwrapArg(ObjectInstance value, std::uint8_t*) { return value.AsByte(); }
		inline const wchar_t* UnwrapArg(ObjectInstance value, const wchar_t**) { return value.AsString(); }
		inline std::wstring       UnwrapArg(ObjectInstance value, std::wstring*) { return std::wstring(value.AsString()); }
		inline ObjectInstance     UnwrapArg(ObjectInstance value, ObjectInstance*) { return value; }

		template<typename T>
		inline T* UnwrapArg(ObjectInstance value, T**)
		{
			if (value.IsNullInstance())
				return nullptr;

			return static_cast<T*>(value.AsNint());
		}

		inline TypeSymbol* GetConstructedTypeDefinition(TypeSymbol* type)
		{
			if (type == nullptr)
				return nullptr;

			if (type->Kind == SyntaxKind::GenericType)
				return static_cast<GenericTypeSymbol*>(type)->UnderlayingType;

			return type;
		}

		inline ConstructorSymbol* FindParameterlessConstructor(TypeSymbol* type)
		{
			TypeSymbol* definition = GetConstructedTypeDefinition(type);
			if (definition == nullptr)
				return nullptr;

			for (ConstructorSymbol* ctor : definition->Constructors)
			{
				if (ctor->Parameters.empty())
					return ctor;
			}

			return nullptr;
		}
	}

	typedef std::span<ObjectInstance> ArgumentsSpan;

	class DelegateRef;

	struct ReturnTargetInfo
	{
		std::byte* Slot = nullptr;
		TypeShape* Shape = nullptr; // non-null => inline (by-value) return slot
	};

	/// <summary>
	/// 
	/// </summary>
	class InvokeResult
	{
		ObjectInstance m_value;
		ObjectInstance m_exception;

	public:
		InvokeResult() = default;
		InvokeResult(ObjectInstance value, ObjectInstance exception)
			: m_value(value), m_exception(exception) { }

		bool IsOk() const { return m_exception.IsNullInstance(); }
		explicit operator bool() const { return IsOk(); }

		[[nodiscard]] ObjectInstance Value() const
		{
			if (!IsOk())
				throw undefined_behaviour("InvokeResult: Value() on a failed invocation");

			return m_value;
		}

		[[nodiscard]] ObjectInstance Exception() const
		{
			if (IsOk())
				throw undefined_behaviour("InvokeResult: Exception() on a successful invocation");

			return m_exception;
		}
	};

	struct CallState
	{
		/*
		* ABI v2 — FOREIGN FUNCTIONS INVOCATION STATE.
		* DO NOT MODIFY THE DATA MEMBERS OF THIS STRUCTURE!
		* ANY CHANGES IN THIS CODE WILL RESULT LOSS OF BACKWARDS COMPATIBILITY AND UNDEFINED BEHAVIOUR!
		* IN CASE OF CHANGES, RECOMPILE DEPENDENT LIBRARIES!
		* Member functions are the library-facing helper API and may evolve.
		*/

		ApplicationDomain& Domain;
		ProgramVirtualImage& Program;
		VirtualMachine& Runtimer;
		GarbageCollector& Collector;

		CallStackFrame* Frame;
		MethodSymbol* Method;
		ArgumentsSpan& Args;

		ReturnTargetInfo ReturnTarget;
		mutable bool ReturnPlaced = false;

		template<typename T>
		void WriteReturn(const T& value) const
		{
			static_assert(std::is_trivially_copyable_v<T>, "WriteReturn requires a trivially copyable type");

			if (ReturnTarget.Shape == nullptr)
				throw undefined_behaviour("WriteReturn: method does not return a by-value type");

			if (sizeof(T) > ReturnTarget.Shape->Size)
				throw undefined_behaviour("WriteReturn: value does not fit the return slot");

			if (ReturnPlaced)
				throw undefined_behaviour("WriteReturn: return value already placed");

			std::memcpy(ReturnTarget.Slot, &value, sizeof(T));
			ReturnPlaced = true;
		}

		void WriteReturn(const ObjectInstance& value) const
		{
			if (ReturnTarget.Shape == nullptr)
				throw undefined_behaviour("WriteReturn: method does not return a by-value type");

			if (ReturnPlaced)
				throw undefined_behaviour("WriteReturn: return value already placed");

			std::memcpy(ReturnTarget.Slot, value.getMemory(), ReturnTarget.Shape->Size);
			ReturnPlaced = true;
		}

		ObjectInstance ReturnView() const
		{
			if (ReturnTarget.Shape == nullptr)
				throw undefined_behaviour("ReturnView: method does not return a by-value type");

			ObjectInstance view(Method->ReturnType, ReturnTarget.Shape, ReturnTarget.Slot);
			ReturnPlaced = true;
			return view;
		}

		void PlaceReturned(ObjectInstance value) const
		{
			if (ReturnTarget.Shape != nullptr)
				throw undefined_behaviour("PlaceReturned: library returned a value for a by-value method");

			if (Method->ReturnType == nullptr || Method->ReturnType == SymbolTable::Primitives::Void)
				throw undefined_behaviour("PlaceReturned: method returns void");

			if (ReturnPlaced)
				throw undefined_behaviour("PlaceReturned: return value already placed");

			*reinterpret_cast<TypeShape**>(ReturnTarget.Slot - CallStackFrame::SlotHeaderBytes) = value.getShape();
			std::byte* stored = value.getMemory();
			std::memcpy(ReturnTarget.Slot, &stored, sizeof(stored));
			ReturnPlaced = true;
		}

		// ------------------------------------------------------------------
		// Observed invocation. Script exceptions come back as InvokeResult::Err
		// (owning reference to the Throwable) instead of being dropped on the
		// calling frame; native C++ exceptions (abort, engine errors) still
		// propagate. See also Propagate.
		// ------------------------------------------------------------------
		InvokeResult TryInvokeMethod(MethodSymbol* method) const;
		InvokeResult TryInvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance> args) const;
		InvokeResult TryInvokeMethod(MethodSymbol* method, ObjectInstance* args, std::size_t count) const;

		/// <summary>
		/// Re-raise on this callback's frame so unwinding continues when the callback returns.
		/// </summary>
		/// <param name="exception"></param>
		void Propagate(ObjectInstance exception) const;

		template<typename... TArgs>
		std::tuple<TArgs...> GetArgs() const;

		ObjectInstance NewObject(TypeSymbol* type, ConstructorSymbol* ctor, std::initializer_list<ObjectInstance> args = {}) const;
		ObjectInstance NewObject(TypeSymbol* type) const;
		ObjectInstance NewObject(ClassSymbol* cls, const std::vector<TypeSymbol*>& typeArgs, std::initializer_list<ObjectInstance> args = {}) const;

		// Throwing invocation flavor: std::runtime_error on Err, which the
		// engine boundary converts into a script exception.
		ObjectInstance CallMethod(MethodSymbol* method, std::initializer_list<ObjectInstance> args = {}) const;
		ObjectInstance CallMethod(MethodSymbol* method, ObjectInstance receiver, std::initializer_list<ObjectInstance> args = {}) const;

		ObjectInstance GetProperty(ObjectInstance obj, PropertySymbol* prop) const;
		void SetProperty(ObjectInstance obj, PropertySymbol* prop, ObjectInstance value) const;

		DelegateRef WrapDelegate(ObjectInstance delegate) const;
	};

	namespace detail
	{
		template<typename T>
		inline T UnwrapArgAtIndex(const CallState& context, std::size_t index)
		{
			if (index >= context.Args.size())
				throw undefined_behaviour("Argument index out of range");

			ObjectInstance value = context.Args[index];
			if (value.IsNullInstance())
				throw undefined_behaviour("Argument is null");

			T* tag = nullptr;
			return UnwrapArg(value, tag);
		}

		template<typename... TArgs, std::size_t... Indices>
		inline std::tuple<TArgs...> GetArgsImpl(const CallState& context, std::index_sequence<Indices...>)
		{
			return std::make_tuple(UnwrapArgAtIndex<TArgs>(context, Indices)...);
		}
	}

	/// <summary>
	/// Extracts the method arguments into a typed tuple. Supported types: bool,
	/// std::int64_t, double, wchar_t, std::uint8_t, const wchar_t*, std::wstring,
	/// ObjectInstance, and any native pointer T*.
	/// </summary>
	template<typename... TArgs>
	inline std::tuple<TArgs...> CallState::GetArgs() const
	{
		constexpr std::size_t expectedCount = sizeof...(TArgs);
		if (Args.size() != expectedCount)
		{
			throw undefined_behaviour(
				"Expected " + std::to_string(expectedCount) +
				" arguments, got " + std::to_string(Args.size()));
		}

		return detail::GetArgsImpl<TArgs...>(*this, std::index_sequence_for<TArgs...>{});
	}

	inline InvokeResult CallState::TryInvokeMethod(MethodSymbol* method, ObjectInstance* args, std::size_t count) const
	{
		if (method == nullptr)
			throw undefined_behaviour("TryInvokeMethod: method is null");

		bool pushedRootFrame = false;
		CallStackFrame* callingFrame = Runtimer.CurrentFrame();
		if (callingFrame == nullptr)
		{
			MethodSymbol* rootMethod = Program.EntryPoint != nullptr ? Program.EntryPoint : method;
			callingFrame = Runtimer.PushFrame(rootMethod);
			pushedRootFrame = true;
		}

		CallStackFrame* frame = Runtimer.PushFrame(method);

		for (std::size_t i = 0; i < count; i++)
			callingFrame->PushCopy(args[i]);

		InvokeResult result;
		try
		{
			Runtimer.InvokeMethodInternal(method, frame);
		}
		catch (...)
		{
			Runtimer.PopFrame();
			if (pushedRootFrame)
				Runtimer.PopFrame();

			throw;
		}

		if (callingFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
		{
			ObjectInstance exception = callingFrame->CurrentException;
			if (!exception.IsNullInstance())
			{
				exception.IncrementReference();
				callingFrame->CurrentException.DecrementReference();
				Collector.CollectInstance(callingFrame->CurrentException);
			}

			callingFrame->InterruptionReason = FrameInterruptionReason::None;
			callingFrame->InterruptionRegister = ObjectInstance();
			callingFrame->CurrentException = ObjectInstance();
			result = InvokeResult(ObjectInstance(), exception);
		}
		else if (method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void && callingFrame->EvalCount() > 0)
		{
			result = InvokeResult(callingFrame->PopValue(), ObjectInstance());
		}

		Runtimer.PopFrame();
		if (pushedRootFrame)
			Runtimer.PopFrame();

		return result;
	}

	inline InvokeResult CallState::TryInvokeMethod(MethodSymbol* method) const
	{
		return TryInvokeMethod(method, nullptr, 0);
	}

	inline InvokeResult CallState::TryInvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance> args) const
	{
		std::vector<ObjectInstance> storage(args.begin(), args.end());
		return TryInvokeMethod(method, storage.data(), storage.size());
	}

	inline void CallState::Propagate(ObjectInstance exception) const
	{
		if (Frame == nullptr)
			throw undefined_behaviour("Propagate: callback has no frame");

		if (exception.IsNullInstance())
			throw undefined_behaviour("Propagate: exception is null");

		exception.IncrementReference();
		Frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
		Frame->InterruptionRegister = exception;
		Frame->CurrentException = exception;
	}

	inline ObjectInstance CallState::NewObject(TypeSymbol* type, ConstructorSymbol* ctor, std::initializer_list<ObjectInstance> args) const
	{
		if (type == nullptr)
			throw undefined_behaviour("NewObject: type is null");

		if (ctor == nullptr)
			throw undefined_behaviour("NewObject: constructor is null");

		ObjectInstance instance = Collector.AllocateInstance(type);

		// Keep the object alive through the constructor call.
		instance.IncrementReference();

		std::vector<ObjectInstance> callArgs;
		callArgs.reserve(1 + args.size());
		callArgs.push_back(instance);
		callArgs.insert(callArgs.end(), args.begin(), args.end());

		try
		{
			Runtimer.InvokeMethod(ctor, callArgs.data(), callArgs.size());
		}
		catch (...)
		{
			instance.DecrementReference();
			throw;
		}

		return instance;
	}

	inline ObjectInstance CallState::NewObject(TypeSymbol* type) const
	{
		ConstructorSymbol* ctor = detail::FindParameterlessConstructor(type);
		if (ctor == nullptr)
			throw undefined_behaviour("Type has no parameterless constructor");

		return NewObject(type, ctor, {});
	}

	inline ObjectInstance CallState::NewObject(ClassSymbol* cls, const std::vector<TypeSymbol*>& typeArgs, std::initializer_list<ObjectInstance> args) const
	{
		if (cls == nullptr)
			throw undefined_behaviour("NewObject: class is null");

		ObjectInstance instance = Collector.AllocateGeneric(cls, typeArgs);

		ConstructorSymbol* ctor = nullptr;
		for (ConstructorSymbol* candidate : cls->Constructors)
		{
			if (candidate->Parameters.size() == args.size())
			{
				ctor = candidate;
				break;
			}
		}

		if (ctor == nullptr)
			throw undefined_behaviour("NewObject: no constructor with matching parameter count");

		instance.IncrementReference();

		std::vector<ObjectInstance> callArgs;
		callArgs.reserve(1 + args.size());
		callArgs.push_back(instance);
		callArgs.insert(callArgs.end(), args.begin(), args.end());

		try
		{
			Runtimer.InvokeMethod(ctor, callArgs.data(), callArgs.size());
		}
		catch (...)
		{
			instance.DecrementReference();
			throw;
		}

		return instance;
	}

	inline ObjectInstance CallState::CallMethod(MethodSymbol* method, std::initializer_list<ObjectInstance> args) const
	{
		if (method == nullptr)
			throw undefined_behaviour("CallMethod: method is null");

		InvokeResult result = TryInvokeMethod(method, args);
		if (!result)
			throw undefined_behaviour("CallMethod: invoked method raised an exception");

		return result.Value();
	}

	inline ObjectInstance CallState::CallMethod(MethodSymbol* method, ObjectInstance receiver, std::initializer_list<ObjectInstance> args) const
	{
		if (method == nullptr)
			throw undefined_behaviour("CallMethod: method is null");

		std::vector<ObjectInstance> callArgs;
		callArgs.reserve(1 + args.size());
		callArgs.push_back(receiver);
		callArgs.insert(callArgs.end(), args.begin(), args.end());

		InvokeResult result = TryInvokeMethod(method, callArgs.data(), callArgs.size());
		if (!result)
			throw undefined_behaviour("CallMethod: invoked method raised an exception");

		return result.Value();
	}

	inline ObjectInstance CallState::GetProperty(ObjectInstance obj, PropertySymbol* prop) const
	{
		if (obj.IsNullInstance())
			throw undefined_behaviour("GetProperty: object is null");

		if (prop == nullptr)
			throw undefined_behaviour("GetProperty: property is null");

		if (prop->Getter == nullptr)
			throw undefined_behaviour("GetProperty: property has no getter");

		return CallMethod(prop->Getter, { obj });
	}

	inline void CallState::SetProperty(ObjectInstance obj, PropertySymbol* prop, ObjectInstance value) const
	{
		if (obj.IsNullInstance())
			throw undefined_behaviour("SetProperty: object is null");

		if (prop == nullptr)
			throw undefined_behaviour("SetProperty: property is null");

		if (prop->Setter == nullptr)
			throw undefined_behaviour("SetProperty: property has no setter");

		CallMethod(prop->Setter, obj, { value });
	}

	/// <summary>
	/// Reads a field value.
	/// </summary>
	inline ObjectInstance GetField(ObjectInstance obj, FieldSymbol* field)
	{
		if (obj.IsNullInstance())
			throw undefined_behaviour("GetField: object is null");

		if (field == nullptr)
			throw undefined_behaviour("GetField: field is null");

		return obj.GetField(field);
	}

	/// <summary>
	/// Writes a field value.
	/// </summary>
	inline void SetField(ObjectInstance obj, FieldSymbol* field, ObjectInstance value)
	{
		if (obj.IsNullInstance())
			throw undefined_behaviour("SetField: object is null");

		if (field == nullptr)
			throw undefined_behaviour("SetField: field is null");

		obj.SetField(field, value);
	}

	/// <summary>
	/// Callable RAII container that holds an ObjectRef to a delegate ObjectInstance.
	/// </summary>
	class SHARD_API DelegateRef
	{
		VirtualMachine* m_runtime = nullptr;
		ObjectRef m_delegate;

	public:
		DelegateRef() = default;

		DelegateRef(VirtualMachine& runtime, ObjectInstance delegate)
			: m_runtime(&runtime), m_delegate(delegate) { }

		DelegateRef(const DelegateRef&) = delete;
		DelegateRef& operator=(const DelegateRef&) = delete;

		DelegateRef(DelegateRef&&) = default;
		DelegateRef& operator=(DelegateRef&&) = default;

		[[nodiscard]] bool IsValid() const noexcept
		{
			return m_runtime != nullptr && !m_delegate.IsNull();
		}

		[[nodiscard]] ObjectInstance Instance() const noexcept
		{
			return m_delegate.Value;
		}

		/// <summary>
		/// Invokes the delegate with the supplied arguments and returns its result.
		/// </summary>
		ObjectInstance operator()(std::initializer_list<ObjectInstance> args = {}) const
		{
			if (!IsValid())
				throw undefined_behaviour("DelegateRef is not valid");

			MethodSymbol* target = m_delegate.Value.getInfo()->Methods.at(0);
			if (target == nullptr)
				throw undefined_behaviour("Delegate has no target method");

			std::vector<ObjectInstance> callArgs(args);
			return m_runtime->InvokeMethod(target, callArgs.data(), callArgs.size());
		}
	};

	/// <summary>
	/// Wraps an ObjectInstance of delegate type into a callable RAII DelegateRef.
	/// </summary>
	inline DelegateRef CallState::WrapDelegate(ObjectInstance delegate) const
	{
		if (delegate.IsNullInstance())
			throw undefined_behaviour("WrapDelegate: delegate is null");

		if (delegate.getInfo()->Kind != SyntaxKind::DelegateType)
			throw undefined_behaviour("WrapDelegate: object is not a delegate");

		return DelegateRef(Runtimer, delegate);
	}
}
