#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/SyntaxVisitor.hpp>
#include <shard/parsing/semantic/visiting/ScopeVisitor.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>

namespace shard
{
	class SHARD_API DeclarationCollector : public SyntaxVisitor, public ScopeVisitor
	{
	protected:
		void Declare(shard::SyntaxSymbol *const symbol) override;

	public:
		inline DeclarationCollector(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax *const node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax *const node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax *const node) override;

		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;
	};
}
