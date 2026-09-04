#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <cstdint>
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
        // -------------------------------------------------------------------------
        // Per-type argument extraction.
        // Overload resolution keeps this free of heavy metaprogramming.
        // -------------------------------------------------------------------------
        inline bool               UnwrapArg(ObjectInstance* value, bool*)               { return value->AsBoolean(); }
        inline std::int64_t       UnwrapArg(ObjectInstance* value, std::int64_t*)       { return value->AsInteger(); }
        inline double             UnwrapArg(ObjectInstance* value, double*)             { return value->AsDouble(); }
        inline wchar_t            UnwrapArg(ObjectInstance* value, wchar_t*)            { return value->AsCharacter(); }
        inline std::uint8_t       UnwrapArg(ObjectInstance* value, std::uint8_t*)       { return value->AsByte(); }
        inline const wchar_t*     UnwrapArg(ObjectInstance* value, const wchar_t**)     { return value->AsString(); }
        inline std::wstring       UnwrapArg(ObjectInstance* value, std::wstring*)       { return std::wstring(value->AsString()); }
        inline ObjectInstance*    UnwrapArg(ObjectInstance* value, ObjectInstance**)    { return value; }

        template<typename T>
        inline T* UnwrapArg(ObjectInstance* value, T**)
        {
            if (value->IsNullInstance())
                return nullptr;

            return static_cast<T*>(value->AsNint());
        }

        template<typename T>
        inline T UnwrapArgAtIndex(const CallState& context, std::size_t index)
        {
            if (index >= context.Args.size())
            {
                throw std::runtime_error("Argument index out of range");
            }

            ObjectInstance* value = context.Args[index];
            if (value == nullptr)
            {
                throw std::runtime_error("Argument is null");
            }

            if constexpr (!std::is_same_v<T, ObjectInstance*> && !std::is_pointer_v<T>)
            {
                if (value == GarbageCollector::NullInstance)
                {
                    throw std::runtime_error("Argument is NullInstance");
                }
            }

            T* tag = nullptr;
            return UnwrapArg(value, tag);
        }

        template<typename... TArgs, std::size_t... Indices>
        inline std::tuple<TArgs...> GetArgsImpl(const CallState& context, std::index_sequence<Indices...>)
        {
            return std::make_tuple(UnwrapArgAtIndex<TArgs>(context, Indices)...);
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

    /// <summary>
    /// Extracts the method arguments from a CallState into a typed tuple.
    /// Supported types: bool, std::int64_t, double, wchar_t, std::uint8_t,
    /// const wchar_t*, std::wstring, ObjectInstance*, and any native pointer T*.
    /// </summary>
    template<typename... TArgs>
    inline std::tuple<TArgs...> GetArgs(const CallState& context)
    {
        constexpr std::size_t expectedCount = sizeof...(TArgs);
        if (context.Args.size() != expectedCount)
        {
            throw std::runtime_error(
                "Expected " + std::to_string(expectedCount) +
                " arguments, got " + std::to_string(context.Args.size()));
        }

        return detail::GetArgsImpl<TArgs...>(context, std::index_sequence_for<TArgs...>{});
    }

    /// <summary>
    /// Constructs a new instance of the given type using the supplied constructor.
    /// The returned object has an owning reference (reference count is 1).
    /// </summary>
    inline ObjectInstance* NewObject(const CallState& context, TypeSymbol* type, ConstructorSymbol* ctor, std::initializer_list<ObjectInstance*> args = {})
    {
        if (type == nullptr)
            throw std::runtime_error("NewObject: type is null");

        if (ctor == nullptr)
            throw std::runtime_error("NewObject: constructor is null");

        ObjectInstance* instance = context.Collector.AllocateInstance(type);
        if (instance == nullptr)
            throw std::runtime_error("NewObject: allocation failed");

        // Keep the object alive through the constructor call.
        instance->IncrementReference();

        try
        {
            std::vector<ObjectInstance*> callArgs;
            callArgs.reserve(1 + args.size());
            callArgs.push_back(instance);
            callArgs.insert(callArgs.end(), args.begin(), args.end());

            context.Runtimer.InvokeMethod(ctor, callArgs.data(), callArgs.size());
        }
        catch (...)
        {
            instance->DecrementReference();
            throw;
        }

        return instance;
    }

    /// <summary>
    /// Constructs a new instance of the given type using its parameterless constructor.
    /// The returned object has an owning reference (reference count is 1).
    /// </summary>
    inline ObjectInstance* NewObject(const CallState& context, TypeSymbol* type)
    {
        ConstructorSymbol* ctor = detail::FindParameterlessConstructor(type);
        if (ctor == nullptr)
            throw std::runtime_error("Type has no parameterless constructor");

        return NewObject(context, type, ctor, {});
    }

    /// <summary>
    /// Constructs a new instance of a generic class.
    /// The returned object has an owning reference (reference count is 1).
    /// </summary>
    inline ObjectInstance* NewObject(const CallState& context, ClassSymbol* cls, const std::vector<TypeSymbol*>& typeArgs, std::initializer_list<ObjectInstance*> args = {})
    {
        if (cls == nullptr)
            throw std::runtime_error("NewObject: class is null");

        ObjectInstance* instance = context.Collector.AllocateGeneric(cls, typeArgs);
        if (instance == nullptr)
            throw std::runtime_error("NewObject: allocation failed");

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
            throw std::runtime_error("NewObject: no constructor with matching parameter count");

        instance->IncrementReference();

        try
        {
            std::vector<ObjectInstance*> callArgs;
            callArgs.reserve(1 + args.size());
            callArgs.push_back(instance);
            callArgs.insert(callArgs.end(), args.begin(), args.end());

            context.Runtimer.InvokeMethod(ctor, callArgs.data(), callArgs.size());
        }
        catch (...)
        {
            instance->DecrementReference();
            throw;
        }

        return instance;
    }

    /// <summary>
    /// Invokes a static method.
    /// </summary>
    inline ObjectInstance* CallMethod(const CallState& context, MethodSymbol* method, std::initializer_list<ObjectInstance*> args = {})
    {
        if (method == nullptr)
            throw std::runtime_error("CallMethod: method is null");

        std::vector<ObjectInstance*> callArgs(args);
        return context.Runtimer.InvokeMethod(method, callArgs.data(), callArgs.size());
    }

    /// <summary>
    /// Invokes an instance method on the given receiver.
    /// </summary>
    inline ObjectInstance* CallMethod(const CallState& context, MethodSymbol* method, ObjectInstance* receiver, std::initializer_list<ObjectInstance*> args = {})
    {
        if (method == nullptr)
            throw std::runtime_error("CallMethod: method is null");

        std::vector<ObjectInstance*> callArgs;
        callArgs.reserve(1 + args.size());
        callArgs.push_back(receiver);
        callArgs.insert(callArgs.end(), args.begin(), args.end());

        return context.Runtimer.InvokeMethod(method, callArgs.data(), callArgs.size());
    }

    /// <summary>
    /// Reads a field value.
    /// </summary>
    inline ObjectInstance* GetField(ObjectInstance* obj, FieldSymbol* field)
    {
        if (obj == nullptr)
            throw std::runtime_error("GetField: object is null");

        if (field == nullptr)
            throw std::runtime_error("GetField: field is null");

        return StableRef(obj->GetField(field));
    }

    /// <summary>
    /// Writes a field value.
    /// </summary>
    inline void SetField(ObjectInstance* obj, FieldSymbol* field, ObjectInstance* value)
    {
        if (obj == nullptr)
            throw std::runtime_error("SetField: object is null");

        if (field == nullptr)
            throw std::runtime_error("SetField: field is null");

        obj->SetField(field, value);
    }

    /// <summary>
    /// Invokes a property getter.
    /// </summary>
    inline ObjectInstance* GetProperty(const CallState& context, ObjectInstance* obj, PropertySymbol* prop)
    {
        if (obj == nullptr)
            throw std::runtime_error("GetProperty: object is null");
        
        if (prop == nullptr)
            throw std::runtime_error("GetProperty: property is null");

        if (prop->Getter == nullptr)
            throw std::runtime_error("GetProperty: property has no getter");

        return CallMethod(context, prop->Getter, obj, {});
    }

    /// <summary>
    /// Invokes a property setter.
    /// </summary>
    inline void SetProperty(const CallState& context, ObjectInstance* obj, PropertySymbol* prop, ObjectInstance* value)
    {
        if (obj == nullptr)
            throw std::runtime_error("SetProperty: object is null");

        if (prop == nullptr)
            throw std::runtime_error("SetProperty: property is null");

        if (prop->Setter == nullptr)
            throw std::runtime_error("SetProperty: property has no setter");

        CallMethod(context, prop->Setter, obj, { value });
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

        DelegateRef(VirtualMachine& runtime, ObjectInstance* delegate)
            : m_runtime(&runtime), m_delegate(delegate) { }

        DelegateRef(const DelegateRef&) = delete;
        DelegateRef& operator=(const DelegateRef&) = delete;

        DelegateRef(DelegateRef&&) = default;
        DelegateRef& operator=(DelegateRef&&) = default;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return m_runtime != nullptr && m_delegate.Instance != nullptr;
        }

        [[nodiscard]] ObjectInstance* Instance() const noexcept
        {
            return m_delegate.Instance;
        }

        /// <summary>
        /// Invokes the delegate with the supplied arguments and returns its result.
        /// </summary>
        ObjectInstance* operator()(std::initializer_list<ObjectInstance*> args = {}) const
        {
            if (!IsValid())
                throw std::runtime_error("DelegateRef is not valid");

            MethodSymbol* target = m_delegate.Instance->DelegateTarget;
            if (target == nullptr)
                throw std::runtime_error("Delegate has no target method");

            std::vector<ObjectInstance*> callArgs(args);
            return m_runtime->InvokeMethod(target, callArgs.data(), callArgs.size());
        }

        operator ObjectInstance*() const { return m_delegate.Instance; }
        ObjectInstance* operator->() const { return m_delegate.Instance; }
    };

    /// <summary>
    /// Wraps an ObjectInstance of delegate type into a callable RAII DelegateRef.
    /// </summary>
    inline DelegateRef WrapDelegate(const CallState& context, ObjectInstance* delegate)
    {
        if (delegate == nullptr)
            throw std::runtime_error("WrapDelegate: delegate is null");

        if (delegate->getInfo()->Kind != SyntaxKind::DelegateType)
            throw std::runtime_error("WrapDelegate: object is not a delegate");

        return DelegateRef(context.Runtimer, delegate);
    }
}
