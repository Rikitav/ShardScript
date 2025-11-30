#pragma once
#include <shard/parsing/visiting/SyntaxVisitor.h>
#include <shard/parsing/visiting/ScopeVisitor.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h>

namespace shard::parsing
{
	class DeclarationCollector : public SyntaxVisitor, ScopeVisitor
	{
	public:
		inline DeclarationCollector(shard::parsing::semantic::SemanticModel& model, shard::parsing::analysis::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::syntax::nodes::CompilationUnitSyntax* node) override;
		void VisitImportDirective(shard::syntax::nodes::ImportDirectiveSyntax* node) override;

		void VisitNamespaceDeclaration(shard::syntax::nodes::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::syntax::nodes::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::syntax::nodes::StructDeclarationSyntax* node) override;
		void VisitDelegateDeclaration(shard::syntax::nodes::DelegateDeclarationSyntax* node) override;

		void VisitMethodDeclaration(shard::syntax::nodes::MethodDeclarationSyntax* node) override;
		void VisitConstructorDeclaration(shard::syntax::nodes::ConstructorDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::syntax::nodes::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::syntax::nodes::PropertyDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(shard::syntax::nodes::AccessorDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::syntax::nodes::VariableStatementSyntax* node) override;

		void VisitStatementsBlock(shard::syntax::nodes::StatementsBlockSyntax* node) override;
	};
}
