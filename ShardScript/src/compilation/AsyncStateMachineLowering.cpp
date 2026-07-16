#include <shard/compilation/AsyncStateMachineLowering.hpp>

#include <shard/compilation/AsyncHoistingPass.hpp>
#include <shard/compilation/AsyncAnalysisPass.hpp>
#include <shard/compilation/AsyncEmissionPass.hpp>

#include <shard/CompilationContext.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>

using namespace shard;

AsyncStateMachineLowering::AsyncStateMachineLowering(CompilationContext& context, SemanticModel& model, SyntaxTree& tree, DiagnosticsContext& diagnostics)
    : Context(context), Model(model), Tree(tree), Diagnostics(diagnostics) { }

void AsyncStateMachineLowering::Prepare()
{
    for (MethodSymbol* method : Model.Table->GetMethodSymbols())
    {
        if (!method->IsAsync || method->HandleType == MethodHandleType::External)
            continue;

        auto nodeOpt = Model.Table->LookupNode(method);
        if (!nodeOpt.has_value())
            continue;

        SyntaxNode* node = nodeOpt.value();
        if (node->Kind != SyntaxKind::MethodDeclaration)
            continue;

        MethodDeclarationSyntax* syntax = static_cast<MethodDeclarationSyntax*>(node);
        if (syntax->Body == nullptr)
            continue;

        // Pass 1: hoist nested awaits to top-level statements.
        AsyncHoistingPass hoisting(Model, Diagnostics);
        if (!hoisting.Rewrite(syntax->Body.get(), method))
        {
            if (!Diagnostics.AnyError)
                Diagnostics.ReportError(syntax->IdentifierToken, L"Async method '" + method->Name + L"' contains expressions not yet supported by async lowering");

            continue;
        }

        // Pass 2: scan for await sites and build the state-machine symbols.
        AsyncAnalysisPass analysis(Context, Model, Diagnostics);
        auto info = analysis.Run(method, syntax);
        if (info.has_value())
            AsyncMethods.push_back(std::move(*info));
    }
}

void AsyncStateMachineLowering::Emit(ProgramVirtualImage& program)
{
    // Pass 3: emit bytecode for each prepared state machine.
    AsyncEmissionPass emission(Model, Diagnostics);
    for (const auto& info : AsyncMethods)
        emission.Run(info, program);
}
