#pragma once
#include <shard/runtime/NativeAsync.hpp>

#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>

namespace shard
{
    namespace detail
    {
        template <typename T>
        struct AsyncValueType
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::Any; }
        };

        template <>
        struct AsyncValueType<std::int64_t>
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::Integer; }
        };

        template <>
        struct AsyncValueType<std::uint8_t>
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::Byte; }
        };

        template <>
        struct AsyncValueType<double>
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::Double; }
        };

        template <>
        struct AsyncValueType<bool>
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::Boolean; }
        };

        template <>
        struct AsyncValueType<wchar_t>
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::Char; }
        };

        template <>
        struct AsyncValueType<std::wstring>
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::String; }
        };

        template <>
        struct AsyncValueType<const wchar_t*>
        {
            static TypeSymbol* Get() { return SymbolTable::Primitives::String; }
        };

        template <typename T>
        struct AsyncValueTypeHelper
        {
            static TypeSymbol* Get(const T&) { return AsyncValueType<T>::Get(); }
        };

        template <>
        struct AsyncValueTypeHelper<ObjectInstance*>
        {
            static TypeSymbol* Get(ObjectInstance* value)
            {
                if (value != nullptr)
                    return const_cast<TypeSymbol*>(value->getInfo());
                return SymbolTable::Primitives::Any;
            }
        };

        template <>
        struct AsyncValueTypeHelper<const ObjectInstance*>
        {
            static TypeSymbol* Get(const ObjectInstance* value)
            {
                if (value != nullptr)
                    return const_cast<TypeSymbol*>(value->getInfo());
                return SymbolTable::Primitives::Any;
            }
        };

        template <typename T>
        inline ObjectInstance* BoxValue(GarbageCollector& collector, T value)
        {
            return collector.FromValue(value);
        }

        template <>
        inline ObjectInstance* BoxValue<ObjectInstance*>(GarbageCollector& collector, ObjectInstance* value)
        {
            (void)collector;
            return value != nullptr ? value : GarbageCollector::NullInstance;
        }

        template <>
        inline ObjectInstance* BoxValue<const ObjectInstance*>(GarbageCollector& collector, const ObjectInstance* value)
        {
            (void)collector;
            return value != nullptr ? const_cast<ObjectInstance*>(value) : GarbageCollector::NullInstance;
        }
    }

    template <typename T>
    AsyncValueScope<T>::AsyncValueScope(std::shared_ptr<detail::AsyncScopeState> state)
        : AsyncScope(std::move(state))
    {
    }

    template <typename T>
    void AsyncValueScope<T>::Complete(T value)
    {
        if (!m_state)
            return;

        ObjectInstance* boxed = detail::BoxValue(*m_state->collector, value);
        m_state->SetValueTaskResult(boxed);
    }

    template <typename T>
    ObjectInstance* DoValueTask(const CallState& ctx, std::function<void(AsyncValueScope<T>&)> work)
    {
        TypeSymbol* resultType = detail::AsyncValueTypeHelper<T>::Get(T{});
        auto state = detail::CreateAsyncScopeState(ctx, resultType);
        ObjectInstance* task = state->task;

        AsyncValueScope<T> scope(std::move(state));
        work(scope);

        return task;
    }

    template <typename T>
    ObjectInstance* CompletedValueTask(const CallState& ctx, T value)
    {
        TypeSymbol* resultType = detail::AsyncValueTypeHelper<T>::Get(value);
        ObjectInstance* task = ctx.Collector.AllocateGeneric(
            SymbolTable::StandardTypes::ValueTask,
            { resultType });

        task->IsTaskLike = true;
        SetTaskState(task, SymbolTable::StandardTypes::ValueTask_StateField, AsyncState::COMPLETED, ctx.Collector);
        task->SetField(SymbolTable::StandardTypes::ValueTask_ResultField->SlotIndex, detail::BoxValue(ctx.Collector, value));
        return task;
    }

    template <typename T>
    ObjectInstance* FaultedValueTask(const CallState& ctx, const std::wstring& message)
    {
        TypeSymbol* resultType = detail::AsyncValueTypeHelper<T>::Get(T{});
        ObjectInstance* task = ctx.Collector.AllocateGeneric(
            SymbolTable::StandardTypes::ValueTask,
            { resultType });

        task->IsTaskLike = true;
        SetTaskState(task, SymbolTable::StandardTypes::ValueTask_StateField, AsyncState::FAULTED, ctx.Collector);
        task->SetField(SymbolTable::StandardTypes::ValueTask_ExceptionField->SlotIndex,
                       CreateRuntimeException(ctx.Collector, message));
        return task;
    }
}
