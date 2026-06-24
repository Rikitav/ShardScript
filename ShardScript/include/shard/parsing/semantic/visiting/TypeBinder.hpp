#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/SyntaxVisitor.hpp>
#include <shard/parsing/semantic/visiting/ScopeVisitor.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>

#include <shard/syntax/SymbolFactory.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
//#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
//#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
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
		SymbolFactory Factory;

	public:
	public:
		inline TypeBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table.get()), Factory(model.Table.get()) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax* node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax* node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax* node) override;
		void VisitInterfaceDeclaration(shard::InterfaceDeclarationSyntax* node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax* node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax* node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax* node) override;
		void VisitOperatorDeclaration(shard::OperatorDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax* node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax* node) override;
		void VisitDeferStatement(shard::DeferStatementSyntax* node) override;
		void VisitTryStatement(shard::TryStatementSyntax* node) override;

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