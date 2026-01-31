#include <shard/SyntaxVisitor.h>
#include <shard/parsing/SyntaxTree.h>

#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.h>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

#include <stdexcept>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.h>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.h>

using namespace shard;

void SyntaxVisitor::VisitSyntaxTree(SyntaxTree& tree)
{
	for (CompilationUnitSyntax* unit : tree.CompilationUnits)
		VisitCompilationUnit(unit);
}

void SyntaxVisitor::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	if (node == nullptr)
		return;

	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);
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
	}
}

void SyntaxVisitor::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);
}

void SyntaxVisitor::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->TypeParameters != nullptr)
		VisitTypeParametersList(node->TypeParameters);

	for (MemberDeclarationSyntax* member : node->Members)
		VisitMemberDeclaration(member);
}

void SyntaxVisitor::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->TypeParameters != nullptr)
		VisitTypeParametersList(node->TypeParameters);

	for (MemberDeclarationSyntax* member : node->Members)
		VisitMemberDeclaration(member);
}

void SyntaxVisitor::VisitDelegateDeclaration(DelegateDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType);

	if (node->TypeParameters != nullptr)
		VisitTypeParametersList(node->TypeParameters);

	if (node->Params != nullptr)
		VisitParametersList(node->Params);
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
	}
}

void SyntaxVisitor::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType);

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void SyntaxVisitor::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType);

	if (node->Params != nullptr)
		VisitParametersList(node->Params);

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body);
}

void SyntaxVisitor::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Params != nullptr)
		VisitParametersList(node->Params);

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body);
}

void SyntaxVisitor::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType);

	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter);
	
	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Setter);
	
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void SyntaxVisitor::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType);

	if (node->Parameters != nullptr)
		VisitParametersList(node->Parameters);

	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter);

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Setter);
}

void SyntaxVisitor::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body);
}

void SyntaxVisitor::VisitStatementsBlock(StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	for (StatementSyntax* statement : node->Statements)
		VisitStatement(statement);
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
	}
}

void SyntaxVisitor::VisitExpressionStatement(ExpressionStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitVariableStatement(VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Type != nullptr)
		VisitType(node->Type);

	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitWhileStatement(WhileStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		VisitExpression(node->ConditionExpression);

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
}

void SyntaxVisitor::VisitUntilStatement(UntilStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		VisitExpression(node->ConditionExpression);

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
}

void SyntaxVisitor::VisitForStatement(ForStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->InitializerStatement != nullptr)
		VisitStatement(node->InitializerStatement);

	if (node->ConditionExpression != nullptr)
		VisitExpression(node->ConditionExpression);

	if (node->AfterRepeatStatement != nullptr)
		VisitStatement(node->AfterRepeatStatement);

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
}

void SyntaxVisitor::VisitReturnStatement(ReturnStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitThrowStatement(ThrowStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
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
		VisitStatement(node->ConditionExpression);

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

void SyntaxVisitor::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->ConditionExpression != nullptr)
		VisitStatement(node->ConditionExpression);

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);

	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

void SyntaxVisitor::VisitElseStatement(ElseStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);

	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
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
		VisitExpression(node->Left);

	if (node->Right != nullptr)
		VisitExpression(node->Right);
}

void SyntaxVisitor::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	for (ExpressionSyntax* expression : node->ValuesExpressions)
		VisitExpression(expression);
}

void SyntaxVisitor::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Params != nullptr)
		VisitParametersList(node->Params);

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body);
}

void SyntaxVisitor::VisitTernaryExpression(TernaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Condition != nullptr)
		VisitExpression(node->Condition);

	if (node->Left != nullptr)
		VisitExpression(node->Left);

	if (node->Right != nullptr)
		VisitExpression(node->Right);
}

void SyntaxVisitor::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Type != nullptr)
		VisitType(node->Type);

	if (node->ArgumentsList != nullptr)
		VisitArgumentsList(node->ArgumentsList);
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
		VisitArgumentsList(node->ArgumentsList);
}

void SyntaxVisitor::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->IndexatorList != nullptr)
		VisitIndexatorList(node->IndexatorList);
}

void SyntaxVisitor::VisitArgumentsList(ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (ArgumentSyntax* argument : node->Arguments)
		VisitArgument(argument);
}

void SyntaxVisitor::VisitIndexatorList(IndexatorListSyntax* node)
{
	if (node == nullptr)
		return;

	for (ArgumentSyntax* argument : node->Arguments)
		VisitArgument(argument);
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

	for (TypeSyntax* type : node->Types)
		VisitType(type);
}

void SyntaxVisitor::VisitArgument(ArgumentSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Expression != nullptr)
		VisitExpression(const_cast<ExpressionSyntax*>(node->Expression));
}

void SyntaxVisitor::VisitParametersList(ParametersListSyntax* node)
{
	for (ParameterSyntax* parameter : node->Parameters)
		VisitParameter(parameter);
}

void SyntaxVisitor::VisitParameter(ParameterSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->Type != nullptr)
		VisitType(const_cast<TypeSyntax*>(node->Type));
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
		VisitType(node->UnderlayingType);
}

void SyntaxVisitor::VisitNullableType(NullableTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->UnderlayingType != nullptr)
		VisitType(node->UnderlayingType);
}

void SyntaxVisitor::VisitGenericType(GenericTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (node->UnderlayingType != nullptr)
		VisitType(node->UnderlayingType);

	if (node->Arguments != nullptr)
		VisitTypeArgumentsList(node->Arguments);
}

void SyntaxVisitor::VisitDelegateType(DelegateTypeSyntax* node)
{
	if (node->ReturnType != nullptr)
		VisitType(node->ReturnType);

	if (node->Params != nullptr)
		VisitParametersList(node->Params);
}
