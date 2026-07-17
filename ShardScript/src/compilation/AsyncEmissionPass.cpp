#include <shard/compilation/AsyncEmissionPass.hpp>

#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/AbstractEmiter.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/Expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace shard;

namespace
{
    struct ParameterSlotRemapper
    {
        const AsyncMethodInfo& Info;
        std::vector<std::uint16_t> OriginalSlots;

        ParameterSlotRemapper(const AsyncMethodInfo& info) : Info(info)
        {
            OriginalSlots.reserve(Info.LiftedParameters.size());
            for (const auto& lifted : Info.LiftedParameters)
            {
                OriginalSlots.push_back(lifted.Symbol->SlotIndex);
                lifted.Symbol->SlotIndex = lifted.MoveNextSlot;
            }
        }

        ~ParameterSlotRemapper()
        {
            for (std::size_t i = 0; i < Info.LiftedParameters.size(); ++i)
                Info.LiftedParameters[i].Symbol->SlotIndex = OriginalSlots[i];
        }
    };

    static bool IsTaskReturnType(TypeSymbol* returnType)
    {
        if (returnType == nullptr)
            return false;

        return returnType == SymbolTable::StandardTypes::Task;
    }

    static bool IsValueTaskReturnType(TypeSymbol* returnType)
    {
        if (returnType == nullptr)
            return false;

        if (returnType == SymbolTable::StandardTypes::ValueTask)
            return true;

        if (returnType->Kind == SyntaxKind::GenericType)
        {
            GenericTypeSymbol* genericReturnType = static_cast<GenericTypeSymbol*>(returnType);
            if (genericReturnType->UnderlayingType == SymbolTable::StandardTypes::ValueTask)
                return true;
        }

        return false;
    }

    static bool IsAsyncReturnType(TypeSymbol* returnType)
    {
        return IsTaskReturnType(returnType) || IsValueTaskReturnType(returnType);
    }

    static TypeSymbol* GetValueTaskElementType(TypeSymbol* returnType)
    {
        if (!IsValueTaskReturnType(returnType))
            return nullptr;

        GenericTypeSymbol* generic = dynamic_cast<GenericTypeSymbol*>(returnType);
        if (generic != nullptr && SymbolTable::StandardTypes::ValueTask_T != nullptr)
            return generic->SubstituteTypeParameters(SymbolTable::StandardTypes::ValueTask_T);

        return nullptr;
    }

    static MethodSymbol* FindCompleteMethod(TypeSymbol* taskType)
    {
        if (taskType == nullptr)
            return nullptr;

        if (IsTaskReturnType(taskType))
            return SymbolTable::StandardTypes::Task_Complete;

        /*
        if (IsValueTaskReturnType(taskType))
            return SymbolTable::StandardTypes::ValueTask_Complete;
        */

        return nullptr;
    }

    static MethodSymbol* FindSetResultMethod(TypeSymbol* taskType)
    {
        if (taskType == nullptr)
            return nullptr;

        /*
        if (IsTaskReturnType(taskType))
            return SymbolTable::StandardTypes::Task_SetResult;
        */

        if (IsValueTaskReturnType(taskType))
            return SymbolTable::StandardTypes::ValueTask_SetResult;

        return nullptr;
    }

    static MethodSymbol* FindInternalRootMethod(TypeSymbol* taskType)
    {
        if (taskType == nullptr)
            return nullptr;

        if (IsTaskReturnType(taskType))
            return SymbolTable::StandardTypes::Task_InternalRoot;

        if (IsValueTaskReturnType(taskType))
            return SymbolTable::StandardTypes::ValueTask_InternalRoot;

        return nullptr;
    }

    static MethodSymbol* FindSetExceptionMethod(TypeSymbol* taskType)
    {
        if (taskType == nullptr)
            return nullptr;

        if (IsTaskReturnType(taskType))
            return SymbolTable::StandardTypes::Task_SetException;

        if (IsValueTaskReturnType(taskType))
            return SymbolTable::StandardTypes::ValueTask_SetException;

        return nullptr;
    }

    static void PatchJumpTarget(std::vector<std::byte>& code, std::size_t jumpInstructionOffset, std::size_t targetAddress)
    {
        // EmitJump/EmitJumpTrue/EmitJumpFalse write the opcode followed by the absolute address.
        constexpr std::size_t addressOffset = sizeof(OpCode);
        ByteCodeEncoder::PasteData(code, jumpInstructionOffset + addressOffset, &targetAddress, sizeof(std::size_t));
    }

    static void RestoreLiftedParametersAndLocals(const AsyncMethodInfo& info, std::vector<std::byte>& code, ByteCodeEncoder& encoder)
    {
        for (const auto& lifted : info.LiftedParameters)
        {
            encoder.EmitLoadVarible(code, 0);                     // this
            encoder.EmitLoadField(code, lifted.Field->SlotIndex); // field value
            encoder.EmitStoreVarible(code, lifted.MoveNextSlot);  // local slot
        }

        for (const auto& lifted : info.LiftedLocals)
        {
            encoder.EmitLoadVarible(code, 0);
            encoder.EmitLoadField(code, lifted.Field->SlotIndex);
            encoder.EmitStoreVarible(code, lifted.MoveNextSlot);
        }
    }

    static void SaveLiftedParametersAndLocals(const AsyncMethodInfo& info, std::vector<std::byte>& code, ByteCodeEncoder& encoder)
    {
        for (const auto& lifted : info.LiftedParameters)
        {
            encoder.EmitLoadVarible(code, 0);                      // this
            encoder.EmitLoadVarible(code, lifted.MoveNextSlot);    // value
            encoder.EmitStoreField(code, lifted.Field->SlotIndex);
        }

        for (const auto& lifted : info.LiftedLocals)
        {
            encoder.EmitLoadVarible(code, 0);                      // this
            encoder.EmitLoadVarible(code, lifted.MoveNextSlot);    // value
            encoder.EmitStoreField(code, lifted.Field->SlotIndex);
        }
    }

    // ========================================================================
    // MoveNext emission
    // ========================================================================
    // The state-machine MoveNext body is emitted by a cluster of mutually
    // recursive static functions that share the bytecode buffer, the encoder,
    // the await-index map, and the user try-region table.  MoveNextEmitter
    // bundles those references so the functions can be free (file-local)
    // rather than capturing lambdas.

    struct ExceptionRegion
    {
        TryStatementSyntax* Try = nullptr;
        std::size_t HandlerAddress = 0;
        std::size_t AfterTryAddress = 0;
        std::vector<std::size_t> EnterBacktracks;
        std::vector<std::size_t> AfterTryBacktracks;
        std::vector<std::size_t> CatchBodyEndBacktracks;
    };

    struct LoopLabels
    {
        std::size_t LoopStart = 0;
        std::size_t BlockEnd = 0;
        std::size_t LoopEnd = 0;
        std::vector<std::size_t> BreakBacktracks;
        std::vector<std::size_t> ContinueBacktracks;
    };

    struct MoveNextEmitter
    {
        SemanticModel& Model;
        DiagnosticsContext& Diagnostics;
        const AsyncMethodInfo& Info;
        std::vector<std::byte>& Code;
        ProgramVirtualImage& Program;
        ByteCodeEncoder& Encoder;
        std::unordered_map<AwaitExpressionSyntax*, std::size_t>& AwaitIndexMap;
        std::unordered_map<TryStatementSyntax*, ExceptionRegion>& Regions;
        std::vector<LoopLabels> LoopStack;
        std::unordered_map<StatementSyntax*, LoopLabels> LoopLabelsByStatement;
    };

    using Continuation = std::function<bool()>;

    static std::size_t IndexInBlock(const std::vector<std::unique_ptr<StatementSyntax>>& block, StatementSyntax* statement);
    static void ConsumePreviousAwaiter(MoveNextEmitter& ctx, std::size_t previousAwaitIndex);
    static void EmitStatementInto(MoveNextEmitter& ctx, MethodSymbol* target, StatementSyntax* statement);
    static void EmitExpressionInto(MoveNextEmitter& ctx, MethodSymbol* target, ExpressionSyntax* expression);
    static void EmitAwaitSuspension(MoveNextEmitter& ctx, std::size_t awaitIndex, std::vector<std::size_t>& nextResumeBacktracks);
    static void EmitTaskComplete(MoveNextEmitter& ctx);
    static bool EmitBlock(MoveNextEmitter& ctx, StatementsBlockSyntax* block, std::size_t startIndex, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries);
    static bool EmitStatementSequence(MoveNextEmitter& ctx, StatementsBlockSyntax* block, std::size_t startIndex, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after);
    static bool EmitStatement(MoveNextEmitter& ctx, StatementSyntax* statement, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after);
    static bool EmitConditionalChain(MoveNextEmitter& ctx, ConditionalClauseBaseSyntax* clause, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation outerAfter);
    static bool EmitConditionalChainImpl(MoveNextEmitter& ctx, ConditionalClauseBaseSyntax* clause, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, std::vector<std::size_t>& afterJumpBacktracks, Continuation chainAfter);
    static bool EmitConditionalChainBody(MoveNextEmitter& ctx, ConditionalClauseBaseSyntax* clause, StatementsBlockSyntax* body, std::size_t startIndex, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, std::vector<std::size_t>& afterJumpBacktracks, Continuation chainAfter);
    static bool EmitStatementContinuation(MoveNextEmitter& ctx, StatementSyntax* statement, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after);
    static void EmitCatchClauses(MoveNextEmitter& ctx, TryStatementSyntax* tryStmt, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, std::vector<std::size_t>& outClauseStarts, std::vector<std::size_t>& outBodyEndBacktracks, std::vector<std::optional<std::size_t>>& outFilterFailBacktracks);
    static bool EmitAfterTry(MoveNextEmitter& ctx, TryStatementSyntax* tryStmt, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after);
    static bool EmitTryStatement(MoveNextEmitter& ctx, TryStatementSyntax* tryStmt, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after);

    static bool EmitLoopStatement(MoveNextEmitter& ctx, StatementSyntax* loop, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after);
    static bool EmitBreakStatement(MoveNextEmitter& ctx, BreakStatementSyntax* node);
    static bool EmitContinueStatement(MoveNextEmitter& ctx, ContinueStatementSyntax* node);

    static std::size_t IndexInBlock(const std::vector<std::unique_ptr<StatementSyntax>>& block, StatementSyntax* statement)
    {
        for (std::size_t i = 0; i < block.size(); ++i)
        {
            if (block[i].get() == statement)
                return i;
        }

        return block.size();
    }

    static void EmitStatementInto(MoveNextEmitter& ctx, MethodSymbol* target, StatementSyntax* statement);
    static void EmitMethodCall(MoveNextEmitter& ctx, MethodSymbol* method);

    // ========================================================================
    // Defer emission.
    // ========================================================================
    // Active defers are derived from the AST position rather than from a
    // mutable emission-time stack. This matters for async lowering because a
    // single lexical block can have its exit point emitted in several segments
    // (the synchronous path after an await and each resume path), and a mutable
    // scope stack consumed while emitting one path would corrupt the others.
    static bool IsLoopBlockParentKind(SyntaxKind kind)
    {
        return kind == SyntaxKind::WhileStatement || kind == SyntaxKind::UntilStatement ||
               kind == SyntaxKind::ForStatement || kind == SyntaxKind::ForEachStatement ||
               kind == SyntaxKind::ForInStatement;
    }

    static void CollectDefersBefore(StatementsBlockSyntax* block, std::size_t beforeIndex, std::vector<DeferStatementSyntax*>& out)
    {
        if (block == nullptr)
            return;

        for (std::size_t i = block->Statements.size(); i-- > 0;)
        {
            if (i >= beforeIndex)
                continue;

            StatementSyntax* stmt = block->Statements[i].get();
            if (stmt->Kind == SyntaxKind::DeferStatement)
                out.push_back(static_cast<DeferStatementSyntax*>(stmt));
        }
    }

    // Walks from `node` to the method root collecting defers, innermost block
    // first and, within each block, most recently declared first.
    static std::vector<DeferStatementSyntax*> CollectDefersUpward(SyntaxNode* node, bool stopAtLoopBlock)
    {
        std::vector<DeferStatementSyntax*> result;
        SyntaxNode* current = node;
        while (current != nullptr)
        {
            SyntaxNode* parent = current->Parent;
            StatementsBlockSyntax* block = dynamic_cast<StatementsBlockSyntax*>(parent);
            if (block == nullptr)
            {
                current = parent;
                continue;
            }

            StatementSyntax* currentStatement = dynamic_cast<StatementSyntax*>(current);
            std::size_t idx = (currentStatement != nullptr)
                ? IndexInBlock(block->Statements, currentStatement)
                : block->Statements.size();

            CollectDefersBefore(block, idx, result);

            bool isLoopBlock = block->Parent != nullptr && IsLoopBlockParentKind(block->Parent->Kind);
            current = block;
            if (stopAtLoopBlock && isLoopBlock)
                break;
        }

        return result;
    }

    static void EmitDefer(MoveNextEmitter& ctx, DeferStatementSyntax* defer)
    {
        if (defer == nullptr)
            return;

        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;

        std::size_t deferOffset = code.size();
        encoder.EmitDefer(code, 0);

        std::size_t jumpOverBacktrack = code.size();
        encoder.EmitJump(code, 0);

        std::size_t expressionStart = code.size();
        if (defer->IsResourceDefer)
        {
            if (defer->Variable != nullptr)
                encoder.EmitLoadVarible(code, defer->Variable->SlotIndex);

            if (defer->DisposeMethod != nullptr)
                EmitMethodCall(ctx, defer->DisposeMethod);
        }
        else if (defer->Statement != nullptr)
        {
            EmitStatementInto(ctx, ctx.Info.MoveNext, defer->Statement.get());
        }

        encoder.EmitDeferBreak(code);
        std::size_t afterExpression = code.size();

        ByteCodeEncoder::PasteData(code, deferOffset + sizeof(OpCode), &expressionStart, sizeof(std::size_t));
        ByteCodeEncoder::PasteData(code, jumpOverBacktrack + sizeof(OpCode), &afterExpression, sizeof(std::size_t));
    }

    static void EmitDeferDrain(MoveNextEmitter& ctx, std::size_t count)
    {
        if (count > 0)
            ctx.Encoder.EmitDeferDrain(ctx.Code, count);
    }

    static void EmitDefers(MoveNextEmitter& ctx, const std::vector<DeferStatementSyntax*>& defers)
    {
        if (!defers.empty())
            EmitDeferDrain(ctx, defers.size());
    }

    static void EmitBlockEndDefers(MoveNextEmitter& ctx, StatementsBlockSyntax* block)
    {
        std::vector<DeferStatementSyntax*> defers;
        CollectDefersBefore(block, (block != nullptr) ? block->Statements.size() : 0, defers);
        if (!defers.empty())
            EmitDeferDrain(ctx, defers.size());
    }

    static bool IsDescendantOf(SyntaxNode* node, SyntaxNode* ancestor)
    {
        SyntaxNode* current = node;
        while (current != nullptr)
        {
            if (current == ancestor)
                return true;
            current = current->Parent;
        }
        return false;
    }

    // Replays the defer registrations and ENTER_TRY instructions that were
    // active at an await expression so that resuming MoveNext restores the
    // same runtime state. Defers are registered outermost-first so that nested
    // scopes drain their defers before outer scopes.
    static void EmitResumeDefersAndEnterTries(MoveNextEmitter& ctx, const AwaitSite& previousSite)
    {
        std::vector<DeferStatementSyntax*> allDefers = CollectDefersUpward(previousSite.Expression, false);
        if (allDefers.empty() && previousSite.ActiveTryStack.empty())
            return;

        std::reverse(allDefers.begin(), allDefers.end());

        const std::vector<TryStatementSyntax*>& activeTries = previousSite.ActiveTryStack;
        std::unordered_map<std::size_t, std::vector<DeferStatementSyntax*>> defersByTry;
        std::vector<DeferStatementSyntax*> outerDefers;

        for (DeferStatementSyntax* defer : allDefers)
        {
            std::size_t containingTry = static_cast<std::size_t>(-1);
            for (std::size_t i = activeTries.size(); i-- > 0;)
            {
                if (IsDescendantOf(defer, activeTries[i]->TryBlock.get()))
                {
                    containingTry = i;
                    break;
                }
            }

            if (containingTry == static_cast<std::size_t>(-1))
                outerDefers.push_back(defer);
            else
                defersByTry[containingTry].push_back(defer);
        }

        for (DeferStatementSyntax* defer : outerDefers)
            EmitDefer(ctx, defer);

        for (std::size_t i = 0; i < activeTries.size(); ++i)
        {
            std::size_t enterBacktrack = ctx.Code.size();
            ctx.Encoder.EmitEnterTry(ctx.Code, 0);
            ctx.Regions[activeTries[i]].EnterBacktracks.push_back(enterBacktrack);

            auto it = defersByTry.find(i);
            if (it != defersByTry.end())
            {
                for (DeferStatementSyntax* defer : it->second)
                    EmitDefer(ctx, defer);
            }
        }
    }

    static void EmitStatementInto(MoveNextEmitter& ctx, MethodSymbol* target, StatementSyntax* statement)
    {
        AbstractEmiter emiter(ctx.Program, ctx.Model, ctx.Diagnostics);
        emiter.SetGeneratingTarget(target);
        emiter.SetPopExpressionStatement(true);
        emiter.VisitStatement(statement);
    }

    static void EmitExpressionInto(MoveNextEmitter& ctx, MethodSymbol* target, ExpressionSyntax* expression)
    {
        AbstractEmiter emiter(ctx.Program, ctx.Model, ctx.Diagnostics);
        emiter.SetGeneratingTarget(target);
        emiter.SetPopExpressionStatement(false);
        emiter.VisitExpression(expression);
    }

    static bool IsInterfaceMember(MethodSymbol* method)
    {
        if (method == nullptr)
            return false;

        SyntaxSymbol* owner = method->Parent;
        while (owner != nullptr)
        {
            if (owner->Kind == SyntaxKind::InterfaceDeclaration)
                return true;

            if (owner->IsType())
                return false;

            owner = owner->Parent;
        }

        return false;
    }

    static void EmitMethodCall(MoveNextEmitter& ctx, MethodSymbol* method)
    {
        if (method == nullptr)
            return;

        if (IsInterfaceMember(method))
            ctx.Encoder.EmitCallInterface(ctx.Code, method);
        else
            ctx.Encoder.EmitCallMethodSymbol(ctx.Code, method);
    }

    static void EmitConditionExpression(MoveNextEmitter& ctx, StatementSyntax* conditionStatement)
    {
        if (conditionStatement != nullptr && conditionStatement->Kind == SyntaxKind::ExpressionStatement)
        {
            ExpressionStatementSyntax* exprStmt = static_cast<ExpressionStatementSyntax*>(conditionStatement);
            EmitExpressionInto(ctx, ctx.Info.MoveNext, exprStmt->Expression.get());
        }
    }

    static void EmitTaskComplete(MoveNextEmitter& ctx)
    {
        const AsyncMethodInfo& info = ctx.Info;
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;

        TypeSymbol* returnType = info.Method->ReturnType;
        if (IsTaskReturnType(returnType))
        {
            MethodSymbol* completeMethod = FindCompleteMethod(returnType);
            if (completeMethod == nullptr)
                return;

            encoder.EmitLoadVarible(code, 0); // this state machine
            encoder.EmitLoadField(code, info.TaskField->SlotIndex);
            encoder.EmitCallMethodSymbol(code, completeMethod);
        }
        else if (IsValueTaskReturnType(returnType))
        {
            MethodSymbol* setResultMethod = FindSetResultMethod(returnType);
            if (setResultMethod == nullptr)
                return;

            TypeSymbol* elementType = GetValueTaskElementType(returnType);

            encoder.EmitLoadVarible(code, 0); // this state machine
            encoder.EmitLoadField(code, info.TaskField->SlotIndex);

            encoder.EmitDefaultValue(code, elementType);
            encoder.EmitCallMethodSymbol(code, setResultMethod);
        }
    }

    static void EmitAwaitSuspension(MoveNextEmitter& ctx, std::size_t awaitIndex, std::vector<std::size_t>& nextResumeBacktracks)
    {
        const AsyncMethodInfo& info = ctx.Info;
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;

        const AwaitSite& site = info.AwaitSites[awaitIndex];
        AwaitExpressionSyntax* awaitExpr = site.Expression;

        std::uint16_t tempAwaitableSlot = static_cast<std::uint16_t>(info.MoveNext->GetEvalStackArgumentsCount() + info.MoveNext->AddVariableCount());
        std::uint16_t tempAwaiterSlot = static_cast<std::uint16_t>(info.MoveNext->GetEvalStackArgumentsCount() + info.MoveNext->AddVariableCount());

        // Evaluate the awaited expression, leaving the awaitable on the stack.
        EmitExpressionInto(ctx, info.MoveNext, awaitExpr->Expression.get());

        // Anchor the awaitable in a temporary local before calling GetAwaiter.
        encoder.EmitStoreVarible(code, tempAwaitableSlot);

        if (awaitExpr->GetAwaiterMethod != nullptr)
        {
            encoder.EmitLoadVarible(code, tempAwaitableSlot);
            encoder.EmitCallMethodSymbol(code, awaitExpr->GetAwaiterMethod);
            encoder.EmitStoreVarible(code, tempAwaiterSlot);
        }
        else
        {
            // Self-awaiter: the awaitable is the awaiter.
            encoder.EmitLoadVarible(code, tempAwaitableSlot);
            encoder.EmitStoreVarible(code, tempAwaiterSlot);
        }

        // Release the temporary anchor now that the awaiter is stored in the field.
        encoder.EmitLoadConstNull(code);
        encoder.EmitStoreVarible(code, tempAwaitableSlot);

        // this._awaiterN = awaiter;
        encoder.EmitLoadVarible(code, 0);              // this
        encoder.EmitLoadVarible(code, tempAwaiterSlot); // awaiter
        encoder.EmitStoreField(code, site.AwaiterField->SlotIndex);

        // if (this._awaiterN.IsCompleted) goto next resume label;
        encoder.EmitLoadVarible(code, 0);
        encoder.EmitLoadField(code, site.AwaiterField->SlotIndex);
        encoder.EmitCallMethodSymbol(code, awaitExpr->IsCompletedMethod);
        nextResumeBacktracks.push_back(code.size());
        encoder.EmitJumpTrue(code, 0);

        // If we are suspending inside a catch clause, preserve the exception being
        // handled so it can be restored when MoveNext resumes.
        if (ctx.Info.CurrentExceptionField != nullptr && site.EnclosingCatch != nullptr)
        {
            encoder.EmitLoadVarible(code, 0);
            encoder.EmitLoadCurrentException(code);
            encoder.EmitStoreField(code, ctx.Info.CurrentExceptionField->SlotIndex);
        }

        // this._state = awaitIndex + 1;
        encoder.EmitLoadVarible(code, 0);
        encoder.EmitLoadConstInt64(code, static_cast<std::int64_t>(awaitIndex + 1));
        encoder.EmitStoreField(code, info.StateField->SlotIndex);

        // this._awaiterN.OnCompleted(this);
        encoder.EmitLoadVarible(code, 0);              // this (becomes parameter)
        encoder.EmitLoadVarible(code, 0);              // this (becomes receiver placeholder)
        encoder.EmitLoadField(code, site.AwaiterField->SlotIndex); // replace top with awaiter
        encoder.EmitCallMethodSymbol(code, awaitExpr->OnCompletedMethod);

        // return;
        encoder.EmitReturn(code);
    }

    static void ConsumePreviousAwaiter(MoveNextEmitter& ctx, std::size_t previousAwaitIndex)
    {
        const AsyncMethodInfo& info = ctx.Info;
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;

        const AwaitSite& site = info.AwaitSites[previousAwaitIndex];
        encoder.EmitLoadVarible(code, 0);
        encoder.EmitLoadField(code, site.AwaiterField->SlotIndex);

        if (site.Expression->GetResultMethod != nullptr)
            encoder.EmitCallMethodSymbol(code, site.Expression->GetResultMethod);
        else
            encoder.EmitPop(code);

        if (site.ResultVariable != nullptr)
        {
            encoder.EmitStoreVarible(code, site.ResultVariable->SlotIndex);
        }
        else if (site.IsReturnAwait)
        {
            TypeSymbol* returnType = info.Method->ReturnType;
            if (IsValueTaskReturnType(returnType))
            {
                std::uint16_t tempSlot = static_cast<std::uint16_t>(info.MoveNext->GetEvalStackArgumentsCount() + info.MoveNext->AddVariableCount());
                encoder.EmitStoreVarible(code, tempSlot);

                encoder.EmitLoadVarible(code, tempSlot);
                encoder.EmitLoadVarible(code, 0);
                encoder.EmitLoadField(code, info.TaskField->SlotIndex);

                MethodSymbol* setResultMethod = FindSetResultMethod(returnType);
                if (setResultMethod != nullptr)
                    encoder.EmitCallMethodSymbol(code, setResultMethod);
            }
            else if (IsTaskReturnType(returnType))
            {
                if (site.Expression->GetResultMethod != nullptr &&
                    site.Expression->GetResultMethod->ReturnType != SymbolTable::Primitives::Void)
                {
                    encoder.EmitPop(code);
                }

                encoder.EmitLoadVarible(code, 0);
                encoder.EmitLoadField(code, info.TaskField->SlotIndex);

                MethodSymbol* completeMethod = FindCompleteMethod(returnType);
                if (completeMethod != nullptr)
                    encoder.EmitCallMethodSymbol(code, completeMethod);
            }

            encoder.EmitReturn(code);
        }
        else if (site.Expression->GetResultMethod != nullptr &&
                 site.Expression->GetResultMethod->ReturnType != SymbolTable::Primitives::Void)
        {
            encoder.EmitPop(code);
        }
    }

    static void EmitCatchClauses(MoveNextEmitter& ctx, TryStatementSyntax* tryStmt, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, std::vector<std::size_t>& outClauseStarts, std::vector<std::optional<std::size_t>>& outFilterFailBacktracks)
    {
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;
        std::vector<std::size_t>& catchBodyEndBacktracks = ctx.Regions[tryStmt].CatchBodyEndBacktracks;

        for (const auto& clauseUnique : tryStmt->CatchClauses)
        {
            CatchClauseSyntax* clause = clauseUnique.get();
            outClauseStarts.push_back(code.size());
            outFilterFailBacktracks.emplace_back(std::nullopt);

            TypeSymbol* catchType = SymbolTable::Primitives::Any;
            if (clause->ExceptionType != nullptr && clause->ExceptionType->Symbol != nullptr)
                catchType = clause->ExceptionType->Symbol;

            if (catchType != SymbolTable::Primitives::Any)
            {
                encoder.EmitDuplicate(code);
                encoder.EmitIsInstance(code, catchType);
                outFilterFailBacktracks.back() = code.size();
                encoder.EmitJumpFalse(code, 0);
            }

            if (clause->Symbol != nullptr)
            {
                encoder.EmitStoreVarible(code, clause->Symbol->SlotIndex);
            }
            else
                encoder.EmitPop(code);

            bool bodyFellThrough = false;
            if (clause->Body != nullptr)
                bodyFellThrough = EmitBlock(ctx, clause->Body.get(), 0, activeStack, leaveActiveTries);

            if (bodyFellThrough)
            {
                catchBodyEndBacktracks.push_back(code.size());
                encoder.EmitJump(code, 0);
            }
        }
    }

    static bool EmitAfterTry(MoveNextEmitter& ctx, TryStatementSyntax* tryStmt, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after)
    {
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;
        std::unordered_map<TryStatementSyntax*, ExceptionRegion>& regions = ctx.Regions;

        encoder.EmitLeaveTry(code);
        std::size_t backtrack = code.size();
        encoder.EmitJump(code, 0);

        ExceptionRegion& region = regions[tryStmt];
        if (region.AfterTryAddress != 0)
            PatchJumpTarget(code, backtrack, region.AfterTryAddress);
        else
            region.AfterTryBacktracks.push_back(backtrack);

        StatementsBlockSyntax* parentBlock = dynamic_cast<StatementsBlockSyntax*>(tryStmt->Parent);
        if (parentBlock != nullptr)
        {
            std::size_t idx = IndexInBlock(parentBlock->Statements, tryStmt) + 1;
            return EmitStatementSequence(ctx, parentBlock, idx, activeStack, leaveActiveTries, after);
        }

        return after();
    }

    static bool EmitTryStatement(MoveNextEmitter& ctx, TryStatementSyntax* tryStmt, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after)
    {
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;
        std::unordered_map<TryStatementSyntax*, ExceptionRegion>& regions = ctx.Regions;

        std::size_t enterBacktrack = code.size();
        encoder.EmitEnterTry(code, 0);
        regions[tryStmt].Try = tryStmt;
        regions[tryStmt].EnterBacktracks.push_back(enterBacktrack);

        std::vector<TryStatementSyntax*> innerStack = activeStack;
        innerStack.push_back(tryStmt);

        Continuation bodyAfter = [&]() -> bool {
            return EmitAfterTry(ctx, tryStmt, activeStack, leaveActiveTries, after);
        };

        bool bodyFellThrough = EmitStatementSequence(ctx, tryStmt->TryBlock.get(), 0, innerStack, false, bodyAfter);

        std::size_t handlerStart = code.size();
        regions[tryStmt].HandlerAddress = handlerStart;

        std::vector<std::size_t> clauseStarts;
        std::vector<std::optional<std::size_t>> filterFailBacktracks;
        EmitCatchClauses(ctx, tryStmt, activeStack, leaveActiveTries, clauseStarts, filterFailBacktracks);

        std::size_t fallbackStart = code.size();
        encoder.EmitThrow(code);

        std::size_t endLabel = code.size();
        encoder.EmitEndCatch(code);
        regions[tryStmt].AfterTryAddress = endLabel;

        for (std::size_t pending : regions[tryStmt].AfterTryBacktracks)
            PatchJumpTarget(code, pending, endLabel);

        for (std::size_t i = 0; i < tryStmt->CatchClauses.size(); ++i)
        {
            if (filterFailBacktracks[i].has_value())
            {
                std::size_t target = (i + 1 < clauseStarts.size())
                    ? clauseStarts[i + 1] : fallbackStart;

                PatchJumpTarget(code, filterFailBacktracks[i].value(), target);
            }
        }

        for (std::size_t backtrack : regions[tryStmt].CatchBodyEndBacktracks)
            PatchJumpTarget(code, backtrack, endLabel);

        return after();
    }

    static bool EmitLoopStatement(MoveNextEmitter& ctx, StatementSyntax* loop, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after)
    {
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;

        LoopLabels labels;

        if (loop->Kind == SyntaxKind::ForEachStatement ||
            loop->Kind == SyntaxKind::ForInStatement)
        {
            auto enumIt = ctx.Info.EnumeratorFields.find(loop);
            if (enumIt == ctx.Info.EnumeratorFields.end())
            {
                EmitStatementInto(ctx, ctx.Info.MoveNext, loop);
                return after();
            }

            FieldSymbol* enumeratorField = enumIt->second;

            ExpressionSyntax* range = nullptr;
            StatementsBlockSyntax* body = nullptr;
            VariableSymbol* loopVariable = nullptr;

            if (loop->Kind == SyntaxKind::ForEachStatement)
            {
                ForEachStatementSyntax* node = static_cast<ForEachStatementSyntax*>(loop);
                range = node->RangeExpression.get();
                body = node->StatementsBlock.get();
                {
                    auto symbolOpt = ctx.Model.Table->LookupSymbol(node);
                    if (symbolOpt.has_value())
                        loopVariable = dynamic_cast<VariableSymbol*>(symbolOpt.value());
                }
            }
            else
            {
                ForInStatementSyntax* node = static_cast<ForInStatementSyntax*>(loop);
                range = node->RangeExpression.get();
                body = node->StatementsBlock.get();
                {
                    auto symbolOpt = ctx.Model.Table->LookupSymbol(node);
                    if (symbolOpt.has_value())
                        loopVariable = dynamic_cast<VariableSymbol*>(symbolOpt.value());
                }
            }

            // enumerator := range.GetEnumerator();
            encoder.EmitLoadVarible(code, 0);                                     // this
            EmitExpressionInto(ctx, ctx.Info.MoveNext, range);                    // this, range
            EmitMethodCall(ctx, TRAIT_ENUMERABLE_GETENUMERATOR);             // this, enumerator
            encoder.EmitStoreField(code, enumeratorField->SlotIndex);             // --

            labels.LoopStart = code.size();

            // while (enumerator.MoveNext())
            encoder.EmitLoadVarible(code, 0);
            encoder.EmitLoadField(code, enumeratorField->SlotIndex);
            EmitMethodCall(ctx, TRAIT_ENUMERATOR_MOVENEXT);
            std::size_t condJumpBacktrack = code.size();
            encoder.EmitJumpFalse(code, 0);

            // loopVariable := enumerator.Current;
            encoder.EmitLoadVarible(code, 0);
            encoder.EmitLoadField(code, enumeratorField->SlotIndex);
            EmitMethodCall(ctx, TRAIT_ENUMERATOR_CURRENT_GET);
            if (loopVariable != nullptr)
                encoder.EmitStoreVarible(code, loopVariable->SlotIndex);
            else
                encoder.EmitPop(code);

            ctx.LoopStack.push_back(labels);
            EmitStatementSequence(ctx, body, 0, activeStack, leaveActiveTries, []() -> bool { return true; });
            LoopLabels bodyLabels = ctx.LoopStack.back();
            ctx.LoopStack.pop_back();

            std::size_t blockEnd = code.size();
            for (std::size_t backtrack : bodyLabels.ContinueBacktracks)
                PatchJumpTarget(code, backtrack, blockEnd);

            encoder.EmitJump(code, labels.LoopStart);

            std::size_t loopEnd = code.size();
            PatchJumpTarget(code, condJumpBacktrack, loopEnd);
            for (std::size_t backtrack : bodyLabels.BreakBacktracks)
                PatchJumpTarget(code, backtrack, loopEnd);

            labels.BlockEnd = blockEnd;
            labels.LoopEnd = loopEnd;
            ctx.LoopLabelsByStatement[loop] = labels;
            return after();
        }

        if (loop->Kind == SyntaxKind::ForStatement)
        {
            ForStatementSyntax* node = static_cast<ForStatementSyntax*>(loop);
            if (node->InitializerStatement != nullptr)
                EmitStatementInto(ctx, ctx.Info.MoveNext, node->InitializerStatement.get());
        }

        labels.LoopStart = code.size();

        ExpressionSyntax* condition = nullptr;
        bool jumpOnFalse = true;
        if (loop->Kind == SyntaxKind::WhileStatement)
        {
            condition = static_cast<WhileStatementSyntax*>(loop)->ConditionExpression.get();
        }
        else if (loop->Kind == SyntaxKind::UntilStatement)
        {
            condition = static_cast<UntilStatementSyntax*>(loop)->ConditionExpression.get();
            jumpOnFalse = false;
        }
        else if (loop->Kind == SyntaxKind::ForStatement)
        {
            condition = static_cast<ForStatementSyntax*>(loop)->ConditionExpression.get();
        }

        std::size_t condJumpBacktrack = 0;
        bool hasCondition = (condition != nullptr);
        if (hasCondition)
        {
            EmitExpressionInto(ctx, ctx.Info.MoveNext, condition);
            condJumpBacktrack = code.size();
            if (jumpOnFalse)
                encoder.EmitJumpFalse(code, 0);
            else
                encoder.EmitJumpTrue(code, 0);
        }

        StatementsBlockSyntax* body = nullptr;
        if (loop->Kind == SyntaxKind::ForStatement)
            body = static_cast<ForStatementSyntax*>(loop)->StatementsBlock.get();
        else if (loop->Kind == SyntaxKind::UntilStatement)
            body = static_cast<UntilStatementSyntax*>(loop)->StatementsBlock.get();
        else
            body = static_cast<WhileStatementSyntax*>(loop)->StatementsBlock.get();

        ctx.LoopStack.push_back(labels);
        EmitStatementSequence(ctx, body, 0, activeStack, leaveActiveTries, []() -> bool { return true; });
        LoopLabels bodyLabels = ctx.LoopStack.back();
        ctx.LoopStack.pop_back();

        std::size_t blockEnd = code.size();
        for (std::size_t backtrack : bodyLabels.ContinueBacktracks)
            PatchJumpTarget(code, backtrack, blockEnd);

        if (loop->Kind == SyntaxKind::ForStatement)
        {
            ForStatementSyntax* node = static_cast<ForStatementSyntax*>(loop);
            if (node->AfterRepeatStatement != nullptr)
                EmitStatementInto(ctx, ctx.Info.MoveNext, node->AfterRepeatStatement.get());
        }

        encoder.EmitJump(code, labels.LoopStart);

        std::size_t loopEnd = code.size();
        if (hasCondition)
            PatchJumpTarget(code, condJumpBacktrack, loopEnd);

        for (std::size_t backtrack : bodyLabels.BreakBacktracks)
            PatchJumpTarget(code, backtrack, loopEnd);

        labels.BlockEnd = blockEnd;
        labels.LoopEnd = loopEnd;
        ctx.LoopLabelsByStatement[loop] = labels;

        return after();
    }

    static bool EmitBreakStatement(MoveNextEmitter& ctx, BreakStatementSyntax* node)
    {
        if (ctx.LoopStack.empty())
            return false;

        EmitDefers(ctx, CollectDefersUpward(node, true));

        LoopLabels& labels = ctx.LoopStack.back();
        if (labels.LoopEnd != 0)
        {
            // The loop was emitted in an earlier segment; jump straight to its end.
            ctx.Encoder.EmitJump(ctx.Code, labels.LoopEnd);
        }
        else
        {
            std::size_t backtrack = ctx.Code.size();
            ctx.Encoder.EmitJump(ctx.Code, 0);
            labels.BreakBacktracks.push_back(backtrack);
        }

        return false;
    }

    static bool EmitContinueStatement(MoveNextEmitter& ctx, ContinueStatementSyntax* node)
    {
        if (ctx.LoopStack.empty())
            return false;

        EmitDefers(ctx, CollectDefersUpward(node, true));

        LoopLabels& labels = ctx.LoopStack.back();
        if (labels.BlockEnd != 0)
        {
            // The loop was emitted in an earlier segment; jump straight to its tail.
            ctx.Encoder.EmitJump(ctx.Code, labels.BlockEnd);
        }
        else
        {
            std::size_t backtrack = ctx.Code.size();
            ctx.Encoder.EmitJump(ctx.Code, 0);
            labels.ContinueBacktracks.push_back(backtrack);
        }

        return false;
    }

    static bool EmitBlock(MoveNextEmitter& ctx, StatementsBlockSyntax* block, std::size_t startIndex, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries)
    {
        return EmitStatementSequence(ctx, block, startIndex, activeStack, leaveActiveTries, []() -> bool { return true; });
    }

    static bool EmitStatementSequence(MoveNextEmitter& ctx, StatementsBlockSyntax* block, std::size_t startIndex, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after)
    {
        if (block == nullptr)
            return after();

        bool isActiveTryBody = false;
        TryStatementSyntax* containingTry = nullptr;
        std::vector<TryStatementSyntax*> outerStack;
        for (std::int64_t i = static_cast<std::int64_t>(activeStack.size()) - 1; i >= 0; --i)
        {
            if (activeStack[i]->TryBlock.get() == block)
            {
                isActiveTryBody = true;
                containingTry = activeStack[i];
                outerStack.assign(activeStack.begin(), activeStack.begin() + i);
                break;
            }
        }

        const auto& statements = block->Statements;
        if (startIndex >= statements.size())
        {
            EmitBlockEndDefers(ctx, block);

            if (isActiveTryBody && leaveActiveTries)
                return EmitAfterTry(ctx, containingTry, outerStack, leaveActiveTries, after);

            return after();
        }

        Continuation rest = [&, startIndex, after]() -> bool {
            return EmitStatementSequence(ctx, block, startIndex + 1, activeStack, leaveActiveTries, after);
        };

        return EmitStatement(ctx, statements[startIndex].get(), activeStack, leaveActiveTries, rest);
    }

    static bool EmitStatement(MoveNextEmitter& ctx, StatementSyntax* statement, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after)
    {
        const AsyncMethodInfo& info = ctx.Info;
        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;
        std::unordered_map<AwaitExpressionSyntax*, std::size_t>& awaitIndexMap = ctx.AwaitIndexMap;

        if (statement->Kind == SyntaxKind::ExpressionStatement ||
            statement->Kind == SyntaxKind::VariableStatement)
        {
            ExpressionSyntax* expr = nullptr;
            if (statement->Kind == SyntaxKind::ExpressionStatement)
                expr = static_cast<ExpressionStatementSyntax*>(statement)->Expression.get();
            else
                expr = static_cast<VariableStatementSyntax*>(statement)->Expression.get();

            if (expr != nullptr && expr->Kind == SyntaxKind::AwaitExpression)
            {
                AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(expr);
                auto it = awaitIndexMap.find(awaitExpr);
                if (it != awaitIndexMap.end())
                {
                    std::size_t awaitIndex = it->second;

                    SaveLiftedParametersAndLocals(info, code, encoder);

                    std::vector<std::size_t> syncResumeBacktracks;
                    EmitAwaitSuspension(ctx, awaitIndex, syncResumeBacktracks);

                    std::size_t consumeLabel = code.size();
                    if (!syncResumeBacktracks.empty())
                        PatchJumpTarget(code, syncResumeBacktracks[0], consumeLabel);

                    ConsumePreviousAwaiter(ctx, awaitIndex);
                    return after();
                }
            }

            EmitStatementInto(ctx, info.MoveNext, statement);
            return after();
        }

        if (statement->Kind == SyntaxKind::DeferStatement)
        {
            DeferStatementSyntax* defer = static_cast<DeferStatementSyntax*>(statement);
            if (defer->IsResourceDefer && defer->Statement != nullptr)
                EmitStatementInto(ctx, info.MoveNext, defer->Statement.get());

            EmitDefer(ctx, defer);
            return after();
        }

        if (statement->Kind == SyntaxKind::ThrowStatement)
        {
            ThrowStatementSyntax* throwStmt = static_cast<ThrowStatementSyntax*>(statement);
            if (throwStmt->Expression != nullptr)
                EmitExpressionInto(ctx, info.MoveNext, throwStmt->Expression.get());

            // The VM's exception dispatch drains the defers that belong to the
            // scopes being unwound; do not emit DEFER_DRAIN here.
            encoder.EmitThrow(code);
            return false;
        }

        if (statement->Kind == SyntaxKind::ReturnStatement)
        {
            ReturnStatementSyntax* retStmt = static_cast<ReturnStatementSyntax*>(statement);
            if (retStmt->Expression != nullptr && retStmt->Expression->Kind == SyntaxKind::AwaitExpression)
            {
                AwaitExpressionSyntax* awaitExpr = static_cast<AwaitExpressionSyntax*>(retStmt->Expression.get());
                auto it = awaitIndexMap.find(awaitExpr);
                if (it != awaitIndexMap.end())
                {
                    std::size_t awaitIndex = it->second;

                    SaveLiftedParametersAndLocals(info, code, encoder);

                    std::vector<std::size_t> syncResumeBacktracks;
                    EmitAwaitSuspension(ctx, awaitIndex, syncResumeBacktracks);

                    std::size_t consumeLabel = code.size();
                    if (!syncResumeBacktracks.empty())
                        PatchJumpTarget(code, syncResumeBacktracks[0], consumeLabel);

                    ConsumePreviousAwaiter(ctx, awaitIndex);
                    return after();
                }
            }

            if (retStmt->Expression != nullptr)
            {
                EmitExpressionInto(ctx, info.MoveNext, retStmt->Expression.get());
                EmitDefers(ctx, CollectDefersUpward(statement, false));

                if (IsValueTaskReturnType(info.Method->ReturnType))
                {
                    MethodSymbol* setResultMethod = FindSetResultMethod(info.Method->ReturnType);
                    if (setResultMethod != nullptr)
                    {
                        encoder.EmitLoadVarible(code, 0);
                        encoder.EmitLoadField(code, info.TaskField->SlotIndex);
                        encoder.EmitCallMethodSymbol(code, setResultMethod);
                    }
                    else
                    {
                        encoder.EmitPop(code);
                        EmitTaskComplete(ctx);
                    }
                }
                else
                {
                    encoder.EmitPop(code);
                    EmitTaskComplete(ctx);
                }
            }
            else
            {
                EmitDefers(ctx, CollectDefersUpward(statement, false));
                EmitTaskComplete(ctx);
            }

            encoder.EmitReturn(code);
            return false;
        }

        if (statement->Kind == SyntaxKind::TryStatement)
        {
            return EmitTryStatement(ctx, static_cast<TryStatementSyntax*>(statement), activeStack, leaveActiveTries, after);
        }

        if (statement->Kind == SyntaxKind::IfStatement ||
            statement->Kind == SyntaxKind::UnlessStatement ||
            statement->Kind == SyntaxKind::ElseStatement)
        {
            return EmitConditionalChain(ctx, static_cast<ConditionalClauseBaseSyntax*>(statement), activeStack, leaveActiveTries, after);
        }

        if (statement->Kind == SyntaxKind::WhileStatement ||
            statement->Kind == SyntaxKind::UntilStatement ||
            statement->Kind == SyntaxKind::ForStatement ||
            statement->Kind == SyntaxKind::ForEachStatement ||
            statement->Kind == SyntaxKind::ForInStatement)
        {
            return EmitLoopStatement(ctx, statement, activeStack, leaveActiveTries, after);
        }

        if (statement->Kind == SyntaxKind::BreakStatement)
        {
            if (!ctx.LoopStack.empty())
                return EmitBreakStatement(ctx, static_cast<BreakStatementSyntax*>(statement));

            EmitStatementInto(ctx, info.MoveNext, statement);
            return after();
        }

        if (statement->Kind == SyntaxKind::ContinueStatement)
        {
            if (!ctx.LoopStack.empty())
                return EmitContinueStatement(ctx, static_cast<ContinueStatementSyntax*>(statement));

            EmitStatementInto(ctx, info.MoveNext, statement);
            return after();
        }

        // Control-flow statements that have no await sites inside them can be
        // emitted as a single unit by the regular emitter.  Awaits in loops and
        // defer/foreach/forin are still rejected by AsyncAnalysisPass.
        EmitStatementInto(ctx, info.MoveNext, statement);
        return after();
    }

    static bool EmitConditionalChain(MoveNextEmitter& ctx, ConditionalClauseBaseSyntax* clause, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation outerAfter)
    {
        std::vector<std::size_t> afterJumpBacktracks;
        Continuation chainAfter = [&, outerAfter]() -> bool {
            std::size_t afterAddress = ctx.Code.size();
            for (std::size_t bt : afterJumpBacktracks)
                PatchJumpTarget(ctx.Code, bt, afterAddress);
            return outerAfter();
        };

        return EmitConditionalChainImpl(ctx, clause, activeStack, leaveActiveTries, afterJumpBacktracks, chainAfter);
    }

    static bool EmitConditionalChainImpl(MoveNextEmitter& ctx, ConditionalClauseBaseSyntax* clause, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, std::vector<std::size_t>& afterJumpBacktracks, Continuation chainAfter)
    {
        if (clause->Kind == SyntaxKind::ElseStatement)
        {
            auto* elseStmt = static_cast<ElseStatementSyntax*>(clause);
            return EmitConditionalChainBody(ctx, elseStmt, elseStmt->StatementsBlock.get(), 0, activeStack, leaveActiveTries, afterJumpBacktracks, chainAfter);
        }

        auto* condClause = static_cast<ConditionalClauseSyntax*>(clause);
        EmitConditionExpression(ctx, condClause->ConditionExpression.get());
        std::size_t conditionJumpBacktrack = ctx.Code.size();

        if (clause->Kind == SyntaxKind::UnlessStatement)
        {
            ctx.Encoder.EmitJumpTrue(ctx.Code, 0);
        }
        else
        {
            ctx.Encoder.EmitJumpFalse(ctx.Code, 0);
        }

        bool bodyFellThrough = EmitStatementSequence(ctx, condClause->StatementsBlock.get(), 0, activeStack, leaveActiveTries, []() -> bool { return true; });

        std::vector<std::byte>& code = ctx.Code;
        ByteCodeEncoder& encoder = ctx.Encoder;

        if (condClause->NextStatement != nullptr)
        {
            if (bodyFellThrough)
            {
                std::size_t afterJumpBacktrack = code.size();
                encoder.EmitJump(code, 0);
                afterJumpBacktracks.push_back(afterJumpBacktrack);
            }

            std::size_t nextClauseStart = code.size();
            PatchJumpTarget(code, conditionJumpBacktrack, nextClauseStart);

            return EmitConditionalChainImpl(ctx, condClause->NextStatement.get(), activeStack, leaveActiveTries, afterJumpBacktracks, chainAfter);
        }

        std::size_t clauseEnd = code.size();
        PatchJumpTarget(code, conditionJumpBacktrack, clauseEnd);
        return chainAfter();
    }

    static bool EmitConditionalChainBody(MoveNextEmitter& ctx, ConditionalClauseBaseSyntax* clause, StatementsBlockSyntax* body, std::size_t startIndex, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, std::vector<std::size_t>& afterJumpBacktracks, Continuation chainAfter)
    {
        Continuation afterBody = [&, clause, chainAfter]() -> bool
        {
            std::vector<std::byte>& code = ctx.Code;
            ByteCodeEncoder& encoder = ctx.Encoder;

            std::size_t afterJumpBacktrack = code.size();
            encoder.EmitJump(code, 0);
            afterJumpBacktracks.push_back(afterJumpBacktrack);

            if (clause->NextStatement != nullptr)
                return EmitConditionalChainImpl(ctx, clause->NextStatement.get(), activeStack, leaveActiveTries, afterJumpBacktracks, chainAfter);

            return chainAfter();
        };

        bool bodyFellThrough = EmitStatementSequence(ctx, body, startIndex, activeStack, leaveActiveTries, afterBody);
        if (!bodyFellThrough)
            return chainAfter();

        return true;
    }

    static bool IsLoopStatementKind(SyntaxKind kind)
    {
        return kind == SyntaxKind::WhileStatement ||
               kind == SyntaxKind::UntilStatement ||
               kind == SyntaxKind::ForStatement ||
               kind == SyntaxKind::ForEachStatement ||
               kind == SyntaxKind::ForInStatement;
    }

    struct LoopStackScope
    {
        MoveNextEmitter& ctx;
        bool Pushed = false;

        LoopStackScope(MoveNextEmitter& ctx, const LoopLabels& labels)
            : ctx(ctx), Pushed(true)
        {
            ctx.LoopStack.push_back(labels);
        }

        ~LoopStackScope()
        {
            if (Pushed)
                ctx.LoopStack.pop_back();
        }
    };

    static StatementSyntax* FindEnclosingLoopWithLabels(MoveNextEmitter& ctx, StatementSyntax* statement, std::size_t& outBlockEnd)
    {
        SyntaxNode* current = statement->Parent;
        while (current != nullptr)
        {
            if (IsLoopStatementKind(current->Kind))
            {
                auto it = ctx.LoopLabelsByStatement.find(static_cast<StatementSyntax*>(current));
                if (it != ctx.LoopLabelsByStatement.end())
                {
                    outBlockEnd = it->second.BlockEnd;
                    return static_cast<StatementSyntax*>(current);
                }

                return nullptr;
            }

            current = current->Parent;
        }

        return nullptr;
    }

    static StatementsBlockSyntax* FindConditionalChainOuterBlock(ConditionalClauseBaseSyntax* clause, StatementSyntax*& outTopClause, std::size_t& outNextIndex)
    {
        StatementSyntax* current = clause;
        while (current != nullptr)
        {
            StatementsBlockSyntax* outerBlock = dynamic_cast<StatementsBlockSyntax*>(current->Parent);
            if (outerBlock != nullptr)
            {
                outTopClause = current;
                outNextIndex = IndexInBlock(outerBlock->Statements, current) + 1;
                return outerBlock;
            }

            SyntaxNode* parent = current->Parent;
            if (parent == nullptr)
                return nullptr;

            current = dynamic_cast<StatementSyntax*>(parent);
        }

        return nullptr;
    }

    static bool EmitStatementContinuation(MoveNextEmitter& ctx, StatementSyntax* statement, const std::vector<TryStatementSyntax*>& activeStack, bool leaveActiveTries, Continuation after)
    {
        if (statement == nullptr)
            return after();

        // If resuming points at the loop statement itself, just jump to its tail.
        if (IsLoopStatementKind(statement->Kind))
        {
            auto it = ctx.LoopLabelsByStatement.find(statement);
            if (it != ctx.LoopLabelsByStatement.end())
            {
                ctx.Encoder.EmitJump(ctx.Code, it->second.BlockEnd);
                return true;
            }
        }

        // For statements nested inside a loop (e.g., inside an if/try in the loop
        // body), the natural continuation must still return to the loop header once
        // the remainder of the loop body has finished executing. Restore the
        // loop label stack so that break/continue statements in the resumed
        // segment are handled correctly.
        std::size_t loopBlockEnd = 0;
        StatementSyntax* enclosingLoop = FindEnclosingLoopWithLabels(ctx, statement, loopBlockEnd);
        std::optional<LoopStackScope> loopScope;
        Continuation loopWrappedAfter = after;
        if (enclosingLoop != nullptr)
        {
            auto it = ctx.LoopLabelsByStatement.find(enclosingLoop);
            if (it != ctx.LoopLabelsByStatement.end())
                loopScope.emplace(ctx, it->second);

            loopWrappedAfter = [&, after, loopBlockEnd]() -> bool
            {
                bool result = after();
                if (!result)
                    return false;

                ctx.Encoder.EmitJump(ctx.Code, loopBlockEnd);
                return true;
            };
        }

        StatementsBlockSyntax* parentBlock = dynamic_cast<StatementsBlockSyntax*>(statement->Parent);
        if (parentBlock == nullptr)
            return loopWrappedAfter();

        SyntaxNode* owner = parentBlock->Parent;
        std::size_t idx = IndexInBlock(parentBlock->Statements, statement);

        if (owner == nullptr)
        {
            return EmitStatementSequence(ctx, parentBlock, idx, activeStack, leaveActiveTries, loopWrappedAfter);
        }

        if (owner->Kind == SyntaxKind::IfStatement || owner->Kind == SyntaxKind::UnlessStatement)
        {
            auto* clause = static_cast<ConditionalClauseSyntax*>(owner);
            Continuation outerAfter = [&, loopWrappedAfter, clause]() -> bool
            {
                StatementSyntax* topClause = nullptr;
                std::size_t outerIdx = 0;
                StatementsBlockSyntax* outerBlock = FindConditionalChainOuterBlock(clause, topClause, outerIdx);
                if (outerBlock == nullptr)
                    return loopWrappedAfter();

                return EmitStatementSequence(ctx, outerBlock, outerIdx, activeStack, leaveActiveTries, loopWrappedAfter);
            };

            std::vector<std::size_t> afterJumpBacktracks;
            Continuation chainAfter = [&, outerAfter]() -> bool
            {
                std::size_t afterAddress = ctx.Code.size();
                for (std::size_t bt : afterJumpBacktracks)
                    PatchJumpTarget(ctx.Code, bt, afterAddress);
                
                return outerAfter();
            };

            return EmitConditionalChainBody(ctx, clause, parentBlock, idx, activeStack, leaveActiveTries, afterJumpBacktracks, chainAfter);
        }

        if (owner->Kind == SyntaxKind::ElseStatement)
        {
            auto* clause = static_cast<ElseStatementSyntax*>(owner);
            Continuation outerAfter = [&, loopWrappedAfter, clause]() -> bool
            {
                StatementSyntax* topClause = nullptr;
                std::size_t outerIdx = 0;
                StatementsBlockSyntax* outerBlock = FindConditionalChainOuterBlock(clause, topClause, outerIdx);
                if (outerBlock == nullptr)
                    return loopWrappedAfter();

                return EmitStatementSequence(ctx, outerBlock, outerIdx, activeStack, leaveActiveTries, loopWrappedAfter);
            };

            std::vector<std::size_t> afterJumpBacktracks;
            Continuation chainAfter = [&, outerAfter]() -> bool
            {
                std::size_t afterAddress = ctx.Code.size();
                for (std::size_t bt : afterJumpBacktracks)
                    PatchJumpTarget(ctx.Code, bt, afterAddress);

                return outerAfter();
            };

            return EmitConditionalChainBody(ctx, clause, parentBlock, idx, activeStack, leaveActiveTries, afterJumpBacktracks, chainAfter);
        }

        if (owner->Kind == SyntaxKind::TryStatement)
        {
            auto* tryStmt = static_cast<TryStatementSyntax*>(owner);
            std::vector<TryStatementSyntax*> innerStack = activeStack;
            if (std::find(innerStack.begin(), innerStack.end(), tryStmt) == innerStack.end())
                innerStack.push_back(tryStmt);

            Continuation outerAfter = [&, loopWrappedAfter, tryStmt, leaveActiveTries]() -> bool
            {
                return EmitAfterTry(ctx, tryStmt, activeStack, leaveActiveTries, loopWrappedAfter);
            };

            return EmitStatementSequence(ctx, parentBlock, idx, innerStack, false, outerAfter);
        }

        if (owner->Kind == SyntaxKind::CatchClause)
        {
            auto* catchClause = static_cast<CatchClauseSyntax*>(owner);
            auto* tryStmt = static_cast<TryStatementSyntax*>(catchClause->Parent);
            ExceptionRegion& region = ctx.Regions[tryStmt];

            Continuation catchAfter = [&]() -> bool
            {
                if (region.AfterTryAddress != 0)
                {
                    ctx.Encoder.EmitJump(ctx.Code, region.AfterTryAddress);
                }
                else
                {
                    std::size_t backtrack = ctx.Code.size();
                    ctx.Encoder.EmitJump(ctx.Code, 0);
                    region.CatchBodyEndBacktracks.push_back(backtrack);
                }
                return true;
            };

            return EmitStatementSequence(ctx, parentBlock, idx, activeStack, leaveActiveTries, catchAfter);
        }

        if (IsLoopStatementKind(owner->Kind))
        {
            auto it = ctx.LoopLabelsByStatement.find(static_cast<StatementSyntax*>(owner));
            if (it != ctx.LoopLabelsByStatement.end())
            {
                std::size_t blockEnd = it->second.BlockEnd;
                Continuation loopEpilogue = [&, blockEnd]() -> bool
                {
                    ctx.Encoder.EmitJump(ctx.Code, blockEnd);
                    return true;
                };

                return EmitStatementSequence(ctx, parentBlock, idx, activeStack, leaveActiveTries, loopEpilogue);
            }
        }

        return EmitStatementSequence(ctx, parentBlock, idx, activeStack, leaveActiveTries, loopWrappedAfter);
    }
}

void AsyncEmissionPass::Run(const AsyncMethodInfo& info, ProgramVirtualImage& program)
{
    if (info.TaskField->SlotIndex == std::numeric_limits<std::uint32_t>::max())
    {
        Diagnostics.ReportError(info.Syntax->IdentifierToken, L"Internal: state-machine field layout was not applied (" + info.StateMachineClass->FullName + L")");
        return;
    }

    ByteCodeEncoder encoder;

    // -------------------------------------------------------------------------
    // Generated state-machine constructor: empty body.
    // -------------------------------------------------------------------------
    {
        auto& code = info.StateMachineCtor->ExecutableByteCode;
        code.clear();
        encoder.EmitReturn(code);
    }

    // -------------------------------------------------------------------------
    // Original async method: replaced by a factory.
    // -------------------------------------------------------------------------
    {
        auto& code = info.Method->ExecutableByteCode;
        code.clear();
        EmitFactoryBody(info, code, encoder);
    }

    // -------------------------------------------------------------------------
    // State-machine MoveNext.
    // -------------------------------------------------------------------------
    {
        auto& code = info.MoveNext->ExecutableByteCode;
        code.clear();
        EmitMoveNextBody(info, code, program, encoder);
    }
}

void AsyncEmissionPass::EmitFactoryBody(const AsyncMethodInfo& info, std::vector<std::byte>& code, ByteCodeEncoder& encoder)
{
    TypeSymbol* taskType = info.Method->ReturnType;
    if (!IsAsyncReturnType(taskType))
    {
        Diagnostics.ReportError(info.Syntax->IdentifierToken, L"Async state-machine lowering supports only 'Task' or 'ValueTask<T>' return types");
        return;
    }

    ClassSymbol* taskClass = nullptr;
    if (GenericTypeSymbol* generic = dynamic_cast<GenericTypeSymbol*>(taskType))
        taskClass = static_cast<ClassSymbol*>(generic->UnderlayingType);
    else
        taskClass = static_cast<ClassSymbol*>(taskType);

    ConstructorSymbol* taskCtor = nullptr;
    if (taskClass != nullptr && !taskClass->Constructors.empty())
        taskCtor = taskClass->Constructors[0];

    MethodSymbol* completeMethod = FindCompleteMethod(taskType);
    MethodSymbol* internalRootMethod = FindInternalRootMethod(taskType);

    bool needsComplete = IsTaskReturnType(taskType);
    if (taskCtor == nullptr || internalRootMethod == nullptr || (needsComplete && completeMethod == nullptr))
    {
        Diagnostics.ReportError(info.Syntax->IdentifierToken, L"Async task type is missing required constructor, 'InternalRoot', or 'Complete' method");
        return;
    }

    std::uint16_t tempSlotStateMachine = static_cast<std::uint16_t>(info.Method->GetEvalStackArgumentsCount() + info.Method->AddVariableCount());
    std::uint16_t tempSlotTask = static_cast<std::uint16_t>(info.Method->GetEvalStackArgumentsCount() + info.Method->AddVariableCount());

    std::uint16_t tempSlotOuterThis = 0;
    bool hasOuterThis = info.OuterThisField != nullptr;
    if (hasOuterThis)
    {
        tempSlotOuterThis = static_cast<std::uint16_t>(info.Method->GetEvalStackArgumentsCount() + info.Method->AddVariableCount());

        encoder.EmitLoadVarible(code, 0);              // original 'this'
        encoder.EmitStoreVarible(code, tempSlotOuterThis);
    }

    // stateMachine = new StateMachine();
    encoder.EmitNewObject(code, info.StateMachineClass, info.StateMachineCtor);
    encoder.EmitStoreVarible(code, tempSlotStateMachine);

    // stateMachine._outerThis = this;   (for instance methods)
    if (hasOuterThis)
    {
        encoder.EmitLoadVarible(code, tempSlotStateMachine); // stateMachine
        encoder.EmitLoadVarible(code, tempSlotOuterThis);    // original 'this'
        encoder.EmitStoreField(code, info.OuterThisField->SlotIndex);
    }

    // Copy lifted parameters into the state machine so MoveNext can read them
    // after resuming from an await.
    for (const auto& lifted : info.LiftedParameters)
    {
        encoder.EmitLoadVarible(code, tempSlotStateMachine);
        encoder.EmitLoadVarible(code, lifted.OriginalSlot);
        encoder.EmitStoreField(code, lifted.Field->SlotIndex);
    }

    // stateMachine._task = new Task();
    encoder.EmitNewObject(code, taskType, taskCtor);
    encoder.EmitStoreVarible(code, tempSlotTask);

    encoder.EmitLoadVarible(code, tempSlotStateMachine);
    encoder.EmitLoadVarible(code, tempSlotTask);
    encoder.EmitStoreField(code, info.TaskField->SlotIndex);

    // Root the returned task so it survives until it is completed/faulted.
    if (internalRootMethod != nullptr)
    {
        encoder.EmitLoadVarible(code, tempSlotStateMachine);
        encoder.EmitLoadField(code, info.TaskField->SlotIndex);
        encoder.EmitCallMethodSymbol(code, internalRootMethod);
    }

    // stateMachine.MoveNext();
    encoder.EmitLoadVarible(code, tempSlotStateMachine);
    encoder.EmitCallMethodSymbol(code, info.MoveNext);

    // return stateMachine._task;
    encoder.EmitLoadVarible(code, tempSlotStateMachine);
    encoder.EmitLoadField(code, info.TaskField->SlotIndex);
    encoder.EmitReturn(code);
}

void AsyncEmissionPass::EmitMoveNextBody(const AsyncMethodInfo& info, std::vector<std::byte>& code, ProgramVirtualImage& program, ByteCodeEncoder& encoder)
{
    EmitMoveNextBodyWithRegions(info, code, program, encoder);
}

void AsyncEmissionPass::EmitMoveNextBodyWithRegions(const AsyncMethodInfo& info, std::vector<std::byte>& code, ProgramVirtualImage& program, ByteCodeEncoder& encoder)
{
    ParameterSlotRemapper parameterRemapper(info);
    const std::size_t awaitCount = info.AwaitSites.size();

    // Map each await expression to its index for fast lookup while walking the AST.
    std::unordered_map<AwaitExpressionSyntax*, std::size_t> awaitIndexMap;
    std::unordered_map<TryStatementSyntax*, ExceptionRegion> regions;

    for (std::size_t i = 0; i < awaitCount; ++i)
        awaitIndexMap[info.AwaitSites[i].Expression] = i;

    // Backtracks for jumps to each segment start (state 0..awaitCount).
    std::vector<std::vector<std::size_t>> segmentBacktracks(awaitCount + 1);
    std::vector<std::size_t> segmentAddresses(awaitCount + 1, std::numeric_limits<std::size_t>::max());
    std::vector<std::size_t> segmentFallThroughBacktracks;

    MoveNextEmitter ctx{ Model, Diagnostics, info, code, program, encoder, awaitIndexMap, regions };

    // -------------------------------------------------------------------------
    // Top-level exception guard: any unhandled exception thrown while running
    // a segment (including when resuming after an await) is captured and stored
    // on the returned task via SetException.
    // -------------------------------------------------------------------------
    std::size_t topEnterTryBacktrack = code.size();
    encoder.EmitEnterTry(code, 0);

    // -------------------------------------------------------------------------
    // Entry dispatch: switch (_state) { case 0: goto segment0; ... }
    // -------------------------------------------------------------------------
    for (std::size_t i = 0; i <= awaitCount; ++i)
    {
        encoder.EmitLoadVarible(code, 0); // this
        encoder.EmitLoadField(code, info.StateField->SlotIndex);
        encoder.EmitLoadConstInt64(code, static_cast<std::int64_t>(i));
        encoder.EmitCompareEqual(code);
        segmentBacktracks[i].push_back(code.size());
        encoder.EmitJumpTrue(code, 0);
    }

    encoder.EmitReturn(code);

    // -------------------------------------------------------------------------
    // Emit each segment of the method.  Segments are emitted sequentially so
    // that the normal control flow of segment 0 is never disturbed by later
    // resume blocks; any segment that falls through jumps to the shared normal
    // exit at the end of the method.
    // -------------------------------------------------------------------------
    bool moveNextFellThrough = false;
    StatementsBlockSyntax* body = info.Syntax->Body.get();

    for (std::size_t segment = 0; segment <= awaitCount; ++segment)
    {
        segmentAddresses[segment] = code.size();
        for (std::size_t backtrack : segmentBacktracks[segment])
            PatchJumpTarget(code, backtrack, segmentAddresses[segment]);

        const std::vector<TryStatementSyntax*>& activeStack =
            (segment == 0) ? std::vector<TryStatementSyntax*>() : info.AwaitSites[segment - 1].ActiveTryStack;

        // Restore lifted parameters and locals into their MoveNext slots before
        // executing any user code that may reference them.
        RestoreLiftedParametersAndLocals(info, code, encoder);

        if (segment > 0)
        {
            // Re-establish the active try regions and any defers that were in
            // scope at the await site before consuming the previous awaiter;
            // GetResult may throw, and that throw must be caught.
            const AwaitSite& previousSite = info.AwaitSites[segment - 1];
            EmitResumeDefersAndEnterTries(ctx, previousSite);

            ConsumePreviousAwaiter(ctx, segment - 1);


            if (info.CurrentExceptionField != nullptr && previousSite.EnclosingCatch != nullptr)
            {
                CatchClauseSyntax* clause = previousSite.EnclosingCatch;

                encoder.EmitLoadVarible(code, 0);
                encoder.EmitLoadField(code, info.CurrentExceptionField->SlotIndex);

                if (clause->Symbol != nullptr)
                {
                    encoder.EmitStoreVarible(code, static_cast<std::uint16_t>(clause->Symbol->SlotIndex));
                    encoder.EmitLoadVarible(code, static_cast<std::uint16_t>(clause->Symbol->SlotIndex));
                }

                encoder.EmitStoreCurrentException(code);
            }
        }

        StatementSyntax* nextStmt = (segment == 0)
            ? (body->Statements.empty() ? nullptr : body->Statements[0].get()) : info.AwaitSites[segment - 1].NextStatement;

        bool segmentFellThrough = false;
        if (nextStmt == nullptr)
        {
            if (activeStack.empty())
            {
                segmentFellThrough = true;
            }
            else
            {
                // The await was the last statement in an active try body.
                // Leave the innermost try and continue from the code that
                // follows it in segment 0.
                segmentFellThrough = EmitAfterTry(ctx, activeStack.back(), std::vector<TryStatementSyntax*>(activeStack.begin(), activeStack.end() - 1), true, []() -> bool { return true; });
            }
        }
        else
        {
            segmentFellThrough = EmitStatementContinuation(ctx, nextStmt, activeStack, true, []() -> bool { return true; });
        }

        if (segmentFellThrough)
        {
            moveNextFellThrough = true;
            std::size_t backtrack = code.size();
            encoder.EmitJump(code, 0);
            segmentFallThroughBacktracks.push_back(backtrack);
        }
    }

    // -------------------------------------------------------------------------
    // Shared normal exit and top-level fault handler.
    // -------------------------------------------------------------------------
    std::size_t topTryEndJumpBacktrack = 0;
    if (moveNextFellThrough)
    {
        encoder.EmitLeaveTry(code);
        topTryEndJumpBacktrack = code.size();
        encoder.EmitJump(code, 0);
    }

    std::size_t topHandlerStart = code.size();
    MethodSymbol* setExceptionMethod = FindSetExceptionMethod(info.Method->ReturnType);
    if (setExceptionMethod != nullptr)
    {
        std::uint16_t exceptionSlot = static_cast<std::uint16_t>(info.MoveNext->GetEvalStackArgumentsCount() + info.MoveNext->AddVariableCount());

        encoder.EmitStoreVarible(code, exceptionSlot);
        encoder.EmitLoadVarible(code, exceptionSlot);
        encoder.EmitLoadVarible(code, 0);
        encoder.EmitLoadField(code, info.TaskField->SlotIndex);
        encoder.EmitCallMethodSymbol(code, setExceptionMethod);
    }
    else
    {
        encoder.EmitPop(code);
    }

    encoder.EmitReturn(code);

    std::size_t normalExit = code.size();
    EmitTaskComplete(ctx);
    encoder.EmitReturn(code);

    PatchJumpTarget(code, topEnterTryBacktrack, topHandlerStart);
    if (moveNextFellThrough)
        PatchJumpTarget(code, topTryEndJumpBacktrack, normalExit);

    for (std::size_t backtrack : segmentFallThroughBacktracks)
        PatchJumpTarget(code, backtrack, normalExit);

    // Patch every user try ENTER_TRY site to the corresponding catch handler.
    for (auto& pair : regions)
    {
        ExceptionRegion& region = pair.second;
        if (region.HandlerAddress != 0)
        {
            for (std::size_t backtrack : region.EnterBacktracks)
                PatchJumpTarget(code, backtrack, region.HandlerAddress);
        }
    }
}
