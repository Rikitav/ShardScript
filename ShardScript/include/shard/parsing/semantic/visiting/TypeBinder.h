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
	class SHARD_API TypeBinder : public SyntaxVisitor, public ScopeVisitor
	{
	public:
		inline TypeBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax *const node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax *const node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax *const node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax *const node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;

		void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node) override;

		void VisitParameter(shard::ParameterSyntax *const node) override;

		void VisitPredefinedType(shard::PredefinedTypeSyntax *const node) override;
		void VisitIdentifierNameType(shard::IdentifierNameTypeSyntax *const node) override;
		void VisitArrayType(shard::ArrayTypeSyntax *const node) override;
		void VisitNullableType(shard::NullableTypeSyntax *const node) override;
		void VisitGenericType(shard::GenericTypeSyntax *const node) override;
		void VisitDelegateType(shard::DelegateTypeSyntax *const node) override;
	};
}