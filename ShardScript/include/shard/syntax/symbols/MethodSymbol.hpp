#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>

#include <span>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace shard
{
    class ApplicationDomain;
    class ProgramVirtualImage;
    class GarbageCollector;
    class VirtualMachine;
    class ObjectInstance;
    class CallStackFrame;

    using ArgumentsSpan = std::span<ObjectInstance*>;

    struct CallState
    {
        /*
        * DO NOT MODIFY THIS STRUCTURE!
        * ANY CHANGES IN THIS CODE WILL RESULT
        * LOSS OF BACKWARDS COMPATIBILITY AND UNDEFINED BEHAVIOUR!
        */
        
        ApplicationDomain& Domain;
        ProgramVirtualImage& Program;
        VirtualMachine& Runtimer;
        GarbageCollector& Collector;

        CallStackFrame* Frame;
        MethodSymbol* Method;
        ArgumentsSpan& Args;
    };

    typedef SHARD_API shard::ObjectInstance* (*MethodSymbolDelegate)(const CallState& context);

    enum class SHARD_API MethodHandleType
    {
        None,
        Body,
        External,
        Lambda,
    };

    class SHARD_API MethodSymbol : public SyntaxSymbol
    {
    public:
        TypeSymbol* ReturnType = nullptr;
        std::vector<ParameterSymbol*> Parameters;
        uint16_t EvalStackLocalsCount = 0;

        MethodHandleType HandleType = MethodHandleType::Body;
        std::vector<std::byte> ExecutableByteCode;
        MethodSymbolDelegate FunctionPointer = nullptr;

        bool IsStatic = false;

        inline MethodSymbol(const std::wstring& name)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(const std::wstring& name, MethodSymbolDelegate delegate)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), FunctionPointer(delegate), HandleType(MethodHandleType::External) { }

        inline MethodSymbol(const MethodSymbol& other) = delete;

        inline virtual ~MethodSymbol() override
        {
            for (ParameterSymbol* parameter : Parameters)
                delete parameter;

            if (FunctionPointer != nullptr)
                FunctionPointer = nullptr;

            for (ParameterSymbol* param : Parameters)
                delete param;
        }
    };
}
