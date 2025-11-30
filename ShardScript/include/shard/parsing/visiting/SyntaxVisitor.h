#pragma once
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>

#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>
#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>

#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>

#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.h>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.h>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>

namespace shard::parsing
{
	class SyntaxVisitor
	{
    protected:
        shard::parsing::semantic::SymbolTable* Table;
        shard::parsing::semantic::NamespaceTree* Namespaces;
        shard::parsing::analysis::DiagnosticsContext& Diagnostics;

        inline SyntaxVisitor(shard::parsing::semantic::SemanticModel& model, shard::parsing::analysis::DiagnosticsContext& diagnostics)
            : Table(model.Table), Namespaces(model.Namespaces), Diagnostics(diagnostics) { }

        template<typename T>
        inline T* LookupSymbol(shard::syntax::SyntaxNode* node)
        {
            return static_cast<T*>(Table->LookupSymbol(node));
        }

	public:
        virtual void VisitSyntaxTree(shard::parsing::lexical::SyntaxTree& tree);
        virtual void VisitCompilationUnit(shard::syntax::nodes::CompilationUnitSyntax* node);
        virtual void VisitImportDirective(shard::syntax::nodes::ImportDirectiveSyntax* node);
        virtual void VisitUsingDirective(shard::syntax::nodes::UsingDirectiveSyntax* node);

        virtual void VisitTypeDeclaration(shard::syntax::nodes::MemberDeclarationSyntax* node);
        virtual void VisitNamespaceDeclaration(shard::syntax::nodes::NamespaceDeclarationSyntax* node);
        virtual void VisitClassDeclaration(shard::syntax::nodes::ClassDeclarationSyntax* node);
        virtual void VisitStructDeclaration(shard::syntax::nodes::StructDeclarationSyntax* node);
        virtual void VisitDelegateDeclaration(shard::syntax::nodes::DelegateDeclarationSyntax* node);

        virtual void VisitMemberDeclaration(shard::syntax::nodes::MemberDeclarationSyntax* node);
        virtual void VisitMethodDeclaration(shard::syntax::nodes::MethodDeclarationSyntax* node);
        virtual void VisitConstructorDeclaration(shard::syntax::nodes::ConstructorDeclarationSyntax* node);
        virtual void VisitFieldDeclaration(shard::syntax::nodes::FieldDeclarationSyntax* node);
        virtual void VisitPropertyDeclaration(shard::syntax::nodes::PropertyDeclarationSyntax* node);
		virtual void VisitAccessorDeclaration(shard::syntax::nodes::AccessorDeclarationSyntax* node);

        virtual void VisitStatementsBlock(shard::syntax::nodes::StatementsBlockSyntax* node);
        virtual void VisitStatement(shard::syntax::nodes::StatementSyntax* node);
        virtual void VisitExpressionStatement(shard::syntax::nodes::ExpressionStatementSyntax* node);
        virtual void VisitVariableStatement(shard::syntax::nodes::VariableStatementSyntax* node);
        virtual void VisitReturnStatement(shard::syntax::nodes::ReturnStatementSyntax* node);
        virtual void VisitThrowStatement(shard::syntax::nodes::ThrowStatementSyntax* node);
        virtual void VisitBreakStatement(shard::syntax::nodes::BreakStatementSyntax* node);
        virtual void VisitContinueStatement(shard::syntax::nodes::ContinueStatementSyntax* node);

        virtual void VisitWhileStatement(shard::syntax::nodes::WhileStatementSyntax* node);
        virtual void VisitForStatement(shard::syntax::nodes::ForStatementSyntax* node);
        virtual void VisitUntilStatement(shard::syntax::nodes::UntilStatementSyntax* node);

        virtual void VisitConditionalClause(shard::syntax::nodes::ConditionalClauseBaseSyntax* node);
        virtual void VisitIfStatement(shard::syntax::nodes::IfStatementSyntax* node);
        virtual void VisitUnlessStatement(shard::syntax::nodes::UnlessStatementSyntax* node);
        virtual void VisitElseStatement(shard::syntax::nodes::ElseStatementSyntax* node);

        virtual void VisitExpression(shard::syntax::nodes::ExpressionSyntax* node);
        virtual void VisitLiteralExpression(shard::syntax::nodes::LiteralExpressionSyntax* node);
        virtual void VisitBinaryExpression(shard::syntax::nodes::BinaryExpressionSyntax* node);
        virtual void VisitUnaryExpression(shard::syntax::nodes::UnaryExpressionSyntax* node);
        virtual void VisitObjectCreationExpression(shard::syntax::nodes::ObjectExpressionSyntax* node);
        virtual void VisitCollectionExpression(shard::syntax::nodes::CollectionExpressionSyntax* node);
        virtual void VisitLambdaExpression(shard::syntax::nodes::LambdaExpressionSyntax* node);
        virtual void VisitTernaryExpression(shard::syntax::nodes::TernaryExpressionSyntax* node);

        virtual void VisitInvocationExpression(shard::syntax::nodes::InvokationExpressionSyntax* node);
        virtual void VisitMemberAccessExpression(shard::syntax::nodes::MemberAccessExpressionSyntax* node);
        virtual void VisitIndexatorExpression(shard::syntax::nodes::IndexatorExpressionSyntax* node);

        virtual void VisitArgumentsList(shard::syntax::nodes::ArgumentsListSyntax* node);
        virtual void VisitIndexatorList(shard::syntax::nodes::IndexatorListSyntax* node);
        virtual void VisitArgument(shard::syntax::nodes::ArgumentSyntax* node);
        virtual void VisitParametersList(shard::syntax::nodes::ParametersListSyntax* node);
        virtual void VisitParameter(shard::syntax::nodes::ParameterSyntax* node);

        virtual void VisitType(shard::syntax::nodes::TypeSyntax* node);
        virtual void VisitPredefinedType(shard::syntax::nodes::PredefinedTypeSyntax* node);
        virtual void VisitIdentifierNameType(shard::syntax::nodes::IdentifierNameTypeSyntax* node);
        virtual void VisitArrayType(shard::syntax::nodes::ArrayTypeSyntax* node);
        virtual void VisitNullableType(shard::syntax::nodes::NullableTypeSyntax* node);
        virtual void VisitGenericType(shard::syntax::nodes::GenericTypeSyntax* node);
        virtual void VisitDelegateType(shard::syntax::nodes::DelegateTypeSyntax* node);
	};
}
