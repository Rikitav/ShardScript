#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/SyntaxVisitor.h>
#include <shard/parsing/semantic/visiting/ScopeVisitor.h>
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
	class SHARD_API ExpressionBinder : public SyntaxVisitor, public ScopeVisitor
	{
		std::unordered_map<shard::ExpressionSyntax*, shard::TypeSymbol*> expressionTypes;

		bool GetIsStaticContext(const shard::ExpressionSyntax* expression);
		void SetExpressionType(shard::ExpressionSyntax* expression, shard::TypeSymbol* type);
		shard::TypeSymbol* GetExpressionType(shard::ExpressionSyntax* expression);
		shard::TypeSymbol* FindTargetReturnType(shard::SemanticScope*& scope);
		shard::TypeSymbol* ResolveLeftDenotation();
		
		shard::TypeSymbol* AnalyzeLiteralExpression(shard::LiteralExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeBinaryExpression(shard::BinaryExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeUnaryExpression(shard::UnaryExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeObjectExpression(shard::ObjectExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeCollectionExpression(shard::CollectionExpressionSyntax *const node);

		shard::TypeSymbol* AnalyzeMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzePropertyAccessExpression(shard::MemberAccessExpressionSyntax *const node, shard::PropertySymbol* property, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeFieldKeywordExpression(shard::MemberAccessExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeInvokationExpression(shard::InvokationExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::TypeSymbol* AnalyzeIndexatorExpression(shard::IndexatorExpressionSyntax *const node, shard::TypeSymbol* currentType);

		shard::ConstructorSymbol* ResolveConstructor(shard::ObjectExpressionSyntax *const node);
		shard::MethodSymbol* ResolveMethod(shard::InvokationExpressionSyntax *const node, shard::TypeSymbol* currentType);
		shard::IndexatorSymbol* ResolveIndexator(shard::IndexatorExpressionSyntax *const node, shard::TypeSymbol* currentType);
		
		bool MatchMethodArguments(std::vector<ParameterSymbol*> parameters, std::vector<shard::ArgumentSyntax*> arguments, shard::GenericTypeSymbol* genericType = nullptr);
		shard::TypeSymbol* SubstituteTypeParameters(shard::TypeSymbol* type, shard::GenericTypeSymbol* genericType);

		shard::TypeSymbol* AnalyzeNumberLiteral(shard::LiteralExpressionSyntax *const node);
		shard::TypeSymbol* AnalyzeDoubleLiteral(shard::LiteralExpressionSyntax *const node);

	public:
		inline ExpressionBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax *const node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax *const node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax *const node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax *const node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax *const node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax *const node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax *const node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax *const node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax *const node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax *const node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax *const node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax *const node) override;

		void VisitWhileStatement(shard::WhileStatementSyntax *const node) override;
		void VisitUntilStatement(shard::UntilStatementSyntax *const node) override;
		void VisitForStatement(shard::ForStatementSyntax *const node) override;
		void VisitIfStatement(shard::IfStatementSyntax *const node) override;
		void VisitUnlessStatement(shard::UnlessStatementSyntax *const node) override;
		void VisitReturnStatement(shard::ReturnStatementSyntax *const node) override;
		
		void VisitLiteralExpression(shard::LiteralExpressionSyntax *const node) override;
		void VisitBinaryExpression(shard::BinaryExpressionSyntax *const node) override;
		void VisitUnaryExpression(shard::UnaryExpressionSyntax *const node) override;
		void VisitObjectCreationExpression(shard::ObjectExpressionSyntax *const node) override;
		void VisitCollectionExpression(shard::CollectionExpressionSyntax *const node) override;
		void VisitLambdaExpression(shard::LambdaExpressionSyntax *const node) override;
		void VisitTernaryExpression(shard::TernaryExpressionSyntax *const node) override;

		void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax *const node) override;
		void VisitInvocationExpression(shard::InvokationExpressionSyntax *const node) override;
		void VisitIndexatorExpression(shard::IndexatorExpressionSyntax *const node) override;
	};
}

