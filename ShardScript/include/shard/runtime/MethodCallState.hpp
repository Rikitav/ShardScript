#include <span>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/ApplicationDomain.hpp>
#include <shard/ShardScriptAPI.hpp>

namespace shard
{
    struct CallState
    {
        /*
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
    };
}