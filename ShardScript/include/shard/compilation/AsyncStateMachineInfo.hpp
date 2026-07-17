#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace shard
{
    // Forward declarations keep this shared header light; the three async
    // lowering passes and the orchestrator all reference these types by pointer.
    class AwaitExpressionSyntax;
    class CatchClauseSyntax;
    class FieldSymbol;
    class StatementSyntax;
    class TryStatementSyntax;
    class VariableSymbol;
    class MethodSymbol;
    class MethodDeclarationSyntax;
    class ClassSymbol;
    class ConstructorSymbol;
    class ParameterSymbol;

    /// <summary>
    /// One await suspension point discovered inside an async method body.
    /// </summary>
    struct AwaitSite
    {
        AwaitExpressionSyntax* Expression = nullptr;
        FieldSymbol* AwaiterField = nullptr;

        // Statement that follows this await in the original method body.
        StatementSyntax* NextStatement = nullptr;

        // User try regions that are active at the resume point after this await, ordered from outermost to innermost.
        std::vector<TryStatementSyntax*> ActiveTryStack;

        // If the await appears as the initializer of a variable declaration, the result must be stored into that variable when the await completes.
        VariableSymbol* ResultVariable = nullptr;

        // True if the await is the expression of a return statement; on resume
        // the awaiter result is used to complete the returned task.
        bool IsReturnAwait = false;

        // If the await appears inside a catch clause, the caught exception must be
        // preserved across the suspension so the catch handler can resume correctly.
        CatchClauseSyntax* EnclosingCatch = nullptr;
    };

    /// <summary>
    /// Everything the lowering needs to know about a single async method: the
    /// await sites, the generated state-machine symbols, and the parameters and
    /// locals lifted into state-machine fields so they survive suspensions.
    /// </summary>
    struct AsyncMethodInfo
    {
        MethodSymbol* Method = nullptr;
        MethodDeclarationSyntax* Syntax = nullptr;

        ClassSymbol* StateMachineClass = nullptr;
        ConstructorSymbol* StateMachineCtor = nullptr;
        MethodSymbol* MoveNext = nullptr;

        FieldSymbol* StateField = nullptr;
        FieldSymbol* TaskField = nullptr;
        FieldSymbol* OuterThisField = nullptr;

        std::vector<AwaitSite> AwaitSites;

        // Lifted parameters and local variables.  Each original symbol gets a field
        // in the state machine so its value survives await suspensions, plus a local
        // slot inside MoveNext where the original body expects to read/write it.
        struct LiftedParameter
        {
            ParameterSymbol* Symbol = nullptr;
            FieldSymbol* Field = nullptr;
            std::uint16_t OriginalSlot = 0;
            std::uint16_t MoveNextSlot = 0;
        };

        struct LiftedLocal
        {
            VariableSymbol* Symbol = nullptr;
            FieldSymbol* Field = nullptr;
            std::uint16_t OriginalSlot = 0;
            std::uint16_t MoveNextSlot = 0;
        };

        std::vector<LiftedParameter> LiftedParameters;
        std::vector<LiftedLocal> LiftedLocals;

        // foreach/for-in loops that contain awaits need their enumerator stored in a
        // state-machine field so it survives across suspensions.
        std::unordered_map<StatementSyntax*, FieldSymbol*> EnumeratorFields;

        // True if the method contains user try/catch blocks that must be preserved
        // across await suspensions.
        bool HasTryStatements = false;

        // Field that holds the exception currently being handled when an await is
        // suspended inside a catch clause.  It is used to restore CurrentException
        // (and the catch variable) on resume.
        FieldSymbol* CurrentExceptionField = nullptr;
    };
}
