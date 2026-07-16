#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <optional>

#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/compilation/AsyncStateMachineInfo.hpp>

namespace shard
{
    class CompilationContext;
    class MethodSymbol;
    class MethodDeclarationSyntax;

    /// <summary>
    /// Async lowering pass 2 (analysis + symbol construction).
    ///
    /// The traversal half is a <see cref="SyntaxVisitor"/> that scans the
    /// (already-hoisted) async method body for await suspension sites, lifting
    /// parameters and locals and recording the active try regions at each site.
    /// The construction half then emits the compiler-generated state-machine
    /// class, its fields, its MoveNext method, and the lifted symbol slots that
    /// emission consumes later.
    /// </summary>
    class SHARD_API AsyncAnalysisPass
    {
        CompilationContext& Context;
        SemanticModel& Model;
        DiagnosticsContext& Diagnostics;

    public:
        inline AsyncAnalysisPass(CompilationContext& context, SemanticModel& model, DiagnosticsContext& diagnostics)
            : Context(context), Model(model), Diagnostics(diagnostics) { }

        /// <summary>
        /// Scans the method body and builds its state-machine symbols.  Returns
        /// nullopt (after reporting a diagnostic) if the body contains statements
        /// the lowering cannot yet suspend across.
        /// </summary>
        std::optional<AsyncMethodInfo> Run(MethodSymbol* method, MethodDeclarationSyntax* syntax);
    };
}
