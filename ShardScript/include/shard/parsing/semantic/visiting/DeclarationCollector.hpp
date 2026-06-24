#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SymbolFactory.hpp>

#include <memory>

#include <shard/SyntaxVisitor.hpp>
#include <shard/parsing/semantic/visiting/ScopeVisitor.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>

namespace shard
{
	class SHARD_API DeclarationCollector : public SyntaxVisitor, public ScopeVisitor
	{
	protected:
		SymbolFactory Factory;

	public:
		inline DeclarationCollector(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table.get()), Factory(model.Table.get()) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax* node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax* node) override;
		void VisitInterfaceDeclaration(shard::InterfaceDeclarationSyntax* node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax* node) override;

		void VisitMethodDeclaration(shard::MethodDeclarationSyntax* node) override;
		void VisitOperatorDeclaration(shard::OperatorDeclarationSyntax* node) override;
		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax* node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax* node) override;
		void VisitDeferStatement(shard::DeferStatementSyntax* node) override;
		void VisitForEachStatement(shard::ForEachStatementSyntax* node) override;
		void VisitTryStatement(shard::TryStatementSyntax* node) override;

	private:
		void ApplyMethodAttributes(shard::MethodSymbol* symbol, const std::vector<std::unique_ptr<shard::AttributeSyntax>>& attributes);
	};
}
