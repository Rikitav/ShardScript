#include <shard/parsing/visiting/ExpressionBinder.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticScope.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxFacts.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/SymbolFactory.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>
#include <shard/syntax/symbols/LeftDenotationSymbol.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>
#include <shard/syntax/symbols/TypeParameterSymbol.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>

#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

using namespace shard;

static bool IsAssignmentOperator(TokenType type)
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

	const BinaryExpressionSyntax* binaryExpr = static_cast<const BinaryExpressionSyntax*>(expression->Parent);
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

	const BinaryExpressionSyntax* binaryExpr = static_cast<const BinaryExpressionSyntax*>(expression->Parent);
	if (!IsAssignmentOperator(binaryExpr->OperatorToken.Type))
		return false;

	if (binaryExpr->Left != expression)
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

	for (UsingDirectiveSyntax* directive : node->Usings)
		VisitUsingDirective(directive);

	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);
	
	PopScope();
}

void ExpressionBinder::VisitUsingDirective(UsingDirectiveSyntax* node)
{
	SemanticScope* current = CurrentScope();
	for (const auto& symbol : node->Namespace->Types)
		current->DeclareSymbol(symbol);
}

void ExpressionBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	NamespaceSymbol* symbol = LookupSymbol<NamespaceSymbol>(node);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		for (MemberDeclarationSyntax* member : node->Members)
		{
			SyntaxSymbol* symbol = Table->LookupSymbol(member);
			Declare(symbol);
		}

		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void ExpressionBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		for (MemberDeclarationSyntax* member : node->Members)
		{
			SyntaxSymbol* symbol = Table->LookupSymbol(member);
			Declare(symbol);
		}

		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void ExpressionBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = LookupSymbol<StructSymbol>(node);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		for (MemberDeclarationSyntax* member : node->Members)
		{
			SyntaxSymbol* symbol = Table->LookupSymbol(member);
			Declare(symbol);
		}

		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void ExpressionBinder::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node);
	if (!symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol != nullptr && node->Body != nullptr)
	{
		PushScope(symbol);

		Declare(new VariableSymbol(L"this", ownerType));
		for (ParameterSymbol* parameter : symbol->Parameters)
			Declare(new VariableSymbol(parameter->Name, parameter->Type));

		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body);
		
		PopScope();
	}
}

void ExpressionBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (!symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	PushScope(symbol);

	if (!symbol->IsStatic)
		Declare(new VariableSymbol(L"this", ownerType));

	for (ParameterSymbol* parameter : symbol->Parameters)
		Declare(new VariableSymbol(parameter->Name, parameter->Type));

	if (node->Body != nullptr)
	{
		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body);

		if (symbol->ReturnType->Name != L"Void")
		{
			if (!CurrentScope()->ReturnFound)
				Diagnostics.ReportError(node->IdentifierToken, L"Method must return a value of type '" + symbol->ReturnType->Name + L"'");
		}
	}

	if (symbol->Name == L"Main")
	{
		Table->EntryPointCandidates.push_back(symbol);
		if (symbol->Accesibility != SymbolAccesibility::Public)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should be public");

		if (!symbol->IsStatic)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should be static");

		if (symbol->IsAbstract)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point cannot be abstract");

		if (symbol->IsExtern)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point cannot be external");

		if (symbol->IsVirtual)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point cannot be virtual");

		if (symbol->Parameters.size() != 0)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should have empty parameters list");

		if (symbol->ReturnType != SymbolTable::Primitives::Void)
			Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should have 'void' return type");
	}

	PopScope();
}

void ExpressionBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (!symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol->ReturnType == SymbolTable::Primitives::Void)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Property '" + node->IdentifierToken.Word + L"' must have return value");
	}

	if (node->Getter != nullptr && node->Getter->Body != nullptr)
	{
		PushScope(symbol);
		if (!symbol->IsStatic)
			Declare(new VariableSymbol(L"this", ownerType));

		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Getter->Body);

		if (!CurrentScope()->ReturnFound)
			Diagnostics.ReportError(node->Getter->KeywordToken, L"Accessor must return a value of type '" + symbol->ReturnType->Name + L"'");

		PopScope();
	}

	if (node->Setter != nullptr && node->Setter->Body != nullptr)
	{
		PushScope(symbol);
		if (!symbol->IsStatic)
			Declare(new VariableSymbol(L"this", ownerType));

		CurrentScope()->ReturnsAnything = false;
		Declare(new VariableSymbol(L"value", symbol->ReturnType));
		VisitStatementsBlock(node->Setter->Body);

		if (CurrentScope()->ReturnsAnything)
			Diagnostics.ReportError(node->Setter->KeywordToken, L"Setter method of '" + node->IdentifierToken.Word + L"' should not return any values");

		PopScope();
	}
}

void ExpressionBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node->InitializerExpression != nullptr)
	{
		VisitExpression(node->InitializerExpression);
	}

	FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (node->InitializerExpression != nullptr)
	{
		TypeSymbol* initExprType = GetExpressionType(node->InitializerExpression);
		if (initExprType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer expression type could not be determined");
			return;
		}

		if (!TypeSymbol::Equals(initExprType, symbol->ReturnType))
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer type mismatch: expected '" + symbol->ReturnType->Name + L"' but got '" + initExprType->Name + L"'");
			return;
		}
	}
}

void ExpressionBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node);
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
		VisitExpression(node->Expression);
		PopScope();
	}

	TypeSymbol* expressionType = GetExpressionType(node->Expression);
	if (expressionType == nullptr)
	{
		return;
	}

	if (symbol->Type == SymbolTable::Primitives::Any)
	{
		symbol->Type = expressionType;
		return;
	}

	if (!TypeSymbol::Equals(symbol->Type, expressionType))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Type mismatch: expected '" + symbol->Type->Name + L"' but got '" + expressionType->Name + L"'");
		return;
	}
}

TypeSymbol* ExpressionBinder::AnalyzeLiteralExpression(LiteralExpressionSyntax* node)
{
	switch (node->LiteralToken.Type)
	{
		case TokenType::NullLiteral:
		{
			node->Type = LiteralExpressionSyntax::AsNull;
			return SymbolTable::Primitives::Null;
		}

		case TokenType::BooleanLiteral:
		{
			node->Type = LiteralExpressionSyntax::AsBoolean;
			node->AsBooleanValue = node->LiteralToken.Word == L"true";
			return SymbolTable::Primitives::Boolean;
		}

		case TokenType::StringLiteral:
		{
			// TODO: add interpolation
			node->Type = LiteralExpressionSyntax::AsString;
			node->AsStringValue = new std::wstring(node->LiteralToken.Word);
			return SymbolTable::Primitives::String;
		}

		case TokenType::DoubleLiteral:
		case TokenType::NumberLiteral:
		{
			return AnalyzeNumberLiteral(node);
		}

		case TokenType::CharLiteral:
		{
			if (node->LiteralToken.Word.size() > 1)
				Diagnostics.ReportError(node->LiteralToken, L"invalid Char literal length");

			node->AsCharValue = node->LiteralToken.Word[0];
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
	VisitExpression(node->Left);
	TypeSymbol* leftType = GetExpressionType(node->Left);
	
	if (leftType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Left operand type could not be determined");
		return nullptr;
	}

	if (node->OperatorToken.Type == TokenType::AssignOperator)
	{
		PushScope(new LeftDenotationSymbol(leftType));
		VisitExpression(node->Right);
		PopScope();

		TypeSymbol* rightType = GetExpressionType(node->Right);
		if (rightType == nullptr)
		{
			Diagnostics.ReportError(node->OperatorToken, L"Right operand type could not be determined");
			return leftType;
		}

		if (!TypeSymbol::Equals(leftType, rightType))
		{
			Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
			return leftType;
		}

		return leftType;
	}

	VisitExpression(node->Right);
	TypeSymbol* rightType = GetExpressionType(node->Right);

	if (rightType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Right operand type could not be determined");
		return leftType;
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
			if (!TypeSymbol::Equals(leftType, rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
			}
			
			return SymbolTable::Primitives::Boolean;
		}
			
		case TokenType::AddOperator:
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
	TypeSymbol* exprType = GetExpressionType(node->Expression);
	
	if (exprType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Operand type could not be determined");
		return nullptr;
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
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		{
			if (exprType != SymbolTable::Primitives::Integer)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Increment/Decrement operator requires integer type, got '" + exprType->Name + L"'");
				return nullptr;
			}
			
			return SymbolTable::Primitives::Integer;
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
	VisitExpression(node->Expression);

	TypeSymbol* type = AnalyzeUnaryExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeObjectExpression(ObjectExpressionSyntax* node)
{
	if (node->TypeSymbol == nullptr)
		return nullptr;

	std::wstring methodName = node->IdentifierToken.Word;
	ConstructorSymbol* method = ResolveConstructor(node);
	if (method == nullptr)
		return nullptr;

	if (!IsSymbolAccessible(method))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return nullptr;
	}

	// Передаем genericType для замены type parameters в параметрах конструктора
	GenericTypeSymbol* genericType = nullptr;
	if (node->TypeSymbol->Kind == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(node->TypeSymbol);

	if (!MatchMethodArguments(method, node->ArgumentsList->Arguments, genericType))
	{
		//Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' argument types do not match");
		return nullptr;
	}

	node->CtorSymbol = method;
	return node->TypeSymbol;
}

void ExpressionBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	VisitType(node->Type);
	VisitArgumentsList(node->ArgumentsList);

	TypeSymbol* type = AnalyzeObjectExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeCollectionExpression(CollectionExpressionSyntax* node)
{
	TypeSymbol* collectionType = nullptr;
	for (size_t i = 0; i < node->ValuesExpressions.size() && collectionType == nullptr; i++)
		collectionType = GetExpressionType(node->ValuesExpressions.at(i));

	for (ExpressionSyntax* expression : node->ValuesExpressions)
	{
		TypeSymbol* exprType = GetExpressionType(expression);
		if (exprType != collectionType)
		{
			Diagnostics.ReportError(node->OpenSquareToken, L"Element have type different from collection's target type");
		}
	}

	return collectionType;
}

void ExpressionBinder::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (ExpressionSyntax* expression : node->ValuesExpressions)
		VisitExpression(expression);

	TypeSymbol* type = AnalyzeCollectionExpression(node);
	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(type);
	arrayType->Size = node->ValuesExpressions.size();
	node->Symbol = arrayType;
	Table->BindSymbol(node, arrayType);
	SetExpressionType(node, arrayType);
}

void ExpressionBinder::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	MethodSymbol* anonymousMethod = new MethodSymbol(L"Lambda", node->Body);
	anonymousMethod->HandleType = MethodHandleType::Lambda;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = SymbolTable::Primitives::Any;
	anonymousMethod->IsStatic = true;

	DelegateTypeSymbol* delegate = new DelegateTypeSymbol(L"Lambda");
	delegate->Accesibility = SymbolAccesibility::Public;
	delegate->AnonymousSymbol = anonymousMethod;
	node->Symbol = delegate;

	/*
	TypeSymbol* targetReturnType = ResolveLeftDenotation();
	if (targetReturnType == nullptr)
	{
		Diagnostics.ReportError(node->LambdaToken, L"Cannot resolve left denotation");
		return;
	}
	*/

	VisitParametersList(node->Params);
	for (ParameterSyntax* parameter : node->Params->Parameters)
	{
		ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
		paramSymbol->Type = parameter->Type->Symbol;

		delegate->Parameters.push_back(paramSymbol);
		anonymousMethod->Parameters.push_back(paramSymbol);
	}

	PushScope(anonymousMethod);
	for (ParameterSymbol* parameter : anonymousMethod->Parameters)
		Declare(parameter);

	CurrentScope()->ReturnFound = false;
	VisitStatementsBlock(node->Body);
	PopScope();

	delegate->ReturnType = anonymousMethod->ReturnType;
	SetExpressionType(node, delegate);
}

void ExpressionBinder::VisitTernaryExpression(TernaryExpressionSyntax* node)
{
	VisitExpression(node->Condition);
	VisitExpression(node->Left);
	VisitExpression(node->Right);

	TypeSymbol* conditionType = GetExpressionType(node->Condition);
	if (conditionType == nullptr)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition return type could not be determined");
	}
	else if (conditionType != SymbolTable::Primitives::Boolean)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition expected to return 'bool', but got '" + conditionType->Name + L"'");
	}

	TypeSymbol* leftType = GetExpressionType(node->Left);
	TypeSymbol* rightType = GetExpressionType(node->Right);
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

bool ExpressionBinder::MatchMethodArguments(MethodSymbol* method, std::vector<ArgumentSyntax*> arguments, GenericTypeSymbol* genericType)
{
	if (method == nullptr)
		return false;
		
	if (method->Parameters.size() != arguments.size())
	{
		Diagnostics.ReportError(SyntaxToken(), L"Method expects " + std::to_wstring(method->Parameters.size()) + L" arguments but got " + std::to_wstring(arguments.size()));
		return false;
	}
	
	for (size_t i = 0; i < method->Parameters.size(); i++)
	{
		ParameterSymbol* param = method->Parameters[i];
		ArgumentSyntax* arg = arguments[i];

		if (param == nullptr || arg == nullptr || arg->Expression == nullptr)
		{
			Diagnostics.ReportError(SyntaxToken(), L"Invalid argument at position " + std::to_wstring(i));
			return false;
		}
		
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		/*
		PushScope(new LeftDenotationSymbol(param->Type));
		VisitExpression(expr);
		PopScope();
		*/
		
		TypeSymbol* argType = GetExpressionType(expr);
		if (argType == nullptr)
		{
			//Diagnostics.ReportError(SyntaxToken(), L"Argument type could not be determined for parameter '" + param->Name + L"'");
			return false;
		}

		TypeSymbol* paramType = param->Type;
		if (paramType->Kind == SyntaxKind::TypeParameter)
		{
			paramType = SubstituteTypeParameters(paramType, genericType);
		}

		if (paramType == SymbolTable::Primitives::Any)
		{
			return true;
		}

		if (!TypeSymbol::Equals(paramType, argType))
		{
			Diagnostics.ReportError(SyntaxToken(), L"Argument type mismatch for parameter '" + param->Name + L"': expected '" + paramType->Name + L"' but got '" + argType->Name + L"'");
			return false;
		}
	}
	
	return true;
}

TypeSymbol* ExpressionBinder::AnalyzeMemberAccessExpression(MemberAccessExpressionSyntax* node, TypeSymbol* currentType)
{
	if (currentType == nullptr)
		return nullptr;

	std::wstring memberName = node->IdentifierToken.Word;
	SyntaxSymbol* symbol = nullptr;

	if (node->PreviousExpression == nullptr)
	{
		// Check if this is the 'field' keyword - resolve to backing field of current property
		if (node->IdentifierToken.Type == TokenType::FieldKeyword)
			return AnalyzeFieldKeywordExpression(node, nullptr);

		std::wstring name = node->IdentifierToken.Word;
		symbol = CurrentScope()->Lookup(name);

		if (symbol == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + name + L"' not found in current scope");
			return nullptr;
		}

		if (!IsSymbolAccessible(symbol))
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + name + L"' is not accessible");
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
		ExpressionSyntax* previousExpression = const_cast<ExpressionSyntax*>(node->PreviousExpression);
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
				symbol = *std::find_if(currentType->Methods.begin(), currentType->Methods.end(), [memberName](const MethodSymbol* method) { return method->Name == memberName; });
				if (symbol == nullptr)
				{
					Diagnostics.ReportError(node->IdentifierToken, L"Member '" + memberName + L"' not found in type '" + currentType->Name + L"'");
					return nullptr;
				}
			}
		}
	}

	if (!IsSymbolAccessible(node->FieldSymbol))
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
			node->VariableSymbol = varSymbol;
			return const_cast<TypeSymbol*>(varSymbol->Type);
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* propertySymbol = static_cast<PropertySymbol*>(symbol);
			node->IsStaticContext = false;
			node->PropertySymbol = propertySymbol;
			return AnalyzePropertyAccessExpression(node, propertySymbol, currentType);
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* fieldSymbol = node->FieldSymbol = static_cast<FieldSymbol*>(symbol);
			bool isStaticContext = GetIsStaticContext(node->PreviousExpression);

			if (isStaticContext && !fieldSymbol->IsStatic)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance field '" + fieldSymbol->FullName + L"' from type context");
				return nullptr;
			}

			if (!isStaticContext && fieldSymbol->IsStatic)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access static field '" + fieldSymbol->FullName + L"' from instance reference");
				return nullptr;
			}

			node->IsStaticContext = false;
			TypeSymbol* fieldType = fieldSymbol->ReturnType;
			
			if (fieldType->Kind == SyntaxKind::TypeParameter)
			{
				if (currentType->Kind == SyntaxKind::GenericType)
				{
					GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(currentType);
					fieldType = SubstituteTypeParameters(fieldType, genericType);
				}
			}
			
			return fieldType;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodSymbol* methodSymbol = static_cast<MethodSymbol*>(symbol);
			DelegateTypeSymbol* delegate = SymbolFactory::Delegate(methodSymbol);
			node->DelegateSymbol = delegate;
			node->IsStaticContext = false;
			return delegate;
		}

		case SyntaxKind::Parameter:
		{
			ParameterSymbol* paramSymbol = static_cast<ParameterSymbol*>(symbol);
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

	if (!IsSymbolAccessible(accessor))
	{
		Diagnostics.ReportError(node->IdentifierToken, (requiresSetter ? L"Setter" : L"Getter") + (L" of property '" + memberName + L"' is not accessible"));
		return nullptr;
	}

	bool isStaticCtx = GetIsStaticContext(node->PreviousExpression);
	if (isStaticCtx && !accessor->IsStatic)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance property '" + memberName + L"' from type context");
		return nullptr;
	}

	if (!isStaticCtx && accessor->IsStatic)
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
		if (symbol->IsType())
			break;

		if (scope->Owner->Kind != SyntaxKind::PropertyDeclaration)
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
		propertySymbol->GenerateBackingField();

	// Resolve 'field' as the backing field
	node->FieldSymbol = propertySymbol->BackingField;
	node->IsStaticContext = propertySymbol->IsStatic;

	if (propertySymbol->BackingField->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Backing field type not resolved for property '" + propertySymbol->Name + L"'");
		return nullptr;
	}

	return propertySymbol->BackingField->ReturnType;
}

TypeSymbol* ExpressionBinder::AnalyzeInvokationExpression(InvokationExpressionSyntax* node, TypeSymbol* currentType)
{
	if (currentType == nullptr)
		return nullptr;

	std::wstring methodName = node->IdentifierToken.Word;
	MethodSymbol* method = ResolveMethod(node, currentType);

	if (method == nullptr)
		return nullptr;

	if (!IsSymbolAccessible(method))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return nullptr;
	}

	// Передаем genericType для замены type parameters в параметрах метода
	GenericTypeSymbol* genericType = nullptr;
	if (currentType->Kind == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(currentType);

	if (!MatchMethodArguments(method, node->ArgumentsList->Arguments, genericType))
	{
		//Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' argument types do not match");
		return nullptr;
	}

	node->Symbol = method;
	node->IsStaticContext = false;

	if (method->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' return type not resolved");
		return nullptr;
	}

	TypeSymbol* returnType = method->ReturnType;
	
	// Если currentType является GenericTypeSymbol, заменяем type parameters на type arguments
	if (returnType->Kind == SyntaxKind::TypeParameter)
	{
		returnType = SubstituteTypeParameters(returnType, genericType);
	}
	
	return returnType;
}

ConstructorSymbol* ExpressionBinder::ResolveConstructor(ObjectExpressionSyntax* node)
{
	TypeSymbol* symbol = node->TypeSymbol;
	if (symbol == nullptr)
		return nullptr;

	if (symbol->Kind == SyntaxKind::GenericType)
		symbol = static_cast<GenericTypeSymbol*>(symbol)->UnderlayingType;

	std::vector<TypeSymbol*> argTypes;
	for (ArgumentSyntax* arg : node->ArgumentsList->Arguments)
	{
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		VisitExpression(expr);

		TypeSymbol* argType = GetExpressionType(expr);
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

MethodSymbol* ExpressionBinder::ResolveMethod(InvokationExpressionSyntax* node, TypeSymbol* currentType)
{
	std::wstring methodName = node->IdentifierToken.Word;
	std::vector<TypeSymbol*> argTypes;

	for (ArgumentSyntax* arg : node->ArgumentsList->Arguments)
	{
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		//PushScope(new LeftDenotationSymbol(param->Type));
		VisitExpression(expr);
		//PopScope();

		TypeSymbol* argType = GetExpressionType(expr);
		if (argType == nullptr)
			return nullptr;

		argTypes.push_back(argType);
	}

	MethodSymbol* method = currentType->FindMethod(methodName, argTypes);
	if (method == nullptr)
		method = SymbolTable::Global::Type->FindMethod(methodName, argTypes);

	if (method == nullptr)
	{
		SyntaxSymbol* delegateFindVar = CurrentScope()->Lookup(methodName);
		if (delegateFindVar != nullptr)
		{
			if (delegateFindVar->Kind == SyntaxKind::VariableStatement)
			{
				VariableSymbol* delegateVar = static_cast<VariableSymbol*>(delegateFindVar);
				if (delegateVar->Type != nullptr)
				{
					if (delegateVar->Type->Kind == SyntaxKind::DelegateType)
					{
						const DelegateTypeSymbol* delegate = static_cast<const DelegateTypeSymbol*>(delegateVar->Type);
						method = delegate->AnonymousSymbol;
					}
				}
			}
		}
	}

	if (method == nullptr)
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
	
	bool isStaticContext = GetIsStaticContext(node->PreviousExpression);
	if (isStaticContext && !method->IsStatic)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot call instance method '" + methodName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && method->IsStatic)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot call static method '" + methodName + L"' on instance reference");
		return nullptr;
	}

	return method;
}

TypeSymbol* ExpressionBinder::AnalyzeIndexatorExpression(IndexatorExpressionSyntax* node, TypeSymbol* currentType)
{
	if (currentType == nullptr)
		return nullptr;

	if (node->PreviousExpression == nullptr)
		return nullptr;

	MemberAccessExpressionSyntax* access = static_cast<MemberAccessExpressionSyntax*>(const_cast<ExpressionSyntax*>(node->PreviousExpression));
	VisitExpression(access);
	currentType = GetExpressionType(access);

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

		if (!MatchMethodArguments(accessor, node->IndexatorList->Arguments, genericType))
		{
			//Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Indexator arguments types do not match");
			return nullptr;
		}
	}

	node->IndexatorSymbol = indexator;
	node->IsStaticContext = node->IsStaticContext;
	return resultType;
}

TypeSymbol* ExpressionBinder::SubstituteTypeParameters(TypeSymbol* type, GenericTypeSymbol* genericType)
{
	if (type == nullptr || genericType == nullptr)
		return type;

	type = genericType->SubstituteTypeParameters(type);
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

static bool IsVolumeRatioPostfix(std::wstring& postfix, long& multiplier)
{
	if (postfix[1] != L'B' && postfix[1] != L'b')
		return false;

	switch (postfix[0])
	{
		default:
			return false;

		case L'k': case L'K': multiplier = (1LL << 10); return true;
		case L'm': case L'M': multiplier = (1LL << 20); return true;
		case L'g': case L'G': multiplier = (1LL << 30); return true;
		case L't': case L'T': multiplier = (1LL << 40); return true;
		case L'p': case L'P': multiplier = (1LL << 50); return true;
	}
}

static bool IsValidIntegerPunctuation(wchar_t symbol)
{
	switch (symbol)
	{
		default:
			return false;

		case '.':
		case 'e':
		case 'E':
		case '+':
		case '-':
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
	node->Type = LiteralExpressionSyntax::AsInteger;
	SyntaxToken token = node->LiteralToken;
	std::wstring word = token.Word;
	size_t size = word.size();

	size_t delimeterIndex = word.find('.');
	if (delimeterIndex != std::string::npos)
	{
		node->Type = LiteralExpressionSyntax::AsDouble;
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
			long dummy2 = 0;

			if (IsVolumeRatioPostfix(postfix, dummy2))
			{
				Diagnostics.ReportError(token, L"Floating point number cannot have suffix");
				return SymbolTable::Primitives::Double;
			}
		}

		try
		{
			size_t pos = 0;
			word[delimeterIndex] = L',';
			node->AsDoubleValue = std::stod(word, &pos);

			if (pos != size)
			{
				for (size_t i = pos; i < size; i++)
				{
					if (!std::isdigit(word[i]) && !IsValidIntegerPunctuation(word[i]))
					{
						Diagnostics.ReportError(token, L"Invalid characters in floating point number");
						return SymbolTable::Primitives::Double;
					}
				}
			}

			return SymbolTable::Primitives::Double;
		}
		catch (const std::exception&)
		{
			Diagnostics.ReportError(token, L"Invalid floating point number format");
			return SymbolTable::Primitives::Double;
		}
	}

	size_t numStart = 0;
	long multiplier = 1;
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
		size_t pos = 0;
		std::wstring numPart = word.substr(numStart, size);
		node->AsIntegerValue = std::stol(numPart, &pos) * multiplier;

		if (pos != size)
		{
			for (size_t i = pos; i < size; i++)
			{
				if (!IsValidIntegerSymbol(word[i], base))
				{
					Diagnostics.ReportError(token, L"Invalid characters in number");
					return SymbolTable::Primitives::Integer;
				}
			}
		}

		// overflow check
		if (node->AsIntegerValue < 0 && multiplier > 1)
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

IndexatorSymbol* ExpressionBinder::ResolveIndexator(IndexatorExpressionSyntax* node, TypeSymbol* currentType)
{
	MemberAccessExpressionSyntax* access = static_cast<MemberAccessExpressionSyntax*>(const_cast<ExpressionSyntax*>(node->PreviousExpression));
	std::wstring methodName = access->IdentifierToken.Word;
	std::vector<TypeSymbol*> argTypes;

	for (ArgumentSyntax* arg : node->IndexatorList->Arguments)
	{
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		VisitExpression(expr);

		TypeSymbol* argType = GetExpressionType(expr);
		if (argType == nullptr)
			return nullptr;

		argTypes.push_back(argType);
	}

	if (currentType->Kind == SyntaxKind::ArrayType)
	{
		if (SymbolTable::Primitives::Array->Indexators.empty())
			return nullptr;

		return SymbolTable::Primitives::Array->Indexators[0];
	}

	IndexatorSymbol* indexator = currentType->FindIndexator(argTypes);
	if (indexator == nullptr)
		indexator = SymbolTable::Global::Type->FindIndexator(argTypes);

	if (indexator == nullptr)
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

	bool isStaticContext = GetIsStaticContext(node->PreviousExpression);
	if (isStaticContext && !indexator->IsStatic)
	{
		Diagnostics.ReportError(access->IdentifierToken, L"Cannot call instance indexator '" + methodName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && indexator->IsStatic)
	{
		Diagnostics.ReportError(access->IdentifierToken, L"Cannot call static indexator '" + methodName + L"' on instance reference");
		return nullptr;
	}

	return indexator;
}

void ExpressionBinder::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
{
	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression);
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType() : GetExpressionType(previous);
	type = AnalyzeMemberAccessExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression);
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType() : GetExpressionType(previous);
	type = AnalyzeInvokationExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	if (node->IndexatorList != nullptr)
		VisitIndexatorList(node->IndexatorList);

	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression);
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType() : GetExpressionType(previous);
	type = AnalyzeIndexatorExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitWhileStatement(WhileStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression);
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression);
		
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
		VisitStatementsBlock(node->StatementsBlock);
}

void ExpressionBinder::VisitUntilStatement(UntilStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression);
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression);
		
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
		VisitStatementsBlock(node->StatementsBlock);
}

void ExpressionBinder::VisitForStatement(ForStatementSyntax* node)
{
	if (node->InitializerStatement != nullptr)
		VisitStatement(node->InitializerStatement);
	
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression);
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression);
		
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
		VisitStatement(node->AfterRepeatStatement);
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
}

void ExpressionBinder::VisitIfStatement(IfStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		ExpressionSyntax* conditionExpr = dynamic_cast<ExpressionStatementSyntax*>(node->ConditionExpression)->Expression;
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
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

void ExpressionBinder::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		ExpressionSyntax* conditionExpr = dynamic_cast<ExpressionSyntax*>(node->ConditionExpression);
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
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

static bool SymbolHasReturnType(const SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::FieldDeclaration:
		case SyntaxKind::PropertyDeclaration:
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
		{
			MethodSymbol* methodSymbol = static_cast<MethodSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* methodSymbol = static_cast<FieldSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

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
		MethodSymbol* delegate = const_cast<MethodSymbol*>(static_cast<const MethodSymbol*>(searchingScope->Owner));
		if (node->Expression == nullptr)
		{
			delegate->ReturnType = SymbolTable::Primitives::Void;
			return;
		}
		else
		{
			VisitExpression(node->Expression);
			delegate->ReturnType = GetExpressionType(node->Expression);
			return;
		}
	}

	if (returnType == SymbolTable::Primitives::Void)
	{
		if (node->Expression != nullptr)
		{
			searchingScope->ReturnsAnything = true;
			VisitExpression(node->Expression);

			Diagnostics.ReportError(node->Expression->Kind == SyntaxKind::LiteralExpression
				? static_cast<LiteralExpressionSyntax*>(node->Expression)->LiteralToken
				: SyntaxToken(), L"Void method cannot return a value");
		}

		return;
	}

	if (node->Expression == nullptr)
	{
		Diagnostics.ReportError(SyntaxToken(), L"Return statement must return a value of type '" + returnType->Name + L"'");
		return;
	}

	searchingScope->ReturnsAnything = true;
	VisitExpression(node->Expression);
	TypeSymbol* returnExprType = GetExpressionType(node->Expression);

	if (returnExprType == nullptr)
	{
		Diagnostics.ReportError(node->Expression->Kind == SyntaxKind::LiteralExpression
			? static_cast<LiteralExpressionSyntax*>(node->Expression)->LiteralToken
			: SyntaxToken(), L"Return expression type could not be determined");
	}
	else if (!TypeSymbol::Equals(returnExprType, returnType))
	{
		Diagnostics.ReportError(node->Expression->Kind == SyntaxKind::LiteralExpression
			? static_cast<LiteralExpressionSyntax*>(node->Expression)->LiteralToken
			: SyntaxToken(), L"Return type mismatch: expected '" + returnType->Name + L"' but got '" + returnExprType->Name + L"'");
	}
}
