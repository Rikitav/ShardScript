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

#define DONT_CARE_RESOLVE_ALL

using namespace std;
using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::semantic;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;
using namespace shard::syntax;

void TypeBinder::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	PushScope(nullptr);
	for (UsingDirectiveSyntax* directive : node->Usings)
		VisitUsingDirective(directive);

	for (MemberDeclarationSyntax* member : node->Members)
		VisitTypeDeclaration(member);

	PopScope();
}

void TypeBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	NamespaceSymbol* symbol = static_cast<NamespaceSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		Declare(symbol);
		PushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void TypeBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = static_cast<ClassSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		Declare(symbol);
		PushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void TypeBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = static_cast<StructSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		Declare(symbol);
		PushScope(symbol);
		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void TypeBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = static_cast<MethodSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		if (node->ReturnType != nullptr)
		{
			symbol->ReturnType = ResolveType(node->ReturnType);
			if (symbol->ReturnType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Return type not found: " + node->ReturnType->ToString());
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
						Diagnostics.ReportError(paramSyntax->Identifier, L"Parameter type not found: " + node->ReturnType->ToString());
					}
				}
			}
		}

		if (node->Body != nullptr)
		{
			PushScope(symbol);
			VisitStatementsBlock(node->Body);
			PopScope();
		}
	}
}

void TypeBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	FieldSymbol* symbol = static_cast<FieldSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr && node->ReturnType != nullptr)
	{
		symbol->ReturnType = ResolveType(node->ReturnType);
		if (symbol->ReturnType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field type not found: " + node->ReturnType->ToString());
		}
	}

	VisitType(node->ReturnType);
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void TypeBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = static_cast<PropertySymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr && node->ReturnType != nullptr)
	{
		// Resolve property return type
		TypeSymbol* propertyType = ResolveType(node->ReturnType);
		symbol->ReturnType = propertyType;
		
		if (propertyType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Property type not found: " + node->ReturnType->ToString());
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
		}
		
		// Resolve setter parameter type
		if (symbol->SetMethod != nullptr && !symbol->SetMethod->Parameters.empty())
		{
			symbol->SetMethod->Parameters[0]->Type = propertyType;
		}
	}

	VisitType(node->ReturnType);
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void TypeBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* symbol = static_cast<VariableSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr && node->Type != nullptr)
	{
		symbol->Type = ResolveType(node->Type);
		if (symbol->Type == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Type not found: " + node->Type->ToString());
		}
	}

	VisitType(node->Type);
	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void TypeBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	VisitType(node->Type);
	VisitArgumentsList(node->Arguments);

	node->Symbol = ResolveType(node->Type);
	if (node->Symbol == nullptr)
	{
		Diagnostics.ReportError(node->NewToken, L"Type not found: " + node->Type->ToString());
	}
}

void TypeBinder::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (ExpressionSyntax* expression : node->ValuesExpressions)
		VisitExpression(expression);
}

static bool IsScopePublicallyAccessible(const SemanticScope* scope)
{
	if (scope == nullptr)
		return false;

	if (scope->Owner->Accesibility != SymbolAccesibility::Public)
		return false;

	return IsScopePublicallyAccessible(scope->Parent);
}

static bool IsScopeNestedAccessible(const SemanticScope* scope, SyntaxSymbol* symbol)
{
	if (scope == nullptr)
		return false;

	if (scope->Owner->Kind == SyntaxKind::NamespaceDeclaration)
		return true;

	if (scope->Owner == symbol->Parent)
		return true;

	return IsScopeNestedAccessible(scope->Parent, symbol);
}

bool TypeBinder::IsSymbolAccessible(SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		throw runtime_error("Cannot resolve nullptr symbol accessibility");

	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
		return true;

	if (symbol->Parent == nullptr)
		throw runtime_error("Cannot resolve symbol accessibility without parent");

	if (IsScopePublicallyAccessible(CurrentScope()))
		return symbol->Accesibility == SymbolAccesibility::Public;

	if (IsScopeNestedAccessible(CurrentScope(), symbol))
		return true;

	return false;
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

		case SyntaxKind::ArrayType:
		{
			ArrayTypeSyntax* array = static_cast<ArrayTypeSyntax*>(typeSyntax);
			TypeSymbol* underlayingType = ResolveType(array->UnderlayingType);

			if (underlayingType == nullptr)
			{
				Diagnostics.ReportError(array->OpenSquareToken, L"Cannot resolve array's underlaying type");
				return nullptr;
			}

			ArrayTypeSymbol* symbol = new ArrayTypeSymbol(underlayingType, 0);
			symbol->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
			return symbol;
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

#ifdef DONT_CARE_RESOLVE_ALL
				for (TypeSymbol* type : Table->GetTypeSymbols())
				{
					if (type->Name == name)
					{
						symbol = type;
						break;
					}
				}
#else
				SemanticScope* currentScope = scopeStack.top();
				symbol = currentScope->Lookup(name);
#endif
				if (symbol == nullptr)
				{
					Diagnostics.ReportError(identifierType->Identifiers[0], L"Symbol wasnt found in current scope");
					return nullptr;
				}

				if (symbol->Kind != SyntaxKind::ClassDeclaration && symbol->Kind != SyntaxKind::StructDeclaration)
				{
					Diagnostics.ReportError(identifierType->Identifiers[0], L"Symbol is not a type");
					return nullptr;
				}
			}
			else
			{
				wstring firstName = identifierType->Identifiers[0].Word;
				SemanticScope* currentScope = CurrentScope();
				symbol = currentScope->Lookup(firstName);
				
				for (size_t i = 1; i < identifierType->Identifiers.size() - 1; i++)
				{
					std::wstring nextName = identifierType->Identifiers[i].Word;
					NamespaceSymbol* namespaceSymbol = static_cast<NamespaceSymbol*>(symbol);

					for (SyntaxSymbol* member : namespaceSymbol->Members)
					{
						if (member->Name != nextName)
							continue;
					
						if (member->Kind != SyntaxKind::NamespaceDeclaration)
						{
							Diagnostics.ReportError(identifierType->Identifiers[i], L"Symbol must be a namespace");
							return nullptr;
						}

						symbol = member;
						break;
					}

					if (symbol == nullptr)
					{
						Diagnostics.ReportError(identifierType->Identifiers[identifierType->Identifiers.size() - 1], L"Symbol is not a '" + namespaceSymbol->Name + L"'s member");
						return nullptr;
					}
				}

				if (symbol->Kind != SyntaxKind::ClassDeclaration && symbol->Kind != SyntaxKind::StructDeclaration)
				{
					Diagnostics.ReportError(identifierType->Identifiers[identifierType->Identifiers.size() - 1], L"Symbol is not a type");
					return nullptr;
				}
			}

			if (!IsSymbolAccessible(symbol))
			{
				Diagnostics.ReportError(identifierType->Identifiers[identifierType->Identifiers.size() - 1], L"Symbol inaccessible");
				return nullptr;
			}

			TypeSymbol* typeSymbol = static_cast<TypeSymbol*>(symbol);
			return typeSymbol;
		}

		default:
			return nullptr;
	}
}