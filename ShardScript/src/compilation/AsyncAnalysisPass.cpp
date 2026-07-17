#include <shard/compilation/AsyncAnalysisPass.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/Expressions/AwaitExpressionSyntax.hpp>

using namespace shard;

namespace
{
    // ========================================================================
    // Variable collection (read-only visitor).
    // ========================================================================
    struct VariableCollector : public SyntaxVisitor
    {
        std::vector<VariableSymbol*> Variables;

        VariableCollector(SemanticModel& model, DiagnosticsContext& diagnostics)
            : SyntaxVisitor(model, diagnostics) { }

        void VisitVariableStatement(VariableStatementSyntax* node) override
        {
            auto symbolOpt = LookupSymbol<VariableSymbol>(node);
            if (symbolOpt.has_value())
                Variables.push_back(symbolOpt.value());

            SyntaxVisitor::VisitVariableStatement(node);
        }

        void VisitForEachStatement(ForEachStatementSyntax* node) override
        {
            auto symbolOpt = LookupSymbol<VariableSymbol>(node);
            if (symbolOpt.has_value())
                Variables.push_back(symbolOpt.value());

            SyntaxVisitor::VisitForEachStatement(node);
        }

        void VisitForInStatement(ForInStatementSyntax* node) override
        {
            auto symbolOpt = LookupSymbol<VariableSymbol>(node);
            if (symbolOpt.has_value())
                Variables.push_back(symbolOpt.value());

            SyntaxVisitor::VisitForInStatement(node);
        }
    };

    // ========================================================================
    static bool IsLoopStatement(SyntaxNode* node)
    {
        return node != nullptr && (
            node->Kind == SyntaxKind::WhileStatement ||
            node->Kind == SyntaxKind::UntilStatement ||
            node->Kind == SyntaxKind::ForStatement ||
            node->Kind == SyntaxKind::ForEachStatement ||
            node->Kind == SyntaxKind::ForInStatement);
    }

    static std::size_t IndexInBlock(const std::vector<std::unique_ptr<StatementSyntax>>& block, StatementSyntax* statement)
    {
        for (std::size_t i = 0; i < block.size(); ++i)
        {
            if (block[i].get() == statement)
                return i;
        }

        return block.size();
    }

    // Finds the statement that should execute after 'statement' completes normally.
    // When 'statement' is the last statement in a loop body, the loop itself is
    // returned so the emitter can run the loop epilogue. When it is inside a nested
    // control structure (if/else/try), the search walks out to the next statement
    // in the enclosing block.
    static StatementSyntax* FindResumeStatement(StatementSyntax* statement)
    {
        StatementsBlockSyntax* block = dynamic_cast<StatementsBlockSyntax*>(statement->Parent);
        if (block == nullptr)
            return nullptr;

        SyntaxNode* owner = block->Parent;
        while (owner != nullptr)
        {
            std::size_t idx = IndexInBlock(block->Statements, statement);
            if (idx + 1 < block->Statements.size())
                return block->Statements[idx + 1].get();

            // 'statement' is the last statement in 'block'; continue from the owner.
            if (IsLoopStatement(owner))
                return static_cast<StatementSyntax*>(owner);

            StatementsBlockSyntax* outerBlock = dynamic_cast<StatementsBlockSyntax*>(owner->Parent);
            if (outerBlock != nullptr)
            {
                statement = static_cast<StatementSyntax*>(owner);
                block = outerBlock;
                owner = block->Parent;
                continue;
            }

            // The owner is not directly inside a block (e.g., an else-clause whose
            // parent is the preceding if-clause). Walk up the clause chain until we
            // reach the statement that actually sits in the enclosing block.
            owner = owner->Parent;
        }

        return nullptr;
    }

    // Await-site scanning (read-only visitor).
    // ========================================================================
    // Replaces the hand-written ScanAsyncStatements/ScanAsyncStatement recursion.
    // Each await that survives hoisting as a top-level Expression/Variable/Return
    // statement becomes a suspension site.
    class AwaitScanVisitor : public SyntaxVisitor
    {
    public:
        AsyncMethodInfo& Info;
        bool Supported = true;

        AwaitScanVisitor(SemanticModel& model, DiagnosticsContext& diagnostics, AsyncMethodInfo& info)
            : SyntaxVisitor(model, diagnostics), Info(info) { }

        void ScanBody(StatementsBlockSyntax* body)
        {
            if (body != nullptr)
                VisitStatementsBlock(body);
        }

        void VisitExpressionStatement(ExpressionStatementSyntax* node) override
        {
            if (node->Expression == nullptr || node->Expression->Kind != SyntaxKind::AwaitExpression)
                return;

            AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->Expression.get());
            if (awaitExpr->AwaiterType == nullptr)
            {
                Supported = false;
                return;
            }

            AwaitSite site;
            site.Expression = awaitExpr;
            site.NextStatement = FindResumeStatement(node);
            site.ActiveTryStack = ActiveTryStack;
            site.EnclosingCatch = CurrentCatch;
            Info.AwaitSites.push_back(site);
        }

        void VisitVariableStatement(VariableStatementSyntax* node) override
        {
            if (node->Expression == nullptr || node->Expression->Kind != SyntaxKind::AwaitExpression)
                return;

            AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->Expression.get());
            Supported = awaitExpr->AwaiterType != nullptr;
            if (!Supported)
                return;

            AwaitSite site;
            site.Expression = awaitExpr;
            site.NextStatement = FindResumeStatement(node);
            site.ActiveTryStack = ActiveTryStack;
            site.EnclosingCatch = CurrentCatch;

            auto symbolOpt = LookupSymbol<VariableSymbol>(node);
            if (symbolOpt.has_value())
                site.ResultVariable = symbolOpt.value();

            Info.AwaitSites.push_back(site);
        }

        void VisitReturnStatement(ReturnStatementSyntax* node) override
        {
            if (node->Expression == nullptr || node->Expression->Kind != SyntaxKind::AwaitExpression)
                return;

            AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(node->Expression.get());
            Supported = awaitExpr->AwaiterType != nullptr;
            if (!Supported)
                return;

            AwaitSite site;
            site.Expression = awaitExpr;
            site.NextStatement = nullptr;
            site.ActiveTryStack = ActiveTryStack;
            site.IsReturnAwait = true;
            site.EnclosingCatch = CurrentCatch;
            Info.AwaitSites.push_back(site);
        }

        void VisitTryStatement(TryStatementSyntax* node) override
        {
            Info.HasTryStatements = true;
            if (node->TryBlock == nullptr)
                return;

            ActiveTryStack.push_back(node);
            VisitStatementsBlock(node->TryBlock.get());
            ActiveTryStack.pop_back();

            if (!Supported)
                return;

            for (const auto& clause : node->CatchClauses)
            {
                if (clause->Body == nullptr)
                    continue;

                CatchClauseSyntax* previousCatch = CurrentCatch;
                CurrentCatch = clause.get();
                VisitStatementsBlock(clause->Body.get());
                CurrentCatch = previousCatch;

                if (!Supported)
                    return;
            }
        }

        void VisitIfStatement(IfStatementSyntax* node) override
        {
            if (node->StatementsBlock == nullptr)
                return;

            VisitStatementsBlock(node->StatementsBlock.get());
            if (!Supported)
                return;

            if (node->NextStatement != nullptr)
                VisitConditionalClause(node->NextStatement.get());
        }

        void VisitUnlessStatement(UnlessStatementSyntax* node) override
        {
            if (node->StatementsBlock == nullptr)
                return;

            VisitStatementsBlock(node->StatementsBlock.get());
            if (!Supported)
                return;

            if (node->NextStatement != nullptr)
                VisitConditionalClause(node->NextStatement.get());
        }

        void VisitElseStatement(ElseStatementSyntax* node) override
        {
            if (node->StatementsBlock == nullptr)
                return;

            VisitStatementsBlock(node->StatementsBlock.get());
        }

        void VisitWhileStatement(WhileStatementSyntax* node) override
        {
            if (!Supported)
                return;

            // Await is not allowed in the loop condition because it is re-evaluated.
            if (node->ConditionExpression != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitExpression(node->ConditionExpression.get());
                if (Info.AwaitSites.size() != before)
                {
                    Supported = false;
                    return;
                }
            }

            if (node->StatementsBlock != nullptr)
                VisitStatementsBlock(node->StatementsBlock.get());
        }

        void VisitUntilStatement(UntilStatementSyntax* node) override
        {
            if (!Supported)
                return;

            if (node->ConditionExpression != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitExpression(node->ConditionExpression.get());
                if (Info.AwaitSites.size() != before)
                {
                    Supported = false;
                    return;
                }
            }

            if (node->StatementsBlock != nullptr)
                VisitStatementsBlock(node->StatementsBlock.get());
        }

        void VisitForStatement(ForStatementSyntax* node) override
        {
            if (!Supported)
                return;

            // Await is not allowed in initializer, condition, or after-repeat because
            // those parts are executed repeatedly and would need their own state.
            if (node->InitializerStatement != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitStatement(node->InitializerStatement.get());
                if (Info.AwaitSites.size() != before)
                {
                    Supported = false;
                    return;
                }
            }

            if (node->ConditionExpression != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitExpression(node->ConditionExpression.get());
                if (Info.AwaitSites.size() != before)
                {
                    Supported = false;
                    return;
                }
            }

            if (node->AfterRepeatStatement != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitStatement(node->AfterRepeatStatement.get());
                if (Info.AwaitSites.size() != before)
                {
                    Supported = false;
                    return;
                }
            }

            if (node->StatementsBlock != nullptr)
                VisitStatementsBlock(node->StatementsBlock.get());
        }

        void VisitForEachStatement(ForEachStatementSyntax* node) override
        {
            if (!Supported)
                return;

            if (node->RangeExpression != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitExpression(node->RangeExpression.get());
                if (Info.AwaitSites.size() != before)
                {
                    Supported = false;
                    return;
                }
            }

            std::size_t before = Info.AwaitSites.size();
            if (node->StatementsBlock != nullptr)
                VisitStatementsBlock(node->StatementsBlock.get());

            if (Info.AwaitSites.size() != before)
                EnumeratorLoops.push_back(node);
        }

        void VisitForInStatement(ForInStatementSyntax* node) override
        {
            if (!Supported)
                return;

            if (node->RangeExpression != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitExpression(node->RangeExpression.get());
                if (Info.AwaitSites.size() != before)
                {
                    Supported = false;
                    return;
                }
            }

            std::size_t before = Info.AwaitSites.size();
            if (node->StatementsBlock != nullptr)
                VisitStatementsBlock(node->StatementsBlock.get());

            if (Info.AwaitSites.size() != before)
                EnumeratorLoops.push_back(node);
        }

        // defer is supported as long as the deferred statement itself does not
        // suspend; an await inside a defer body would have to run on scope exit
        // after the enclosing await already completed, which is not supported.
        void VisitDeferStatement(DeferStatementSyntax* node) override
        {
            if (node->Statement != nullptr)
            {
                std::size_t before = Info.AwaitSites.size();
                VisitStatement(node->Statement.get());
                if (Info.AwaitSites.size() != before)
                    Supported = false;
            }
        }

        std::vector<StatementSyntax*> EnumeratorLoops;

    private:
        CatchClauseSyntax* CurrentCatch = nullptr;
        std::vector<TryStatementSyntax*> ActiveTryStack;
    };

    // ========================================================================
    // Catch-clause collection.
    // ========================================================================
    // Kept as a plain recursive helper rather than a visitor: it intentionally
    // descends only through try-blocks and catch-clause bodies (not loops or
    // conditionals), which does not map to the uniform visitor traversal.
    void CollectMethodCatchClauses(StatementsBlockSyntax* block, std::vector<CatchClauseSyntax*>& clauses)
    {
        if (block == nullptr)
            return;

        for (const auto& statementUnique : block->Statements)
        {
            StatementSyntax* statement = statementUnique.get();
            if (statement->Kind == SyntaxKind::TryStatement)
            {
                TryStatementSyntax* tryStmt = static_cast<TryStatementSyntax*>(statement);
                for (const auto& clauseUnique : tryStmt->CatchClauses)
                    clauses.push_back(clauseUnique.get());

                if (tryStmt->TryBlock != nullptr)
                    CollectMethodCatchClauses(tryStmt->TryBlock.get(), clauses);

                for (const auto& clauseUnique : tryStmt->CatchClauses)
                {
                    if (clauseUnique->Body != nullptr)
                        CollectMethodCatchClauses(clauseUnique->Body.get(), clauses);
                }
            }
        }
    }
}

std::optional<AsyncMethodInfo> AsyncAnalysisPass::Run(MethodSymbol* method, MethodDeclarationSyntax* syntax)
{
    static int counter = 0;

    AsyncMethodInfo info;
    info.Method = method;
    info.Syntax = syntax;

    AwaitScanVisitor scan(Model, Diagnostics, info);
    scan.ScanBody(syntax->Body.get());

    if (!scan.Supported && !Diagnostics.AnyError)
    {
        Diagnostics.ReportError(syntax->IdentifierToken, L"Async method '" + method->Name + L"' contains statements not yet supported by async lowering");
        return std::nullopt;
    }

    if (!scan.Supported)
        return std::nullopt;

    std::wstring className = L"<" + method->Name + L">k__AsyncStateMachine_" + std::to_wstring(counter++);

    SymbolBuilder<ClassSymbol> clsBuilder(Context, className, SymbolTable::Global::Namespace);
    info.StateMachineClass = clsBuilder
        .Implements(SymbolTable::StandardTypes::IAsyncState)
        .DeclareGlobal();

    info.StateField = clsBuilder
        .AddField(L"_state", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE);

    info.TaskField = clsBuilder
        .AddField(L"_task", method->ReturnType, LINK_INSTANCE, ACS_PRIVATE);

    for (std::size_t i = 0; i < scan.EnumeratorLoops.size(); ++i)
    {
        std::wstring fieldName = L"<e>" + std::to_wstring(i);
        FieldSymbol* field = clsBuilder
            .AddField(fieldName, SymbolTable::StandardTypes::IEnumerator, LINK_INSTANCE, ACS_PRIVATE);

        info.EnumeratorFields[scan.EnumeratorLoops[i]] = field;
    }

    for (const AwaitSite& site : info.AwaitSites)
    {
        if (site.EnclosingCatch != nullptr)
        {
            TypeSymbol* exceptionFieldType = SymbolTable::StandardTypes::RuntimeException != nullptr
                ? SymbolTable::StandardTypes::RuntimeException
                : SymbolTable::Primitives::Any;
            info.CurrentExceptionField = clsBuilder
                .AddField(L"_currentException", exceptionFieldType, LINK_INSTANCE, ACS_PRIVATE);
            break;
        }
    }

    if (method->Linking == LINK_INSTANCE && method->Parent != nullptr && method->Parent->IsType())
    {
        info.OuterThisField = clsBuilder
            .AddField(L"_outerThis", static_cast<TypeSymbol*>(method->Parent), LINK_INSTANCE, ACS_PRIVATE);
    }

    for (std::size_t i = 0; i < info.AwaitSites.size(); ++i)
    {
        std::wstring fieldName = L"_awaiter" + std::to_wstring(i);
        FieldSymbol* field = clsBuilder
            .AddField(fieldName, info.AwaitSites[i].Expression->AwaiterType, LINK_INSTANCE, ACS_PRIVATE);

        info.AwaitSites[i].AwaiterField = field;
    }

    info.MoveNext = clsBuilder
        .AddMethod(L"MoveNext", SymbolTable::Primitives::Void, LINK_INSTANCE)
        .IsImplementationOf(SymbolTable::StandardTypes::IAsyncState_MoveNext);

    info.MoveNext->HandleType = MethodHandleType::Body;

    // Lift parameters and local variables into state-machine fields so their values survive await suspensions.
    for (ParameterSymbol* parameter : method->Parameters)
    {
        std::wstring fieldName = L"<p>" + parameter->Name;
        FieldSymbol* field = clsBuilder
            .AddField(fieldName, const_cast<TypeSymbol*>(parameter->Type), LINK_INSTANCE, ACS_PRIVATE)
            .Get();

        AsyncMethodInfo::LiftedParameter lifted;
        lifted.Symbol = parameter;
        lifted.Field = field;
        lifted.OriginalSlot = parameter->SlotIndex;
        lifted.MoveNextSlot = static_cast<std::uint16_t>(info.MoveNext->GetEvalStackArgumentsCount() + info.MoveNext->AddVariableCount());
        info.LiftedParameters.push_back(lifted);
    }

    {
        VariableCollector collector(Model, Diagnostics);
        collector.VisitStatementsBlock(syntax->Body.get());

        for (VariableSymbol* variable : collector.Variables)
        {
            if (variable == nullptr || variable->Type == nullptr)
                continue;

            std::wstring fieldName = L"<l>" + variable->Name;
            FieldSymbol* field = clsBuilder
                .AddField(fieldName, const_cast<TypeSymbol*>(variable->Type), LINK_INSTANCE, ACS_PRIVATE);

            AsyncMethodInfo::LiftedLocal lifted
            {
                .Symbol = variable,
                .Field = field,
                .OriginalSlot = variable->SlotIndex,
                .MoveNextSlot = static_cast<std::uint16_t>(info.MoveNext->GetEvalStackArgumentsCount() + info.MoveNext->AddVariableCount())
            };

            // The original method body is replaced by the factory, so it is safe to permanently retarget local variable slots at MoveNext.
            variable->SlotIndex = lifted.MoveNextSlot;
            info.LiftedLocals.push_back(lifted);
        }
    }

    info.StateMachineCtor = clsBuilder.AddInit().Get();
    info.StateMachineCtor->HandleType = MethodHandleType::Body;

    // Assign local-variable slots in MoveNext for every catch variable introduced inside this async method.
    std::vector<CatchClauseSyntax*> catchClauses;
    CollectMethodCatchClauses(syntax->Body.get(), catchClauses);

    for (CatchClauseSyntax* clause : catchClauses)
    {
        if (clause->Symbol != nullptr)
            clause->Symbol->SlotIndex = info.MoveNext->GetEvalStackArgumentsCount() + info.MoveNext->AddVariableCount();
    }

    method->AsyncStateMachineClass = info.StateMachineClass;
    method->AsyncStateMachineMoveNext = info.MoveNext;

    return info;
}
