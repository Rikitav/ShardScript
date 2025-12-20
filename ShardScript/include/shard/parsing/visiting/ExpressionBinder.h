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

namespace shard::parsing
{
	class SHARD_API ExpressionBinder : public SyntaxVisitor, ScopeVisitor
	{
		std::unordered_map<shard::syntax::nodes::ExpressionSyntax*, shard::syntax::symbols::TypeSymbol*> expressionTypes;

		bool GetIsStaticContext(const shard::syntax::nodes::ExpressionSyntax* expression);
		void SetExpressionType(shard::syntax::nodes::ExpressionSyntax* expression, shard::syntax::symbols::TypeSymbol* type);
		shard::syntax::symbols::TypeSymbol* GetExpressionType(shard::syntax::nodes::ExpressionSyntax* expression);
		shard::syntax::symbols::TypeSymbol* FindTargetReturnType(shard::parsing::semantic::SemanticScope*& scope);
		shard::syntax::symbols::TypeSymbol* ResolveLeftDenotation();
		
		shard::syntax::symbols::TypeSymbol* AnalyzeLiteralExpression(shard::syntax::nodes::LiteralExpressionSyntax* node);
		shard::syntax::symbols::TypeSymbol* AnalyzeBinaryExpression(shard::syntax::nodes::BinaryExpressionSyntax* node);
		shard::syntax::symbols::TypeSymbol* AnalyzeUnaryExpression(shard::syntax::nodes::UnaryExpressionSyntax* node);
		shard::syntax::symbols::TypeSymbol* AnalyzeObjectExpression(shard::syntax::nodes::ObjectExpressionSyntax* node);
		shard::syntax::symbols::TypeSymbol* AnalyzeCollectionExpression(shard::syntax::nodes::CollectionExpressionSyntax* node);

		shard::syntax::symbols::TypeSymbol* AnalyzeMemberAccessExpression(shard::syntax::nodes::MemberAccessExpressionSyntax* node, shard::syntax::symbols::TypeSymbol* currentType);
		shard::syntax::symbols::TypeSymbol* AnalyzePropertyAccessExpression(shard::syntax::nodes::MemberAccessExpressionSyntax* node, shard::syntax::symbols::PropertySymbol* property, shard::syntax::symbols::TypeSymbol* currentType);
		shard::syntax::symbols::TypeSymbol* AnalyzeFieldKeywordExpression(shard::syntax::nodes::MemberAccessExpressionSyntax* node, shard::syntax::symbols::TypeSymbol* currentType);
		shard::syntax::symbols::TypeSymbol* AnalyzeInvokationExpression(shard::syntax::nodes::InvokationExpressionSyntax* node, shard::syntax::symbols::TypeSymbol* currentType);
		shard::syntax::symbols::TypeSymbol* AnalyzeIndexatorExpression(shard::syntax::nodes::IndexatorExpressionSyntax* node, shard::syntax::symbols::TypeSymbol* currentType);

		shard::syntax::symbols::ConstructorSymbol* ResolveConstructor(shard::syntax::nodes::ObjectExpressionSyntax* node);
		shard::syntax::symbols::MethodSymbol* ResolveMethod(shard::syntax::nodes::InvokationExpressionSyntax* node, shard::syntax::symbols::TypeSymbol* currentType);
		shard::syntax::symbols::IndexatorSymbol* ResolveIndexator(shard::syntax::nodes::IndexatorExpressionSyntax* node, shard::syntax::symbols::TypeSymbol* currentType);
		
		bool MatchMethodArguments(shard::syntax::symbols::MethodSymbol* method, std::vector<shard::syntax::nodes::ArgumentSyntax*> arguments, shard::syntax::symbols::GenericTypeSymbol* genericType = nullptr);
		shard::syntax::symbols::TypeSymbol* SubstituteTypeParameters(shard::syntax::symbols::TypeSymbol* type, shard::syntax::symbols::GenericTypeSymbol* genericType);

		shard::syntax::symbols::TypeSymbol* AnalyzeNumberLiteral(shard::syntax::nodes::LiteralExpressionSyntax* node);

	public:
		inline ExpressionBinder(shard::parsing::semantic::SemanticModel& model, shard::parsing::analysis::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table) { }

		void VisitCompilationUnit(shard::syntax::nodes::CompilationUnitSyntax* node) override;
		void VisitUsingDirective(shard::syntax::nodes::UsingDirectiveSyntax* node) override;

		void VisitNamespaceDeclaration(shard::syntax::nodes::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::syntax::nodes::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::syntax::nodes::StructDeclarationSyntax* node) override;

		void VisitConstructorDeclaration(shard::syntax::nodes::ConstructorDeclarationSyntax* node) override;
		void VisitMethodDeclaration(shard::syntax::nodes::MethodDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::syntax::nodes::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::syntax::nodes::PropertyDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::syntax::nodes::VariableStatementSyntax* node) override;

		void VisitWhileStatement(shard::syntax::nodes::WhileStatementSyntax* node) override;
		void VisitUntilStatement(shard::syntax::nodes::UntilStatementSyntax* node) override;
		void VisitForStatement(shard::syntax::nodes::ForStatementSyntax* node) override;
		void VisitIfStatement(shard::syntax::nodes::IfStatementSyntax* node) override;
		void VisitUnlessStatement(shard::syntax::nodes::UnlessStatementSyntax* node) override;
		void VisitReturnStatement(shard::syntax::nodes::ReturnStatementSyntax* node) override;
		
		void VisitLiteralExpression(shard::syntax::nodes::LiteralExpressionSyntax* node) override;
		void VisitBinaryExpression(shard::syntax::nodes::BinaryExpressionSyntax* node) override;
		void VisitUnaryExpression(shard::syntax::nodes::UnaryExpressionSyntax* node) override;
		void VisitObjectCreationExpression(shard::syntax::nodes::ObjectExpressionSyntax* node) override;
		void VisitCollectionExpression(shard::syntax::nodes::CollectionExpressionSyntax* node) override;
		void VisitLambdaExpression(shard::syntax::nodes::LambdaExpressionSyntax* node) override;
		void VisitTernaryExpression(shard::syntax::nodes::TernaryExpressionSyntax* node) override;

		void VisitMemberAccessExpression(shard::syntax::nodes::MemberAccessExpressionSyntax* node) override;
		void VisitInvocationExpression(shard::syntax::nodes::InvokationExpressionSyntax* node) override;
		void VisitIndexatorExpression(shard::syntax::nodes::IndexatorExpressionSyntax* node) override;
	};
}

