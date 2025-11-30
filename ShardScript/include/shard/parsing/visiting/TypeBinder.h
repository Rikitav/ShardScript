#pragma once
#include <shard/parsing/visiting/SyntaxVisitor.h>
#include <shard/parsing/visiting/ScopeVisitor.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/syntax/symbols/TypeSymbol.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
//#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
//#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.h>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

namespace shard::parsing
{
	class TypeBinder : public SyntaxVisitor, ScopeVisitor
	{
		//shard::syntax::symbols::TypeSymbol* ResolveType(shard::syntax::nodes::TypeSyntax* typeSyntax);

	public:
		inline TypeBinder(shard::parsing::semantic::SemanticModel& model, shard::parsing::analysis::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::syntax::nodes::CompilationUnitSyntax* node) override;
		void VisitUsingDirective(shard::syntax::nodes::UsingDirectiveSyntax* node) override;
		void VisitImportDirective(shard::syntax::nodes::ImportDirectiveSyntax* node) override;

		void VisitNamespaceDeclaration(shard::syntax::nodes::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::syntax::nodes::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::syntax::nodes::StructDeclarationSyntax* node) override;
		void VisitDelegateDeclaration(shard::syntax::nodes::DelegateDeclarationSyntax* node) override;

		void VisitConstructorDeclaration(shard::syntax::nodes::ConstructorDeclarationSyntax* node) override;
		void VisitMethodDeclaration(shard::syntax::nodes::MethodDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::syntax::nodes::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::syntax::nodes::PropertyDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::syntax::nodes::VariableStatementSyntax* node) override;

		void VisitObjectCreationExpression(shard::syntax::nodes::ObjectExpressionSyntax* node) override;
		//void VisitCollectionExpression(shard::syntax::nodes::CollectionExpressionSyntax* node) override;
		//void VisitMemberAccessExpression(shard::syntax::nodes::MemberAccessExpressionSyntax* node) override;

		void VisitParameter(shard::syntax::nodes::ParameterSyntax* node) override;

		void VisitPredefinedType(shard::syntax::nodes::PredefinedTypeSyntax* node) override;
		void VisitIdentifierNameType(shard::syntax::nodes::IdentifierNameTypeSyntax* node) override;
		void VisitArrayType(shard::syntax::nodes::ArrayTypeSyntax* node) override;
		void VisitNullableType(shard::syntax::nodes::NullableTypeSyntax* node) override;
		void VisitGenericType(shard::syntax::nodes::GenericTypeSyntax* node) override;
		void VisitDelegateType(shard::syntax::nodes::DelegateTypeSyntax* node) override;
	};
}