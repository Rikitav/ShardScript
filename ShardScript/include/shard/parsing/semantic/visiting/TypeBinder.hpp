#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/SyntaxVisitor.hpp>
#include <shard/parsing/semantic/visiting/ScopeVisitor.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
//#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
//#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>

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