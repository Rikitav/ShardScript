#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/visiting/SyntaxVisitor.h>
#include <shard/parsing/visiting/ScopeVisitor.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <unordered_map>
#include <vector>

namespace shard
{
	class SHARD_API ExpressionBinder : public SyntaxVisitor, ScopeVisitor
	{
		std::unordered_map<shard::ExpressionSyntax*, shard::TypeSymbol*> expressionTypes;

		bool GetIsStaticContext(const shard::ExpressionSyntax* expression);
		void SetExpressionType(shard::ExpressionSyntax* expression, shard::TypeSymbol* type);
		shard::TypeSymbol* GetExpressionType(shard::ExpressionSyntax* expression);
		shard::TypeSymbol* FindTargetReturnType(shard::SemanticScope*& scope);
		shard::TypeSymbol* ResolveLeftDenotation();
		
		shard::TypeSymbol* AnalyzeLiteralExpression(shard::LiteralExpressionSyntax* node);
		shard::TypeSymbol* AnalyzeBinaryExpression(shard::BinaryExpressionSyntax* node);
		shard::TypeSymbol* AnalyzeUnaryExpression(shard::UnaryExpressionSyntax* node);
		shard::TypeSymbol* AnalyzeObjectExpression(shard::ObjectExpressionSyntax* node);
		shard::TypeSymbol* AnalyzeCollectionExpression(shard::CollectionExpressionSyntax* node);

		shard::TypeSymbol* AnalyzeMemberAccessExpression(shard::MemberAccessExpressionSyntax* node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzePropertyAccessExpression(shard::MemberAccessExpressionSyntax* node, shard::PropertySymbol* property, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeFieldKeywordExpression(shard::MemberAccessExpressionSyntax* node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeInvokationExpression(shard::InvokationExpressionSyntax* node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeIndexatorExpression(shard::IndexatorExpressionSyntax* node, shard::TypeSymbol* currentType);

		shard::ConstructorSymbol* ResolveConstructor(shard::ObjectExpressionSyntax* node);
		shard::MethodSymbol* ResolveMethod(shard::InvokationExpressionSyntax* node, shard::TypeSymbol* currentType);
		shard::IndexatorSymbol* ResolveIndexator(shard::IndexatorExpressionSyntax* node, shard::TypeSymbol* currentType);
		
		bool MatchMethodArguments(std::vector<ParameterSymbol*> parameters, std::vector<shard::ArgumentSyntax*> arguments, shard::GenericTypeSymbol* genericType = nullptr);
		shard::TypeSymbol* SubstituteTypeParameters(shard::TypeSymbol* type, shard::GenericTypeSymbol* genericType);

		shard::TypeSymbol* AnalyzeNumberLiteral(shard::LiteralExpressionSyntax* node);

	public:
		inline ExpressionBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax* node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax* node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax* node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax* node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax* node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax* node) override;

		void VisitWhileStatement(shard::WhileStatementSyntax* node) override;
		void VisitUntilStatement(shard::UntilStatementSyntax* node) override;
		void VisitForStatement(shard::ForStatementSyntax* node) override;
		void VisitIfStatement(shard::IfStatementSyntax* node) override;
		void VisitUnlessStatement(shard::UnlessStatementSyntax* node) override;
		void VisitReturnStatement(shard::ReturnStatementSyntax* node) override;
		
		void VisitLiteralExpression(shard::LiteralExpressionSyntax* node) override;
		void VisitBinaryExpression(shard::BinaryExpressionSyntax* node) override;
		void VisitUnaryExpression(shard::UnaryExpressionSyntax* node) override;
		void VisitObjectCreationExpression(shard::ObjectExpressionSyntax* node) override;
		void VisitCollectionExpression(shard::CollectionExpressionSyntax* node) override;
		void VisitLambdaExpression(shard::LambdaExpressionSyntax* node) override;
		void VisitTernaryExpression(shard::TernaryExpressionSyntax* node) override;

		void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax* node) override;
		void VisitInvocationExpression(shard::InvokationExpressionSyntax* node) override;
		void VisitIndexatorExpression(shard::IndexatorExpressionSyntax* node) override;
	};
}

