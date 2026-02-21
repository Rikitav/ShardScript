#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>

#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>

#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>

#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>

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
