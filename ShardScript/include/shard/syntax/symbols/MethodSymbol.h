#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/InboundVariablesContext.h>

#include <string>
#include <vector>
#include <utility>

namespace shard
{
    typedef SHARD_API shard::ObjectInstance* (*MethodSymbolDelegate)(const MethodSymbol* symbol, shard::InboundVariablesContext* arguments);

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
        //shard::StatementsBlockSyntax* Body = nullptr;
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

        template<typename... Args>
        shard::ObjectInstance* invoke(Args&&... args)
        {
            shard::InboundVariablesContext* ctx = new shard::InboundVariablesContext(nullptr);
            for (std::pair<std::wstring, shard::ObjectInstance*> pair : args)
                ctx->SetVariable(pair.first, pair.second);

            return this->FunctionPointer(this, ctx);
        }
    };
}
