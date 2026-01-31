#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/SyntaxVisitor.h>
#include <shard/parsing/semantic/visiting/ScopeVisitor.h>
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

namespace shard
{
	class SHARD_API TypeBinder : public SyntaxVisitor, ScopeVisitor
	{
	public:
		inline TypeBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax* node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax* node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax* node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax* node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax* node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax* node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax* node) override;

		void VisitObjectCreationExpression(shard::ObjectExpressionSyntax* node) override;

		void VisitParameter(shard::ParameterSyntax* node) override;

		void VisitPredefinedType(shard::PredefinedTypeSyntax* node) override;
		void VisitIdentifierNameType(shard::IdentifierNameTypeSyntax* node) override;
		void VisitArrayType(shard::ArrayTypeSyntax* node) override;
		void VisitNullableType(shard::NullableTypeSyntax* node) override;
		void VisitGenericType(shard::GenericTypeSyntax* node) override;
		void VisitDelegateType(shard::DelegateTypeSyntax* node) override;
	};
}