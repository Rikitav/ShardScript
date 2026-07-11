#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/ScopeVisitor.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
//#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
//#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>

#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/QualifiedNameTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/PredefinedTypeSyntax.hpp>

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

	private:
		void ImportNamespace(shard::UsingDirectiveSyntax* node, shard::NamespaceNode* nsNode);
		bool IsAmbiguousName(const std::wstring& name);
		shard::NamespaceNode* ResolveNamespaceType(shard::TypeSyntax* type);

	public:

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax* node) override;
		void VisitInterfaceDeclaration(shard::InterfaceDeclarationSyntax* node) override;
		void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax* node) override;
		void VisitEnumDeclaration(shard::EnumDeclarationSyntax* node) override;

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
		void VisitQualifiedNameType(shard::QualifiedNameTypeSyntax* node) override;
		void VisitArrayType(shard::ArrayTypeSyntax* node) override;
		void VisitNullableType(shard::NullableTypeSyntax* node) override;
		void VisitGenericType(shard::GenericTypeSyntax* node) override;
		void VisitDelegateType(shard::DelegateTypeSyntax* node) override;
	};
}