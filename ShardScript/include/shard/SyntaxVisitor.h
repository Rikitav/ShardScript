#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/SyntaxTree.h>
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
        inline T *const LookupSymbol(shard::SyntaxNode *const node)
        {
            return static_cast<T *const>(Table->LookupSymbol(node));
        }

	public:
        virtual void VisitSyntaxTree(shard::SyntaxTree& tree);
        virtual void VisitCompilationUnit(shard::CompilationUnitSyntax *const node);
        virtual void VisitUsingDirective(shard::UsingDirectiveSyntax *const node);

        virtual void VisitTypeDeclaration(shard::MemberDeclarationSyntax *const node);
        virtual void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node);
        virtual void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node);
        virtual void VisitStructDeclaration(shard::StructDeclarationSyntax *const node);
        virtual void VisitDelegateDeclaration(shard::DelegateDeclarationSyntax *const node);

        virtual void VisitMemberDeclaration(shard::MemberDeclarationSyntax *const node);
        virtual void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node);
        virtual void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node);
        virtual void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node);
        virtual void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node);
        virtual void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node);
		virtual void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node);

        virtual void VisitStatementsBlock(shard::StatementsBlockSyntax *const node);
        virtual void VisitStatement(shard::StatementSyntax *const node);
        virtual void VisitExpressionStatement(shard::ExpressionStatementSyntax *const node);
        virtual void VisitVariableStatement(shard::VariableStatementSyntax *const node);
        virtual void VisitReturnStatement(shard::ReturnStatementSyntax *const node);
        virtual void VisitThrowStatement(shard::ThrowStatementSyntax *const node);
        virtual void VisitBreakStatement(shard::BreakStatementSyntax *const node);
        virtual void VisitContinueStatement(shard::ContinueStatementSyntax *const node);

        virtual void VisitWhileStatement(shard::WhileStatementSyntax *const node);
        virtual void VisitForStatement(shard::ForStatementSyntax *const node);
        virtual void VisitUntilStatement(shard::UntilStatementSyntax *const node);

        virtual void VisitConditionalClause(shard::ConditionalClauseBaseSyntax *const node);
        virtual void VisitIfStatement(shard::IfStatementSyntax *const node);
        virtual void VisitUnlessStatement(shard::UnlessStatementSyntax *const node);
        virtual void VisitElseStatement(shard::ElseStatementSyntax *const node);

        virtual void VisitExpression(shard::ExpressionSyntax *const node);
        virtual void VisitLiteralExpression(shard::LiteralExpressionSyntax *const node);
        virtual void VisitBinaryExpression(shard::BinaryExpressionSyntax *const node);
        virtual void VisitUnaryExpression(shard::UnaryExpressionSyntax *const node);
        virtual void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node);
        virtual void VisitCollectionExpression(shard::CollectionExpressionSyntax *const node);
        virtual void VisitLambdaExpression(shard::LambdaExpressionSyntax *const node);
        virtual void VisitTernaryExpression(shard::TernaryExpressionSyntax *const node);

        virtual void VisitInvocationExpression(shard::InvokationExpressionSyntax *const node);
        virtual void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node);
        virtual void VisitIndexatorExpression(shard::IndexatorExpressionSyntax *const node);

        virtual void VisitArgumentsList(shard::ArgumentsListSyntax *const node);
        virtual void VisitArgument(shard::ArgumentSyntax *const node);
        virtual void VisitParametersList(shard::ParametersListSyntax *const node);
        virtual void VisitParameter(shard::ParameterSyntax *const node);
        virtual void VisitIndexatorList(shard::IndexatorListSyntax *const node);
        virtual void VisitTypeParametersList(shard::TypeParametersListSyntax *const node);
        virtual void VisitTypeArgumentsList(shard::TypeArgumentsListSyntax *const node);

        virtual void VisitType(shard::TypeSyntax *const node);
        virtual void VisitPredefinedType(shard::PredefinedTypeSyntax *const node);
        virtual void VisitIdentifierNameType(shard::IdentifierNameTypeSyntax *const node);
        virtual void VisitArrayType(shard::ArrayTypeSyntax *const node);
        virtual void VisitNullableType(shard::NullableTypeSyntax *const node);
        virtual void VisitGenericType(shard::GenericTypeSyntax *const node);
        virtual void VisitDelegateType(shard::DelegateTypeSyntax *const node);
	};
}
