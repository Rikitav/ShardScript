#include <shard/parsing/semantic/visiting/ExpressionBinder.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxFacts.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SymbolFactory.hpp>

#include <shard/syntax/symbols/LeftDenotationSymbol.hpp>
#include <shard/syntax/symbols/LiteralSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/OperatorSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>

#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <climits>
#include <exception>
#include <stdexcept>

using namespace shard;

static void GeneratePropertyBackingField(SymbolFactory& factory, PropertySymbol* symbol)
{
	FieldSymbol* backingField = factory.Field(L"<" + symbol->Name + L">k__BackingField", symbol->ReturnType, symbol->Linking);
	backingField->Accesibility = SymbolAccesibility::Private;
	backingField->DefaultValueExpression = symbol->DefaultValueExpression;

	symbol->BackingField = backingField;
}

static OperatorSymbol* ResolveOperatorMethod(TypeSymbol* ownerType, shard::TokenType opToken, const std::vector<TypeSymbol*>& paramTypes)
{
	if (ownerType == nullptr || !IsOverloadableOperator(opToken))
		return nullptr;

	return ownerType->FindOperator(opToken, paramTypes);
}

static bool IsAssignmentOperator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::AssignOperator:
		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
			return true;
		default:
			return false;
	}
}

static bool IsAssignmentContext(const MemberAccessExpressionSyntax* expression)
{
	if (expression->Parent == nullptr)
		return false;

	if (expression->Parent->Kind != SyntaxKind::BinaryExpression)
		return false;

	BinaryExpressionSyntax* binaryExpr = static_cast<BinaryExpressionSyntax*>(expression->Parent);
	if (binaryExpr->OperatorToken.Type != TokenType::AssignOperator)
		return false;

	return true;
}

static bool IsAssignmentContext(const IndexatorExpressionSyntax* expression)
{
	if (expression->Parent == nullptr)
		return false;

	if (expression->Parent->Kind != SyntaxKind::BinaryExpression)
		return false;

	BinaryExpressionSyntax* binaryExpr = static_cast<BinaryExpressionSyntax*>(expression->Parent);
	if (!IsAssignmentOperator(binaryExpr->OperatorToken.Type))
		return false;

	if (binaryExpr->Left.get() != expression)
		return false;

	return true;
}

bool ExpressionBinder::GetIsStaticContext(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
	{
		return true;
	}

	if (IsLinkedExpressionNode(expression->Kind))
	{
		const LinkedExpressionNode* asNode = static_cast<const LinkedExpressionNode*>(expression);
		return asNode->IsStaticContext;
	}

	return false;
}

void ExpressionBinder::SetExpressionType(ExpressionSyntax* expression, TypeSymbol* type)
{
	if (expression != nullptr)
		expressionTypes[expression] = type;
}

TypeSymbol* ExpressionBinder::GetExpressionType(ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return nullptr;
	
	auto it = expressionTypes.find(expression);
	return it != expressionTypes.end() ? it->second : nullptr;
}

void ExpressionBinder::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	PushScope(nullptr);

	for (const auto& directive : node->Usings)
		VisitUsingDirective(directive.get());

	if (node->Namespace != nullptr)
	{
		NamespaceSymbol* symbol = LookupSymbol<NamespaceSymbol>(node->Namespace.get()).value_or(nullptr);
		if (symbol != nullptr)
		{
			PushScope(symbol);
			if (symbol->Node != nullptr)
				CurrentScope()->Namespace = symbol->Node;
		}
	}

	for (const auto& member : node->Members)
	{
		SyntaxSymbol* symbol = Table->LookupSymbol(member.get()).value_or(nullptr);
		if (symbol != nullptr)
			Declare(symbol);
	}

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());

	if (node->Namespace != nullptr)
		PopScope();

	PopScope();
}

void ExpressionBinder::VisitUsingDirective(UsingDirectiveSyntax* node)
{
	if (node->Namespace == nullptr)
		return;

	SemanticScope* current = CurrentScope();
	for (const auto& symbol : node->Namespace->Members)
	{
		if (symbol != nullptr)
			current->DeclareSymbol(symbol);
	}
}

void ExpressionBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	// Namespace declarations are now handled inline in VisitCompilationUnit
}

void ExpressionBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node).value_or(nullptr);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		for (const auto& member : node->Members)
		{
			SyntaxSymbol* symbol = Table->LookupSymbol(member.get()).value_or(nullptr);
			if (symbol != nullptr)
				Declare(symbol);
		}

		for (const auto& member : node->Members)
			VisitMemberDeclaration(member.get());

		PopScope();
	}
}

void ExpressionBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = LookupSymbol<StructSymbol>(node).value_or(nullptr);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		for (const auto& member : node->Members)
		{
			SyntaxSymbol* symbol = Table->LookupSymbol(member.get()).value_or(nullptr);
			if (symbol != nullptr)
				Declare(symbol);
		}

		for (const auto& member : node->Members)
			VisitMemberDeclaration(member.get());

		PopScope();
	}
}

void ExpressionBinder::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr || symbol->Parent == nullptr || !symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol != nullptr && node->Body != nullptr)
	{
		PushScope(symbol);

		auto thisVar = Factory.Parameter(L"this", ownerType);
		if (symbol->Linking == LINK_INSTANCE) // how tf not?
			CurrentScope()->DeclareSymbol(thisVar);

		int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
		for (const auto& parameter : symbol->Parameters)
		{
			parameter->SlotIndex = counter++;
			CurrentScope()->DeclareSymbol(parameter);
		}

		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body.get());
		
		PopScope();
	}
}

void ExpressionBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->Parent == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot find parent node of method.");
		return;
	}

	/*
	if (!symbol->Parent->IsType())
		return;
	*/

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	PushScope(symbol);

	auto thisVar = Factory.Parameter(L"this", ownerType);
	if (symbol->Linking == LINK_INSTANCE)
		CurrentScope()->DeclareSymbol(thisVar);

	int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
	for (const auto& parameter : symbol->Parameters)
	{
		parameter->SlotIndex = counter++;
		CurrentScope()->DeclareSymbol(parameter);
	}

	if (node->Body != nullptr)
	{
		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body.get());

		if (symbol->ReturnType != nullptr)
		{
			if (symbol->ReturnType->Name != L"Void")
			{
				if (!CurrentScope()->ReturnFound)
					Diagnostics.ReportError(node->IdentifierToken, L"Method must return a value of type '" + symbol->ReturnType->Name + L"'");
			}
		}
	}

	PopScope();
}

void ExpressionBinder::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("operator symbol not found");

	if (symbol->Parent == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Cannot find parent node of operator.");
		return;
	}

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	PushScope(symbol);

	auto thisVar = Factory.Parameter(L"this", ownerType);
	if (symbol->Linking == LINK_INSTANCE)
		CurrentScope()->DeclareSymbol(thisVar);

	int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
	for (const auto& parameter : symbol->Parameters)
	{
		parameter->SlotIndex = counter++;
		CurrentScope()->DeclareSymbol(parameter);
	}

	if (node->Body != nullptr)
	{
		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body.get());

		if (symbol->ReturnType != nullptr)
		{
			if (symbol->ReturnType->Name != L"Void")
			{
				if (!CurrentScope()->ReturnFound)
					Diagnostics.ReportError(node->OperatorToken, L"Operator must return a value of type '" + symbol->ReturnType->Name + L"'");
			}
		}
	}

	if (node->OperatorToken.Type == TokenType::Delimeter)
	{
		if (symbol->Parameters.size() != 1 || symbol->Parameters[0]->Type != SymbolTable::Primitives::String)
			Diagnostics.ReportError(node->OperatorToken, L"Access operator ('.') parameter must be of type 'string'");
	}

	PopScope();
}

void ExpressionBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (!symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol->ReturnType == SymbolTable::Primitives::Void)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Property '" + node->IdentifierToken.Word + L"' must have return value");
	}

	PushScope(symbol);
	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	PopScope();
}

void ExpressionBinder::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
	IndexatorSymbol* symbol = LookupSymbol<IndexatorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (!symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol->ReturnType == SymbolTable::Primitives::Void)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Indexator '" + node->IdentifierToken.Word + L"' must have return value");
	}

	PushScope(symbol);
	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	PopScope();
}

void ExpressionBinder::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	AccessorSymbol* symbol = LookupSymbol<AccessorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	PushScope(symbol);
	
	if (node->KeywordToken.Type == TokenType::GetKeyword)
	{
		// Getter
		if (symbol->Parent == nullptr)
		{
			PopScope();
			return;
		}

		PropertySymbol* propSymbol = static_cast<PropertySymbol*>(symbol->Parent);
		TypeSymbol* ownerType = propSymbol->Parent != nullptr ? static_cast<TypeSymbol*>(propSymbol->Parent) : nullptr;

		auto thisVar = Factory.Parameter(L"this", ownerType);
		if (symbol->Linking == LINK_INSTANCE)
			CurrentScope()->DeclareSymbol(thisVar);

		int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
		for (const auto& parameter : symbol->Parameters)
		{
			parameter->SlotIndex = counter++;
			CurrentScope()->DeclareSymbol(parameter);
		}

		if (node->Body != nullptr)
		{
			SemanticScope* scope = CurrentScope();
			scope->ReturnFound = false;

			VisitStatementsBlock(node->Body.get());

			if (!scope->ReturnFound)
			{
				if (symbol->ReturnType != nullptr)
					Diagnostics.ReportError(node->KeywordToken, L"Accessor must return a value of type '" + symbol->ReturnType->Name + L"'");
				else
					Diagnostics.ReportError(node->KeywordToken, L"Accessor must return a value");
			}
		}

		PopScope();
	}
	else if (node->KeywordToken.Type == TokenType::SetKeyword)
	{
		// Setter
		if (symbol->Parent == nullptr)
		{
			PopScope();
			return;
		}

		PropertySymbol* propSymbol = static_cast<PropertySymbol*>(symbol->Parent);
		TypeSymbol* ownerType = propSymbol->Parent != nullptr ? static_cast<TypeSymbol*>(propSymbol->Parent) : nullptr;

		auto thisVar = Factory.Parameter(L"this", ownerType);
		if (symbol->Linking == LINK_INSTANCE)
			CurrentScope()->DeclareSymbol(thisVar);

		int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
		for (const auto& parameter : symbol->Parameters)
		{
			parameter->SlotIndex = counter++;
			CurrentScope()->DeclareSymbol(parameter);
		}

		if (node->Body != nullptr)
		{
			SemanticScope* scope = CurrentScope();
			scope->ReturnsAnything = false;
			VisitStatementsBlock(node->Body.get());

			if (scope->ReturnsAnything)
				Diagnostics.ReportError(node->KeywordToken, L"Setter method of '" + node->IdentifierToken.Word + L"' should not return any values");
		}

		PopScope();
	}
	else
	{
		PopScope();
		throw std::runtime_error("Unknown accessor type");
	}

}

void ExpressionBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node->InitializerExpression != nullptr)
	{
		VisitExpression(node->InitializerExpression.get());
	}

	FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (node->InitializerExpression != nullptr)
	{
		TypeSymbol* initExprType = GetExpressionType(node->InitializerExpression.get());
		if (initExprType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer expression type could not be determined");
			return;
		}

		if (symbol->ReturnType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field return type not resolved");
			return;
		}

		if (!TypeSymbol::IsAssignableFrom(symbol->ReturnType, initExprType))
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer type mismatch: expected '" + symbol->ReturnType->Name + L"' but got '" + initExprType->Name + L"'");
			return;
		}
	}
}

void ExpressionBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	Declare(symbol);
	if (symbol->Type == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Variable type not resolved");
		return;
	}

	if (node->Expression != nullptr)
	{
		PushScope(new LeftDenotationSymbol(symbol->Type));
		VisitExpression(node->Expression.get());
		PopScope();
	}

	TypeSymbol* expressionType = GetExpressionType(node->Expression.get());
	if (expressionType == nullptr)
	{
		return;
	}

	if (symbol->Type == SymbolTable::Primitives::Any)
	{
		symbol->Type = expressionType;
		return;
	}

	if (!TypeSymbol::IsAssignableFrom(symbol->Type, expressionType))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Type mismatch: expected '" + symbol->Type->Name + L"' but got '" + expressionType->Name + L"'");
		return;
	}
}

TypeSymbol* ExpressionBinder::AnalyzeLiteralExpression(LiteralExpressionSyntax* node)
{
	LiteralSymbol* symbol = static_cast<LiteralSymbol*>(Table->BindSymbol(node, std::make_unique<LiteralSymbol>(node->LiteralToken.Type)));
	switch (node->LiteralToken.Type)
	{
		case TokenType::NullLiteral:
		{
			return SymbolTable::Primitives::Null;
		}

		case TokenType::BooleanLiteral:
		{
			symbol->AsBooleanValue = node->LiteralToken.Word == L"true";
			return SymbolTable::Primitives::Boolean;
		}

		case TokenType::StringLiteral:
		{
			// TODO: add interpolation
			return SymbolTable::Primitives::String;
		}

		case TokenType::DoubleLiteral:
		{
			return AnalyzeDoubleLiteral(node);
		}

		case TokenType::NumberLiteral:
		{
			return AnalyzeNumberLiteral(node);
		}

		case TokenType::CharLiteral:
		{
			// TODO: add \u support
			if (node->LiteralToken.Word.size() > 1)
				Diagnostics.ReportError(node->LiteralToken, L"invalid Char literal length");

			//symbol->AsCharValue = node->LiteralToken.Word[0];
			return SymbolTable::Primitives::Char;
		}

		default:
			return nullptr;
	}
}

void ExpressionBinder::VisitLiteralExpression(LiteralExpressionSyntax* node)
{
	TypeSymbol* type = AnalyzeLiteralExpression(node);
	SetExpressionType(node, type);
	
	if (type == nullptr && node->LiteralToken.Type != TokenType::NullLiteral)
	{
		std::wstring tokenType = L"unknown";
		Diagnostics.ReportError(node->LiteralToken, L"Unsupported literal type: " + tokenType);
	}
}

TypeSymbol* ExpressionBinder::AnalyzeBinaryExpression(BinaryExpressionSyntax* node)
{
	VisitExpression(node->Left.get());
	TypeSymbol* leftType = GetExpressionType(node->Left.get());
	
	if (leftType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Left operand type could not be determined");
		return nullptr;
	}

	if (node->OperatorToken.Type == TokenType::AssignOperator)
	{
		LeftDenotationSymbol leftDenotation(leftType);
		PushScope(&leftDenotation);
		VisitExpression(node->Right.get());
		PopScope();

		TypeSymbol* rightType = GetExpressionType(node->Right.get());
		if (rightType == nullptr)
		{
			Diagnostics.ReportError(node->OperatorToken, L"Right operand type could not be determined");
			return leftType;
		}

		if (leftType != SymbolTable::Primitives::Any && !TypeSymbol::IsAssignableFrom(leftType, rightType))
		{
			Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in assignment: expected '" + leftType->Name + L"' but got '" + rightType->Name + L"'");
			return leftType;
		}

		return leftType;
	}

	VisitExpression(node->Right.get());
	TypeSymbol* rightType = GetExpressionType(node->Right.get());

	if (rightType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Right operand type could not be determined");
		return leftType;
	}

	{
		std::vector<TypeSymbol*> paramTypes = { leftType, rightType };
		OperatorSymbol* opMethod = ResolveOperatorMethod(leftType, node->OperatorToken.Type, paramTypes);
		if (opMethod != nullptr)
		{
			node->ToOperator = opMethod;
			return opMethod->ReturnType;
		}

		if (!leftType->IsPrimitive() &&
			node->OperatorToken.Type != TokenType::EqualsOperator &&
			node->OperatorToken.Type != TokenType::NotEqualsOperator)
		{
			Diagnostics.ReportError(node->OperatorToken, L"Operator '" + GetOperatorMethodName(node->OperatorToken.Type) + L"' is not defined for type '" + leftType->Name + L"'");
			return nullptr;
		}
	}

	switch (node->OperatorToken.Type)
	{
		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::OrOperator:
		case TokenType::AndOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		{
			bool nullComparison =
				(leftType == SymbolTable::Primitives::Null && rightType->Inlining == TypeInlining::ByReference) ||
				(rightType == SymbolTable::Primitives::Null && leftType->Inlining == TypeInlining::ByReference);

			if (!nullComparison && !TypeSymbol::Equals(leftType, rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
			}
			
			return SymbolTable::Primitives::Boolean;
		}
			
		case TokenType::AddOperator:
		{
			if (leftType == SymbolTable::Primitives::String || rightType == SymbolTable::Primitives::String)
				return leftType;

			if (!TypeSymbol::Equals(leftType, rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			return leftType;
		}

		case TokenType::SubOperator:
		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
		case TokenType::PowOperator:
		{
			if (!TypeSymbol::Equals(leftType, rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			return leftType;
		}
			
		case TokenType::AddAssignOperator:
		{
			if (leftType == SymbolTable::Primitives::String)
				return leftType;

			if (!TypeSymbol::Equals(leftType, rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			return leftType;
		}

		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
		{
			if (!TypeSymbol::Equals(leftType, rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			return leftType;
		}
			
		default:
			return leftType;
	}
}

void ExpressionBinder::VisitBinaryExpression(BinaryExpressionSyntax* node)
{
	TypeSymbol* type = AnalyzeBinaryExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeUnaryExpression(UnaryExpressionSyntax* node)
{
	TypeSymbol* exprType = GetExpressionType(node->Expression.get());
	
	if (exprType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Operand type could not be determined");
		return nullptr;
	}

	{
		bool tryOperatorMethod = exprType->IsPrimitive() ||
			(node->OperatorToken.Type != TokenType::IncrementOperator && node->OperatorToken.Type != TokenType::DecrementOperator);

		if (tryOperatorMethod)
		{
			std::vector<TypeSymbol*> paramTypes = { exprType };
			OperatorSymbol* opMethod = ResolveOperatorMethod(exprType, node->OperatorToken.Type, paramTypes);
			if (opMethod != nullptr)
			{
				node->ToOperator = opMethod;
				return opMethod->ReturnType;
			}
		}

		if (!exprType->IsPrimitive())
		{
			Diagnostics.ReportError(node->OperatorToken, L"Operator '" + GetOperatorMethodName(node->OperatorToken.Type) + L"' is not defined for type '" + exprType->Name + L"'");
			return nullptr;
		}
	}

	switch (node->OperatorToken.Type)
	{
		case TokenType::NotOperator:
		{
			if (exprType != SymbolTable::Primitives::Boolean)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Logical NOT operator requires boolean type, got '" + exprType->Name + L"'");
				return nullptr;
			}

			return SymbolTable::Primitives::Boolean;
		}

		case TokenType::SubOperator:
		case TokenType::AddOperator:
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		{
			if (exprType != SymbolTable::Primitives::Integer && exprType != SymbolTable::Primitives::Double)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Arithmetic unary operator requires numeric type, got '" + exprType->Name + L"'");
				return nullptr;
			}

			return exprType;
		}
			
		default:
		{
			Diagnostics.ReportError(node->OperatorToken, L"Unknown unary operator");
			return nullptr;
		}
	}
}

void ExpressionBinder::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	VisitExpression(node->Expression.get());

	TypeSymbol* type = AnalyzeUnaryExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeObjectExpression(ObjectExpressionSyntax* node)
{
	if (node->Symbol == nullptr)
		return nullptr;

	std::wstring methodName = node->IdentifierToken.Word;
	ConstructorSymbol* method = ResolveConstructor(node);
	if (method == nullptr)
		return node->Symbol;

	if (!IsSymbolAccessible(method, Table->LookupNode(method).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return node->Symbol;
	}

	GenericTypeSymbol* genericType = nullptr;
	if (node->Symbol->Kind == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(node->Symbol);

	if (node->ArgumentsList == nullptr)
		return node->Symbol;

	if (!MatchMethodArguments(node->IdentifierToken, method->Parameters, node->ArgumentsList->Arguments, genericType))
		return node->Symbol;

	node->CtorSymbol = method;
	return node->Symbol;
}

void ExpressionBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	VisitType(node->Type.get());
	VisitArgumentsList(node->ArgumentsList.get());

	TypeSymbol* type = AnalyzeObjectExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeCollectionExpression(CollectionExpressionSyntax* node)
{
	TypeSymbol* collectionType = nullptr;
	for (std::size_t i = 0; i < node->ValuesExpressions.size() && collectionType == nullptr; i++)
		collectionType = GetExpressionType(node->ValuesExpressions.at(i).get());

	for (const auto& expression : node->ValuesExpressions)
	{
		TypeSymbol* exprType = GetExpressionType(expression.get());
		if (exprType != collectionType)
		{
			Diagnostics.ReportError(node->OpenSquareToken, L"Element have type different from collection's target type");
		}
	}

	return collectionType;
}

void ExpressionBinder::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (const auto& expression : node->ValuesExpressions)
		VisitExpression(expression.get());

	TypeSymbol* type = AnalyzeCollectionExpression(node);
	auto arrayType = std::make_unique<ArrayTypeSymbol>(type);

	arrayType->Length = node->ValuesExpressions.size();
	ArrayTypeSymbol* rawArrayType = arrayType.get();
	node->Symbol = rawArrayType;

	Table->BindSymbol(node, std::move(arrayType));
	SetExpressionType(node, rawArrayType);
}

void ExpressionBinder::VisitRangeExpression(RangeExpressionSyntax* node)
{
	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());

	TypeSymbol* leftType = GetExpressionType(node->Left.get());
	TypeSymbol* rightType = GetExpressionType(node->Right.get());

	if (leftType != SymbolTable::Primitives::Integer || rightType != SymbolTable::Primitives::Integer)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Range bounds must be integers");
	}

	ArrayTypeSymbol* arrayType = Factory.Array(SymbolTable::Primitives::Integer);
	SetExpressionType(node, arrayType);
}

void ExpressionBinder::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	MethodSymbol* anonymousMethod = Factory.CreateAnonymousMethod(L"Lambda", SymbolTable::Primitives::Any);

	DelegateTypeSymbol* delegate = Factory.Delegate(anonymousMethod);
	node->Symbol = delegate;

	bool hasExplicitReturnType = false;
	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());
		if (node->ReturnType->Symbol != nullptr)
		{
			anonymousMethod->ReturnType = node->ReturnType->Symbol;
			delegate->ReturnType = node->ReturnType->Symbol;
			hasExplicitReturnType = true;
		}
	}

	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		for (const auto& parameter : node->ParametersList->Parameters)
		{
			TypeSymbol* paramType = parameter->Type != nullptr ? parameter->Type->Symbol : SymbolTable::Primitives::Any;
			ParameterSymbol* paramSymbol = Factory.Parameter(parameter->Identifier.Word, paramType);
			delegate->Parameters.push_back(paramSymbol);
			anonymousMethod->Parameters.push_back(paramSymbol);
		}
	}

	PushScope(anonymousMethod);
	for (const auto& parameter : anonymousMethod->Parameters)
		Declare(parameter);

	CurrentScope()->ReturnFound = false;
	VisitStatementsBlock(node->Body.get());
	PopScope();

	if (!hasExplicitReturnType)
		delegate->ReturnType = anonymousMethod->ReturnType;

	SetExpressionType(node, delegate);
}

void ExpressionBinder::VisitTernaryExpression(TernaryExpressionSyntax* node)
{
	VisitExpression(node->Condition.get());
	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());

	TypeSymbol* conditionType = GetExpressionType(node->Condition.get());
	if (conditionType == nullptr)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition return type could not be determined");
	}
	else if (conditionType != SymbolTable::Primitives::Boolean)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition expected to return 'bool', but got '" + conditionType->Name + L"'");
	}

	TypeSymbol* leftType = GetExpressionType(node->Left.get());
	TypeSymbol* rightType = GetExpressionType(node->Right.get());
	SetExpressionType(node, leftType == nullptr ? rightType : leftType);

	if (leftType == nullptr)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's, left expression's return type could not be determined");
	}
	else
	{
		if (rightType == nullptr)
		{
			Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's, right expression's return type could not be determined");
		}
		else if (rightType != leftType)
		{
			Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition expected to return '" + leftType->Name + L"', but got '" + rightType->Name + L"'");
		}
	}
}

void ExpressionBinder::VisitIfExpression(IfExpressionSyntax* node)
{
	VisitExpression(node->Condition.get());
	VisitExpression(node->ThenExpression.get());
	VisitExpression(node->ElseExpression.get());

	TypeSymbol* conditionType = GetExpressionType(node->Condition.get());
	if (conditionType == nullptr)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's condition return type could not be determined");
	}
	else if (conditionType != SymbolTable::Primitives::Boolean)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's condition expected to return 'bool', but got '" + conditionType->Name + L"'");
	}

	TypeSymbol* thenType = GetExpressionType(node->ThenExpression.get());
	TypeSymbol* elseType = GetExpressionType(node->ElseExpression.get());
	SetExpressionType(node, thenType == nullptr ? elseType : thenType);

	if (thenType == nullptr)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's then-branch return type could not be determined");
	}
	else if (elseType == nullptr)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's else-branch return type could not be determined");
	}
	else if (thenType != elseType)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's branches have different types: '" + thenType->Name + L"' and '" + elseType->Name + L"'");
	}
}

void ExpressionBinder::VisitSwitchExpression(SwitchExpressionSyntax* node)
{
	VisitExpression(node->Expression.get());
	TypeSymbol* exprType = GetExpressionType(node->Expression.get());

	TypeSymbol* armType = nullptr;
	for (const auto& arm : node->Arms)
	{
		VisitExpression(arm->Pattern.get());
		VisitExpression(arm->Expression.get());

		TypeSymbol* currentArmType = GetExpressionType(arm->Expression.get());
		if (armType == nullptr)
			armType = currentArmType;
	}

	SetExpressionType(node, armType);
}

void ExpressionBinder::VisitTryStatement(TryStatementSyntax* node)
{
	if (node->TryBlock != nullptr)
		VisitStatementsBlock(node->TryBlock.get());

	for (const auto& clause : node->CatchClauses)
	{
		VariableSymbol* catchVariable = clause->Symbol;
		if (catchVariable != nullptr)
		{
			PushScope(catchVariable);
			Declare(catchVariable);
		}

		if (clause->Body != nullptr)
			VisitStatementsBlock(clause->Body.get());

		if (catchVariable != nullptr)
			PopScope();
	}
}

bool ExpressionBinder::MatchMethodArguments(SyntaxToken blameToken, std::vector<ParameterSymbol*>& parameters, std::vector<std::unique_ptr<ArgumentSyntax>>& arguments, GenericTypeSymbol* genericType, std::size_t parameterOffset)
{
	if (parameterOffset > parameters.size())
		parameterOffset = parameters.size();

	std::size_t expectedArguments = parameters.size() - parameterOffset;
	if (expectedArguments != arguments.size())
	{
		Diagnostics.ReportError(blameToken, L"Method expects " + std::to_wstring(expectedArguments) + L" arguments but got " + std::to_wstring(arguments.size()));
		return false;
	}
	
	for (std::size_t i = parameterOffset; i < parameters.size(); i++)
	{
		ParameterSymbol* param = parameters[i];
		ArgumentSyntax* arg = arguments[i - parameterOffset].get();

		if (param == nullptr || arg == nullptr || arg->Expression == nullptr)
		{
			Diagnostics.ReportError(blameToken, L"Invalid argument at position " + std::to_wstring(i - parameterOffset));
			return false;
		}
		
		/*
		PushScope(new LeftDenotationSymbol(param->Type));
		VisitExpression(expr);
		PopScope();
		*/
		
		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
		{
			//Diagnostics.ReportError(blameToken, L"Argument type could not be determined for parameter '" + param->Name + L"'");
			return false;
		}

		TypeSymbol* paramType = param->Type;
		if (paramType == nullptr)
		{
			return false;
		}

		if (paramType->Kind == SyntaxKind::TypeParameter)
		{
			paramType = SubstituteTypeParameters(paramType, genericType);
		}

		if (paramType == SymbolTable::Primitives::Any)
		{
			return true;
		}

		if (!TypeSymbol::IsAssignableFrom(paramType, argType))
		{
			Diagnostics.ReportError(blameToken, L"Argument type mismatch for parameter '" + param->Name + L"': expected '" + paramType->Name + L"' but got '" + argType->Name + L"'");
			return false;
		}
	}
	
	return true;
}

TypeSymbol* ExpressionBinder::AnalyzeMemberAccessExpression(MemberAccessExpressionSyntax* node, TypeSymbol* currentType)
{
	std::wstring memberName = node->IdentifierToken.Word;
	SyntaxSymbol* symbol = nullptr;

	if (node->PreviousExpression == nullptr)
	{
		// Check if this is the 'field' keyword - resolve to backing field of current property
		if (node->IdentifierToken.Type == TokenType::FieldKeyword)
			return AnalyzeFieldKeywordExpression(node, nullptr);

		symbol = CurrentScope()->Lookup(memberName).value_or(nullptr);
		if (symbol == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + memberName + L"' not found in current scope");
			return nullptr;
		}

		if (!IsSymbolAccessible(symbol, Table->LookupNode(symbol).value_or(nullptr), node))
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + memberName + L"' is not accessible");
			return nullptr;
		}

		if (symbol->IsType())
		{
			node->IsStaticContext = true;
			return static_cast<TypeSymbol*>(symbol);
		}
	}
	else
	{
		if (currentType == nullptr)
			return nullptr;

		ExpressionSyntax* previousExpression = node->PreviousExpression.get();
		VisitExpression(previousExpression);
		currentType = GetExpressionType(previousExpression);

		if (currentType == nullptr)
			return nullptr;

		if (!currentType->IsType())
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Cannot access member on non-type '" + currentType->Name + L"'");
			return nullptr;
		}

		// First check for property (properties take precedence over fields)
		symbol = currentType->FindProperty(memberName);
		if (symbol == nullptr)
		{
			// Check for field
			symbol = currentType->FindField(memberName);
			if (symbol == nullptr)
			{
				auto methodIt = std::find_if(
					currentType->Methods.begin(), currentType->Methods.end(),
					[memberName](const MethodSymbol* method) { return method->Name == memberName; });

				if (methodIt == currentType->Methods.end())
				{
					OperatorSymbol* accessOp = currentType->FindOperator(TokenType::Delimeter, { SymbolTable::Primitives::String });
					if (accessOp != nullptr)
					{
						node->ToOperator = accessOp;
						return accessOp->ReturnType;
					}

					Diagnostics.ReportError(node->IdentifierToken, L"Member '" + memberName + L"' not found in type '" + currentType->Name + L"'");
					return nullptr;
				}

				symbol = *methodIt;
			}
		}
	}

	if (!IsSymbolAccessible(node->ToField, Table->LookupNode(node->ToField).value_or(nullptr), node))
	{
		std::wstring declName;
		Diagnostics.ReportError(node->IdentifierToken, declName + L" '" + memberName + L"' is not accessible");
		return nullptr;
	}

	switch (symbol->Kind)
	{
		case SyntaxKind::VariableStatement:
		{
			VariableSymbol* varSymbol = static_cast<VariableSymbol*>(symbol);
			node->IsStaticContext = false;
			node->ToVariable = varSymbol;
			return const_cast<TypeSymbol*>(varSymbol->Type);
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* propertySymbol = static_cast<PropertySymbol*>(symbol);
			node->IsStaticContext = false;
			node->ToProperty = propertySymbol;
			return AnalyzePropertyAccessExpression(node, propertySymbol, currentType);
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* fieldSymbol = static_cast<FieldSymbol*>(symbol);
			TypeSymbol* fieldType = fieldSymbol->ReturnType;
			
			if (fieldType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Field '" + memberName + L"' type not resolved");
				return nullptr;
			}

			if (fieldType->Kind == SyntaxKind::TypeParameter)
			{
				if (currentType->Kind == SyntaxKind::GenericType)
				{
					GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(currentType);
					fieldType = SubstituteTypeParameters(fieldType, genericType);
				}
			}

			node->IsStaticContext = false;
			node->ToField = fieldSymbol;

			bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());
			if (isStaticContext && fieldSymbol->Linking == LINK_INSTANCE)
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance field '" + fieldSymbol->FullName + L"' from type context");

			if (!isStaticContext && fieldSymbol->Linking == LINK_STATIC)
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access static field '" + fieldSymbol->FullName + L"' from instance reference");

			return fieldType;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodSymbol* methodSymbol = static_cast<MethodSymbol*>(symbol);
			DelegateTypeSymbol* delegate = Factory.Delegate(methodSymbol);

			node->ToDelegate = delegate;
			node->IsStaticContext = false;
			
			return delegate;
		}

		case SyntaxKind::Parameter:
		{
			ParameterSymbol* paramSymbol = static_cast<ParameterSymbol*>(symbol);
			node->ToParameter = paramSymbol;
			node->IsStaticContext = false;
			return paramSymbol->Type;
		}

		default:
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + memberName + L"' is not a variable, parameter or field (found " + std::to_wstring(static_cast<int>(symbol->Kind)) + L")");
			return nullptr;
		}
	}
}

TypeSymbol* ExpressionBinder::AnalyzePropertyAccessExpression(MemberAccessExpressionSyntax* node, PropertySymbol* property, TypeSymbol* currentType)
{
	if (property == nullptr)
		return nullptr;

	if (currentType == nullptr)
		return nullptr;

	std::wstring memberName = node->IdentifierToken.Word;
	if (property->ReturnType == nullptr)
	{
		if (currentType->Kind == SyntaxKind::ArrayType)
		{
			ArrayTypeSymbol* array = static_cast<ArrayTypeSymbol*>(currentType);
			if (array->UnderlayingType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' type not resolved");
				return nullptr;
			}
		}
		else
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' type not resolved");
			return nullptr;
		}
	}

	bool requiresSetter = IsAssignmentContext(node);
	AccessorSymbol* accessor = requiresSetter ? property->Setter : property->Getter;

	if (accessor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' does not have a " + (requiresSetter ? L"set" : L"get") + L" accessor");
		return nullptr;
	}

	if (!IsSymbolAccessible(accessor, Table->LookupNode(accessor).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->IdentifierToken, (requiresSetter ? L"Setter" : L"Getter") + (L" of property '" + memberName + L"' is not accessible"));
		return nullptr;
	}

	bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());
	if (isStaticContext && accessor->Linking == LINK_INSTANCE)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance property '" + memberName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && accessor->Linking == LINK_STATIC)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot access static property '" + memberName + L"' from instance reference");
		return nullptr;
	}

	node->IsStaticContext = false;
	if (currentType->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* array = static_cast<ArrayTypeSymbol*>(currentType);
		return array->UnderlayingType;
	}

	TypeSymbol* propertyType = property->ReturnType;
	
	// Если currentType является GenericTypeSymbol, заменяем type parameters на type arguments
	if (currentType->Kind == SyntaxKind::GenericType && propertyType != nullptr && propertyType->Kind == SyntaxKind::TypeParameter)
	{
		GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(currentType);
		propertyType = SubstituteTypeParameters(propertyType, genericType);
	}
	
	return propertyType;
}

TypeSymbol* ExpressionBinder::AnalyzeFieldKeywordExpression(MemberAccessExpressionSyntax* node, TypeSymbol* currentType)
{
	// Find PropertySymbol in current scope chain
	PropertySymbol* propertySymbol = nullptr;
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		SyntaxSymbol* symbol = const_cast<SyntaxSymbol*>(scope->Owner);
		if (symbol == nullptr)
			continue;

		if (symbol->IsType())
			break;

		if (symbol->Kind != SyntaxKind::PropertyDeclaration)
			continue;

		propertySymbol = static_cast<PropertySymbol*>(symbol);
		break;
	}

	if (propertySymbol == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Keyword 'field' can only be used in property accessors");
		return nullptr;
	}

	// Create backing field if it doesn't exist yet (for explicit property bodies using 'field')
	if (propertySymbol->BackingField == nullptr)
	{
		GeneratePropertyBackingField(Factory, propertySymbol);
		if (propertySymbol->BackingField == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Backing field type not resolved for property '" + propertySymbol->Name + L"'");
			return nullptr;
		}
	}

	// Resolve 'field' as the backing field
	node->ToField = propertySymbol->BackingField;
	node->IsStaticContext = propertySymbol->Linking == LINK_STATIC;

	if (propertySymbol->BackingField->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Backing field type not resolved for property '" + propertySymbol->Name + L"'");
		return nullptr;
	}

	return propertySymbol->BackingField->ReturnType;
}

TypeSymbol* ExpressionBinder::AnalyzeInvokationExpression(InvokationExpressionSyntax* node, TypeSymbol* currentType)
{
	/*
	if (currentType == nullptr)
		return nullptr;
	*/

	std::wstring methodName = node->IdentifierToken.Word;
	MethodSymbol* method = ResolveMethod(node, currentType);

	if (method == nullptr)
	{
		return nullptr;
	}

	if (!IsSymbolAccessible(method, Table->LookupNode(method).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return nullptr;
	}

	GenericTypeSymbol* genericType = nullptr;
	if (currentType != nullptr && currentType->Kind == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(currentType);

	if (node->IsExtensionMethodInvocation)
	{
		std::vector<ParameterSymbol*> effectiveParameters(method->Parameters.begin() + 1, method->Parameters.end());
		if (!MatchMethodArguments(node->IdentifierToken, effectiveParameters, node->ArgumentsList->Arguments, genericType))
			return nullptr;

		node->Symbol = method;
		node->ReceiverType = currentType;
		node->IsStaticContext = true;
	}
	else
	{
		if (!MatchMethodArguments(node->IdentifierToken, method->Parameters, node->ArgumentsList->Arguments, genericType))
			return nullptr;

		node->Symbol = method;
		node->ReceiverType = currentType;
		node->IsStaticContext = false;
	}

	if (method->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' return type not resolved");
		return nullptr;
	}

	TypeSymbol* returnType = method->ReturnType;
	if (returnType->Kind == SyntaxKind::TypeParameter)
		returnType = SubstituteTypeParameters(returnType, genericType);
	
	return returnType;
}

ConstructorSymbol* ExpressionBinder::ResolveConstructor(ObjectExpressionSyntax* node)
{
	TypeSymbol* symbol = node->Symbol;
	if (symbol == nullptr)
		return nullptr;

	if (symbol->Kind == SyntaxKind::GenericType)
		symbol = static_cast<GenericTypeSymbol*>(symbol)->UnderlayingType;

	std::vector<TypeSymbol*> argTypes;
	if (node->ArgumentsList == nullptr)
		return nullptr;

	for (const auto& arg : node->ArgumentsList->Arguments)
	{
		if (arg == nullptr || arg->Expression == nullptr)
			return nullptr;

		VisitExpression(arg->Expression.get());

		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
			return nullptr;

		argTypes.push_back(argType);
	}

	ConstructorSymbol* method = symbol->FindConstructor(argTypes);
	if (method == nullptr)
	{
		std::wstringstream diag;
		diag << "No constructor of \"" << symbol->FullName << "\" found that accepts ";

		switch (argTypes.size())
		{
			case 0:
			{
				diag << "no arguments";
				break;
			}

			case 1:
			{
				TypeSymbol* type = argTypes.at(0);
				std::wstring typeName = type == nullptr ? L"<error>" : type->Name;
				diag << "argument (" << typeName << ")";
				break;
			}

			default:
			{
				TypeSymbol* type = argTypes.at(0);
				diag << L"arguments (" << type->Name;

				for (int i = 1; i < argTypes.size(); i++)
				{
					type = argTypes.at(i);
					diag << ", " << type->Name;
				}

				diag << ")";
				break;
			}
		}

		Diagnostics.ReportError(node->IdentifierToken, diag.str());
		return nullptr;
	}

	return method;
}

static bool IsExtensionMethodCandidate(MethodSymbol* method, TypeSymbol* receiverType, const std::vector<TypeSymbol*>& argTypes)
{
	if (method == nullptr || method->Linking != LINK_STATIC)
		return false;

	if (method->Parameters.empty())
		return false;

	if (receiverType == nullptr)
		return false;

	if (!TypeSymbol::IsAssignableFrom(method->Parameters[0]->Type, receiverType))
		return false;

	if (method->Parameters.size() != argTypes.size() + 1)
		return false;

	for (std::size_t i = 1; i < method->Parameters.size(); ++i)
	{
		TypeSymbol* paramType = method->Parameters[i]->Type;
		TypeSymbol* argType = argTypes[i - 1];
		if (paramType == nullptr || argType == nullptr)
			return false;

		if (!TypeSymbol::IsAssignableFrom(paramType, argType))
			return false;
	}

	return true;
}

MethodSymbol* ExpressionBinder::ResolveMethod(InvokationExpressionSyntax* node, TypeSymbol* currentType)
{
	std::wstring methodName = node->IdentifierToken.Word;
	std::vector<TypeSymbol*> argTypes;

	if (node->ArgumentsList == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' invocation has no arguments list");
		return nullptr;
	}

	for (const auto& arg : node->ArgumentsList->Arguments)
	{
		//PushScope(new LeftDenotationSymbol(param->Type));
		VisitExpression(arg->Expression.get());
		//PopScope();

		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
			return nullptr;

		argTypes.push_back(argType);
	}

	// Try to find method inside current type
	MethodSymbol* symbol = nullptr;
	if (currentType != nullptr)
	{
		symbol = currentType->FindMethod(methodName, argTypes);
		if (symbol == nullptr)
			symbol = SymbolTable::Global::Type->FindMethod(methodName, argTypes);
	}

	if (symbol == nullptr)
	{
		SyntaxSymbol* lookupMethod = CurrentScope()->Lookup(methodName).value_or(nullptr);
		if (lookupMethod != nullptr)
		{
			if (lookupMethod->Kind == SyntaxKind::VariableStatement)
			{
				VariableSymbol* delegateVar = static_cast<VariableSymbol*>(lookupMethod);
				if (delegateVar->Type != nullptr)
				{
					if (delegateVar->Type->Kind == SyntaxKind::DelegateType)
					{
						const DelegateTypeSymbol* delegate = static_cast<const DelegateTypeSymbol*>(delegateVar->Type);
						if (node->PreviousExpression == nullptr)
						{
							auto target = std::make_unique<MemberAccessExpressionSyntax>(node->IdentifierToken, nullptr, node);
							target->ToVariable = delegateVar;
							node->PreviousExpression = std::move(target);
						}
						node->Symbol = delegate->AnonymousSymbol;
						node->IsDelegateInvocation = true;
						return node->Symbol;
					}
				}
			}
			else if (lookupMethod->Kind == SyntaxKind::Parameter)
			{
				ParameterSymbol* delegateParam = static_cast<ParameterSymbol*>(lookupMethod);
				if (delegateParam->Type != nullptr)
				{
					if (delegateParam->Type->Kind == SyntaxKind::DelegateType)
					{
						const DelegateTypeSymbol* delegate = static_cast<const DelegateTypeSymbol*>(delegateParam->Type);
						if (node->PreviousExpression == nullptr)
						{
							auto target = std::make_unique<MemberAccessExpressionSyntax>(node->IdentifierToken, nullptr, node);
							target->ToParameter = delegateParam;
							node->PreviousExpression = std::move(target);
						}
						node->Symbol = delegate->AnonymousSymbol;
						node->IsDelegateInvocation = true;
						return node->Symbol;
					}
				}
			}
			else if (lookupMethod->Kind == SyntaxKind::FieldDeclaration)
			{
				FieldSymbol* delegateField = static_cast<FieldSymbol*>(lookupMethod);
				if (delegateField->ReturnType != nullptr)
				{
					if (delegateField->ReturnType->Kind == SyntaxKind::DelegateType)
					{
						const DelegateTypeSymbol* delegate = static_cast<const DelegateTypeSymbol*>(delegateField->ReturnType);
						if (node->PreviousExpression == nullptr && delegateField->Linking == LINK_STATIC)
						{
							auto target = std::make_unique<MemberAccessExpressionSyntax>(node->IdentifierToken, nullptr, node);
							target->ToField = delegateField;
							node->PreviousExpression = std::move(target);
						}
						node->Symbol = delegate->AnonymousSymbol;
						node->IsDelegateInvocation = true;
						return node->Symbol;
					}
				}
			}
			else if (lookupMethod->Kind == SyntaxKind::MethodDeclaration)
			{
				symbol = static_cast<MethodSymbol*>(lookupMethod);
			}
		}
	}

	if (symbol == nullptr)
	{
		std::wstringstream diag;
		diag << "No method \"" << methodName << "\" found that accepts ";

		switch (argTypes.size())
		{
			case 0:
			{
				diag << "no arguments";
				break;
			}

			case 1:
			{
				TypeSymbol* type = argTypes.at(0);
				std::wstring typeName = type == nullptr ? L"<error>" : type->Name;
				diag << "argument (" << typeName << ")";
				break;
			}

			default:
			{
				TypeSymbol* type = argTypes.at(0);
				diag << L"arguments (" << type->Name;

				for (int i = 1; i < argTypes.size(); i++)
				{
					type = argTypes.at(i);
					diag << ", " << type->Name;
				}

				diag << ")";
				break;
			}
		}

		Diagnostics.ReportError(node->IdentifierToken, diag.str());
		return nullptr;
	}
	
	bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());
	if (isStaticContext && symbol->Linking == LINK_INSTANCE)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot call instance method '" + methodName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && symbol->Linking == LINK_STATIC)
	{
		if (IsExtensionMethodCandidate(symbol, currentType, argTypes))
		{
			node->IsExtensionMethodInvocation = true;
		}
		else
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Cannot call static method '" + methodName + L"' on instance reference");
			return nullptr;
		}
	}

	return symbol;
}

TypeSymbol* ExpressionBinder::AnalyzeIndexatorExpression(IndexatorExpressionSyntax* node, TypeSymbol* currentType)
{
	if (currentType == nullptr)
		return nullptr;

	if (node->PreviousExpression == nullptr)
		return nullptr;

	VisitExpression(node->PreviousExpression.get());
	currentType = GetExpressionType(node->PreviousExpression.get());

	IndexatorSymbol* indexator = ResolveIndexator(node, currentType);
	TypeSymbol* resultType = AnalyzePropertyAccessExpression(node, indexator, currentType);

	if (resultType == nullptr)
		return nullptr;

	if (indexator == nullptr)
		return nullptr;

	bool requiresSetter = IsAssignmentContext(node);
	AccessorSymbol* accessor = requiresSetter ? indexator->Setter : indexator->Getter;

	if (accessor != nullptr)
	{
		GenericTypeSymbol* genericType = nullptr;
		if (currentType->Kind == SyntaxKind::GenericType)
			genericType = static_cast<GenericTypeSymbol*>(currentType);

		if (!MatchMethodArguments(node->IdentifierToken, indexator->Parameters, node->IndexatorList->Arguments, genericType))
		{
			//Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Indexator arguments types do not match");
			return nullptr;
		}
	}

	node->ToProperty = indexator;
	node->IsStaticContext = node->IsStaticContext;
	return resultType;
}

TypeSymbol* ExpressionBinder::SubstituteTypeParameters(TypeSymbol* type, GenericTypeSymbol* genericType)
{
	if (type == nullptr || genericType == nullptr)
		return type;

	type = genericType->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(type));
	if (type == nullptr)
		Diagnostics.ReportError(SyntaxToken(), L"Failed to substitute type");

	return type;
}

static bool IsNumBasePrefix(std::wstring& prefix, int& base)
{
	if (prefix[0] != L'0')
		return false;

	switch (prefix[1])
	{
		default:
			return false;

		case L'x': case L'X': base = 16; return true;
		case L'd': case L'D': base = 10; return true;
		case L'b': case L'B': base = 2; return true;
	}
}

static bool IsVolumeRatioPostfix(std::wstring& postfix, std::size_t& multiplier)
{
	if (postfix[1] != L'B' && postfix[1] != L'b')
		return false;

	switch (postfix[0])
	{
		default:
			return false;

		case L'k': case L'K': multiplier = ((std::size_t)1 << 10); return true;
		case L'm': case L'M': multiplier = ((std::size_t)1 << 20); return true;
		case L'g': case L'G': multiplier = ((std::size_t)1 << 30); return true;
		case L't': case L'T': multiplier = ((std::size_t)1 << 40); return true;
		case L'p': case L'P': multiplier = ((std::size_t)1 << 50); return true;
	}
}

static bool IsValidIntegerPunctuation(wchar_t symbol)
{
	switch (symbol)
	{
		default:
			return false;

		case '.':
			return true;
	}
}

static bool IsValidIntegerSymbol(wchar_t symbol, int base)
{
	if (symbol >= L'0' && symbol <= L'1')
		return base >= 2;

	if (symbol >= L'2' && symbol <= L'9')
		return base >= 10;

	if (symbol >= L'a' && symbol <= L'f')
		return base >= 16;

	if (symbol >= L'A' && symbol <= L'F')
		return base >= 16;

	return false;
}

TypeSymbol* ExpressionBinder::AnalyzeNumberLiteral(LiteralExpressionSyntax* node)
{
	LiteralSymbol* symbol = LookupSymbol<LiteralSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return SymbolTable::Primitives::Integer;

	SyntaxToken token = node->LiteralToken;
	std::wstring word = token.Word;
	std::size_t size = word.size();

	std::size_t numStart = 0;
	std::size_t multiplier = 1;
	int base = 10;

	if (size >= 2)
	{
		std::wstring prefix = word.substr(0, 2);
		if (IsNumBasePrefix(prefix, base))
			numStart += 2;

		std::wstring postfix = word.substr(size - 2);
		if (IsVolumeRatioPostfix(postfix, multiplier))
			size -= 2;
	}

	try
	{
		std::size_t pos = 0;
		std::wstring numPart = word.substr(numStart, size);
		symbol->AsIntegerValue = std::stol(numPart, &pos) * multiplier;

		if (pos != size)
		{
			for (std::size_t i = pos; i < size; i++)
			{
				if (!IsValidIntegerSymbol(word[i], base))
				{
					Diagnostics.ReportError(token, L"Invalid characters in number");
					return SymbolTable::Primitives::Integer;
				}
			}
		}

		// overflow check
		if (symbol->AsIntegerValue < 0 && multiplier > 1)
		{
			long check = std::stol(numPart, nullptr, base);
			if (check > LONG_MAX / multiplier)
			{
				Diagnostics.ReportError(token, L"Multiplication overflow");
				return SymbolTable::Primitives::Integer;
			}
		}

		return SymbolTable::Primitives::Integer;
	}
	catch (const std::out_of_range&)
	{
		Diagnostics.ReportError(token, L"Number out of range");
		return SymbolTable::Primitives::Integer;
	}
	catch (const std::exception&)
	{
		Diagnostics.ReportError(token, L"Invalid number format");
		return SymbolTable::Primitives::Integer;
	}
}

TypeSymbol* ExpressionBinder::AnalyzeDoubleLiteral(LiteralExpressionSyntax* node)
{
	LiteralSymbol* symbol = LookupSymbol<LiteralSymbol>(node).value_or(nullptr);

	SyntaxToken token = node->LiteralToken;
	std::wstring word = token.Word;
	std::size_t size = word.size();

	std::size_t delimeterIndex = word.find('.');
	//word.pop_back(); // deleting symbol 'f'

	if (word.size() >= 2)
	{
		std::wstring prefix = word.substr(0, 2);
		int dummy = 0;

		if (IsNumBasePrefix(prefix, dummy))
		{
			Diagnostics.ReportError(token, L"Floating point number cannot have base prefix");
			return SymbolTable::Primitives::Double;
		}

		std::wstring postfix = word.substr(size - 2);
		std::size_t dummy2 = 0;

		if (IsVolumeRatioPostfix(postfix, dummy2))
		{
			Diagnostics.ReportError(token, L"Floating point number cannot have suffix");
			return SymbolTable::Primitives::Double;
		}
	}

	try
	{
		return SymbolTable::Primitives::Double;
		std::size_t pos = 0;
		word[delimeterIndex] = L',';
		symbol->AsDoubleValue = std::stod(word, &pos);

		if (pos != size)
		{
			for (std::size_t i = pos; i < size; i++)
			{
				if (!std::isdigit(word[i]) && !IsValidIntegerPunctuation(word[i]))
				{
					Diagnostics.ReportError(token, L"Invalid characters in floating point number");
					return SymbolTable::Primitives::Double;
				}
			}
		}
	}
	catch (const std::out_of_range&)
	{
		Diagnostics.ReportError(node->LiteralToken, L"Number out of range");
	}
	catch (const std::exception&)
	{
		Diagnostics.ReportError(node->LiteralToken, L"Invalid floating point number format");
	}

	return SymbolTable::Primitives::Double;
}

IndexatorSymbol* ExpressionBinder::ResolveIndexator(IndexatorExpressionSyntax* node, TypeSymbol* currentType)
{
	MemberAccessExpressionSyntax* access = static_cast<MemberAccessExpressionSyntax*>(const_cast<ExpressionSyntax*>(node->PreviousExpression.get()));
	std::wstring methodName = access->IdentifierToken.Word;
	std::vector<TypeSymbol*> argTypes;

	for (const auto& arg : node->IndexatorList->Arguments)
	{
		VisitExpression(arg->Expression.get());

		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
			return nullptr;

		argTypes.push_back(argType);
	}

	if (currentType->Kind == SyntaxKind::ArrayType)
	{
		if (SymbolTable::Primitives::Array == nullptr || SymbolTable::Primitives::Array->Indexators.empty())
			return nullptr;

		return SymbolTable::Primitives::Array->Indexators[0];
	}

	IndexatorSymbol* symbol = currentType->FindIndexator(argTypes);
	if (symbol == nullptr)
		symbol = SymbolTable::Global::Type->FindIndexator(argTypes);

	if (symbol == nullptr)
	{
		std::wstringstream diag;
		diag << "No indeaxtors for type \"" << currentType->Name << "\" found that accepts ";

		switch (argTypes.size())
		{
			case 0:
			{
				diag << "no arguments";
				break;
			}

			case 1:
			{
				TypeSymbol* type = argTypes.at(0);
				std::wstring typeName = type == nullptr ? L"<error>" : type->Name;
				diag << "argument (" << typeName << ")";
				break;
			}

			default:
			{
				TypeSymbol* type = argTypes.at(0);
				diag << L"arguments (" << type->Name;

				for (int i = 1; i < argTypes.size(); i++)
				{
					type = argTypes.at(i);
					diag << ", " << type->Name;
				}

				diag << ")";
				break;
			}
		}

		Diagnostics.ReportError(access->IdentifierToken, diag.str());
		return nullptr;
	}

	bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());
	if (isStaticContext && symbol->Linking == LINK_INSTANCE)
	{
		Diagnostics.ReportError(access->IdentifierToken, L"Cannot call instance indexator '" + methodName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && symbol->Linking == LINK_STATIC)
	{
		Diagnostics.ReportError(access->IdentifierToken, L"Cannot call static indexator '" + methodName + L"' on instance reference");
		return nullptr;
	}

	return symbol;
}

void ExpressionBinder::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
{
	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression.get());
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType().value_or(nullptr) : GetExpressionType(previous);
	type = AnalyzeMemberAccessExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression.get());
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType().value_or(nullptr) : GetExpressionType(previous);
	type = AnalyzeInvokationExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	if (node->IndexatorList != nullptr)
		VisitIndexatorList(node->IndexatorList.get());

	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression.get());
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType().value_or(nullptr) : GetExpressionType(previous);
	type = AnalyzeIndexatorExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitWhileStatement(WhileStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression.get());
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression.get());
		
		if (conditionType == nullptr)
		{
			Diagnostics.ReportError(node->KeywordToken, L"While loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			Diagnostics.ReportError(node->KeywordToken, L"While loop condition must be boolean, got '" + conditionType->Name + L"'");
		}
	}
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void ExpressionBinder::VisitUntilStatement(UntilStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression.get());
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression.get());
		
		if (conditionType == nullptr)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Until loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Until loop condition must be boolean, got '" + conditionType->Name + L"'");
		}
	}
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void ExpressionBinder::VisitForStatement(ForStatementSyntax* node)
{
	if (node->InitializerStatement != nullptr)
		VisitStatement(node->InitializerStatement.get());
	
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression.get());
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression.get());
		
		if (conditionType == nullptr)
		{
			Diagnostics.ReportError(node->FirstSemicolon, L"For loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			Diagnostics.ReportError(node->FirstSemicolon, L"For loop condition must be boolean, got '" + conditionType->Name + L"'");
		}
	}
	
	if (node->AfterRepeatStatement != nullptr)
		VisitStatement(node->AfterRepeatStatement.get());
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void ExpressionBinder::VisitForEachStatement(ForEachStatementSyntax* node)
{
	VariableSymbol* variable = static_cast<VariableSymbol*>(Table->LookupSymbol(node).value_or(nullptr));

	if (node->RangeExpression != nullptr)
	{
		VisitExpression(node->RangeExpression.get());
		TypeSymbol* rangeType = GetExpressionType(node->RangeExpression.get());

		if (rangeType != nullptr && rangeType->Kind == SyntaxKind::ArrayType)
		{
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(rangeType);
			if (variable != nullptr)
				variable->Type = arrayType->UnderlayingType;
		}
		else
		{
			Diagnostics.ReportError(node->InKeywordToken, L"For-in expression must be an array");
		}
	}

	if (variable != nullptr)
	{
		Declare(variable);
		PushScope(variable);
	}

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());

	if (variable != nullptr)
		PopScope();
}

void ExpressionBinder::VisitIfStatement(IfStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		if (node->ConditionExpression->Kind != SyntaxKind::ExpressionStatement)
		{
			Diagnostics.ReportError(node->KeywordToken, L"");
		}
		else
		{
			ExpressionStatementSyntax* statement = static_cast<ExpressionStatementSyntax*>(node->ConditionExpression.get());
			ExpressionSyntax* conditionExpr = statement->Expression.get();

			if (conditionExpr != nullptr)
			{
				VisitExpression(conditionExpr);
				TypeSymbol* conditionType = GetExpressionType(conditionExpr);
				
				if (conditionType == nullptr)
				{
					Diagnostics.ReportError(node->KeywordToken, L"If condition type could not be determined");
				}
				else if (conditionType != SymbolTable::Primitives::Boolean)
				{
					Diagnostics.ReportError(node->KeywordToken, L"If condition must be boolean, got '" + conditionType->Name + L"'");
				}
			}
			else
			{
				Diagnostics.ReportError(node->KeywordToken, L"If condition must be an expression");
			}
		}
	}
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement.get());
}

void ExpressionBinder::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		if (node->ConditionExpression->Kind != SyntaxKind::ExpressionStatement)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Unless condition must be an expression statement");
		}
		else
		{
			ExpressionStatementSyntax* statement = static_cast<ExpressionStatementSyntax*>(node->ConditionExpression.get());
			ExpressionSyntax* conditionExpr = statement->Expression.get();

		if (conditionExpr != nullptr)
		{
			VisitExpression(conditionExpr);
			TypeSymbol* conditionType = GetExpressionType(conditionExpr);
			
			if (conditionType == nullptr)
			{
				Diagnostics.ReportError(node->KeywordToken, L"Unless condition type could not be determined");
			}
			else if (conditionType != SymbolTable::Primitives::Boolean)
			{
				Diagnostics.ReportError(node->KeywordToken, L"Unless condition must be boolean, got '" + conditionType->Name + L"'");
			}
		}
			else
			{
				Diagnostics.ReportError(node->KeywordToken, L"Unless condition must be an expression");
			}
		}
	}

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement.get());
}

static bool SymbolHasReturnType(const SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		return false;

	switch (symbol->Kind)
	{
		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::OperatorDeclaration:
		case SyntaxKind::FieldDeclaration:
		case SyntaxKind::PropertyDeclaration:
		case SyntaxKind::IndexatorDeclaration:
		case SyntaxKind::AccessorDeclaration:
			return true;

		default:
			return false;
	}
}

TypeSymbol* ExpressionBinder::FindTargetReturnType(SemanticScope*& scope)
{
	scope = CurrentScope();
	SyntaxSymbol* symbol = const_cast<SyntaxSymbol*>(scope->Owner);
	
	while (!SymbolHasReturnType(symbol))
	{
		if (scope->Parent == nullptr)
			return nullptr;

		scope = const_cast<SemanticScope*>(scope->Parent);
		symbol = const_cast<SyntaxSymbol*>(scope->Owner);
	}

	switch (symbol->Kind)
	{
		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::OperatorDeclaration:
		{
			MethodSymbol* methodSymbol = static_cast<MethodSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* methodSymbol = static_cast<FieldSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		case SyntaxKind::AccessorDeclaration:
		{
			AccessorSymbol* methodSymbol = static_cast<AccessorSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		case SyntaxKind::IndexatorDeclaration:
		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* methodSymbol = static_cast<PropertySymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		default:
			return nullptr;
	}
}

TypeSymbol* ExpressionBinder::ResolveLeftDenotation()
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		const SyntaxSymbol* owner = scope->Owner;
		if (owner == nullptr)
			break;

		if (owner->Kind == SyntaxKind::Argument)
		{
			const LeftDenotationSymbol* param = static_cast<const LeftDenotationSymbol*>(owner);
			return const_cast<TypeSymbol*>(param->ExpectedType);
		}
	}

	return nullptr;
}

void ExpressionBinder::VisitReturnStatement(ReturnStatementSyntax* node)
{
	SemanticScope* searchingScope = nullptr;
	TypeSymbol* returnType = FindTargetReturnType(searchingScope);

	if (searchingScope == nullptr)
	{
		Diagnostics.ReportError(node->KeywordToken, L"Couldnt find scope to return within");
		return;
	}

	searchingScope->ReturnFound = true;
	if (returnType == SymbolTable::Primitives::Any)
	{
		if (searchingScope->Owner == nullptr || searchingScope->Owner->Kind != SyntaxKind::MethodDeclaration)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Return within invalid scope");
			return;
		}

		MethodSymbol* delegate = const_cast<MethodSymbol*>(static_cast<const MethodSymbol*>(searchingScope->Owner));
		if (node->Expression == nullptr)
		{
			delegate->ReturnType = SymbolTable::Primitives::Void;
			return;
		}
		else
		{
			VisitExpression(node->Expression.get());
			delegate->ReturnType = GetExpressionType(node->Expression.get());
			return;
		}
	}

	if (returnType == SymbolTable::Primitives::Void)
	{
		if (node->Expression != nullptr)
		{
			searchingScope->ReturnsAnything = true;
			VisitExpression(node->Expression.get());

			Diagnostics.ReportError(node->KeywordToken, L"Void method cannot return a value");
		}

		return;
	}

	if (returnType == nullptr)
	{
		Diagnostics.ReportError(node->KeywordToken, L"Return target type could not be determined");
		return;
	}

	if (node->Expression == nullptr)
	{
		Diagnostics.ReportError(SyntaxToken(), L"Return statement must return a value of type '" + returnType->Name + L"'");
		return;
	}

	searchingScope->ReturnsAnything = true;
	VisitExpression(node->Expression.get());
	TypeSymbol* returnExprType = GetExpressionType(node->Expression.get());

	if (returnExprType == nullptr)
	{
		Diagnostics.ReportError(node->KeywordToken, L"Return expression type could not be determined");
	}
	else if (!TypeSymbol::IsAssignableFrom(returnType, returnExprType))
	{
		Diagnostics.ReportError(node->KeywordToken, L"Return type mismatch: expected '" + returnType->Name + L"' but got '" + returnExprType->Name + L"'");
	}
}

void ExpressionBinder::VisitDeferStatement(DeferStatementSyntax* node)
{
	if (node->Statement == nullptr)
		return;

	VisitStatement(node->Statement.get());

	if (node->Statement->Kind == SyntaxKind::VariableStatement)
	{
		VariableStatementSyntax* variableStatement = static_cast<VariableStatementSyntax*>(node->Statement.get());
		VariableSymbol* variable = LookupSymbol<VariableSymbol>(variableStatement).value_or(nullptr);
		if (variable == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"Variable declared in defer statement could not be resolved");
			return;
		}

		TypeSymbol* variableType = const_cast<TypeSymbol*>(variable->Type);
		if (variableType == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"Variable type could not be determined");
			return;
		}

		InterfaceSymbol* disposable = TRAIT_DISPOSABLE;
		if (disposable == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"IDisposable interface is not defined");
			return;
		}

		if (!TypeSymbol::IsAssignableFrom(disposable, variableType))
		{
			Diagnostics.ReportError(node->DeferToken, L"Type '" + variableType->Name + L"' declared in defer statement must implement IDisposable");
			return;
		}

		std::wstring disposeName = L"Dispose";
		MethodSymbol* interfaceDispose = disposable->FindMethod(disposeName, std::vector<TypeSymbol*>());
		if (interfaceDispose == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"IDisposable.Dispose() method not found");
			return;
		}

		MethodSymbol* implementation = variableType->FindInterfaceImplementation(interfaceDispose);
		if (implementation == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"Type '" + variableType->Name + L"' does not provide an implementation for IDisposable.Dispose()");
			return;
		}

		node->Variable = variable;
		node->DisposeMethod = implementation;
		node->IsResourceDefer = true;
	}
}

void ExpressionBinder::VisitCastExpression(CastExpressionSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	if (node->TargetType != nullptr)
		VisitType(node->TargetType.get());

	if (node->TargetType == nullptr || node->TargetType->Symbol == nullptr)
	{
		SetExpressionType(node, nullptr);
		return;
	}

	TypeSymbol* targetType = node->TargetType->Symbol;
	if (targetType->Inlining == TypeInlining::ByValue)
	{
		Diagnostics.ReportError(node->OperatorToken, L"The 'as' operator may only be used with reference types");
	}

	SetExpressionType(node, targetType);
}

void ExpressionBinder::VisitIsExpression(IsExpressionSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	if (node->TargetType != nullptr)
		VisitType(node->TargetType.get());

	if (node->TargetType == nullptr || node->TargetType->Symbol == nullptr)
	{
		SetExpressionType(node, SymbolTable::Primitives::Boolean);
		return;
	}

	SetExpressionType(node, SymbolTable::Primitives::Boolean);
}
