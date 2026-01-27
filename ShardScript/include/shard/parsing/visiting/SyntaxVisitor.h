#pragma once
#include <shard/ShardScriptAPI.h>

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
#include <shard/syntax/nodes/TypeArgumentsListSyntax.h>
#include <shard/syntax/nodes/TypeParametersListSyntax.h>

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
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h>

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
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>

#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.h>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.h>

namespace shard
{
	class SHARD_API SyntaxVisitor
	{
    protected:
        shard::SymbolTable* Table;
        shard::NamespaceTree* Namespaces;
        shard::DiagnosticsContext& Diagnostics;

        inline SyntaxVisitor(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
            : Table(model.Table), Namespaces(model.Namespaces), Diagnostics(diagnostics) { }

        template<typename T>
        inline T* LookupSymbol(shard::SyntaxNode* node)
        {
            return static_cast<T*>(Table->LookupSymbol(node));
        }

	public:
        virtual void VisitSyntaxTree(shard::SyntaxTree& tree);
        virtual void VisitCompilationUnit(shard::CompilationUnitSyntax* node);
        virtual void VisitUsingDirective(shard::UsingDirectiveSyntax* node);

        virtual void VisitTypeDeclaration(shard::MemberDeclarationSyntax* node);
        virtual void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax* node);
        virtual void VisitClassDeclaration(shard::ClassDeclarationSyntax* node);
        virtual void VisitStructDeclaration(shard::StructDeclarationSyntax* node);
        virtual void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax* node);

        virtual void VisitMemberDeclaration(shard::MemberDeclarationSyntax* node);
        virtual void VisitMethodDeclaration(shard::MethodDeclarationSyntax* node);
        virtual void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax* node);
        virtual void VisitFieldDeclaration(shard::FieldDeclarationSyntax* node);
        virtual void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax* node);
        virtual void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax* node);
		virtual void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax* node);

        virtual void VisitStatementsBlock(shard::StatementsBlockSyntax* node);
        virtual void VisitStatement(shard::StatementSyntax* node);
        virtual void VisitExpressionStatement(shard::ExpressionStatementSyntax* node);
        virtual void VisitVariableStatement(shard::VariableStatementSyntax* node);
        virtual void VisitReturnStatement(shard::ReturnStatementSyntax* node);
        virtual void VisitThrowStatement(shard::ThrowStatementSyntax* node);
        virtual void VisitBreakStatement(shard::BreakStatementSyntax* node);
        virtual void VisitContinueStatement(shard::ContinueStatementSyntax* node);

        virtual void VisitWhileStatement(shard::WhileStatementSyntax* node);
        virtual void VisitForStatement(shard::ForStatementSyntax* node);
        virtual void VisitUntilStatement(shard::UntilStatementSyntax* node);

        virtual void VisitConditionalClause(shard::ConditionalClauseBaseSyntax* node);
        virtual void VisitIfStatement(shard::IfStatementSyntax* node);
        virtual void VisitUnlessStatement(shard::UnlessStatementSyntax* node);
        virtual void VisitElseStatement(shard::ElseStatementSyntax* node);

        virtual void VisitExpression(shard::ExpressionSyntax* node);
        virtual void VisitLiteralExpression(shard::LiteralExpressionSyntax* node);
        virtual void VisitBinaryExpression(shard::BinaryExpressionSyntax* node);
        virtual void VisitUnaryExpression(shard::UnaryExpressionSyntax* node);
        virtual void VisitObjectCreationExpression(shard::ObjectExpressionSyntax* node);
        virtual void VisitCollectionExpression(shard::CollectionExpressionSyntax* node);
        virtual void VisitLambdaExpression(shard::LambdaExpressionSyntax* node);
        virtual void VisitTernaryExpression(shard::TernaryExpressionSyntax* node);

        virtual void VisitInvocationExpression(shard::InvokationExpressionSyntax* node);
        virtual void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax* node);
        virtual void VisitIndexatorExpression(shard::IndexatorExpressionSyntax* node);

        virtual void VisitArgumentsList(shard::ArgumentsListSyntax* node);
        virtual void VisitArgument(shard::ArgumentSyntax* node);
        virtual void VisitParametersList(shard::ParametersListSyntax* node);
        virtual void VisitParameter(shard::ParameterSyntax* node);
        virtual void VisitIndexatorList(shard::IndexatorListSyntax* node);
        virtual void VisitTypeParametersList(shard::TypeParametersListSyntax* node);
        virtual void VisitTypeArgumentsList(shard::TypeArgumentsListSyntax* node);

        virtual void VisitType(shard::TypeSyntax* node);
        virtual void VisitPredefinedType(shard::PredefinedTypeSyntax* node);
        virtual void VisitIdentifierNameType(shard::IdentifierNameTypeSyntax* node);
        virtual void VisitArrayType(shard::ArrayTypeSyntax* node);
        virtual void VisitNullableType(shard::NullableTypeSyntax* node);
        virtual void VisitGenericType(shard::GenericTypeSyntax* node);
        virtual void VisitDelegateType(shard::DelegateTypeSyntax* node);
	};
}
