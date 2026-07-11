#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/semantic/ScopeVisitor.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TypeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForInStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>

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
		shard::TypeSymbol* SubstituteTypeParameters(shard::TypeSymbol* type, shard::GenericTypeSymbol* genericType, shard::MethodSymbol* method, const std::vector<TypeSymbol*>& methodTypeArgs);
		shard::TypeSymbol* FindEnumerableElementType(shard::TypeSymbol* rangeType, bool& isArrayRange, bool allowArray = true);

		shard::TypeSymbol* SubstituteMethodTypeParameter(shard::TypeSymbol* type, shard::MethodSymbol* method, const std::vector<TypeSymbol*>& methodTypeArgs);
		shard::TypeSymbol* ResolveTypeExpression(shard::TypeSyntax* type);
		bool TryInferTypeArgument(shard::TypeSymbol* pattern, shard::TypeSymbol* concrete, shard::MethodSymbol* method, shard::GenericTypeSymbol* genericType, std::vector<TypeSymbol*>& outMethodTypeArgs);
		bool InferMethodTypeArguments(shard::MethodSymbol* method, const std::vector<TypeSymbol*>& argTypes, shard::GenericTypeSymbol* genericType, std::vector<TypeSymbol*>& outMethodTypeArgs);
		bool MatchGenericMethodArguments(shard::MethodSymbol* method, const std::vector<TypeSymbol*>& argTypes, shard::GenericTypeSymbol* genericType, const std::vector<TypeSymbol*>& methodTypeArgs);

		bool CollectArgumentTypes(shard::InvokationExpressionSyntax* node, std::vector<shard::TypeSymbol*>& outArgTypes);
		bool CollectExplicitTypeArguments(shard::InvokationExpressionSyntax* node, std::vector<shard::TypeSymbol*>& outTypeArgs);
		shard::MethodSymbol* TryResolveDelegateInvocation(shard::InvokationExpressionSyntax* node, shard::SyntaxSymbol* lookup);
		bool HasAnyMethodNamed(const std::wstring& name, shard::TypeSymbol* currentType);
		bool TryMatchMethod(shard::MethodSymbol* method, const std::wstring& expectedName, const std::vector<shard::TypeSymbol*>& argTypes, shard::GenericTypeSymbol* genericType, const std::vector<shard::TypeSymbol*>& explicitTypeArgs, std::vector<shard::TypeSymbol*>& outMethodTypeArgs);
		shard::MethodSymbol* FindMethodOverload(const std::vector<shard::MethodSymbol*>& candidates, const std::wstring& name, const std::vector<shard::TypeSymbol*>& argTypes, shard::GenericTypeSymbol* genericType, const std::vector<shard::TypeSymbol*>& explicitTypeArgs, std::vector<shard::TypeSymbol*>& outMethodTypeArgs);
		void ReportNoMatchingMethod(const std::wstring& methodName, shard::SyntaxToken blameToken);
		void ReportNoMatchingOverload(const std::wstring& methodName, const std::vector<shard::TypeSymbol*>& argTypes, shard::SyntaxToken blameToken);

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
		void VisitTypeExpression(shard::TypeExpressionSyntax* node) override;
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

