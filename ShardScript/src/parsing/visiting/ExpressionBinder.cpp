#include <shard/parsing/visiting/ExpressionBinder.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticScope.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SymbolAccesibility.h>

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

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>

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

using namespace std;
using namespace shard::parsing;
using namespace shard::parsing::semantic;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;
using namespace shard::syntax;

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

void ExpressionBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	NamespaceSymbol* symbol = static_cast<NamespaceSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		PushScope(symbol);
		for (TypeSymbol* type : Table->GetTypeSymbols())
		{
			if (type->Parent == symbol)
				Declare(type);
		}

		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void ExpressionBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = static_cast<ClassSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		PushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void ExpressionBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = static_cast<StructSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		PushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void ExpressionBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = static_cast<MethodSymbol*>(Table->LookupSymbol(node));
	TypeSymbol* ownerType = OwnerType();

	if (symbol != nullptr && node->Body != nullptr)
	{
		PushScope(symbol);
		if (!symbol->IsStatic)
			Declare(new VariableSymbol(L"this", ownerType));

		for (ParameterSymbol* parameter : symbol->Parameters)
			Declare(new VariableSymbol(parameter->Name, parameter->Type));

		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body);

		if (symbol->ReturnType->Name != L"Void")
		{
			if (!CurrentScope()->ReturnFound)
				Diagnostics.ReportError(node->IdentifierToken, L"Method must return a value of type '" + symbol->ReturnType->Name + L"'");
		}

		if (symbol->Name == L"Main")
		{
			Table->EntryPointCandidates.push_back(symbol);
			if (symbol->Accesibility != SymbolAccesibility::Public)
				Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should be public");

			if (!symbol->IsStatic)
				Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should be static");

			if (symbol->Parameters.size() != 0)
				Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should have empty parameters list");

			if (symbol->ReturnType != SymbolTable::Primitives::Void)
				Diagnostics.ReportError(node->IdentifierToken, L"Main entry point should have 'void' return type");
		}
		
		PopScope();
	}
}

void ExpressionBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = static_cast<PropertySymbol*>(Table->LookupSymbol(node));
	TypeSymbol* ownerType = OwnerType();

	if (symbol->ReturnType->Name == L"Void")
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Property '" + node->IdentifierToken.Word + L"' must have return value");
	}

	if (node->GetBody != nullptr)
	{
		PushScope(symbol);
		if (!symbol->IsStatic)
			Declare(new VariableSymbol(L"this", ownerType));

		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->GetBody);

		if (!CurrentScope()->ReturnFound)
			Diagnostics.ReportError(node->IdentifierToken, L"Method must return a value of type '" + symbol->ReturnType->Name + L"'");

		PopScope();
	}

	if (node->SetBody != nullptr)
	{
		PushScope(symbol);
		if (!symbol->IsStatic)
			Declare(new VariableSymbol(L"this", ownerType));

		CurrentScope()->ReturnsAnything = false;
		Declare(new VariableSymbol(L"value", symbol->ReturnType));
		VisitStatementsBlock(node->SetBody);

		if (CurrentScope()->ReturnsAnything)
			Diagnostics.ReportError(node->SetKeywordToken, L"Setter method of '" + node->IdentifierToken.Word + L"' should not return any values");

		PopScope();
	}
}

void ExpressionBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node->InitializerExpression != nullptr)
	{
		VisitExpression(node->InitializerExpression);
	}

	FieldSymbol* fieldSymbol = static_cast<FieldSymbol*>(Table->LookupSymbol(node));
	if (fieldSymbol->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Field type not resolved");
		return;
	}

	if (fieldSymbol != nullptr && fieldSymbol->ReturnType != nullptr)
	{
		TypeSymbol* initExprType = GetExpressionType(node->InitializerExpression);
		if (initExprType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer expression type could not be determined");
			return;
		}

		if (initExprType != fieldSymbol->ReturnType)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer type mismatch: expected '" + fieldSymbol->ReturnType->Name + L"' but got '" + initExprType->Name + L"'");
			return;
		}
	}
}

void ExpressionBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression);

	VariableSymbol* varSymbol = static_cast<VariableSymbol*>(Table->LookupSymbol(node));
	Declare(varSymbol);

	if (varSymbol->Type == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Variable type not resolved");
		return;
	}

	TypeSymbol* expressionType = GetExpressionType(node->Expression);
	if (expressionType == nullptr)
	{
		//Diagnostics.ReportError(node->IdentifierToken, L"Expression type could not be determined");
		return;
	}

	if (varSymbol->Type->Kind == SyntaxKind::CollectionExpression && expressionType->Kind == SyntaxKind::CollectionExpression)
		return;

	if (varSymbol->Type != expressionType)
		Diagnostics.ReportError(node->IdentifierToken, L"Type mismatch: expected '" + varSymbol->Type->Name + L"' but got '" + expressionType->Name + L"'");
}

TypeSymbol* ExpressionBinder::AnalyzeLiteralExpression(LiteralExpressionSyntax* node)
{
	switch (node->LiteralToken.Type)
	{
		case TokenType::BooleanLiteral:
			return SymbolTable::Primitives::Boolean;

		case TokenType::NumberLiteral:
			return SymbolTable::Primitives::Integer;

		case TokenType::CharLiteral:
			return SymbolTable::Primitives::Char;

		case TokenType::StringLiteral:
			return SymbolTable::Primitives::String;

		case TokenType::NullLiteral:
			return nullptr;

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
		wstring tokenType = L"unknown";
		Diagnostics.ReportError(node->LiteralToken, L"Unsupported literal type: " + tokenType);
	}
}

TypeSymbol* ExpressionBinder::AnalyzeBinaryExpression(BinaryExpressionSyntax* node)
{
	TypeSymbol* leftType = GetExpressionType(node->Left);
	TypeSymbol* rightType = GetExpressionType(node->Right);
	
	if (leftType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Left operand type could not be determined");
		return nullptr;
	}
	
	if (rightType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Right operand type could not be determined");
		return nullptr;
	}
	
	switch (node->OperatorToken.Type)
	{
		case TokenType::AssignOperator:
			return leftType;
			
		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		{
			if (leftType != rightType)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
			}
			
			return SymbolTable::Primitives::Boolean;
		}
			
		case TokenType::AddOperator:
		case TokenType::SubOperator:
		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
		case TokenType::PowOperator:
		{
			if (leftType != rightType)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			return leftType;
		}
			
		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
		{
			if (leftType != rightType)
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
	VisitExpression(node->Left);
	VisitExpression(node->Right);

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
	if (node->Symbol == nullptr)
	{
		Diagnostics.ReportError(node->NewToken, L"Object creation type not resolved");
		return nullptr;
	}

	return node->Symbol;
}

void ExpressionBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	VisitArgumentsList(node->Arguments);

	TypeSymbol* type = AnalyzeObjectExpression(node);
	SetExpressionType(node, type);
	VisitType(node->Type);
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
			Diagnostics.ReportError(node->OpenSquareToken, L"Element have type different from collection's");
		}
	}

	return collectionType;
}

void ExpressionBinder::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (ExpressionSyntax* expression : node->ValuesExpressions)
		VisitExpression(expression);

	TypeSymbol* type = AnalyzeCollectionExpression(node);
	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(type, node->ValuesExpressions.size());
	node->Symbol = arrayType;
	Table->BindSymbol(node, arrayType);
	SetExpressionType(node, arrayType);
}

bool ExpressionBinder::MatchMethodArguments(MethodSymbol* method, vector<ArgumentSyntax*> arguments)
{
	if (method == nullptr)
		return false;
		
	if (method->Parameters.size() != arguments.size())
	{
		Diagnostics.ReportError(SyntaxToken(), L"Method expects " + to_wstring(method->Parameters.size()) + L" arguments but got " + to_wstring(arguments.size()));
		return false;
	}
	
	for (size_t i = 0; i < method->Parameters.size(); i++)
	{
		ParameterSymbol* param = method->Parameters[i];
		ArgumentSyntax* arg = arguments[i];
		
		if (param == nullptr || arg == nullptr || arg->Expression == nullptr)
		{
			Diagnostics.ReportError(SyntaxToken(), L"Invalid argument at position " + to_wstring(i));
			return false;
		}
		
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		VisitExpression(expr);
		TypeSymbol* argType = GetExpressionType(expr);
		
		if (param->Type == nullptr)
		{
			Diagnostics.ReportError(SyntaxToken(), L"Parameter '" + param->Name + L"' type not resolved");
			return false;
		}
		
		if (argType == nullptr)
		{
			Diagnostics.ReportError(SyntaxToken(), L"Argument type could not be determined for parameter '" + param->Name + L"'");
			return false;
		}
		
		if (param->Type != argType)
		{
			Diagnostics.ReportError(SyntaxToken(), L"Argument type mismatch for parameter '" + param->Name + L"': expected '" + param->Type->Name + L"' but got '" + argType->Name + L"'");
			return false;
		}
	}
	
	return true;
}

TypeSymbol* ExpressionBinder::AnalyzeLinkedExpression(LinkedExpressionSyntax* syntax)
{
	if (syntax->Nodes.empty())
	{
		Diagnostics.ReportError(SyntaxToken(), L"Empty linked expression");
		return nullptr;
	}
	
	TypeSymbol* currentType = OwnerType();
    bool isStaticContext = true;

	for (LinkedExpressionNode* node : syntax->Nodes)
	{
		if (currentType == nullptr)
		{
			//Diagnostics.ReportError(node->IdentifierToken, L"Cannot access member: previous expression has no type");
			return nullptr;
		}

		switch (node->Kind)
		{
			case SyntaxKind::MemberAccessExpression:
			{
				MemberAccessExpressionSyntax* memberAccess = static_cast<MemberAccessExpressionSyntax*>(node);
				currentType = AnalyzeMemberAccessExpression(memberAccess, isStaticContext, currentType);
				break;
			}

			case SyntaxKind::InvokationExpression:
			{
				InvokationExpressionSyntax* invocation = static_cast<InvokationExpressionSyntax*>(node);
				currentType = AnalyzeInvokationExpression(invocation, isStaticContext, currentType);
				break;
			}

			case SyntaxKind::IndexatorExpression:
			{
				IndexatorExpressionSyntax* indexing = static_cast<IndexatorExpressionSyntax*>(node);
				currentType = AnalyzeIndexatorExpression(indexing, isStaticContext, currentType);
				break;
			}

			default:
			{
				Diagnostics.ReportError(SyntaxToken(), L"Unknown linked expression node type");
				return nullptr;
			}
		}
	}

	return currentType;
}

void ExpressionBinder::VisitLinkedExpression(LinkedExpressionSyntax* node)
{
	for (LinkedExpressionNode* exprNode : node->Nodes)
		VisitLinkedExpressionNode(exprNode);

	TypeSymbol* type = AnalyzeLinkedExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeMemberAccessExpression(MemberAccessExpressionSyntax* node, bool& isStaticContext, TypeSymbol* currentType)
{
	if (node->PrevNode == nullptr)
	{
		// Check if this is the 'field' keyword - resolve to backing field of current property
		if (node->IdentifierToken.Type == TokenType::FieldKeyword)
			return AnalyzeFieldKeywordExpression(node, isStaticContext, nullptr);

		wstring name = node->IdentifierToken.Word;
		SyntaxSymbol* symbol = CurrentScope()->Lookup(name);

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

		switch (symbol->Kind)
		{
			case SyntaxKind::VariableStatement:
			{
				VariableSymbol* varSymbol = static_cast<VariableSymbol*>(symbol);
				isStaticContext = false;
				return const_cast<TypeSymbol*>(varSymbol->Type);
			}

			case SyntaxKind::FieldDeclaration:
			{
				FieldSymbol* fieldSymbol = static_cast<FieldSymbol*>(symbol);

				if (!IsSymbolAccessible(fieldSymbol))
				{
					Diagnostics.ReportError(node->IdentifierToken, L"Field '" + name + L"' is not accessible");
					return nullptr;
				}

				isStaticContext = false;
				return fieldSymbol->ReturnType;
			}

			case SyntaxKind::Parameter:
			{
				ParameterSymbol* paramSymbol = static_cast<ParameterSymbol*>(symbol);
				isStaticContext = false;
				return paramSymbol->Type;
			}

			default:
			{
				if (symbol->IsType())
				{
					isStaticContext = true;
					return static_cast<TypeSymbol*>(symbol);
				}

				Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + name + L"' is not a variable, parameter or field (found " + to_wstring(static_cast<int>(symbol->Kind)) + L")");
				return nullptr;
			}
		}
	}
	else
	{
		if (!currentType->IsType())
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Cannot access member on non-type '" + currentType->Name + L"'");
			return nullptr;
		}

		// First check for property (properties take precedence over fields)
		wstring memberName = node->IdentifierToken.Word;
		PropertySymbol* property = currentType->FindProperty(memberName);

		if (property != nullptr)
		{
			if (!IsSymbolAccessible(property))
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' is not accessible");
				return nullptr;
			}

			if (isStaticContext && !property->IsStatic)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance property '" + memberName + L"' from type context");
				return nullptr;
			}

			if (!isStaticContext && property->IsStatic)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access static property '" + memberName + L"' from instance reference");
				return nullptr;
			}

			if (property->ReturnType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' type not resolved");
				return nullptr;
			}

			// Set property flag and symbol
			node->PropertySymbol = property;
			node->IsProperty = true;
			isStaticContext = false;
			return property->ReturnType;
		}
		else
		{
			// Check for field
			FieldSymbol* field = currentType->FindField(memberName);

			if (field == nullptr)
			{
				MethodSymbol* method = currentType->FindMethod(memberName, vector<TypeSymbol*>());
				if (method == nullptr)
				{
					Diagnostics.ReportError(node->IdentifierToken, L"Member '" + memberName + L"' not found in type '" + currentType->Name + L"'");
				}
				else
				{
					Diagnostics.ReportError(node->IdentifierToken, L"Cannot access method '" + memberName + L"' without invocation");
				}

				return nullptr;
			}

			if (!IsSymbolAccessible(field))
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Field '" + memberName + L"' is not accessible");
				return nullptr;
			}

			if (isStaticContext && !field->IsStatic)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance field '" + memberName + L"' from type context");
				return nullptr;
			}

			if (!isStaticContext && field->IsStatic)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access static field '" + memberName + L"' from instance reference");
				return nullptr;
			}

			if (field->ReturnType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Field '" + memberName + L"' type not resolved");
				return nullptr;
			}

			// Set field symbol
			node->FieldSymbol = field;
			isStaticContext = false;
			return field->ReturnType;
		}
	}
}

TypeSymbol* ExpressionBinder::AnalyzeFieldKeywordExpression(MemberAccessExpressionSyntax* node, bool& isStaticContext, TypeSymbol* currentType)
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
	node->IsProperty = false;
	node->PropertySymbol = nullptr;

	isStaticContext = propertySymbol->BackingField->IsStatic;

	if (propertySymbol->BackingField->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Backing field type not resolved for property '" + propertySymbol->Name + L"'");
		return nullptr;
	}

	return propertySymbol->BackingField->ReturnType;
}

TypeSymbol* ExpressionBinder::AnalyzeInvokationExpression(InvokationExpressionSyntax* node, bool& isStaticContext, TypeSymbol* currentType)
{
	wstring methodName = node->IdentifierToken.Word;
	MethodSymbol* method = ResolveMethod(node, isStaticContext, currentType);

	if (method == nullptr)
		return nullptr;

	if (!IsSymbolAccessible(method))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return nullptr;
	}

	if (!MatchMethodArguments(method, node->ArgumentsList->Arguments))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' argument types do not match");
		return nullptr;
	}

	node->Symbol = method;
	isStaticContext = false;

	if (method->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' return type not resolved");
		return nullptr;
	}

	return method->ReturnType;
}

MethodSymbol* ExpressionBinder::ResolveMethod(InvokationExpressionSyntax* node, bool& isStaticContext, TypeSymbol* currentType)
{
	wstring methodName = node->IdentifierToken.Word;
	vector<TypeSymbol*> argTypes;

	for (ArgumentSyntax* arg : node->ArgumentsList->Arguments)
	{
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		VisitExpression(expr);

		TypeSymbol* argType = GetExpressionType(expr);
		argTypes.push_back(argType);
	}

	MethodSymbol* method = currentType->FindMethod(methodName, argTypes);
	if (method == nullptr)
		method = Table->GlobalType->FindMethod(methodName, argTypes);

	if (method == nullptr)
	{
		wstringstream diag;
		diag << "No method \"" << methodName << "\" found that accepts ";

		if (argTypes.size() == 0)
		{
			diag << "no arguments";
		}
		else if (argTypes.size() == 1)
		{
			TypeSymbol* type = argTypes.at(0);
			wstring typeName = type == nullptr ? L"<error>" : type->Name;
			diag << "argument (" << typeName << ")";
		}
		else
		{
			TypeSymbol* type = argTypes.at(0);
			wstring typeName = type == nullptr ? L"<error>" : type->Name;
			diag << L"arguments (" << typeName;

			for (int i = 1; i < argTypes.size(); i++)
			{
				type = argTypes.at(i);
				typeName = type == nullptr ? L"<error>" : type->Name;
				diag << ", " << typeName;
			}

			diag << ")";
		}

		Diagnostics.ReportError(node->IdentifierToken, diag.str());
		return nullptr;
	}

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

TypeSymbol* ExpressionBinder::AnalyzeIndexatorExpression(IndexatorExpressionSyntax* node, bool& isStaticContext, TypeSymbol* currentType)
{
	wstring methodName = node->MemberAccess->IdentifierToken.Word;
	MethodSymbol* method = ResolveIndexator(node, isStaticContext, currentType);

	if (!IsSymbolAccessible(method))
	{
		Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Indexator '" + methodName + L"' is not accessible");
		return nullptr;
	}

	if (!MatchMethodArguments(method, node->IndexatorList->Arguments))
	{
		Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Indexator '" + methodName + L"' argument types do not match");
		return nullptr;
	}

	node->Symbol = method;
	isStaticContext = false;

	if (currentType->Kind == SyntaxKind::CollectionExpression)
	{
		ArrayTypeSymbol* array = static_cast<ArrayTypeSymbol*>(currentType);
		return array->UnderlayingType;
	}

	if (method->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Indexator '" + methodName + L"' return type not resolved");
		return nullptr;
	}

	return method->ReturnType;
}

MethodSymbol* ExpressionBinder::ResolveIndexator(IndexatorExpressionSyntax* node, bool& isStaticContext, TypeSymbol* currentType)
{
	wstring methodName = node->MemberAccess->IdentifierToken.Word;
	vector<TypeSymbol*> argTypes;

	for (ArgumentSyntax* arg : node->IndexatorList->Arguments)
	{
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		VisitExpression(expr);

		TypeSymbol* argType = GetExpressionType(expr);
		argTypes.push_back(argType);
	}

	if (currentType->Kind == SyntaxKind::CollectionExpression)
	{
		return SymbolTable::Primitives::Array->Indexators[0];
	}

	MethodSymbol* method = currentType->FindIndexator(argTypes);
	if (method == nullptr)
		method = Table->GlobalType->FindIndexator(argTypes);

	if (method == nullptr)
	{
		wstringstream diag;
		diag << "No indeaxtors for type \"" << currentType->Name << "\" found that accepts ";

		if (argTypes.size() == 0)
		{
			diag << "no arguments";
		}
		else if (argTypes.size() == 1)
		{
			TypeSymbol* type = argTypes.at(0);
			wstring typeName = type == nullptr ? L"<error>" : type->Name;
			diag << "argument (" << typeName << ")";
		}
		else
		{
			TypeSymbol* type = argTypes.at(0);
			wstring typeName = type == nullptr ? L"<error>" : type->Name;
			diag << L"arguments (" << typeName;

			for (int i = 1; i < argTypes.size(); i++)
			{
				type = argTypes.at(i);
				typeName = type == nullptr ? L"<error>" : type->Name;
				diag << ", " << typeName;
			}

			diag << ")";
		}

		Diagnostics.ReportError(node->MemberAccess->IdentifierToken, diag.str());
		return nullptr;
	}

	if (isStaticContext && !method->IsStatic)
	{
		Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Cannot call instance method '" + methodName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && method->IsStatic)
	{
		Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Cannot call static method '" + methodName + L"' on instance reference");
		return nullptr;
	}

	return method;
}

void ExpressionBinder::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
{
	// Already handled in VisitLinkedExpression
}

void ExpressionBinder::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	if (node->ArgumentsList != nullptr)
		VisitArgumentsList(node->ArgumentsList);
}

void ExpressionBinder::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	if (node->IndexatorList != nullptr)
		VisitIndexatorList(node->IndexatorList);
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
	if (returnType != SymbolTable::Primitives::Void)
	{
		if (node->Expression == nullptr)
		{
			Diagnostics.ReportError(SyntaxToken(), L"Return statement must return a value of type '" + returnType->Name + L"'");
		}
		else
		{
			searchingScope->ReturnsAnything = true;
			VisitExpression(node->Expression);
			TypeSymbol* returnExprType = GetExpressionType(node->Expression);

			if (returnExprType == nullptr)
			{
				Diagnostics.ReportError(node->Expression->Kind == SyntaxKind::LiteralExpression
					? static_cast<LiteralExpressionSyntax*>(node->Expression)->LiteralToken
					: SyntaxToken(), L"Return expression type could not be determined");
			}
			else if (returnExprType != returnType)
			{
				Diagnostics.ReportError(node->Expression->Kind == SyntaxKind::LiteralExpression
					? static_cast<LiteralExpressionSyntax*>(node->Expression)->LiteralToken
					: SyntaxToken(), L"Return type mismatch: expected '" + returnType->Name + L"' but got '" + returnExprType->Name + L"'");
			}
		}
	}
	else
	{
		if (node->Expression != nullptr)
		{
			searchingScope->ReturnsAnything = true;
			VisitExpression(node->Expression);

			Diagnostics.ReportError(node->Expression->Kind == SyntaxKind::LiteralExpression
				? static_cast<LiteralExpressionSyntax*>(node->Expression)->LiteralToken
				: SyntaxToken(), L"Void method cannot return a value");
		}
	}
}
