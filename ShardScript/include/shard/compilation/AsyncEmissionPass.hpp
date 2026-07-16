#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <cstddef>
#include <vector>

#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/compilation/AsyncStateMachineInfo.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>

namespace shard
{
    class ByteCodeEncoder;

    /// <summary>
    /// Async lowering pass 3 (bytecode generation, NOT a visitor).
    ///
    /// Consumes a prepared <see cref="AsyncMethodInfo"/> and emits the three
    /// generated bodies: an empty state-machine constructor, the factory that
    /// replaces the original async method, and the MoveNext state machine that
    /// splits the method into resume segments at await points.
    /// </summary>
    class SHARD_API AsyncEmissionPass
    {
        SemanticModel& Model;
        DiagnosticsContext& Diagnostics;

    public:
        inline AsyncEmissionPass(SemanticModel& model, DiagnosticsContext& diagnostics)
            : Model(model), Diagnostics(diagnostics) { }

        void Run(const AsyncMethodInfo& info, ProgramVirtualImage& program);

    private:
        void EmitFactoryBody(const AsyncMethodInfo& info, std::vector<std::byte>& code, ByteCodeEncoder& encoder);
        void EmitMoveNextBody(const AsyncMethodInfo& info, std::vector<std::byte>& code, ProgramVirtualImage& program, ByteCodeEncoder& encoder);
        void EmitMoveNextBodyWithRegions(const AsyncMethodInfo& info, std::vector<std::byte>& code, ProgramVirtualImage& program, ByteCodeEncoder& encoder);
    };
}
