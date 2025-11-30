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
#include <shard/syntax/symbols/DelegateTypeSymbol.h>

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

void TypeBinder::VisitImportDirective(ImportDirectiveSyntax* node)
{
	VisitType(node->ReturnType);
	VisitParametersList(node->Params);

	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node);
	if (symbol != nullptr)
	{
		if (node->ReturnType != nullptr)
			symbol->ReturnType = node->ReturnType->Symbol;

		if (node->Params != nullptr)
		{
			if (node->Params->Parameters.size() == symbol->Parameters.size())
			{
				for (size_t i = 0; i < node->Params->Parameters.size(); i++)
				{
					ParameterSyntax* paramSyntax = node->Params->Parameters[i];
					ParameterSymbol* paramSymbol = symbol->Parameters[i];

					if (paramSyntax->Type->Symbol != nullptr)
					{
						paramSymbol->Type = paramSyntax->Type->Symbol;
					}
				}
			}
		}
	}
}

void TypeBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	NamespaceSymbol* symbol = LookupSymbol<NamespaceSymbol>(node);
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
	ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node);
	if (symbol != nullptr)
	{
		//Declare(symbol);
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

void TypeBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = LookupSymbol<StructSymbol>(node);
	if (symbol != nullptr)
	{
		//Declare(symbol);
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

void TypeBinder::VisitDelegateDeclaration(DelegateDeclarationSyntax* node)
{
	DelegateTypeSymbol* symbol = LookupSymbol<DelegateTypeSymbol>(node);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		VisitType(node->ReturnType);
		symbol->ReturnType = node->ReturnType->Symbol;
		symbol->AnonymousSymbol->ReturnType = node->ReturnType->Symbol;

		for (ParameterSyntax* param : node->Params->Parameters)
		{
			VisitParameter(param);
			ParameterSymbol* paramSymbol = LookupSymbol<ParameterSymbol>(param);
			paramSymbol->Type = param->Type->Symbol;

			symbol->Parameters.push_back(paramSymbol);
			symbol->AnonymousSymbol->Parameters.push_back(paramSymbol);
		}

		PopScope();
	}

}

void TypeBinder::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node);
	if (symbol != nullptr)
	{
		if (node->Params != nullptr)
		{
			if (node->Params->Parameters.size() != symbol->Parameters.size())
			{
				for (size_t i = 0; i < node->Params->Parameters.size(); i++)
				{
					ParameterSyntax* paramSyntax = node->Params->Parameters[i];
					ParameterSymbol* paramSymbol = symbol->Parameters[i];

					if (paramSyntax->Type != nullptr)
					{
						paramSymbol->Type = paramSyntax->Type->Symbol;
						if (paramSymbol->Type == nullptr)
						{
							//Diagnostics.ReportError(paramSyntax->Identifier, L"Parameter type not found: " + const_cast<TypeSyntax*>(paramSyntax->Type)->ToString());
						}
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
	VisitType(node->ReturnType);
	VisitParametersList(node->Params);

	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node);
	if (symbol != nullptr)
	{
		if (node->ReturnType != nullptr)
			symbol->ReturnType = node->ReturnType->Symbol;

		if (node->Params != nullptr)
		{
			if (node->Params->Parameters.size() == symbol->Parameters.size())
			{
				for (size_t i = 0; i < node->Params->Parameters.size(); i++)
				{
					ParameterSyntax* paramSyntax = node->Params->Parameters[i];
					ParameterSymbol* paramSymbol = symbol->Parameters[i];

					if (paramSyntax->Type != nullptr)
					{
						paramSymbol->Type = paramSyntax->Type->Symbol;
						if (paramSymbol->Type == nullptr)
						{
							//Diagnostics.ReportError(paramSyntax->Identifier, L"Parameter type not found: " + const_cast<TypeSyntax*>(paramSyntax->Type)->ToString());
						}
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
	VisitType(node->ReturnType);
	FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node);
	if (symbol != nullptr && node->ReturnType != nullptr)
	{
		symbol->ReturnType = node->ReturnType->Symbol;
	}

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void TypeBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	VisitType(node->ReturnType);
	PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node);
	if (symbol != nullptr && node->ReturnType != nullptr)
	{
		// Resolve property return type
		TypeSymbol* propertyType = node->ReturnType->Symbol;
		symbol->ReturnType = propertyType;
		
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

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Getter);

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression);
}

void TypeBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VisitType(node->Type);

	VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node);
	symbol->Type = node->Type->Symbol;

	if (node->Expression != nullptr)
		VisitExpression(node->Expression);
}

void TypeBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	VisitType(node->Type);
	node->TypeSymbol = node->Type->Symbol;
	VisitArgumentsList(node->ArgumentsList);
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

void TypeBinder::VisitParameter(ParameterSyntax* node)
{
	TypeSyntax* type = const_cast<TypeSyntax*>(node->Type);
	VisitType(type);

	ParameterSymbol* paramSymbol = new ParameterSymbol(node->Identifier.Word);
	Table->BindSymbol(node, paramSymbol);

	node->Symbol = paramSymbol->Type = type->Symbol;
	if (node->Symbol == nullptr)
		Diagnostics.ReportError(node->Identifier, L"Parameter type not found: " + type->ToString());
}

void TypeBinder::VisitPredefinedType(PredefinedTypeSyntax* node)
{
	switch (node->TypeToken.Type)
	{
		case TokenType::BooleanKeyword:
		{
			node->Symbol = SymbolTable::Primitives::Boolean;
			break;
		}

		case TokenType::IntegerKeyword:
		{
			node->Symbol = SymbolTable::Primitives::Integer;
			break;
		}

		case TokenType::CharKeyword:
		{
			node->Symbol = SymbolTable::Primitives::Char;
			break;
		}

		case TokenType::StringKeyword:
		{
			node->Symbol = SymbolTable::Primitives::String;
			break;
		}

		case TokenType::VoidKeyword:
		{
			node->Symbol = SymbolTable::Primitives::Void;
			break;
		}

		case TokenType::VarKeyword:
		{
			node->Symbol = SymbolTable::Primitives::Any;
			break;
		}

		default:
		{
			Diagnostics.ReportError(node->TypeToken, L"Unknown primitive : " + node->TypeToken.Word);
			break;
		}
	}
}

void TypeBinder::VisitIdentifierNameType(IdentifierNameTypeSyntax* node)
{
	std::wstring name = node->Identifier.Word;
	SemanticScope* currentScope = CurrentScope();
	SyntaxSymbol* symbol = currentScope->Lookup(name);

	if (symbol == nullptr)
	{
		Diagnostics.ReportError(node->Identifier, L"Symbol wasnt found in current scope");
		return;
	}

	if (!symbol->IsType())
	{
		Diagnostics.ReportError(node->Identifier, L"Symbol is not a type");
		return;
	}

	if (!IsSymbolAccessible(symbol))
	{
		Diagnostics.ReportError(node->Identifier, L"Symbol inaccessible");
		return;
	}

	TypeSymbol* typeSymbol = static_cast<TypeSymbol*>(symbol);
	//Table->BindSymbol(typeSyntax, typeSymbol);
	node->Symbol = typeSymbol;
}

void TypeBinder::VisitArrayType(ArrayTypeSyntax* node)
{
	if (node->UnderlayingType == nullptr)
		return;

	VisitType(node->UnderlayingType);
	TypeSymbol* underlayingType = node->UnderlayingType->Symbol;

	if (underlayingType == nullptr)
		return;

	ArrayTypeSymbol* symbol = new ArrayTypeSymbol(underlayingType);
	symbol->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	
	Table->BindSymbol(node, symbol);
	node->Symbol = symbol;
}

void TypeBinder::VisitNullableType(NullableTypeSyntax* node)
{
	if (node->UnderlayingType == nullptr)
		return;

	VisitType(node->UnderlayingType);
	TypeSymbol* underlayingType = node->UnderlayingType->Symbol;
	
	if (underlayingType == nullptr)
		return;

	//NullableTypeSymbol* symbol = new NullableTypeSymbol(node->UnderlayingType->Symbol);
	//Table->BindSymbol(node, symbol);
	//node->Symbol = symbol;
}

void TypeBinder::VisitGenericType(GenericTypeSyntax* node)
{
	return; // unsupported

	/*
	VisitType(node->UnderlayingType);
	for (TypeSyntax* type : node->TypeArguments)
		VisitType(type);
	*/

	if (node->UnderlayingType == nullptr)
		return;
	
	VisitType(node->UnderlayingType);
	TypeSymbol* underlayingType = node->UnderlayingType->Symbol;

	if (underlayingType == nullptr)
		return;

	GenericTypeSymbol* symbol = new GenericTypeSymbol(underlayingType);
	Table->BindSymbol(node, symbol);
	node->Symbol = symbol;

	size_t argsCount = node->TypeArguments.size();
	size_t paramsCount = underlayingType->TypeParameters.size();

	if (argsCount != paramsCount)
	{
		Diagnostics.ReportError(node->OpenListToken, L"\'" + underlayingType->FullName + L" requires " + std::to_wstring(paramsCount) + L" type arguments");
		return;
	}

	for (TypeSyntax* typeArg : node->TypeArguments)
	{
		VisitType(typeArg);
		TypeSymbol* typeArgSymbol = typeArg->Symbol;
	}

	// under construction
}

void TypeBinder::VisitDelegateType(DelegateTypeSyntax* node)
{
	VisitType(node->ReturnType);
	VisitParametersList(node->Params);

	MethodSymbol* anonymousMethod = new MethodSymbol(L"");
	anonymousMethod->HandleType = MethodHandleType::AnonymousMethod;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = node->ReturnType->Symbol;
	anonymousMethod->IsStatic = true;

	DelegateTypeSymbol* symbol = new DelegateTypeSymbol();
	symbol->ReturnType = node->ReturnType->Symbol;
	symbol->AnonymousSymbol = anonymousMethod;

	for (ParameterSyntax* param : node->Params->Parameters)
	{
		VisitParameter(param);
		ParameterSymbol* paramSymbol = LookupSymbol<ParameterSymbol>(param);
		paramSymbol->Type = param->Type->Symbol;

		symbol->Parameters.push_back(paramSymbol);
		anonymousMethod->Parameters.push_back(paramSymbol);
	}

	node->Symbol = symbol;
	Table->BindSymbol(node, symbol);
}
