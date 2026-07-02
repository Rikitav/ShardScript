#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>

#include <optional>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/NamespaceTree.hpp>

#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/TypeArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/TypeParametersListSyntax.hpp>

#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>

#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TypeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IsExpressionSyntax.hpp>

#include <shard/parsing/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/DelegateTypeSyntax.hpp>

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
        virtual void VisitEnumDeclaration(EnumDeclarationSyntax* node);
        virtual void VisitEnumFieldDeclaration(EnumFieldDeclarationSyntax* node);

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
        virtual void VisitTypeExpression(TypeExpressionSyntax* node);
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
