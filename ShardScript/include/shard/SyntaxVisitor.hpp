#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>

#include <optional>
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
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForInStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>

#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IsExpressionSyntax.hpp>

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
            : Table(model.Table.get()), Namespaces(model.Namespaces.get()), Diagnostics(diagnostics) { }

        template<typename T>
        inline std::optional<T*> LookupSymbol(SyntaxNode* node)
        {
            auto symbol = Table->LookupSymbol(node);
            if (!symbol.has_value())
                return std::nullopt;

            return static_cast<T*>(symbol.value());
        }

	public:
        virtual void VisitSyntaxTree(SyntaxTree& tree);
        virtual void VisitCompilationUnit(CompilationUnitSyntax* node);
        virtual void VisitUsingDirective(UsingDirectiveSyntax* node);

        virtual void VisitTypeDeclaration(MemberDeclarationSyntax* node);
        virtual void VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node);
        virtual void VisitClassDeclaration(ClassDeclarationSyntax* node);
        virtual void VisitStructDeclaration(StructDeclarationSyntax* node);
        virtual void VisitInterfaceDeclaration(InterfaceDeclarationSyntax* node);
        virtual void VisitDelegateDeclaration(DelegateDeclarationSyntax* node);

        virtual void VisitMemberDeclaration(MemberDeclarationSyntax* node);
        virtual void VisitMethodDeclaration(MethodDeclarationSyntax* node);
        virtual void VisitOperatorDeclaration(OperatorDeclarationSyntax* node);
        virtual void VisitConstructorDeclaration(ConstructorDeclarationSyntax* node);
        virtual void VisitFieldDeclaration(FieldDeclarationSyntax* node);
        virtual void VisitPropertyDeclaration(PropertyDeclarationSyntax* node);
        virtual void VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node);
		virtual void VisitAccessorDeclaration(AccessorDeclarationSyntax* node);

        virtual void VisitStatementsBlock(StatementsBlockSyntax* node);
        virtual void VisitStatement(StatementSyntax* node);
        virtual void VisitExpressionStatement(ExpressionStatementSyntax* node);
        virtual void VisitVariableStatement(VariableStatementSyntax* node);
        virtual void VisitReturnStatement(ReturnStatementSyntax* node);
        virtual void VisitThrowStatement(ThrowStatementSyntax* node);
        virtual void VisitBreakStatement(BreakStatementSyntax* node);
        virtual void VisitContinueStatement(ContinueStatementSyntax* node);
        virtual void VisitDeferStatement(DeferStatementSyntax* node);

        virtual void VisitWhileStatement(WhileStatementSyntax* node);
        virtual void VisitForStatement(ForStatementSyntax* node);
        virtual void VisitForEachStatement(ForEachStatementSyntax* node);
        virtual void VisitForInStatement(ForInStatementSyntax* node);
        virtual void VisitUntilStatement(UntilStatementSyntax* node);

        virtual void VisitConditionalClause(ConditionalClauseBaseSyntax* node);
        virtual void VisitIfStatement(IfStatementSyntax* node);
        virtual void VisitUnlessStatement(UnlessStatementSyntax* node);
        virtual void VisitElseStatement(ElseStatementSyntax* node);
        virtual void VisitTryStatement(TryStatementSyntax* node);

        virtual void VisitExpression(ExpressionSyntax* node);
        virtual void VisitLiteralExpression(LiteralExpressionSyntax* node);
        virtual void VisitBinaryExpression(BinaryExpressionSyntax* node);
        virtual void VisitUnaryExpression(UnaryExpressionSyntax* node);
        virtual void VisitObjectCreationExpression(ObjectExpressionSyntax* node);
        virtual void VisitCollectionExpression(CollectionExpressionSyntax* node);
        virtual void VisitRangeExpression(RangeExpressionSyntax* node);
        virtual void VisitLambdaExpression(LambdaExpressionSyntax* node);
        virtual void VisitTernaryExpression(TernaryExpressionSyntax* node);
        virtual void VisitIfExpression(IfExpressionSyntax* node);
        virtual void VisitSwitchExpression(SwitchExpressionSyntax* node);

        virtual void VisitInvocationExpression(InvokationExpressionSyntax* node);
        virtual void VisitMemberAccessExpression(MemberAccessExpressionSyntax* node);
        virtual void VisitIndexatorExpression(IndexatorExpressionSyntax* node);

        virtual void VisitCastExpression(CastExpressionSyntax* node);
        virtual void VisitIsExpression(IsExpressionSyntax* node);

        virtual void VisitArgumentsList(ArgumentsListSyntax* node);
        virtual void VisitArgument(ArgumentSyntax* node);
        virtual void VisitParametersList(ParametersListSyntax* node);
        virtual void VisitParameter(ParameterSyntax* node);
        virtual void VisitIndexatorList(IndexatorListSyntax* node);
        virtual void VisitTypeParametersList(TypeParametersListSyntax* node);
        virtual void VisitTypeArgumentsList(TypeArgumentsListSyntax* node);

        virtual void VisitType(TypeSyntax* node);
        virtual void VisitPredefinedType(PredefinedTypeSyntax* node);
        virtual void VisitIdentifierNameType(IdentifierNameTypeSyntax* node);
        virtual void VisitArrayType(ArrayTypeSyntax* node);
        virtual void VisitNullableType(NullableTypeSyntax* node);
        virtual void VisitGenericType(GenericTypeSyntax* node);
        virtual void VisitDelegateType(DelegateTypeSyntax* node);
	};
}
