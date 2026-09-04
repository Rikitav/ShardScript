#pragma once
#include <span>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/ApplicationDomain.hpp>
#include <shard/ShardScriptAPI.hpp>

#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace shard
{
    typedef std::span<ObjectInstance*> ArgumentsSpan;

    struct ReturnTargetInfo
    {
        std::byte* Slot = nullptr;
        TypeShape* Shape = nullptr; // non-null => inline (by-value) return slot
    };

    struct CallState
    {
        /*
        * ABI v2 — FOREIGN FUNCTIONS ONVOKATION STATE.
        * DO NOT MODIFY THIS STRUCTURE!
        * ANY CHANGES IN THIS CODE WILL RESULT LOSS OF BACKWARDS COMPATIBILITY AND UNDEFINED BEHAVIOUR!
        * IN CASE OF CHANGES, RECOMPILE DEPENDENT LIBRARIES!
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
                throw std::runtime_error("WriteReturn: method does not return a by-value type");

            if (sizeof(T) > ReturnTarget.Shape->Size)
                throw std::runtime_error("WriteReturn: value does not fit the return slot");

            if (ReturnPlaced)
                throw std::runtime_error("WriteReturn: return value already placed");

            std::memcpy(ReturnTarget.Slot + CallStackFrame::SlotHeaderBytes, &value, sizeof(T));
            ReturnPlaced = true;
        }

        void WriteReturn(const ObjectInstance& value) const
        {
            if (ReturnTarget.Shape == nullptr)
                throw std::runtime_error("WriteReturn: method does not return a by-value type");

            if (ReturnPlaced)
                throw std::runtime_error("WriteReturn: return value already placed");

            std::memcpy(ReturnTarget.Slot + CallStackFrame::SlotHeaderBytes, value.getMemory(), ReturnTarget.Shape->Size);
            ReturnPlaced = true;
        }

        ObjectInstance ReturnView() const
        {
            if (ReturnTarget.Shape == nullptr)
                throw std::runtime_error("ReturnView: method does not return a by-value type");

            ObjectInstance view(Method->ReturnType, ReturnTarget.Shape, ReturnTarget.Slot + CallStackFrame::SlotHeaderBytes);
            view.IsView = true;
            ReturnPlaced = true;
            return view;
        }

        void PlaceReturned(ObjectInstance* value) const
        {
            if (ReturnTarget.Shape != nullptr)
                throw std::runtime_error("PlaceReturned: library returned a pointer for a by-value method");

            if (Method->ReturnType == nullptr || Method->ReturnType == SymbolTable::Primitives::Void)
                throw std::runtime_error("PlaceReturned: method returns void");

            if (ReturnPlaced)
                throw std::runtime_error("PlaceReturned: return value already placed");

            *reinterpret_cast<ObjectInstance**>(ReturnTarget.Slot + CallStackFrame::SlotHeaderBytes) = value;
            ReturnPlaced = true;
        }
    };
}
