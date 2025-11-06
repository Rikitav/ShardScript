#include <shard/parsing/visiting/TypeBinder.h>
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
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>

#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <vector>
#include <string>

using namespace std;
using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::semantic;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;
using namespace shard::syntax;

void TypeBinder::pushScope(SyntaxSymbol* symbol)
{
	SemanticScope* newScope = new SemanticScope(symbol, scopeStack.top());
	scopeStack.push(newScope);
}

void TypeBinder::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	pushScope(nullptr);
	for (UsingDirectiveSyntax* directive : node->Usings)
		VisitUsingDirective(directive);

	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);

	scopeStack.pop();
}

void TypeBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	NamespaceSymbol* symbol = static_cast<NamespaceSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr)
	{
		scopeStack.top()->DeclareSymbol(symbol);
		pushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		scopeStack.pop();
	}
}

void TypeBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = static_cast<ClassSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr)
	{
		scopeStack.top()->DeclareSymbol(symbol);
		pushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		scopeStack.pop();
	}
}

void TypeBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = static_cast<StructSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr)
	{
		scopeStack.top()->DeclareSymbol(symbol);
		pushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		scopeStack.pop();
	}
}

void TypeBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = static_cast<MethodSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr)
	{
		if (node->ReturnType != nullptr)
		{
			symbol->ReturnType = ResolveType(node->ReturnType);
			if (symbol->ReturnType == nullptr)
			{
				wstring typeName = L"unknown";
				if (node->ReturnType->Kind == SyntaxKind::IdentifierNameType)
				{
					IdentifierNameTypeSyntax* idType = static_cast<IdentifierNameTypeSyntax*>(node->ReturnType);
					if (!idType->Identifiers.empty())
					{
						typeName = idType->Identifiers[0].Word;
					}
				}

				Diagnostics.ReportError(node->IdentifierToken, L"Return type not found: " + typeName);
			}
		}

		if (node->Params != nullptr && node->Params->Parameters.size() == symbol->Parameters.size())
		{
			for (size_t i = 0; i < node->Params->Parameters.size(); i++)
			{
				ParameterSyntax* paramSyntax = node->Params->Parameters[i];
				ParameterSymbol* paramSymbol = symbol->Parameters[i];
				
				if (paramSyntax->Type != nullptr)
				{
					paramSymbol->Type = ResolveType(const_cast<TypeSyntax*>(paramSyntax->Type));
					if (paramSymbol->Type == nullptr)
					{
						wstring typeName = L"unknown";
						if (paramSyntax->Type->Kind == SyntaxKind::IdentifierNameType)
						{
							IdentifierNameTypeSyntax* idType = static_cast<IdentifierNameTypeSyntax*>(const_cast<TypeSyntax*>(paramSyntax->Type));
							if (!idType->Identifiers.empty())
							{
								typeName = idType->Identifiers[0].Word;
							}
						}

						Diagnostics.ReportError(paramSyntax->Identifier, L"Parameter type not found: " + typeName);
					}
				}
			}
		}

		if (node->Body != nullptr)
		{
			pushScope(symbol);
			VisitStatementsBlock(node->Body);
			scopeStack.pop();
		}
	}
}

void TypeBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	FieldSymbol* symbol = static_cast<FieldSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr && node->ReturnType != nullptr)
	{
		symbol->ReturnType = ResolveType(node->ReturnType);
		if (symbol->ReturnType == nullptr)
		{
			wstring typeName = L"unknown";
			if (node->ReturnType->Kind == SyntaxKind::IdentifierNameType)
			{
				IdentifierNameTypeSyntax* idType = static_cast<IdentifierNameTypeSyntax*>(node->ReturnType);
				if (!idType->Identifiers.empty())
				{
					typeName = idType->Identifiers[0].Word;
				}
			}

			Diagnostics.ReportError(node->IdentifierToken, L"Field type not found: " + typeName);
		}
	}

	VisitType(node->ReturnType);
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void TypeBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = static_cast<PropertySymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr && node->ReturnType != nullptr)
	{
		// Resolve property return type
		TypeSymbol* propertyType = ResolveType(node->ReturnType);
		symbol->ReturnType = propertyType;
		
		if (propertyType == nullptr)
		{
			wstring typeName = L"unknown";
			if (node->ReturnType->Kind == SyntaxKind::IdentifierNameType)
			{
				IdentifierNameTypeSyntax* idType = static_cast<IdentifierNameTypeSyntax*>(node->ReturnType);
				if (!idType->Identifiers.empty())
				{
					typeName = idType->Identifiers[0].Word;
				}
			}

			Diagnostics.ReportError(node->IdentifierToken, L"Property type not found: " + typeName);
		}
		
		// Resolve backing field type if it exists
		if (symbol->BackingField != nullptr)
		{
			symbol->BackingField->ReturnType = propertyType;
		}
		
		// Resolve getter return type
		if (symbol->GetMethod != nullptr)
		{
			symbol->GetMethod->ReturnType = propertyType;
			
			// For auto-properties, generate getter body if needed
			if (node->GetBody == nullptr && symbol->BackingField != nullptr)
			{
				// Generate: return field;
				symbol->GetMethod->Body = GenerateAutoPropertyGetterBody(symbol, node);
			}
		}
		
		// Resolve setter parameter type
		if (symbol->SetMethod != nullptr && !symbol->SetMethod->Parameters.empty())
		{
			symbol->SetMethod->Parameters[0]->Type = propertyType;
			
			// For auto-properties, generate setter body if needed
			if (node->SetBody == nullptr && symbol->BackingField != nullptr)
			{
				// Generate: field = value;
				symbol->SetMethod->Body = GenerateAutoPropertySetterBody(symbol, node);
			}
		}
	}

	VisitType(node->ReturnType);
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

StatementsBlockSyntax* TypeBinder::GenerateAutoPropertyGetterBody(PropertySymbol* property, PropertyDeclarationSyntax* node)
{
	// Create: return backingField;
	StatementsBlockSyntax* body = new StatementsBlockSyntax(node);
	body->OpenBraceToken = SyntaxToken(TokenType::OpenBrace, L"{", TextLocation(), false);
	body->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"}", TextLocation(), false);
	
	// Create return statement
	ReturnStatementSyntax* returnStmt = new ReturnStatementSyntax(body);
	returnStmt->KeywordToken = SyntaxToken(TokenType::ReturnKeyword, L"return", TextLocation(), false);
	returnStmt->SemicolonToken = SyntaxToken(TokenType::Semicolon, L";", TextLocation(), false);
	
	// Create member access expression: field
	LinkedExpressionSyntax* linkedExpr = new LinkedExpressionSyntax(returnStmt);
	MemberAccessExpressionSyntax* thisAccess = new MemberAccessExpressionSyntax(SyntaxToken(TokenType::Identifier, L"this", TextLocation(), false), nullptr, linkedExpr);
	MemberAccessExpressionSyntax* fieldAccess = new MemberAccessExpressionSyntax(SyntaxToken(TokenType::Identifier, property->BackingField->Name, TextLocation(), false), nullptr, linkedExpr);

	fieldAccess->Symbol = property->BackingField;
	fieldAccess->PrevNode = fieldAccess;
	thisAccess->NextNode = fieldAccess;

	linkedExpr->First = thisAccess;
	linkedExpr->Last = fieldAccess;
	linkedExpr->Nodes.push_back(thisAccess);
	linkedExpr->Nodes.push_back(fieldAccess);
	
	returnStmt->Expression = linkedExpr;
	body->Statements.push_back(returnStmt);
	
	return body;
}

StatementsBlockSyntax* TypeBinder::GenerateAutoPropertySetterBody(PropertySymbol* property, PropertyDeclarationSyntax* node)
{
	// Create: backingField = value;
	StatementsBlockSyntax* body = new StatementsBlockSyntax(node);
	body->OpenBraceToken = SyntaxToken(TokenType::OpenBrace, L"{", TextLocation(), false);
	body->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"}", TextLocation(), false);
	
	// Create assignment expression: field = value;
	LinkedExpressionSyntax* fieldExpr = new LinkedExpressionSyntax(body);
	MemberAccessExpressionSyntax* thisAccess = new MemberAccessExpressionSyntax(SyntaxToken(TokenType::Identifier, L"this", TextLocation(), false), nullptr, fieldExpr);
	MemberAccessExpressionSyntax* fieldAccess = new MemberAccessExpressionSyntax(SyntaxToken(TokenType::Identifier, property->BackingField->Name, TextLocation(), false), nullptr, fieldExpr);
	
	fieldAccess->Symbol = property->BackingField;
	fieldAccess->PrevNode = fieldAccess;
	thisAccess->NextNode = fieldAccess;

	fieldExpr->First = thisAccess;
	fieldExpr->Last = fieldAccess;
	fieldExpr->Nodes.push_back(thisAccess);
	fieldExpr->Nodes.push_back(fieldAccess);
	
	// Create variable access: value
	LinkedExpressionSyntax* valueExpr = new LinkedExpressionSyntax(body);
	MemberAccessExpressionSyntax* valueAccess = new MemberAccessExpressionSyntax(SyntaxToken(TokenType::Identifier, L"value", TextLocation(), false), nullptr, valueExpr);
	valueExpr->Nodes.push_back(valueAccess);
	valueExpr->First = valueAccess;
	valueExpr->Last = valueAccess;
	
	// Create binary expression: field = value
	BinaryExpressionSyntax* assignExpr = new BinaryExpressionSyntax(SyntaxToken(TokenType::AssignOperator, L"=", TextLocation(), false), body);
	assignExpr->Left = fieldExpr;
	assignExpr->Right = valueExpr;
	
	// Create expression statement
	ExpressionStatementSyntax* exprStmt = new ExpressionStatementSyntax(assignExpr, body);
	exprStmt->SemicolonToken = SyntaxToken(TokenType::Semicolon, L";", TextLocation(), false);
	assignExpr->Parent = exprStmt;
	
	body->Statements.push_back(exprStmt);
	
	return body;
}

void TypeBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* symbol = static_cast<VariableSymbol*>(symbolTable->LookupSymbol(node));
	if (symbol != nullptr && node->Type != nullptr)
	{
		symbol->Type = ResolveType(node->Type);
		if (symbol->Type == nullptr)
		{
			wstring typeName = L"unknown";
			if (node->Type->Kind == SyntaxKind::IdentifierNameType)
			{
				IdentifierNameTypeSyntax* idType = static_cast<IdentifierNameTypeSyntax*>(node->Type);
				if (!idType->Identifiers.empty())
				{
					typeName = idType->Identifiers[0].Word;
				}
			}
			Diagnostics.ReportError(node->IdentifierToken, L"Variable type not found: " + typeName);
		}
	}

	VisitType(node->Type);
	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void TypeBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	if (node->Type != nullptr)
	{
		node->Symbol = ResolveType(node->Type);
		if (node->Symbol == nullptr)
		{
			wstring typeName = L"unknown";
			if (node->Type->Kind == SyntaxKind::IdentifierNameType)
			{
				IdentifierNameTypeSyntax* idType = static_cast<IdentifierNameTypeSyntax*>(node->Type);
				if (!idType->Identifiers.empty())
				{
					typeName = idType->Identifiers[0].Word;
				}
			}

			Diagnostics.ReportError(node->NewToken, L"Type not found: " + typeName);
		}
	}

	VisitType(node->Type);
	VisitArgumentsList(node->Arguments);
}

bool TypeBinder::IsSymbolAccessible(SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		return false;

	if (symbol->Accesibility == SymbolAccesibility::Public)
		return true;

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

TypeSymbol* TypeBinder::ResolveType(TypeSyntax* typeSyntax)
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
					std::wstring nextName = identifierType->Identifiers[i].Word;
					
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

			if (symbol->Kind != SyntaxKind::ClassDeclaration && symbol->Kind != SyntaxKind::StructDeclaration)
				return nullptr;

			TypeSymbol* typeSymbol = static_cast<TypeSymbol*>(symbol);
			if (!IsSymbolAccessible(symbol))
			{
				Diagnostics.ReportError(identifierType->Identifiers[0], L"Symbol inaccessible");
				return nullptr;
			}

			return typeSymbol;
		}

		default:
			return nullptr;
	}
}