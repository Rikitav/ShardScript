#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxToken.hpp>

namespace shard
{
    class SemanticModel;
    class DiagnosticsContext;
    class TypeSymbol;
    class InterfaceSymbol;
    class MethodSymbol;
    class PropertySymbol;

    class SHARD_API SemanticValidator
    {
    public:
        static void ValidateInterfaceImplementation(
            TypeSymbol* typeSymbol,
            InterfaceSymbol* interfaceSymbol,
            DiagnosticsContext& diagnostics,
            SyntaxToken errorToken);

        static void ValidateExplicitInterfaceImplementations(
            TypeSymbol* typeSymbol,
            InterfaceSymbol* interfaceSymbol,
            DiagnosticsContext& diagnostics,
            SyntaxToken errorToken);

        static void ValidateAllInterfaceImplementations(
            SemanticModel& semanticModel,
            DiagnosticsContext& diagnostics);
    };
}
