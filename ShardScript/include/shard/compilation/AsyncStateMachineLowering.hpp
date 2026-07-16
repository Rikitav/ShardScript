#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <memory>
#include <vector>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/compilation/AsyncStateMachineInfo.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>

namespace shard
{
    class CompilationContext;

    /// <summary>
    /// Orchestrates async method lowering as a linear three-pass pipeline,
    /// mirroring <c>SemanticAnalyzer</c>'s construct-each-pass-then-run style:
    /// <list type="number">
    /// <item><see cref="AsyncHoistingPass"/> rewrites the body so nested awaits
    /// become top-level statements.</item>
    /// <item><see cref="AsyncAnalysisPass"/> scans the rewritten body for await
    /// sites and builds the state-machine symbols.</item>
    /// <item><see cref="AsyncEmissionPass"/> emits the factory, MoveNext, and
    /// constructor bytecode.</item>
    /// </list>
    /// Hoisting and analysis run in <see cref="Prepare"/> (before layout); emission
    /// runs in <see cref="Emit"/> (after layout, when field slots are known).
    /// </summary>
    class SHARD_API AsyncStateMachineLowering
    {
        CompilationContext& Context;
        SemanticModel& Model;
        DiagnosticsContext& Diagnostics;
        SyntaxTree& Tree;

        std::vector<AsyncMethodInfo> AsyncMethods;

    public:
        AsyncStateMachineLowering(CompilationContext& context, SemanticModel& model, SyntaxTree& tree, DiagnosticsContext& diagnostics);

        /// <summary>
        /// Creates state-machine symbols for every async method.  Must be called before
        /// layout generation so that the generated fields receive slot indices.
        /// </summary>
        void Prepare();

        /// <summary>
        /// Emits bytecode for the generated MoveNext methods and the factory bodies of
        /// the original async methods.  Must be called after layout generation.
        /// </summary>
        void Emit(ProgramVirtualImage& program);
    };
}
