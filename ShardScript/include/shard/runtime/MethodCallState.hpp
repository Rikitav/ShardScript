#pragma once
#include <span>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/RuntimeException.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
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

namespace
{
	static shard::TypeSymbol* GetConstructedTypeDefinition(shard::TypeSymbol* type)
	{
		if (type == nullptr)
			return nullptr;

		if (type->Kind == shard::SyntaxKind::GenericType)
			return static_cast<shard::GenericTypeSymbol*>(type)->UnderlayingType;

		return type;
	}

	static shard::ConstructorSymbol* FindParameterlessConstructor(shard::TypeSymbol* type)
	{
		shard::TypeSymbol* definition = GetConstructedTypeDefinition(type);
		if (definition == nullptr)
			return nullptr;

		for (shard::ConstructorSymbol* ctor : definition->Constructors)
		{
			if (ctor->Parameters.empty())
				return ctor;
		}

		return nullptr;
	}
}

namespace shard
{
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
			: m_runtime(&runtime), m_delegate(delegate) {
		}

		DelegateRef(const DelegateRef&) = delete;
		DelegateRef& operator=(const DelegateRef&) = delete;

		DelegateRef(DelegateRef&&) = default;
		DelegateRef& operator=(DelegateRef&&) = default;

		[[nodiscard]] inline bool IsValid() const noexcept
		{
			return m_runtime != nullptr && !m_delegate.IsNull();
		}

		[[nodiscard]] inline ObjectInstance Instance() const noexcept
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
	/// TODO: Add summary
	/// </summary>
	class InvokeResult
	{
		friend class CallState;

		const CallState* m_callState;
		const MethodSymbol* m_callingMethod;
		const TypeShape* m_returnType;

		std::byte* m_returned = nullptr;
		std::size_t m_retSize = 0;

	public:
		InvokeResult(const CallState* callState, const MethodSymbol* callingMethod, const TypeShape* returnType);
		~InvokeResult();

		InvokeResult(const InvokeResult&) = delete;
		InvokeResult& operator=(const InvokeResult&) = delete;

		InvokeResult(InvokeResult&&) = default;
		InvokeResult& operator=(InvokeResult&&) = default;

		explicit operator bool() const;
		bool IsOk() const;

		[[nodiscard]] ObjectInstance Value() const;
		[[nodiscard]] ObjectInstance Exception() const;
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

		CallStackFrame *const Frame;
		MethodSymbol *const Method;
		const std::span<ObjectInstance> Args;

		mutable bool ReturnPlaced = false;

		void WriteReturned(const ObjectInstance& value) const;
		void PlaceReturned(ObjectInstance value) const;
		ObjectInstance ReturnView() const;

		template<typename T>
		void WriteReturn(const T& value) const;

		template<typename... TArgs>
		std::tuple<TArgs...> GetArgs() const;

		/// <summary>
		/// Re-raise on this callback's frame so unwinding continues when the callback returns.
		/// </summary>
		/// <param name="exception"></param>
		void Propagate(ObjectInstance exception) const;

		int TryInvokeMethodImpl(
			const MethodSymbol* method,
			const ObjectInstance* argv,
			const std::size_t argc,
			const TypeSymbol* const*,
			const std::size_t typec,
			void* returnBuffer,
			const std::size_t returnBufferSize
		) const;

		[[nodiscard]] InvokeResult TryInvokeMethod(MethodSymbol* method) const;
		[[nodiscard]] InvokeResult TryInvokeMethod(MethodSymbol* method, const std::span<ObjectInstance> args) const;
		[[nodiscard]] InvokeResult TryInvokeMethod(MethodSymbol* method, const std::initializer_list<ObjectInstance> args) const;
		[[nodiscard]] InvokeResult TryInvokeMethod(MethodSymbol* method, const std::span<ObjectInstance> args, const std::span<TypeSymbol*> typeArguments) const;
		[[nodiscard]] InvokeResult TryInvokeMethod(MethodSymbol* method, const std::initializer_list<ObjectInstance> args, const std::initializer_list<TypeSymbol*> typeArguments) const;

		[[nodiscard]] InvokeResult NewObject(TypeSymbol* type) const;
		[[nodiscard]] InvokeResult NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::span<ObjectInstance> args) const;
		[[nodiscard]] InvokeResult NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::initializer_list<ObjectInstance> args) const;
		[[nodiscard]] InvokeResult NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::span<ObjectInstance> args, const std::span<TypeSymbol*> typeArgs) const;
		[[nodiscard]] InvokeResult NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::initializer_list<ObjectInstance> args, const std::initializer_list<TypeSymbol*> typeArgs) const;

		[[nodiscard]] InvokeResult GetProperty(ObjectInstance obj, PropertySymbol* prop) const;
		[[nodiscard]] InvokeResult SetProperty(ObjectInstance obj, PropertySymbol* prop, ObjectInstance value) const;

		[[nodiscard]] DelegateRef WrapDelegate(ObjectInstance delegate) const;
	};

	template<typename T>
	inline void CallState::WriteReturn(const T& value) const
	{
		static_assert(std::is_trivially_copyable_v<T>, "WriteReturn requires a trivially copyable type");

		if (Frame->ReturnShape() == nullptr)
			throw undefined_behaviour("WriteReturn: method does not return a by-value type");

		if (sizeof(T) > Frame->ReturnShape()->Size)
			throw undefined_behaviour("WriteReturn: value does not fit the return slot");

		if (ReturnPlaced)
			throw undefined_behaviour("WriteReturn: return value already placed");

		std::memcpy(Frame->ReturnSlotMemory(), &value, sizeof(T));
		ReturnPlaced = true;
	}

	namespace detail
	{
		inline bool				UnwrapArg(ObjectInstance value, bool*)				{ return value.AsBoolean(); }
		inline std::int64_t		UnwrapArg(ObjectInstance value, std::int64_t*)		{ return value.AsInteger(); }
		inline double			UnwrapArg(ObjectInstance value, double*)			{ return value.AsDouble(); }
		inline wchar_t			UnwrapArg(ObjectInstance value, wchar_t*)			{ return value.AsCharacter(); }
		inline std::uint8_t		UnwrapArg(ObjectInstance value, std::uint8_t*)		{ return value.AsByte(); }
		inline const wchar_t*	UnwrapArg(ObjectInstance value, const wchar_t**)	{ return value.AsString(); }
		inline std::wstring     UnwrapArg(ObjectInstance value, std::wstring*)		{ return std::wstring(value.AsString()); }
		inline ObjectInstance   UnwrapArg(ObjectInstance value, ObjectInstance*)	{ return value; }

		template<typename T>
		inline T* UnwrapArg(ObjectInstance value, T**)
		{
			if (value.IsNullInstance())
				return nullptr;

			return static_cast<T*>(value.AsNint());
		}

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
}
