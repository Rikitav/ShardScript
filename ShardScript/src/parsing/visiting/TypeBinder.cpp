#include <shard/parsing/visiting/TypeBinder.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxToken.h>

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

#include <shard/syntax/nodes/TypeSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>

#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>

#include <vector>
#include <string>
#include <shard/syntax/symbols/GenericTypeSymbol.h>

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

void TypeBinder::VisitUsingDirective(UsingDirectiveSyntax* node)
{
	NamespaceNode* nsNode = Namespaces->Root;
	for (SyntaxToken token : node->TokensList)
	{
		NamespaceNode* nextNsNode = nsNode->Lookup(token.Word);
		if (nextNsNode == nullptr)
		{
			Diagnostics.ReportError(token, L"Identifier \'" + token.Word + L"\' doesnt exists in namespace \'" + nsNode->Owners.at(0)->FullName + L"\'");
			return;
		}

		nsNode = nextNsNode;
		continue;
	}

	node->Namespace = nsNode;
	SemanticScope* current = CurrentScope();
	for (const auto& symbol : nsNode->Types)
		current->DeclareSymbol(symbol);
}

void TypeBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	NamespaceSymbol* symbol = static_cast<NamespaceSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		Declare(symbol);
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

void TypeBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = static_cast<ClassSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
		//Declare(symbol);
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
		//Declare(symbol);
		PushScope(symbol);

		for (MemberDeclarationSyntax* member : node->Members)
			VisitMemberDeclaration(member);

		PopScope();
	}
}

void TypeBinder::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	MethodSymbol* symbol = static_cast<MethodSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
	{
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
						Diagnostics.ReportError(paramSyntax->Identifier, L"Parameter type not found: " + const_cast<TypeSyntax*>(paramSyntax->Type)->ToString());
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
						Diagnostics.ReportError(paramSyntax->Identifier, L"Parameter type not found: " + const_cast<TypeSyntax*>(paramSyntax->Type)->ToString());
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
			symbol->BackingField->ReturnType = propertyType;
		
		// Resolve getter return type
		if (symbol->Getter != nullptr && symbol->Getter->Method != nullptr)
			symbol->Getter->Method->ReturnType = propertyType;
		
		// Resolve setter parameter type
		if (symbol->Setter != nullptr && symbol->Setter->Method != nullptr && !symbol->Setter->Method->Parameters.empty())
			symbol->Setter->Method->Parameters[0]->Type = propertyType;
	}

	VisitType(node->ReturnType);
	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Getter);

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void TypeBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* symbol = static_cast<VariableSymbol*>(Table->LookupSymbol(node));
	if (symbol != nullptr)
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
	VisitArgumentsList(node->ArgumentsList);

	node->TypeSymbol = ResolveType(node->Type);
	if (node->TypeSymbol == nullptr)
	{
		Diagnostics.ReportError(node->NewToken, L"Type not found: " + node->Type->ToString());
	}
}

/*
void TypeBinder::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (ExpressionSyntax* expression : node->ValuesExpressions)
		VisitExpression(expression);
}

void TypeBinder::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
{

}
*/

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

				case TokenType::VarKeyword:
					return SymbolTable::Primitives::Any;

				default:
					return nullptr;
			}
		}

		case SyntaxKind::ArrayType:
		{
			ArrayTypeSyntax* array = static_cast<ArrayTypeSyntax*>(typeSyntax);
			if (array->UnderlayingType == nullptr)
				return nullptr;

			TypeSymbol* underlayingType = ResolveType(array->UnderlayingType);
			if (underlayingType == nullptr)
				return nullptr;

			ArrayTypeSymbol* symbol = new ArrayTypeSymbol(underlayingType, 0);
			symbol->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
			Table->BindSymbol(typeSyntax, symbol);
			return symbol;
		}

		case SyntaxKind::IdentifierNameType:
		{
			IdentifierNameTypeSyntax* identifierType = static_cast<IdentifierNameTypeSyntax*>(typeSyntax);
			std::wstring name = identifierType->Identifier.Word;

			SemanticScope* currentScope = CurrentScope();
			SyntaxSymbol* symbol = currentScope->Lookup(name);

			if (symbol == nullptr)
			{
				Diagnostics.ReportError(identifierType->Identifier, L"Symbol wasnt found in current scope");
				return nullptr;
			}

			if (!symbol->IsType())
			{
				Diagnostics.ReportError(identifierType->Identifier, L"Symbol is not a type");
				return nullptr;
			}

			if (!IsSymbolAccessible(symbol))
			{
				Diagnostics.ReportError(identifierType->Identifier, L"Symbol inaccessible");
				return nullptr;
			}

			TypeSymbol* typeSymbol = static_cast<TypeSymbol*>(symbol);
			//Table->BindSymbol(typeSyntax, typeSymbol);
			return typeSymbol;
		}

		case SyntaxKind::GenericType:
		{
			/*
			GenericTypeSyntax* genericType = static_cast<GenericTypeSyntax*>(typeSyntax);
			TypeSymbol* underlayingType = ResolveType(genericType->UnderlayingType);

			GenericTypeSymbol* symbol = new GenericTypeSymbol(genericType);
			Table->BindSymbol(typeSyntax, symbol);

			size_t argsCount = genericType->TypeArguments.size();
			size_t paramsCount = underlayingType->TypeParameters.size();

			if (argsCount != paramsCount)
			{
				Diagnostics.ReportError(genericType->OpenListToken, L"\'" + underlayingType->FullName + L" requires " + paramsCount + " type arguments");
				return nullptr;
			}

			for (TypeSyntax* typeArg : genericType->TypeArguments)
			{
				TypeSymbol* typeArgSymbol = ResolveType(typeArg);
			}

			return symbol;
			//*/

			return nullptr;
		}

		default:
			return nullptr;
	}
}