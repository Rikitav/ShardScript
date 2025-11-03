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
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>

#include <shard/syntax/nodes/TypeSyntax.h>
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

#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>

#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>
#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>

#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

using namespace std;
using namespace shard::parsing;
using namespace shard::parsing::semantic;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;
using namespace shard::syntax;

void ExpressionBinder::pushScope(SyntaxSymbol* symbol)
{
	SemanticScope* newScope = new SemanticScope(symbol, scopeStack.top());
	scopeStack.push(newScope);
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
	pushScope(nullptr);
	for (UsingDirectiveSyntax* directive : node->Usings)
		VisitUsingDirective(directive);

	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);
	
	scopeStack.pop();
}

void ExpressionBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	NamespaceSymbol* symbol = static_cast<NamespaceSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr)
	{
		pushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);
		scopeStack.pop();
	}
}

void ExpressionBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = static_cast<ClassSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr)
	{
		pushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);
		scopeStack.pop();
	}
}

void ExpressionBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = static_cast<StructSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr)
	{
		pushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);
		scopeStack.pop();
	}
}

void ExpressionBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = static_cast<MethodSymbol*>(symbolTable->LookupSymbol(node));
	TypeSymbol* ownerType = static_cast<TypeSymbol*>((SyntaxSymbol*)scopeStack.top()->Owner);

	if (symbol != nullptr && node->Body != nullptr)
	{
		pushScope(symbol);

		if (!symbol->IsStatic)
		{
			VariableSymbol* thisVarSymbol = new VariableSymbol(L"this");
			thisVarSymbol->Type = ownerType;
			scopeStack.top()->DeclareSymbol(thisVarSymbol);
		}

		for (ParameterSymbol* parameter : symbol->Parameters)
		{
			wstring paramName = parameter->Name;
			VariableSymbol* paramVar = new VariableSymbol(paramName);
			paramVar->Type = parameter->Type;
			scopeStack.top()->DeclareSymbol(paramVar);
		}

		VisitStatementsBlock(node->Body);

		if (symbol->ReturnType->Name != L"Void")
		{
			bool hasReturn = false;
			for (StatementSyntax* statement : node->Body->Statements)
			{
				if (statement->Kind == SyntaxKind::ReturnStatement)
				{
					hasReturn = true;
					ReturnStatementSyntax* returnStmt = static_cast<ReturnStatementSyntax*>(statement);
					if (returnStmt->Expression != nullptr)
					{
						TypeSymbol* returnExprType = GetExpressionType(returnStmt->Expression);
						if (returnExprType == nullptr)
						{
							Diagnostics.ReportError(returnStmt->Expression->Kind == SyntaxKind::LiteralExpression
								? static_cast<LiteralExpressionSyntax*>(returnStmt->Expression)->LiteralToken
								: SyntaxToken(), "Return expression type could not be determined");
						}
						else if (returnExprType != symbol->ReturnType)
						{
							string expectedName(symbol->ReturnType->Name.begin(), symbol->ReturnType->Name.end());
							string actualName(returnExprType->Name.begin(), returnExprType->Name.end());

							Diagnostics.ReportError(returnStmt->Expression->Kind == SyntaxKind::LiteralExpression
								? static_cast<LiteralExpressionSyntax*>(returnStmt->Expression)->LiteralToken
								: SyntaxToken(), "Return type mismatch: expected '" + expectedName + "' but got '" + actualName + "'");
						}
					}
					else
					{
						string returnTypeName(symbol->ReturnType->Name.begin(), symbol->ReturnType->Name.end());
						Diagnostics.ReportError(SyntaxToken(), "Return statement must return a value of type '" + returnTypeName + "'");
					}
				}
			}
			
			if (!hasReturn)
			{
				string returnTypeName(symbol->ReturnType->Name.begin(), symbol->ReturnType->Name.end());
				Diagnostics.ReportError(node->IdentifierToken, "Method must return a value of type '" + returnTypeName + "'");
			}
		}
		else
		{
			for (StatementSyntax* statement : node->Body->Statements)
			{
				if (statement->Kind == SyntaxKind::ReturnStatement)
				{
					ReturnStatementSyntax* returnStmt = static_cast<ReturnStatementSyntax*>(statement);
					if (returnStmt->Expression != nullptr)
					{
						Diagnostics.ReportError(returnStmt->Expression->Kind == SyntaxKind::LiteralExpression
							? static_cast<LiteralExpressionSyntax*>(returnStmt->Expression)->LiteralToken
							: SyntaxToken(), "Void method cannot return a value");
					}
				}
			}
		}
		
		scopeStack.pop();
	}

	if (symbol->Name == L"Main")
	{
		symbolTable->EntryPointCandidates.push_back(symbol);
		if (symbol->Accesibility != SymbolAccesibility::Public)
			Diagnostics.ReportError(node->IdentifierToken, "Main entry point should be public");

		if (!symbol->IsStatic)
			Diagnostics.ReportError(node->IdentifierToken, "Main entry point should be static");

		if (symbol->Parameters.size() != 0)
			Diagnostics.ReportError(node->IdentifierToken, "Main entry point should have empty parameters list");

		if (symbol->ReturnType != SymbolTable::Primitives::Void)
			Diagnostics.ReportError(node->IdentifierToken, "Main entry point should have 'void' return type");
	}
}

void ExpressionBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node->InitializerExpression != nullptr)
	{
		VisitExpression(node->InitializerExpression);
		
		FieldSymbol* fieldSymbol = static_cast<FieldSymbol*>(symbolTable->LookupSymbol(node));
		if (fieldSymbol != nullptr && fieldSymbol->ReturnType != nullptr)
		{
			TypeSymbol* initExprType = GetExpressionType(node->InitializerExpression);
			if (initExprType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, "Field initializer expression type could not be determined");
			}
			else if (initExprType != fieldSymbol->ReturnType)
			{
				string expectedName(fieldSymbol->ReturnType->Name.begin(), fieldSymbol->ReturnType->Name.end());
				string actualName(initExprType->Name.begin(), initExprType->Name.end());
				Diagnostics.ReportError(node->IdentifierToken, "Field initializer type mismatch: expected '" + expectedName + "' but got '" + actualName + "'");
			}
		}
		else if (fieldSymbol == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, "Field symbol not found");
		}
		else if (fieldSymbol->ReturnType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, "Field type not resolved");
		}
	}
}

void ExpressionBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
	
	if (node->Type != nullptr && node->Expression != nullptr)
	{
		VariableSymbol* varSymbol = static_cast<VariableSymbol*>(symbolTable->LookupSymbol(node));
		scopeStack.top()->DeclareSymbol(varSymbol);

		TypeSymbol* expectedType = varSymbol != nullptr ? varSymbol->Type : nullptr;
		TypeSymbol* actualType = GetExpressionType(node->Expression);
		
		if (expectedType != nullptr && actualType != nullptr && expectedType != actualType)
		{
			string expectedName(expectedType->Name.begin(), expectedType->Name.end());
			string actualName(actualType->Name.begin(), actualType->Name.end());
			Diagnostics.ReportError(node->IdentifierToken, "Type mismatch: expected '" + expectedName + "' but got '" + actualName + "'");
		}
		else if (expectedType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, "Variable type not resolved");
		}
		else if (actualType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, "Expression type could not be determined");
		}
	}
}

bool ExpressionBinder::IsSymbolAccessible(SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		return false;

	if (symbol->Accesibility == SymbolAccesibility::Public)
		return true;

	TypeSymbol* declaringType = nullptr;
	for (const SemanticScope* scope = scopeStack.top(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner != nullptr && 
			(scope->Owner->Kind == SyntaxKind::ClassDeclaration || scope->Owner->Kind == SyntaxKind::StructDeclaration))
		{
			declaringType = static_cast<TypeSymbol*>(const_cast<SyntaxSymbol*>(scope->Owner));
			break;
		}
	}

	if (symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration)
	{
		TypeSymbol* typeSymbol = static_cast<TypeSymbol*>(symbol);
		
		if (typeSymbol->Accesibility == SymbolAccesibility::Private)
		{
			SyntaxNode* typeNode = symbolTable->GetSyntaxNode(typeSymbol);
			if (typeNode != nullptr)
			{
				const SyntaxNode* parent = typeNode->Parent;
				if (parent != nullptr && (parent->Kind == SyntaxKind::ClassDeclaration || parent->Kind == SyntaxKind::StructDeclaration))
				{
					SyntaxSymbol* parentSymbol = symbolTable->LookupSymbol(const_cast<SyntaxNode*>(parent));
					if (parentSymbol != nullptr)
					{
						for (const SemanticScope* scope = scopeStack.top(); scope != nullptr; scope = scope->Parent)
						{
							if (scope->Owner == parentSymbol)
								return true;
						}
					}
				}
			}
			return false;
		}
		
		return true;
	}

	if (symbol->Accesibility == SymbolAccesibility::Private)
	{
		SyntaxNode* symbolNode = symbolTable->GetSyntaxNode(symbol);
		if (symbolNode != nullptr)
		{
			const SyntaxNode* parent = symbolNode->Parent;
			while (parent != nullptr)
			{
				if (parent->Kind == SyntaxKind::ClassDeclaration || parent->Kind == SyntaxKind::StructDeclaration)
				{
					SyntaxSymbol* parentSymbol = symbolTable->LookupSymbol(const_cast<SyntaxNode*>(parent));
					if (parentSymbol == declaringType)
						return true;
					break;
				}
				parent = parent->Parent;
			}
		}
		return false;
	}
	else if (symbol->Accesibility == SymbolAccesibility::Protected)
	{
		SyntaxNode* symbolNode = symbolTable->GetSyntaxNode(symbol);
		if (symbolNode != nullptr)
		{
			const SyntaxNode* parent = symbolNode->Parent;
			while (parent != nullptr)
			{
				if (parent->Kind == SyntaxKind::ClassDeclaration || parent->Kind == SyntaxKind::StructDeclaration)
				{
					SyntaxSymbol* parentSymbol = symbolTable->LookupSymbol(const_cast<SyntaxNode*>(parent));
					if (parentSymbol == declaringType)
						return true;
					break;
				}
				parent = parent->Parent;
			}
		}
		return false;
	}

	return true;
}

TypeSymbol* ExpressionBinder::ResolveType(TypeSyntax* typeSyntax)
{
	if (typeSyntax == nullptr)
		return nullptr;

	switch (typeSyntax->Kind)
	{
		case SyntaxKind::PredefinedType:
		{
			PredefinedTypeSyntax* predefined = static_cast<PredefinedTypeSyntax*>(typeSyntax);
			switch (predefined->TypeToken.Type)
			{
				case TokenType::BooleanKeyword:
					return SymbolTable::Primitives::Boolean;

				case TokenType::IntegerKeyword:
					return SymbolTable::Primitives::Integer;

				case TokenType::CharKeyword:
					return SymbolTable::Primitives::Char;

				case TokenType::StringKeyword:
					return SymbolTable::Primitives::String;

				case TokenType::VoidKeyword:
					return SymbolTable::Primitives::Void;

				default:
					return nullptr;
			}
		}

		case SyntaxKind::IdentifierNameType:
		{
			IdentifierNameTypeSyntax* identifierType = static_cast<IdentifierNameTypeSyntax*>(typeSyntax);
			
			if (identifierType->Identifiers.empty())
				return nullptr;

			SyntaxSymbol* symbol = nullptr;
			if (identifierType->Identifiers.size() == 1)
			{
				wstring name = identifierType->Identifiers[0].Word;
				SemanticScope* currentScope = scopeStack.top();
				symbol = currentScope->Lookup(name);
				
				if (symbol == nullptr)
				{
					vector<TypeSymbol*> allTypes = symbolTable->GetTypeSymbols();
					for (TypeSymbol* type : allTypes)
					{
						if (type->Name == name && (type->Kind == SyntaxKind::ClassDeclaration || type->Kind == SyntaxKind::StructDeclaration))
						{
							symbol = type;
							break;
						}
					}
				}
			}
			else
			{
				wstring firstName = identifierType->Identifiers[0].Word;
				SemanticScope* currentScope = scopeStack.top();
				symbol = currentScope->Lookup(firstName);
				
				for (size_t i = 1; i < identifierType->Identifiers.size() && symbol != nullptr; i++)
				{
					if (symbol->Kind != SyntaxKind::NamespaceDeclaration)
						return nullptr;

					NamespaceSymbol* namespaceSymbol = static_cast<NamespaceSymbol*>(symbol);
					wstring nextName = identifierType->Identifiers[i].Word;
					
					SyntaxNode* namespaceNode = symbolTable->GetSyntaxNode(namespaceSymbol);
					if (namespaceNode == nullptr)
						return nullptr;

					NamespaceDeclarationSyntax* namespaceDecl = static_cast<NamespaceDeclarationSyntax*>(namespaceNode);
					
					symbol = nullptr;
					for (MemberDeclarationSyntax* member : namespaceDecl->Members)
					{
						if (member->Kind == SyntaxKind::ClassDeclaration || member->Kind == SyntaxKind::StructDeclaration)
						{
							SyntaxToken identifierToken;
							if (member->Kind == SyntaxKind::ClassDeclaration)
							{
								ClassDeclarationSyntax* classDecl = static_cast<ClassDeclarationSyntax*>(member);
								identifierToken = classDecl->IdentifierToken;
							}
							else if (member->Kind == SyntaxKind::StructDeclaration)
							{
								StructDeclarationSyntax* structDecl = static_cast<StructDeclarationSyntax*>(member);
								identifierToken = structDecl->IdentifierToken;
							}

							if (identifierToken.Word == nextName)
							{
								symbol = symbolTable->LookupSymbol(member);
								break;
							}
						}
						else if (member->Kind == SyntaxKind::NamespaceDeclaration)
						{
							NamespaceDeclarationSyntax* nestedNamespace = static_cast<NamespaceDeclarationSyntax*>(member);
							if (nestedNamespace->IdentifierToken.Word == nextName)
							{
								symbol = symbolTable->LookupSymbol(nestedNamespace);
								break;
							}
						}
					}
				}
			}

			if (symbol == nullptr)
				return nullptr;

			if (symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration)
			{
				return static_cast<TypeSymbol*>(symbol);
			}

			return nullptr;
		}

		default:
			return nullptr;
	}
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
		string tokenType = "unknown";
		Diagnostics.ReportError(node->LiteralToken, "Unsupported literal type: " + tokenType);
	}
}

TypeSymbol* ExpressionBinder::AnalyzeBinaryExpression(BinaryExpressionSyntax* node)
{
	VisitExpression(node->Left);
	VisitExpression(node->Right);
	
	TypeSymbol* leftType = GetExpressionType(node->Left);
	TypeSymbol* rightType = GetExpressionType(node->Right);
	
	if (leftType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, "Left operand type could not be determined");
		return nullptr;
	}
	
	if (rightType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, "Right operand type could not be determined");
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
				string leftName(leftType->Name.begin(), leftType->Name.end());
				string rightName(rightType->Name.begin(), rightType->Name.end());
				Diagnostics.ReportError(node->OperatorToken, "Type mismatch in comparison: '" + leftName + "' and '" + rightName + "'");
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
				string leftName(leftType->Name.begin(), leftType->Name.end());
				string rightName(rightType->Name.begin(), rightType->Name.end());
				Diagnostics.ReportError(node->OperatorToken, "Type mismatch in arithmetic operation: '" + leftName + "' and '" + rightName + "'");
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
				string leftName(leftType->Name.begin(), leftType->Name.end());
				string rightName(rightType->Name.begin(), rightType->Name.end());
				Diagnostics.ReportError(node->OperatorToken, "Type mismatch in assignment operation: '" + leftName + "' and '" + rightName + "'");
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
	VisitExpression(node->Expression);
	
	TypeSymbol* exprType = GetExpressionType(node->Expression);
	
	if (exprType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, "Operand type could not be determined");
		return nullptr;
	}
	
	switch (node->OperatorToken.Type)
	{
		case TokenType::NotOperator:
		{
			if (exprType != SymbolTable::Primitives::Boolean)
			{
				string typeName(exprType->Name.begin(), exprType->Name.end());
				Diagnostics.ReportError(node->OperatorToken, "Logical NOT operator requires boolean type, got '" + typeName + "'");
				return nullptr;
			}

			return SymbolTable::Primitives::Boolean;
		}

		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		{
			if (exprType != SymbolTable::Primitives::Integer)
			{
				string typeName(exprType->Name.begin(), exprType->Name.end());
				Diagnostics.ReportError(node->OperatorToken, "Increment/Decrement operator requires integer type, got '" + typeName + "'");
				return nullptr;
			}
			
			return SymbolTable::Primitives::Integer;
		}
			
		default:
			return exprType;
	}
}

void ExpressionBinder::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	TypeSymbol* type = AnalyzeUnaryExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeObjectExpression(ObjectExpressionSyntax* node)
{
	if (node->Symbol == nullptr)
	{
		Diagnostics.ReportError(node->NewToken, "Object creation type not resolved");
		return nullptr;
	}
	
	VisitArgumentsList(node->Arguments);
	return node->Symbol;
}

void ExpressionBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	TypeSymbol* type = AnalyzeObjectExpression(node);
	SetExpressionType(node, type);
	VisitType(node->Type);
}

bool ExpressionBinder::MatchMethodArguments(MethodSymbol* method, ArgumentsListSyntax* arguments)
{
	if (method == nullptr)
		return false;
		
	if (arguments == nullptr)
		return method->Parameters.empty();
	
	if (method->Parameters.size() != arguments->Arguments.size())
	{
		Diagnostics.ReportError(SyntaxToken(), "Method expects " + to_string(method->Parameters.size()) + " arguments but got " + to_string(arguments->Arguments.size()));
		return false;
	}
	
	for (size_t i = 0; i < method->Parameters.size(); i++)
	{
		ParameterSymbol* param = method->Parameters[i];
		ArgumentSyntax* arg = arguments->Arguments[i];
		
		if (param == nullptr || arg == nullptr || arg->Expression == nullptr)
		{
			Diagnostics.ReportError(SyntaxToken(), "Invalid argument at position " + to_string(i));
			return false;
		}
		
		ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
		VisitExpression(expr);
		TypeSymbol* argType = GetExpressionType(expr);
		
		if (param->Type == nullptr)
		{
			string paramName(param->Name.begin(), param->Name.end());
			Diagnostics.ReportError(SyntaxToken(), "Parameter '" + paramName + "' type not resolved");
			return false;
		}
		
		if (argType == nullptr)
		{
			string paramName(param->Name.begin(), param->Name.end());
			Diagnostics.ReportError(SyntaxToken(), "Argument type could not be determined for parameter '" + paramName + "'");
			return false;
		}
		
		if (param->Type != argType)
		{
			string paramName(param->Type->Name.begin(), param->Type->Name.end());
			string argName(argType->Name.begin(), argType->Name.end());
			string paramVarName(param->Name.begin(), param->Name.end());
			Diagnostics.ReportError(SyntaxToken(), "Argument type mismatch for parameter '" + paramVarName + "': expected '" + paramName + "' but got '" + argName + "'");
			return false;
		}
	}
	
	return true;
}

TypeSymbol* ExpressionBinder::AnalyzeLinkedExpression(LinkedExpressionSyntax* node)
{
	if (node->Nodes.empty() || node->First == nullptr)
	{
		Diagnostics.ReportError(SyntaxToken(), "Empty linked expression");
		return nullptr;
	}
	
    LinkedExpressionNode* current = node->First;
    TypeSymbol* currentType = nullptr;
    bool isStaticContext = false;
	
	while (current != nullptr)
	{
		switch (current->Kind)
		{
			case SyntaxKind::MemberAccessExpression:
			{
				MemberAccessExpressionSyntax* memberAccess = static_cast<MemberAccessExpressionSyntax*>(current);
				
				if (current == node->First)
				{
					wstring name = memberAccess->IdentifierToken.Word;
					SemanticScope* currentScope = scopeStack.top();
					SyntaxSymbol* symbol = currentScope->Lookup(name);
					
                    if (symbol == nullptr)
                    {
                        vector<TypeSymbol*> allTypes = symbolTable->GetTypeSymbols();
                        for (TypeSymbol* t : allTypes)
                        {
                            if (t->Name == name)
                            {
                                currentType = t;
                                isStaticContext = true;
                                symbol = t;
                                break;
                            }
                        }

                        if (symbol == nullptr)
                        {
                            string nameStr(name.begin(), name.end());
                            Diagnostics.ReportError(memberAccess->IdentifierToken, "Symbol '" + nameStr + "' not found in current scope");
                            return nullptr;
                        }
                    }
					
					if (!IsSymbolAccessible(symbol))
					{
						string nameStr(name.begin(), name.end());
						Diagnostics.ReportError(memberAccess->IdentifierToken, "Symbol '" + nameStr + "' is not accessible");
						return nullptr;
					}
					
                    if (symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration)
                    {
                        currentType = static_cast<TypeSymbol*>(symbol);
                        isStaticContext = true;
                    }
                    else if (symbol->Kind == SyntaxKind::VariableStatement)
					{
						VariableSymbol* varSymbol = static_cast<VariableSymbol*>(symbol);
						currentType = varSymbol->Type;
                        isStaticContext = false;
						
						if (currentType == nullptr)
						{
							string nameStr(name.begin(), name.end());
							Diagnostics.ReportError(memberAccess->IdentifierToken, "Variable '" + nameStr + "' type not resolved");
							return nullptr;
						}
					}
                    else if (symbol->Kind == SyntaxKind::Parameter)
					{
						ParameterSymbol* paramSymbol = static_cast<ParameterSymbol*>(symbol);
						currentType = paramSymbol->Type;
                        isStaticContext = false;
						
						if (currentType == nullptr)
						{
							string nameStr(name.begin(), name.end());
							Diagnostics.ReportError(memberAccess->IdentifierToken, "Parameter '" + nameStr + "' type not resolved");
							return nullptr;
						}
					}
                    else if (symbol->Kind == SyntaxKind::FieldDeclaration)
					{
						FieldSymbol* fieldSymbol = static_cast<FieldSymbol*>(symbol);
						
						if (!IsSymbolAccessible(fieldSymbol))
						{
							string nameStr(name.begin(), name.end());
							Diagnostics.ReportError(memberAccess->IdentifierToken, "Field '" + nameStr + "' is not accessible");
							return nullptr;
						}
						
                        currentType = fieldSymbol->ReturnType;
                        isStaticContext = false;
						
						if (currentType == nullptr)
						{
							string nameStr(name.begin(), name.end());
							Diagnostics.ReportError(memberAccess->IdentifierToken, "Field '" + nameStr + "' type not resolved");
							return nullptr;
						}
					}
					else
					{
						string nameStr(name.begin(), name.end());
						Diagnostics.ReportError(memberAccess->IdentifierToken, "Symbol '" + nameStr + "' is not a variable, parameter or field (found " + to_string(static_cast<int>(symbol->Kind)) + ")");
						return nullptr;
					}
				}
				else
				{
					if (currentType == nullptr)
					{
						Diagnostics.ReportError(memberAccess->IdentifierToken, "Cannot access member: previous expression has no type");
						return nullptr;
					}
					
					if (currentType->Kind != SyntaxKind::ClassDeclaration && currentType->Kind != SyntaxKind::StructDeclaration)
					{
						string typeName(currentType->Name.begin(), currentType->Name.end());
						Diagnostics.ReportError(memberAccess->IdentifierToken, "Cannot access member on non-type '" + typeName + "'");
						return nullptr;
					}
					
					wstring memberName = memberAccess->IdentifierToken.Word;
                    FieldSymbol* field = memberAccess->Symbol = currentType->FindField(memberName);
					
					if (field == nullptr)
					{
						MethodSymbol* method = currentType->FindMethod(memberName, vector<TypeSymbol*>());
						if (method == nullptr)
						{
							string typeName(currentType->Name.begin(), currentType->Name.end());
							string memberStr(memberName.begin(), memberName.end());
							Diagnostics.ReportError(memberAccess->IdentifierToken, "Member '" + memberStr + "' not found in type '" + typeName + "'");
						}
						else
						{
							string memberStr(memberName.begin(), memberName.end());
							Diagnostics.ReportError(memberAccess->IdentifierToken, "Cannot access method '" + memberStr + "' without invocation");
						}
						return nullptr;
					}
					
                    if (!IsSymbolAccessible(field))
					{
						string memberStr(memberName.begin(), memberName.end());
						Diagnostics.ReportError(memberAccess->IdentifierToken, "Field '" + memberStr + "' is not accessible");
						return nullptr;
					}

                    if (isStaticContext && !field->IsStatic)
                    {
                        string memberStr(memberName.begin(), memberName.end());
                        Diagnostics.ReportError(memberAccess->IdentifierToken, "Cannot access instance field '" + memberStr + "' from type context");
                        return nullptr;
                    }
                    if (!isStaticContext && field->IsStatic)
                    {
                        string memberStr(memberName.begin(), memberName.end());
                        Diagnostics.ReportError(memberAccess->IdentifierToken, "Cannot access static field '" + memberStr + "' from instance reference");
                        return nullptr;
                    }
					
                    currentType = field->ReturnType;
                    isStaticContext = false;
					
					if (currentType == nullptr)
					{
						string memberStr(memberName.begin(), memberName.end());
						Diagnostics.ReportError(memberAccess->IdentifierToken, "Field '" + memberStr + "' type not resolved");
						return nullptr;
					}
				}
				
				break;
			}
			
			case SyntaxKind::InvokationExpression:
			{
				InvokationExpressionSyntax* invocation = static_cast<InvokationExpressionSyntax*>(current);
				wstring methodName = invocation->IdentifierToken.Word;
				MethodSymbol* method = nullptr;

				vector<TypeSymbol*> argTypes;
				if (invocation->ArgumentsList != nullptr)
				{
					for (ArgumentSyntax* arg : invocation->ArgumentsList->Arguments)
					{
						if (arg->Expression != nullptr)
						{
							ExpressionSyntax* expr = const_cast<ExpressionSyntax*>(arg->Expression);
							VisitExpression(expr);
							TypeSymbol* argType = GetExpressionType(expr);
							argTypes.push_back(argType);

							if (argType == nullptr)
							{
								string methodStr(methodName.begin(), methodName.end());
								Diagnostics.ReportError(invocation->IdentifierToken, "Argument type could not be determined for method '" + methodStr + "'");
							}
						}
						else
						{
							argTypes.push_back(nullptr);
							string methodStr(methodName.begin(), methodName.end());
							Diagnostics.ReportError(invocation->IdentifierToken, "Null argument provided to method '" + methodStr + "'");
						}
					}
				}

				if (currentType == nullptr)
				{
					isStaticContext = true;
					method = symbolTable->GlobalType->FindMethod(methodName, argTypes);
					if (method == nullptr)
					{
						string methodStr(methodName.begin(), methodName.end());
						Diagnostics.ReportError(invocation->IdentifierToken, "Global scope's member '" + methodStr + "' is not a method");
						return nullptr;
					}
				}
				else
				{
					if (currentType->Kind != SyntaxKind::ClassDeclaration && currentType->Kind != SyntaxKind::StructDeclaration)
					{
						string typeName(currentType->Name.begin(), currentType->Name.end());
						Diagnostics.ReportError(invocation->IdentifierToken, "Cannot invoke method on non-type '" + typeName + "'");
						return nullptr;
					}

					method = currentType->FindMethod(methodName, argTypes);
					if (method == nullptr)
					{
						string typeName(currentType->Name.begin(), currentType->Name.end());
						string methodStr(methodName.begin(), methodName.end());
						Diagnostics.ReportError(invocation->IdentifierToken, "Method '" + methodStr + "' not found in type '" + typeName + "' or argument types do not match");
						return nullptr;
					}
				}

                if (!IsSymbolAccessible(method))
				{
					string methodStr(methodName.begin(), methodName.end());
					Diagnostics.ReportError(invocation->IdentifierToken, "Method '" + methodStr + "' is not accessible");
					return nullptr;
				}
				
				if (!MatchMethodArguments(method, invocation->ArgumentsList))
				{
					string methodStr(methodName.begin(), methodName.end());
					Diagnostics.ReportError(invocation->IdentifierToken, "Method '" + methodStr + "' argument types do not match");
					return nullptr;
				}
				
                if (isStaticContext && !method->IsStatic)
                {
                    string methodStr(methodName.begin(), methodName.end());
                    Diagnostics.ReportError(invocation->IdentifierToken, "Cannot call instance method '" + methodStr + "' from type context");
                    return nullptr;
                }

                if (!isStaticContext && method->IsStatic)
                {
                    string methodStr(methodName.begin(), methodName.end());
                    Diagnostics.ReportError(invocation->IdentifierToken, "Cannot call static method '" + methodStr + "' on instance reference");
                    return nullptr;
                }

				invocation->Symbol = method;
                currentType = method->ReturnType;
                isStaticContext = false;
				
				if (currentType == nullptr)
				{
					string methodStr(methodName.begin(), methodName.end());
					Diagnostics.ReportError(invocation->IdentifierToken, "Method '" + methodStr + "' return type not resolved");
					return nullptr;
				}
				
				break;
			}
			
			case SyntaxKind::IndexatorExpression:
			{
				Diagnostics.ReportError(SyntaxToken(), "Indexer expressions are not yet supported");
				return nullptr;
			}
			
			default:
			{
				Diagnostics.ReportError(SyntaxToken(), "Unknown linked expression node type");
				return nullptr;
			}
		}
		
		current = current->NextNode;
	}
	
	return currentType;
}

void ExpressionBinder::VisitLinkedExpression(LinkedExpressionSyntax* node)
{
	TypeSymbol* type = AnalyzeLinkedExpression(node);
	SetExpressionType(node, type);
	
	for (LinkedExpressionNode* exprNode : node->Nodes)
		VisitLinkedExpressionNode(exprNode);
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

void ExpressionBinder::VisitWhileStatement(WhileStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression);
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression);
		
		if (conditionType == nullptr)
		{
			Diagnostics.ReportError(node->KeywordToken, "While loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			string typeName(conditionType->Name.begin(), conditionType->Name.end());
			Diagnostics.ReportError(node->KeywordToken, "While loop condition must be boolean, got '" + typeName + "'");
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
			Diagnostics.ReportError(node->KeywordToken, "Until loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			string typeName(conditionType->Name.begin(), conditionType->Name.end());
			Diagnostics.ReportError(node->KeywordToken, "Until loop condition must be boolean, got '" + typeName + "'");
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
			Diagnostics.ReportError(node->FirstSemicolon, "For loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			string typeName(conditionType->Name.begin(), conditionType->Name.end());
			Diagnostics.ReportError(node->FirstSemicolon, "For loop condition must be boolean, got '" + typeName + "'");
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
				Diagnostics.ReportError(node->KeywordToken, "If condition type could not be determined");
			}
			else if (conditionType != SymbolTable::Primitives::Boolean)
			{
				string typeName(conditionType->Name.begin(), conditionType->Name.end());
				Diagnostics.ReportError(node->KeywordToken, "If condition must be boolean, got '" + typeName + "'");
			}
		}
		else
		{
			Diagnostics.ReportError(node->KeywordToken, "If condition must be an expression");
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
				Diagnostics.ReportError(node->KeywordToken, "Unless condition type could not be determined");
			}
			else if (conditionType != SymbolTable::Primitives::Boolean)
			{
				string typeName(conditionType->Name.begin(), conditionType->Name.end());
				Diagnostics.ReportError(node->KeywordToken, "Unless condition must be boolean, got '" + typeName + "'");
			}
		}
		else
		{
			Diagnostics.ReportError(node->KeywordToken, "Unless condition must be an expression");
		}
	}
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock);
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement);
}

void ExpressionBinder::VisitReturnStatement(ReturnStatementSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

