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

namespace shard::syntax::symbols
{
    typedef SHARD_API shard::runtime::ObjectInstance* (*MethodSymbolDelegate)(MethodSymbol* symbol, shard::runtime::InboundVariablesContext* arguments);

    enum class SHARD_API MethodHandleType
    {
        None,
        ObjectInstance,
        ForeignInterface,
        FunctionPointer,
        AnonymousMethod,
    };

    class SHARD_API MethodSymbol : public SyntaxSymbol
    {
    public:
        TypeSymbol* ReturnType = nullptr;
        MethodSymbol* OverriddenMethod = nullptr;
        std::vector<ParameterSymbol*> Parameters;

        MethodHandleType HandleType = MethodHandleType::ObjectInstance;
        shard::syntax::nodes::StatementsBlockSyntax* Body = nullptr;
        MethodSymbolDelegate FunctionPointer = nullptr;
        std::wstring ForeighInterfacePath;

        bool IsVirtual = false;
        bool IsOverride = false;
        bool IsStatic = false;

        inline MethodSymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(std::wstring name, shard::syntax::nodes::StatementsBlockSyntax* body)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), Body(body), HandleType(MethodHandleType::ObjectInstance) { }

        inline MethodSymbol(std::wstring name, MethodSymbolDelegate delegate)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), FunctionPointer(delegate), HandleType(MethodHandleType::FunctionPointer) { }

        inline MethodSymbol(const MethodSymbol& other) = delete;

        inline virtual ~MethodSymbol() override
        {
            for (ParameterSymbol* parameter : Parameters)
                delete parameter;

            if (FunctionPointer != nullptr)
                FunctionPointer = nullptr;

            if (Body != nullptr)
                delete Body;

            for (ParameterSymbol* param : Parameters)
                delete param;
        }

        template<typename... Args>
        shard::runtime::ObjectInstance* invoke(MethodSymbol* symbol, Args&&... args)
        {
            shard::runtime::InboundVariablesContext* ctx = new shard::runtime::InboundVariablesContext(nullptr);
            for (std::pair<std::wstring, shard::runtime::ObjectInstance*> pair : args)
                ctx->SetVariable(pair.first, pair.second);

            return symbol->FunctionPointer(symbol, ctx);
        }
    };
}
