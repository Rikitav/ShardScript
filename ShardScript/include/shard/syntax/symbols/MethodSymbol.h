#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace shard
{
    class ObjectInstance;
    class VirtualMachine;
    class ArgumentsSpan;

    typedef SHARD_API shard::ObjectInstance* (*MethodSymbolDelegate)(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments);

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
