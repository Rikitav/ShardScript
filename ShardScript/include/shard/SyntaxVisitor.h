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
        SymbolTable* Table;
        NamespaceTree* Namespaces;
        DiagnosticsContext& Diagnostics;

        inline SyntaxVisitor(SemanticModel& model, DiagnosticsContext& diagnostics)
            : Table(model.Table), Namespaces(model.Namespaces), Diagnostics(diagnostics) { }

        template<typename T>
        inline T *const LookupSymbol(SyntaxNode *const node)
        {
            return static_cast<T *const>(Table->LookupSymbol(node));
        }

	public:
        virtual void VisitSyntaxTree(SyntaxTree& tree);
        virtual void VisitCompilationUnit(CompilationUnitSyntax *const node);
        virtual void VisitUsingDirective(UsingDirectiveSyntax *const node);

        virtual void VisitTypeDeclaration(MemberDeclarationSyntax *const node);
        virtual void VisitNamespaceDeclaration(NamespaceDeclarationSyntax *const node);
        virtual void VisitClassDeclaration(ClassDeclarationSyntax *const node);
        virtual void VisitStructDeclaration(StructDeclarationSyntax *const node);
        virtual void VisitDelegateDeclaration(DelegateDeclarationSyntax *const node);

        virtual void VisitMemberDeclaration(MemberDeclarationSyntax *const node);
        virtual void VisitMethodDeclaration(MethodDeclarationSyntax *const node);
        virtual void VisitConstructorDeclaration(ConstructorDeclarationSyntax *const node);
        virtual void VisitFieldDeclaration(FieldDeclarationSyntax *const node);
        virtual void VisitPropertyDeclaration(PropertyDeclarationSyntax *const node);
        virtual void VisitIndexatorDeclaration(IndexatorDeclarationSyntax *const node);
		virtual void VisitAccessorDeclaration(AccessorDeclarationSyntax *const node);

        virtual void VisitStatementsBlock(StatementsBlockSyntax *const node);
        virtual void VisitStatement(StatementSyntax *const node);
        virtual void VisitExpressionStatement(ExpressionStatementSyntax *const node);
        virtual void VisitVariableStatement(VariableStatementSyntax *const node);
        virtual void VisitReturnStatement(ReturnStatementSyntax *const node);
        virtual void VisitThrowStatement(ThrowStatementSyntax *const node);
        virtual void VisitBreakStatement(BreakStatementSyntax *const node);
        virtual void VisitContinueStatement(ContinueStatementSyntax *const node);

        virtual void VisitWhileStatement(WhileStatementSyntax *const node);
        virtual void VisitForStatement(ForStatementSyntax *const node);
        virtual void VisitUntilStatement(UntilStatementSyntax *const node);

        virtual void VisitConditionalClause(ConditionalClauseBaseSyntax *const node);
        virtual void VisitIfStatement(IfStatementSyntax *const node);
        virtual void VisitUnlessStatement(UnlessStatementSyntax *const node);
        virtual void VisitElseStatement(ElseStatementSyntax *const node);

        virtual void VisitExpression(ExpressionSyntax *const node);
        virtual void VisitLiteralExpression(LiteralExpressionSyntax *const node);
        virtual void VisitBinaryExpression(BinaryExpressionSyntax *const node);
        virtual void VisitUnaryExpression(UnaryExpressionSyntax *const node);
        virtual void VisitObjectCreationExpression(ObjectExpressionSyntax *const node);
        virtual void VisitCollectionExpression(CollectionExpressionSyntax *const node);
        virtual void VisitLambdaExpression(LambdaExpressionSyntax *const node);
        virtual void VisitTernaryExpression(TernaryExpressionSyntax *const node);

        virtual void VisitInvocationExpression(InvokationExpressionSyntax *const node);
        virtual void VisitMemberAccessExpression(MemberAccessExpressionSyntax *const node);
        virtual void VisitIndexatorExpression(IndexatorExpressionSyntax *const node);

        virtual void VisitArgumentsList(ArgumentsListSyntax *const node);
        virtual void VisitArgument(ArgumentSyntax *const node);
        virtual void VisitParametersList(ParametersListSyntax *const node);
        virtual void VisitParameter(ParameterSyntax *const node);
        virtual void VisitIndexatorList(IndexatorListSyntax *const node);
        virtual void VisitTypeParametersList(TypeParametersListSyntax *const node);
        virtual void VisitTypeArgumentsList(TypeArgumentsListSyntax *const node);

        virtual void VisitType(TypeSyntax *const node);
        virtual void VisitPredefinedType(PredefinedTypeSyntax *const node);
        virtual void VisitIdentifierNameType(IdentifierNameTypeSyntax *const node);
        virtual void VisitArrayType(ArrayTypeSyntax *const node);
        virtual void VisitNullableType(NullableTypeSyntax *const node);
        virtual void VisitGenericType(GenericTypeSyntax *const node);
        virtual void VisitDelegateType(DelegateTypeSyntax *const node);
	};
}
