#pragma once
#include <shard/parsing/visiting/SyntaxVisitor.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <stack>

namespace shard::parsing
{
	class TypeBinder : public SyntaxVisitor
	{
		shard::parsing::semantic::SymbolTable* symbolTable;
		std::stack<shard::parsing::semantic::SemanticScope*> scopeStack;
		shard::parsing::analysis::DiagnosticsContext& Diagnostics;

		void pushScope(shard::syntax::SyntaxSymbol* symbol);
		shard::syntax::symbols::TypeSymbol* ResolveType(shard::syntax::nodes::TypeSyntax* typeSyntax);
		bool IsSymbolAccessible(shard::syntax::SyntaxSymbol* symbol);
		shard::syntax::nodes::StatementsBlockSyntax* GenerateAutoPropertyGetterBody(shard::syntax::symbols::PropertySymbol* property, shard::syntax::nodes::PropertyDeclarationSyntax* node);
		shard::syntax::nodes::StatementsBlockSyntax* GenerateAutoPropertySetterBody(shard::syntax::symbols::PropertySymbol* property, shard::syntax::nodes::PropertyDeclarationSyntax* node);

	public:
		inline TypeBinder(shard::parsing::semantic::SymbolTable* symbolTable, shard::parsing::analysis::DiagnosticsContext& diagnostics) : symbolTable(symbolTable), Diagnostics(diagnostics)
		{
			scopeStack.push(symbolTable->GlobalScope);
		}

		void VisitCompilationUnit(shard::syntax::nodes::CompilationUnitSyntax* node) override;
		void VisitNamespaceDeclaration(shard::syntax::nodes::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::syntax::nodes::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::syntax::nodes::StructDeclarationSyntax* node) override;
		void VisitMethodDeclaration(shard::syntax::nodes::MethodDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::syntax::nodes::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::syntax::nodes::PropertyDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::syntax::nodes::VariableStatementSyntax* node) override;
		void VisitObjectCreationExpression(shard::syntax::nodes::ObjectExpressionSyntax* node) override;
	};
}