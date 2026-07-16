#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticModel.hpp>

namespace shard
{
    class MethodSymbol;
    class StatementsBlockSyntax;

    /// <summary>
    /// Async lowering pass 1 (tree-rewrite, NOT a visitor).
    ///
    /// Async state-machine lowering splits a method at await expressions that
    /// appear as top-level statements.  To support awaits nested inside larger
    /// expressions this pass hoists them out into compiler-generated variable
    /// statements inserted immediately before the statement that contains them,
    /// preserving evaluation order for unconditionally-evaluated sub-expressions.
    /// Awaits inside short-circuiting/conditional branches are left unsupported
    /// and report a diagnostic.
    /// </summary>
    class SHARD_API AsyncHoistingPass
    {
        SemanticModel& Model;
        DiagnosticsContext& Diagnostics;

    public:
        inline AsyncHoistingPass(SemanticModel& model, DiagnosticsContext& diagnostics)
            : Model(model), Diagnostics(diagnostics) { }

        /// <summary>
        /// Rewrites the statements of an async method body in place, hoisting
        /// nested awaits to top-level variable statements.  Returns false (and
        /// reports diagnostics) if the body contains an unsupported await shape.
        /// </summary>
        bool Rewrite(StatementsBlockSyntax* body, MethodSymbol* method);
    };
}
