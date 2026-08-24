#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/ScopeVisitor.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <unordered_set>

namespace shard
{
    // =========================================================================
    //  Fourth semantic-analysis phase.
    //
    //  This is the last cross-symbol checking gate. It validates every type
    //  in the symbol table, whether the symbol was produced from source code
    //  by the AST visitors or registered by a native library through
    //  SymbolBuilder. Earlier phases (DeclarationCollector, TypeBinder and
    //  ExpressionBinder) resolve names and expression types; this phase checks
    //  global invariants such as interface implementation completeness,
    //  signature compatibility and symbol-table consistency.
    // =========================================================================
    class SHARD_API SemanticValidator : public SyntaxVisitor, public ScopeVisitor
    {
    public:
        SemanticValidator(SemanticModel& model, DiagnosticsContext& diagnostics);

        // Runs the fourth phase over the syntax tree and then performs the
        // symbol-table-wide validation pass.
        void Validate(SyntaxTree& syntaxTree);

        // Validates every type that has not already been verified or marked
        // ready. This entry point is used directly after a native library
        // registers its symbols, when no syntax tree exists for them.
        void ValidateAllSymbols();

        // Binds interface implementation methods into InterfaceMethodMap without
        // emitting diagnostics. TypeBinder uses this so later phases can resolve
        // interface members before the final validation pass runs.
        static void BindInterfaceImplementations(TypeSymbol* typeSymbol, TypeSymbol* interfaceType);

    private:
        void ValidateType(TypeSymbol* typeSymbol);

        void ValidateInterfaceImplementations(TypeSymbol* typeSymbol);
        void ValidateEnumValues(TypeSymbol* typeSymbol);

        SyntaxToken GetErrorToken(SyntaxSymbol* symbol);

        static void CollectAllInterfaces(const TypeSymbol* type, std::vector<TypeSymbol*>& out, std::unordered_set<TypeSymbol*>& visited);
        static std::vector<TypeSymbol*> GetAllInterfaces(const TypeSymbol* type);
    };
}
