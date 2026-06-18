#include <stdexcept>

#include <shard/SyntaxVisitor.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/SwitchExpressionSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>

#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>

using namespace shard;

void SyntaxVisitor::VisitSyntaxTree(SyntaxTree& tree)
{
	for (const auto& unit : tree.CompilationUnits)
		VisitCompilationUnit(unit.get());
}

void SyntaxVisitor::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	if (node == nullptr)
		return;

	for (const auto& directive : node->Usings)
		VisitUsingDirective(directive.get());

	if (node->Namespace != nullptr)
		VisitNamespaceDeclaration(node->Namespace.get());

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());
}

void SyntaxVisitor::VisitUsingDirective(UsingDirectiveSyntax* node)
{
	if (node == nullptr)
		return;

	// ...
	return;
}

void SyntaxVisitor::VisitTypeDeclaration(MemberDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	switch (node->Kind)
	{
		default:
			throw std::runtime_error("unknown type kind");

		case SyntaxKind::NamespaceDeclaration:
		{
			NamespaceDeclarationSyntax* declNode = static_cast<NamespaceDeclarationSyntax*>(node);
			VisitNamespaceDeclaration(declNode);
			return;
		}

		case SyntaxKind::ClassDeclaration:
		{
			ClassDeclarationSyntax* declNode = static_cast<ClassDeclarationSyntax*>(node);
			VisitClassDeclaration(declNode);
			return;
		}

		case SyntaxKind::StructDeclaration:
		{
			StructDeclarationSyntax* declNode = static_cast<StructDeclarationSyntax*>(node);
			VisitStructDeclaration(declNode);
			return;
		}

		case SyntaxKind::DelegateDeclaration:
		{
			DelegateDeclarationSyntax* declNode = static_cast<DelegateDeclarationSyntax*>(node);
			VisitDelegateDeclaration(declNode);
			return;
		}

		case SyntaxKind::InterfaceDeclaration:
		{
			InterfaceDeclarationSyntax* declNode = static_cast<InterfaceDeclarationSyntax*>(node);
			VisitInterfaceDeclaration(declNode);
			return;
		}
	}
}

void SyntaxVisitor::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->TypeParameters != nullptr)
		VisitTypeParametersList(node->TypeParameters.get());

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());
}

void SyntaxVisitor::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->TypeParameters != nullptr)
		VisitTypeParametersList(node->TypeParameters.get());

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());
}

void SyntaxVisitor::VisitInterfaceDeclaration(InterfaceDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->TypeParameters != nullptr)
		VisitTypeParametersList(node->TypeParameters.get());

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());
}

void SyntaxVisitor::VisitDelegateDeclaration(DelegateDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType.get());

	if (node->TypeParameters != nullptr)
		VisitTypeParametersList(node->TypeParameters.get());

	if (node->ParametersList != nullptr)
		VisitParametersList(node->ParametersList.get());
}

void SyntaxVisitor::VisitMemberDeclaration(MemberDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	switch (node->Kind)
	{
		default:
			throw std::runtime_error("unknown member kind");

		case SyntaxKind::NamespaceDeclaration:
		{
			NamespaceDeclarationSyntax* declNode = static_cast<NamespaceDeclarationSyntax*>(node);
			VisitNamespaceDeclaration(declNode);
			return;
		}

		case SyntaxKind::ClassDeclaration:
		{
			ClassDeclarationSyntax* declNode = static_cast<ClassDeclarationSyntax*>(node);
			VisitClassDeclaration(declNode);
			return;
		}

		case SyntaxKind::StructDeclaration:
		{
			StructDeclarationSyntax* declNode = static_cast<StructDeclarationSyntax*>(node);
			VisitStructDeclaration(declNode);
			return;
		}

		case SyntaxKind::DelegateDeclaration:
		{
			DelegateDeclarationSyntax* declNode = static_cast<DelegateDeclarationSyntax*>(node);
			VisitDelegateDeclaration(declNode);
			return;
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldDeclarationSyntax* declNode = static_cast<FieldDeclarationSyntax*>(node);
			VisitFieldDeclaration(declNode);
			return;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodDeclarationSyntax* declNode = static_cast<MethodDeclarationSyntax*>(node);
			VisitMethodDeclaration(declNode);
			return;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertyDeclarationSyntax* declNode = static_cast<PropertyDeclarationSyntax*>(node);
			VisitPropertyDeclaration(declNode);
			return;
		}

		case SyntaxKind::IndexatorDeclaration:
		{
			IndexatorDeclarationSyntax* declNode = static_cast<IndexatorDeclarationSyntax*>(node);
			VisitIndexatorDeclaration(declNode);
			return;
		}

		case SyntaxKind::AccessorDeclaration:
		{
			AccessorDeclarationSyntax* declNode = static_cast<AccessorDeclarationSyntax*>(node);
			VisitAccessorDeclaration(declNode);
			return;
		}

		case SyntaxKind::ConstructorDeclaration:
		{
			ConstructorDeclarationSyntax* declNode = static_cast<ConstructorDeclarationSyntax*>(node);
			VisitConstructorDeclaration(declNode);
			return;
		}

		case SyntaxKind::InterfaceDeclaration:
		{
			InterfaceDeclarationSyntax* declNode = static_cast<InterfaceDeclarationSyntax*>(node);
			VisitInterfaceDeclaration(declNode);
			return;
		}
	}
}

void SyntaxVisitor::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType.get());

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression.get());
}

void SyntaxVisitor::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType.get());

	if (node->ParametersList != nullptr)
		VisitParametersList(node->ParametersList.get());

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());
}

void SyntaxVisitor::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ParametersList != nullptr)
		VisitParametersList(node->ParametersList.get());

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());
}

void SyntaxVisitor::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType.get());

	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());
	
	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Setter.get());
	
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression.get());
}

void SyntaxVisitor::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType.get());

	if (node->ParametersList != nullptr)
		VisitParametersList(node->ParametersList.get());

	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Setter.get());
}

void SyntaxVisitor::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());
}

void SyntaxVisitor::VisitStatementsBlock(StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	for (const auto& statement : node->Statements)
		VisitStatement(statement.get());
}

void SyntaxVisitor::VisitStatement(StatementSyntax* node)
{
	if (node == nullptr)
		return;

	switch (node->Kind)
	{
		default:
			throw std::runtime_error("unknown statement kind");

		case SyntaxKind::ExpressionStatement:
		{
			ExpressionStatementSyntax* statement = static_cast<ExpressionStatementSyntax*>(node);
			VisitExpressionStatement(statement);
			return;
		}

		case SyntaxKind::ReturnStatement:
		{
			ReturnStatementSyntax* statement = static_cast<ReturnStatementSyntax*>(node);
			VisitReturnStatement(statement);
			return;
		}

		case SyntaxKind::ThrowStatement:
		{
			ThrowStatementSyntax* statement = static_cast<ThrowStatementSyntax*>(node);
			VisitThrowStatement(statement);
			return;
		}

		case SyntaxKind::BreakStatement:
		{
			BreakStatementSyntax* statement = static_cast<BreakStatementSyntax*>(node);
			VisitBreakStatement(statement);
			return;
		}

		case SyntaxKind::ContinueStatement:
		{
			ContinueStatementSyntax* statement = static_cast<ContinueStatementSyntax*>(node);
			VisitContinueStatement(statement);
			return;
		}

		case SyntaxKind::VariableStatement:
		{
			VariableStatementSyntax* statement = dynamic_cast<VariableStatementSyntax*>(node);
			VisitVariableStatement(statement);
			return;
		}

		case SyntaxKind::WhileStatement:
		{
			WhileStatementSyntax* statement = dynamic_cast<WhileStatementSyntax*>(node);
			VisitWhileStatement(statement);
			return;
		}

		case SyntaxKind::UntilStatement:
		{
			UntilStatementSyntax* statement = dynamic_cast<UntilStatementSyntax*>(node);
			VisitUntilStatement(statement);
			return;
		}

		case SyntaxKind::ForStatement:
		{
			ForStatementSyntax* statement = dynamic_cast<ForStatementSyntax*>(node);
			VisitForStatement(statement);
			return;
		}

		case SyntaxKind::ForEachStatement:
		{
			ForEachStatementSyntax* statement = dynamic_cast<ForEachStatementSyntax*>(node);
			VisitForEachStatement(statement);
			return;
		}

		case SyntaxKind::IfStatement:
		{
			IfStatementSyntax* statement = dynamic_cast<IfStatementSyntax*>(node);
			VisitIfStatement(statement);
			return;
		}

		case SyntaxKind::UnlessStatement:
		{
			UnlessStatementSyntax* statement = dynamic_cast<UnlessStatementSyntax*>(node);
			VisitUnlessStatement(statement);
			return;
		}

		case SyntaxKind::TryStatement:
		{
			TryStatementSyntax* statement = dynamic_cast<TryStatementSyntax*>(node);
			VisitTryStatement(statement);
			return;
		}
	}
}

void SyntaxVisitor::VisitExpressionStatement(ExpressionStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());
}

void SyntaxVisitor::VisitVariableStatement(VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Type != nullptr)
		VisitType(node->Type.get());

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());
}

void SyntaxVisitor::VisitWhileStatement(WhileStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		VisitExpression(node->ConditionExpression.get());

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void SyntaxVisitor::VisitUntilStatement(UntilStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		VisitExpression(node->ConditionExpression.get());

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void SyntaxVisitor::VisitForStatement(ForStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->InitializerStatement != nullptr)
		VisitStatement(node->InitializerStatement.get());

	if (node->ConditionExpression != nullptr)
		VisitExpression(node->ConditionExpression.get());

	if (node->AfterRepeatStatement != nullptr)
		VisitStatement(node->AfterRepeatStatement.get());

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void SyntaxVisitor::VisitForEachStatement(ForEachStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->RangeExpression != nullptr)
		VisitExpression(node->RangeExpression.get());

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void SyntaxVisitor::VisitReturnStatement(ReturnStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());
}

void SyntaxVisitor::VisitThrowStatement(ThrowStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());
}

void SyntaxVisitor::VisitBreakStatement(BreakStatementSyntax* node)
{
	if (node == nullptr)
		return;

	// ...
	return;
}

void SyntaxVisitor::VisitContinueStatement(ContinueStatementSyntax* node)
{
	if (node == nullptr)
		return;

	// ...
	return;
}

void SyntaxVisitor::VisitConditionalClause(ConditionalClauseBaseSyntax* node)
{
	if (node == nullptr)
		return;

	switch (node->Kind)
	{
		default:
			throw std::runtime_error("unknown conditional clause kind");

		case SyntaxKind::IfStatement:
		{
			IfStatementSyntax* statement = dynamic_cast<IfStatementSyntax*>(node);
			VisitIfStatement(statement);
			return;
		}

		case SyntaxKind::UnlessStatement:
		{
			UnlessStatementSyntax* statement = dynamic_cast<UnlessStatementSyntax*>(node);
			VisitUnlessStatement(statement);
			return;
		}

		case SyntaxKind::ElseStatement:
		{
			ElseStatementSyntax* statement = dynamic_cast<ElseStatementSyntax*>(node);
			VisitElseStatement(statement);
			return;
		}
	}
}

void SyntaxVisitor::VisitIfStatement(IfStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		VisitStatement(node->ConditionExpression.get());

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());

	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement.get());
}

void SyntaxVisitor::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		VisitStatement(node->ConditionExpression.get());

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());

	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement.get());
}

void SyntaxVisitor::VisitElseStatement(ElseStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());

	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement.get());
}

void SyntaxVisitor::VisitTryStatement(TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->TryBlock != nullptr)
		VisitStatementsBlock(node->TryBlock.get());

	for (const auto& clause : node->CatchClauses)
	{
		if (clause->Body != nullptr)
			VisitStatementsBlock(clause->Body.get());
	}
}

void SyntaxVisitor::VisitIfExpression(IfExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Condition != nullptr)
		VisitExpression(node->Condition.get());

	if (node->ThenExpression != nullptr)
		VisitExpression(node->ThenExpression.get());

	if (node->ElseExpression != nullptr)
		VisitExpression(node->ElseExpression.get());
}

void SyntaxVisitor::VisitSwitchExpression(SwitchExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	for (const auto& arm : node->Arms)
	{
		if (arm->Pattern != nullptr)
			VisitExpression(arm->Pattern.get());

		if (arm->Expression != nullptr)
			VisitExpression(arm->Expression.get());
	}
}

void SyntaxVisitor::VisitExpression(ExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	switch (node->Kind)
	{
		default:
			throw std::runtime_error("unknown expression kind");

		case SyntaxKind::LiteralExpression:
		{
			LiteralExpressionSyntax* expression = static_cast<LiteralExpressionSyntax*>(node);
			VisitLiteralExpression(expression);
			return;
		}

		case SyntaxKind::BinaryExpression:
		{
			BinaryExpressionSyntax* expression = static_cast<BinaryExpressionSyntax*>(node);
			VisitBinaryExpression(expression);
			return;
		}

		case SyntaxKind::UnaryExpression:
		{
			UnaryExpressionSyntax* expression = static_cast<UnaryExpressionSyntax*>(node);
			VisitUnaryExpression(expression);
			return;
		}

		case SyntaxKind::MemberAccessExpression:
		{
			MemberAccessExpressionSyntax* expression = static_cast<MemberAccessExpressionSyntax*>(node);
			VisitMemberAccessExpression(expression);
			return;
		}

		case SyntaxKind::InvokationExpression:
		{
			InvokationExpressionSyntax* expression = static_cast<InvokationExpressionSyntax*>(node);
			VisitInvocationExpression(expression);
			return;
		}

		case SyntaxKind::IndexatorExpression:
		{
			IndexatorExpressionSyntax* expression = static_cast<IndexatorExpressionSyntax*>(node);
			VisitIndexatorExpression(expression);
			return;
		}

		case SyntaxKind::ObjectExpression:
		{
			ObjectExpressionSyntax* expression = static_cast<ObjectExpressionSyntax*>(node);
			VisitObjectCreationExpression(expression);
			return;
		}

		case SyntaxKind::CollectionExpression:
		{
			CollectionExpressionSyntax* expression = static_cast<CollectionExpressionSyntax*>(node);
			VisitCollectionExpression(expression);
			return;
		}

		case SyntaxKind::RangeExpression:
		{
			RangeExpressionSyntax* expression = static_cast<RangeExpressionSyntax*>(node);
			VisitRangeExpression(expression);
			return;
		}

		case SyntaxKind::LambdaExpression:
		{
			LambdaExpressionSyntax* expression = static_cast<LambdaExpressionSyntax*>(node);
			VisitLambdaExpression(expression);
			return;
		}

		case SyntaxKind::TernaryExpression:
		{
			TernaryExpressionSyntax* expression = static_cast<TernaryExpressionSyntax*>(node);
			VisitTernaryExpression(expression);
			return;
		}

		case SyntaxKind::IfExpression:
		{
			IfExpressionSyntax* expression = static_cast<IfExpressionSyntax*>(node);
			VisitIfExpression(expression);
			return;
		}

		case SyntaxKind::SwitchExpression:
		{
			SwitchExpressionSyntax* expression = static_cast<SwitchExpressionSyntax*>(node);
			VisitSwitchExpression(expression);
			return;
		}

		case SyntaxKind::CastExpression:
		{
			CastExpressionSyntax* expression = static_cast<CastExpressionSyntax*>(node);
			VisitCastExpression(expression);
			return;
		}

		case SyntaxKind::IsExpression:
		{
			IsExpressionSyntax* expression = static_cast<IsExpressionSyntax*>(node);
			VisitIsExpression(expression);
			return;
		}
	}
}

void SyntaxVisitor::VisitLiteralExpression(LiteralExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// ..
	return;
}

void SyntaxVisitor::VisitBinaryExpression(BinaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Left != nullptr)
		VisitExpression(node->Left.get());

	if (node->Right != nullptr)
		VisitExpression(node->Right.get());
}

void SyntaxVisitor::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());
}

void SyntaxVisitor::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	for (const auto& expression : node->ValuesExpressions)
		VisitExpression(expression.get());
}

void SyntaxVisitor::VisitRangeExpression(RangeExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Left != nullptr)
		VisitExpression(node->Left.get());

	if (node->Right != nullptr)
		VisitExpression(node->Right.get());
}

void SyntaxVisitor::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ParametersList != nullptr)
		VisitParametersList(node->ParametersList.get());

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());
}

void SyntaxVisitor::VisitTernaryExpression(TernaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Condition != nullptr)
		VisitExpression(node->Condition.get());

	if (node->Left != nullptr)
		VisitExpression(node->Left.get());

	if (node->Right != nullptr)
		VisitExpression(node->Right.get());
}

void SyntaxVisitor::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Type != nullptr)
		VisitType(node->Type.get());

	if (node->ArgumentsList != nullptr)
		VisitArgumentsList(node->ArgumentsList.get());
}

void SyntaxVisitor::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	// ...
	return;
}

void SyntaxVisitor::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ArgumentsList != nullptr)
		VisitArgumentsList(node->ArgumentsList.get());
}

void SyntaxVisitor::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->IndexatorList != nullptr)
		VisitIndexatorList(node->IndexatorList.get());
}

void SyntaxVisitor::VisitArgumentsList(ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const auto& argument : node->Arguments)
		VisitArgument(argument.get());
}

void SyntaxVisitor::VisitIndexatorList(IndexatorListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const auto& argument : node->Arguments)
		VisitArgument(argument.get());
}

void SyntaxVisitor::VisitTypeParametersList(TypeParametersListSyntax* node)
{
	if (node == nullptr)
		return;
	
	// ...
}

void SyntaxVisitor::VisitTypeArgumentsList(TypeArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const auto& type : node->Types)
		VisitType(type.get());
}

void SyntaxVisitor::VisitArgument(ArgumentSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());
}

void SyntaxVisitor::VisitParametersList(ParametersListSyntax* node)
{
	for (const auto& parameter : node->Parameters)
		VisitParameter(parameter.get());
}

void SyntaxVisitor::VisitParameter(ParameterSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Type != nullptr)
		VisitType(node->Type.get());
}

void SyntaxVisitor::VisitType(TypeSyntax* node)
{
	if (node == nullptr)
		return;

	switch (node->Kind)
	{
		default:
			throw std::runtime_error("unknown type syntax kind");

		case SyntaxKind::PredefinedType:
		{
			PredefinedTypeSyntax* expression = static_cast<PredefinedTypeSyntax*>(node);
			VisitPredefinedType(expression);
			return;
		}

		case SyntaxKind::IdentifierNameType:
		{
			IdentifierNameTypeSyntax* expression = static_cast<IdentifierNameTypeSyntax*>(node);
			VisitIdentifierNameType(expression);
			return;
		}

		case SyntaxKind::ArrayType:
		{
			ArrayTypeSyntax* expression = static_cast<ArrayTypeSyntax*>(node);
			VisitArrayType(expression);
			return;
		}

		case SyntaxKind::NullableType:
		{
			NullableTypeSyntax* expression = static_cast<NullableTypeSyntax*>(node);
			VisitNullableType(expression);
			return;
		}

		case SyntaxKind::GenericType:
		{
			GenericTypeSyntax* expression = static_cast<GenericTypeSyntax*>(node);
			VisitGenericType(expression);
			return;
		}

		case SyntaxKind::DelegateType:
		{
			DelegateTypeSyntax* expression = static_cast<DelegateTypeSyntax*>(node);
			VisitDelegateType(expression);
			return;
		}
	}
}

void SyntaxVisitor::VisitPredefinedType(PredefinedTypeSyntax* node)
{
	if (node == nullptr)
		return;

	// ...
	return;
}

void SyntaxVisitor::VisitIdentifierNameType(IdentifierNameTypeSyntax* node)
{
	if (node == nullptr)
		return;

	// ...
	return;
}

void SyntaxVisitor::VisitArrayType(ArrayTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->UnderlayingType != nullptr)
		VisitType(node->UnderlayingType.get());
}

void SyntaxVisitor::VisitNullableType(NullableTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->UnderlayingType != nullptr)
		VisitType(node->UnderlayingType.get());
}

void SyntaxVisitor::VisitGenericType(GenericTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->UnderlayingType != nullptr)
		VisitType(node->UnderlayingType.get());

	if (node->Arguments != nullptr)
		VisitTypeArgumentsList(node->Arguments.get());
}

void SyntaxVisitor::VisitDelegateType(DelegateTypeSyntax* node)
{
	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType.get());

	if (node->Params != nullptr)
		VisitParametersList(node->Params.get());
}

void SyntaxVisitor::VisitCastExpression(CastExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	if (node->TargetType != nullptr)
		VisitType(node->TargetType.get());
}

void SyntaxVisitor::VisitIsExpression(IsExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	if (node->TargetType != nullptr)
		VisitType(node->TargetType.get());
}
