#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>

namespace shard
{
    /// <summary>
    /// Post-emission bytecode pass that computes the evaluation-region requirements
    /// of a method body: Layout.MaxEvalDepth (maximum number of eval entries live
    /// above the locals region at any point) and Layout.EvalSlotPayload (largest
    /// inline payload any instruction pushes).
    ///
    /// The pass builds a basic-block CFG over the emitted bytecode and runs a
    /// max-monotone dataflow over per-instruction eval-stack effects. It is
    /// deliberately over-approximating: modeled pops are never larger than the
    /// real pops, so the computed depth is always >= the real one and the frame
    /// is never under-allocated.
    ///
    /// Exception handlers are entered with the eval stack truncated to the locals
    /// region plus the thrown exception (depth 1); ENTER_TRY adds a handler edge
    /// with that fixed entry depth. Deferred expression bodies are drained at
    /// statement boundaries where the eval stack is balanced, and exceptionally
    /// after the same truncation, so they are analyzed at entry depth 0.
    ///
    /// On malformed bytecode the pass leaves MaxEvalDepth at 0, which the
    /// Stage 4 frame allocator must treat as "no static eval region".
    /// </summary>
    class SHARD_API FrameLayoutPass
    {
    public:
        static void Run(MethodSymbol& method);
    };
}
