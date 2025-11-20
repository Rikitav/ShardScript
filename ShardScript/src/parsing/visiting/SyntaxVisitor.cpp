#include <shard/parsing/visiting/SyntaxVisitor.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>

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

using namespace shard::parsing;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;
using namespace shard::syntax;
using namespace shard::syntax::nodes;

void SyntaxVisitor::VisitSyntaxTree(SyntaxTree& tree)
{
	for (CompilationUnitSyntax* unit : tree.CompilationUnits)
		VisitCompilationUnit(unit);
}

void SyntaxVisitor::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);
}

void SyntaxVisitor::VisitUsingDirective(UsingDirectiveSyntax* node)
{
	// ...
	return;
}

void SyntaxVisitor::VisitImportDirective(ImportDirectiveSyntax* node)
{
	// ...
	return;
}

void SyntaxVisitor::VisitTypeDeclaration(MemberDeclarationSyntax* node)
{
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
	}
}

void SyntaxVisitor::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);
}

void SyntaxVisitor::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	for (MemberDeclarationSyntax* member : node->Members)
		VisitMemberDeclaration(member);
}

void SyntaxVisitor::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	for (MemberDeclarationSyntax* member : node->Members)
		VisitMemberDeclaration(member);
}

void SyntaxVisitor::VisitMemberDeclaration(MemberDeclarationSyntax* node)
{
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

		case SyntaxKind::AccessorDeclaration:
		{
			AccessorDeclarationSyntax* declNode = static_cast<AccessorDeclarationSyntax*>(node);
			VisitAccessorDeclaration(declNode);
			return;
		}
	}
}

void SyntaxVisitor::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void SyntaxVisitor::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	VisitParametersList(node->Params);
	VisitStatementsBlock(node->Body);
}

void SyntaxVisitor::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter);
	
	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Setter);
	
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void SyntaxVisitor::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body);
}

void SyntaxVisitor::VisitStatementsBlock(StatementsBlockSyntax* node)
{
	for (StatementSyntax* statement : node->Statements)
		VisitStatement(statement);
}

void SyntaxVisitor::VisitStatement(StatementSyntax* node)
{
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
	VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitVariableStatement(VariableStatementSyntax* node)
{
	VisitType(node->Type);
	VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitWhileStatement(WhileStatementSyntax* node)
{
	VisitExpression(node->ConditionExpression);
	VisitStatementsBlock(node->StatementsBlock);
}

void SyntaxVisitor::VisitUntilStatement(UntilStatementSyntax* node)
{
	VisitExpression(node->ConditionExpression);
	VisitStatementsBlock(node->StatementsBlock);
}

void SyntaxVisitor::VisitForStatement(ForStatementSyntax* node)
{
	VisitStatement(node->InitializerStatement);
	VisitExpression(node->ConditionExpression);
	VisitStatement(node->AfterRepeatStatement);
	VisitStatementsBlock(node->StatementsBlock);
}

void SyntaxVisitor::VisitReturnStatement(ReturnStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitThrowStatement(ThrowStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitBreakStatement(BreakStatementSyntax* node)
{
	// ...
	return;
}

void SyntaxVisitor::VisitContinueStatement(ContinueStatementSyntax* node)
{
	// ...
	return;
}

void SyntaxVisitor::VisitConditionalClause(ConditionalClauseBaseSyntax* node)
{
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
	VisitStatement(node->ConditionExpression);
	VisitStatementsBlock(node->StatementsBlock);
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

void SyntaxVisitor::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	VisitStatement(node->ConditionExpression);
	VisitStatementsBlock(node->StatementsBlock);

	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

void SyntaxVisitor::VisitElseStatement(ElseStatementSyntax* node)
{
	VisitStatementsBlock(node->StatementsBlock);

	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

void SyntaxVisitor::VisitExpression(ExpressionSyntax* node)
{
	switch (node->Kind)
	{
		default:
			throw std::runtime_error("unknown expression kind");

		case SyntaxKind::LiteralExpression:
		{
			LiteralExpressionSyntax* expression = dynamic_cast<LiteralExpressionSyntax*>(node);
			VisitLiteralExpression(expression);
			return;
		}

		case SyntaxKind::BinaryExpression:
		{
			BinaryExpressionSyntax* expression = dynamic_cast<BinaryExpressionSyntax*>(node);
			VisitBinaryExpression(expression);
			return;
		}

		case SyntaxKind::UnaryExpression:
		{
			UnaryExpressionSyntax* expression = dynamic_cast<UnaryExpressionSyntax*>(node);
			VisitUnaryExpression(expression);
			return;
		}

		case SyntaxKind::MemberAccessExpression:
		{
			MemberAccessExpressionSyntax* expression = dynamic_cast<MemberAccessExpressionSyntax*>(node);
			VisitMemberAccessExpression(expression);
			return;
		}

		case SyntaxKind::InvokationExpression:
		{
			InvokationExpressionSyntax* expression = dynamic_cast<InvokationExpressionSyntax*>(node);
			VisitInvocationExpression(expression);
			return;
		}

		case SyntaxKind::IndexatorExpression:
		{
			IndexatorExpressionSyntax* expression = dynamic_cast<IndexatorExpressionSyntax*>(node);
			VisitIndexatorExpression(expression);
			return;
		}

		case SyntaxKind::ObjectExpression:
		{
			ObjectExpressionSyntax* expression = dynamic_cast<ObjectExpressionSyntax*>(node);
			VisitObjectCreationExpression(expression);
			return;
		}

		case SyntaxKind::CollectionExpression:
		{
			CollectionExpressionSyntax* expression = dynamic_cast<CollectionExpressionSyntax*>(node);
			VisitCollectionExpression(expression);
			return;
		}
	}
}

void SyntaxVisitor::VisitLiteralExpression(LiteralExpressionSyntax* node)
{
	// ..
	return;
}

void SyntaxVisitor::VisitBinaryExpression(BinaryExpressionSyntax* node)
{
	VisitExpression(node->Left);
	VisitExpression(node->Right);
}

void SyntaxVisitor::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	VisitExpression(node->Expression);
}

void SyntaxVisitor::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (ExpressionSyntax* expression : node->ValuesExpressions)
		VisitExpression(expression);
}

void SyntaxVisitor::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	VisitType(node->Type);
	VisitArgumentsList(node->Arguments);
}

void SyntaxVisitor::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
{
	// ...
	return;
}

void SyntaxVisitor::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	VisitArgumentsList(node->ArgumentsList);
}

void SyntaxVisitor::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	VisitIndexatorList(node->IndexatorList);
}

void SyntaxVisitor::VisitArgumentsList(ArgumentsListSyntax* node)
{
	for (ArgumentSyntax* argument : node->Arguments)
		VisitArgument(argument);
}

void SyntaxVisitor::VisitIndexatorList(IndexatorListSyntax* node)
{
	for (ArgumentSyntax* argument : node->Arguments)
		VisitArgument(argument);
}

void SyntaxVisitor::VisitArgument(ArgumentSyntax* node)
{
	VisitExpression((ExpressionSyntax*)node->Expression);
}

void SyntaxVisitor::VisitParametersList(ParametersListSyntax* node)
{
	for (ParameterSyntax* parameter : node->Parameters)
		VisitParameter(parameter);
}

void SyntaxVisitor::VisitParameter(ParameterSyntax* node)
{
	VisitType((TypeSyntax*)node->Type);
}

void SyntaxVisitor::VisitType(TypeSyntax* node)
{
	switch (node->Kind)
	{
		case SyntaxKind::PredefinedType:
		{
			PredefinedTypeSyntax* expression = dynamic_cast<PredefinedTypeSyntax*>(node);
			VisitPredefinedType(expression);
			return;
		}

		case SyntaxKind::IdentifierNameType:
		{
			IdentifierNameTypeSyntax* expression = dynamic_cast<IdentifierNameTypeSyntax*>(node);
			VisitIdentifierNameType(expression);
			return;
		}

		case SyntaxKind::ArrayType:
		{
			ArrayTypeSyntax* expression = dynamic_cast<ArrayTypeSyntax*>(node);
			VisitArrayType(expression);
			return;
		}

		case SyntaxKind::NullableType:
		{
			NullableTypeSyntax* expression = dynamic_cast<NullableTypeSyntax*>(node);
			VisitNullableType(expression);
			return;
		}

		case SyntaxKind::GenericType:
		{
			GenericTypeSyntax* expression = dynamic_cast<GenericTypeSyntax*>(node);
			VisitGenericType(expression);
			return;
		}
	}
}

void SyntaxVisitor::VisitPredefinedType(PredefinedTypeSyntax* node)
{
	// ...
	return;
}

void SyntaxVisitor::VisitIdentifierNameType(IdentifierNameTypeSyntax* node)
{
	// ...
	return;
}

void SyntaxVisitor::VisitArrayType(ArrayTypeSyntax* node)
{
	VisitType(node->UnderlayingType);
}

void SyntaxVisitor::VisitNullableType(NullableTypeSyntax* node)
{
	VisitType(node->UnderlayingType);
}

void SyntaxVisitor::VisitGenericType(GenericTypeSyntax* node)
{
	VisitType(node->UnderlayingType);
	for (TypeSyntax* type : node->TypeArguments)
		VisitType(type);
}
