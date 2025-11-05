#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/InboundVariablesContext.h>

#include <string>
#include <vector>

namespace shard::runtime
{
	class AbstractInterpreter; // Forward declaration
}

namespace shard::syntax::symbols
{
    typedef shard::runtime::ObjectInstance* (*MethodSymbolDelegate)(shard::runtime::AbstractInterpreter* interpreter, shard::runtime::InboundVariablesContext* arguments);

    enum class MethodHandleType
    {
        None,
        ObjectInstance,
        ForeignInterface,
        FunctionPointer,
    };

    class MethodSymbol : public SyntaxSymbol
    {
    public:
        TypeSymbol* ReturnType = nullptr;
        MethodSymbol* OverriddenMethod = nullptr;
        std::vector<ParameterSymbol*> Parameters;

        MethodHandleType HandleType = MethodHandleType::ObjectInstance;
        shard::syntax::nodes::StatementsBlockSyntax* Body = nullptr;
        MethodSymbolDelegate FunctionPointer = nullptr;

        bool IsVirtual = false;
        bool IsOverride = false;
        bool IsStatic = false;

        inline MethodSymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), HandleType(MethodHandleType::None) { }

        inline MethodSymbol(std::wstring name, shard::syntax::nodes::StatementsBlockSyntax* body)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), Body(body), HandleType(MethodHandleType::ObjectInstance) { }

        inline MethodSymbol(std::wstring name, MethodSymbolDelegate delegate)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), FunctionPointer(delegate), HandleType(MethodHandleType::FunctionPointer) { }

        inline ~MethodSymbol() override
        {
            for (ParameterSymbol* parameter : Parameters)
                delete parameter;

            if (FunctionPointer != nullptr)
                FunctionPointer = nullptr;
        }
    };
}
