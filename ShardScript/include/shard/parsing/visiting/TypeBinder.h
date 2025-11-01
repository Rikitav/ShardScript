#pragma once
#include <shard/parsing/visiting/SyntaxVisitor.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <stack>

namespace shard::parsing
{
	class TypeBinder : public SyntaxVisitor
	{
		shard::parsing::semantic::SymbolTable* symbolTable;
		std::stack<shard::parsing::semantic::SemanticScope*> scopeStack;
		shard::parsing::analysis::DiagnosticsContext& Diagnostics;

	public:
		inline TypeBinder(shard::parsing::semantic::SymbolTable* symbolTable, shard::parsing::analysis::DiagnosticsContext& diagnostics) : symbolTable(symbolTable), Diagnostics(diagnostics)
		{
			scopeStack.push(symbolTable->GlobalScope);
		}

		void VisitCompilationUnit(shard::syntax::nodes::CompilationUnitSyntax* node) override;
	};
}