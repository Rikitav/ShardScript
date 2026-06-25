#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/SyntaxVisitor.hpp>
#include <shard/parsing/semantic/visiting/ScopeVisitor.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>
#include <shard/parsing/semantic/SemanticModel.hpp>

#include <shard/syntax/SymbolFactory.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IsExpressionSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForInStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <unordered_map>
#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API ExpressionBinder : public SyntaxVisitor, public ScopeVisitor
	{
		SymbolFactory Factory;
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

		bool MatchMethodArguments(SyntaxToken blameToken, std::vector<ParameterSymbol*>& parameters, std::vector<std::unique_ptr<ArgumentSyntax>>& arguments, GenericTypeSymbol* genericType = nullptr, std::size_t parameterOffset = 0);
		shard::TypeSymbol* SubstituteTypeParameters(shard::TypeSymbol* type, shard::GenericTypeSymbol* genericType);

		shard::TypeSymbol* AnalyzeNumberLiteral(shard::LiteralExpressionSyntax* node);
		shard::TypeSymbol* AnalyzeDoubleLiteral(shard::LiteralExpressionSyntax* node);

	public:
		inline ExpressionBinder(shard::SemanticModel& model, shard::DiagnosticsContext& diagnostics)
			: SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table.get()), Factory(model.Table.get()) { }

		void VisitCompilationUnit(shard::CompilationUnitSyntax* node) override;
		void VisitUsingDirective(shard::UsingDirectiveSyntax* node) override;

		void VisitNamespaceDeclaration(shard::NamespaceDeclarationSyntax* node) override;
		void VisitClassDeclaration(shard::ClassDeclarationSyntax* node) override;
		void VisitStructDeclaration(shard::StructDeclarationSyntax* node) override;

		void VisitConstructorDeclaration(shard::ConstructorDeclarationSyntax* node) override;
		void VisitMethodDeclaration(shard::MethodDeclarationSyntax* node) override;
		void VisitOperatorDeclaration(shard::OperatorDeclarationSyntax* node) override;
		void VisitFieldDeclaration(shard::FieldDeclarationSyntax* node) override;
		void VisitPropertyDeclaration(shard::PropertyDeclarationSyntax* node) override;
		void VisitIndexatorDeclaration(shard::IndexatorDeclarationSyntax* node) override;
		void VisitAccessorDeclaration(shard::AccessorDeclarationSyntax* node) override;
		void VisitVariableStatement(shard::VariableStatementSyntax* node) override;

		void VisitWhileStatement(shard::WhileStatementSyntax* node) override;
		void VisitUntilStatement(shard::UntilStatementSyntax* node) override;
		void VisitForStatement(shard::ForStatementSyntax* node) override;
		void VisitForEachStatement(shard::ForEachStatementSyntax* node) override;
		void VisitForInStatement(shard::ForInStatementSyntax* node) override;
		void VisitIfStatement(shard::IfStatementSyntax* node) override;
		void VisitUnlessStatement(shard::UnlessStatementSyntax* node) override;
		void VisitReturnStatement(shard::ReturnStatementSyntax* node) override;
		void VisitDeferStatement(shard::DeferStatementSyntax* node) override;
		
		void VisitLiteralExpression(shard::LiteralExpressionSyntax* node) override;
		void VisitBinaryExpression(shard::BinaryExpressionSyntax* node) override;
		void VisitUnaryExpression(shard::UnaryExpressionSyntax* node) override;
		void VisitObjectCreationExpression(shard::ObjectExpressionSyntax* node) override;
		void VisitCollectionExpression(shard::CollectionExpressionSyntax* node) override;
		void VisitRangeExpression(shard::RangeExpressionSyntax* node) override;
		void VisitLambdaExpression(shard::LambdaExpressionSyntax* node) override;
		void VisitTernaryExpression(shard::TernaryExpressionSyntax* node) override;
		void VisitIfExpression(shard::IfExpressionSyntax* node) override;
		void VisitSwitchExpression(shard::SwitchExpressionSyntax* node) override;
		void VisitTryStatement(shard::TryStatementSyntax* node) override;

		void VisitMemberAccessExpression(shard::MemberAccessExpressionSyntax* node) override;
		void VisitInvocationExpression(shard::InvokationExpressionSyntax* node) override;
		void VisitIndexatorExpression(shard::IndexatorExpressionSyntax* node) override;

		void VisitCastExpression(shard::CastExpressionSyntax* node) override;
		void VisitIsExpression(shard::IsExpressionSyntax* node) override;
	};
}

