#include <shard/syntax/SymbolFactory.h>
#include <shard/syntax/SyntaxHelpers.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/nodes/Types/DelegateTypeSyntax.h>

#include <shard/syntax/symbols/DelegateTypeSymbol.h>

using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

StructSymbol* SymbolFactory::Struct(StructDeclarationSyntax* node)
{
	std::wstring structName = node->IdentifierToken.Word;
	StructSymbol* symbol = new StructSymbol(structName);
	SetAccesibility(symbol, node->Modifiers);

	return symbol;
}

ClassSymbol* SymbolFactory::Class(ClassDeclarationSyntax* node)
{
	std::wstring className = node->IdentifierToken.Word;
	ClassSymbol* symbol = new ClassSymbol(className);
	SetAccesibility(symbol, node->Modifiers);

	return symbol;
}

FieldSymbol* SymbolFactory::Field(FieldDeclarationSyntax* node)
{
	std::wstring fieldName = node->IdentifierToken.Word;
	FieldSymbol* symbol = new FieldSymbol(fieldName);
	symbol->DefaultValueExpression = node->InitializerExpression;
	SetAccesibility(symbol, node->Modifiers);

	return symbol;
}

PropertySymbol* SymbolFactory::Property(PropertyDeclarationSyntax* node)
{
	std::wstring propertyName = node->IdentifierToken.Word;
	PropertySymbol* symbol = new PropertySymbol(propertyName);
	SetAccesibility(symbol, node->Modifiers);
	symbol->DefaultValueExpression = node->InitializerExpression;

	// Create backing field for auto-properties
	bool isAutoProperty =
		(node->Getter != nullptr && node->Getter->Body == nullptr) ||
		(node->Setter != nullptr && node->Setter->Body == nullptr);

	if (isAutoProperty)
		symbol->GenerateBackingField();

	return symbol;
}

AccessorSymbol* SymbolFactory::Accessor(AccessorDeclarationSyntax* node, PropertySymbol* propertySymbol, bool setProperty)
{
	std::wstring accessorName = propertySymbol->Name + L"_" + node->KeywordToken.Word;
	AccessorSymbol* symbol = new AccessorSymbol(accessorName);
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsStatic = propertySymbol->IsStatic;
	SetAccesibility(symbol, node->Modifiers);

	if (symbol->IsExtern)
		symbol->HandleType = MethodHandleType::External;

	if (setProperty)
	{
		switch (node->KeywordToken.Type)
		{
			case TokenType::GetKeyword:
			{
				propertySymbol->Getter = symbol;
				break;
			}

			case TokenType::SetKeyword:
			{
				propertySymbol->Setter = symbol;
				break;
			}
		}
	}

	return symbol;
}

MethodSymbol* SymbolFactory::Method(MethodDeclarationSyntax* node)
{
    std::wstring methodName = node->IdentifierToken.Word;
    MethodSymbol* symbol = new MethodSymbol(methodName, node->Body);
    SetAccesibility(symbol, node->Modifiers);

	if (symbol->IsExtern)
		symbol->HandleType = MethodHandleType::External;

    for (ParameterSyntax* parameter : node->Params->Parameters)
    {
        ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
        symbol->Parameters.push_back(paramSymbol);
    }

    return symbol;
}

ConstructorSymbol* SymbolFactory::Constructor(ConstructorDeclarationSyntax* node)
{
	std::wstring methodName = node->IdentifierToken.Word;
	ConstructorSymbol* symbol = new ConstructorSymbol(methodName, node->Body);
	symbol->ReturnType = shard::parsing::semantic::SymbolTable::Primitives::Void;
	SetAccesibility(symbol, node->Modifiers);

	if (symbol->IsExtern)
		symbol->HandleType = MethodHandleType::External;

	for (ParameterSyntax* parameter : node->Params->Parameters)
	{
		ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
		symbol->Parameters.push_back(paramSymbol);
	}

	return symbol;
}

DelegateTypeSymbol* SymbolFactory::Delegate(DelegateDeclarationSyntax* node)
{
	MethodSymbol* anonymousMethod = new MethodSymbol(L"");
	anonymousMethod->HandleType = MethodHandleType::Lambda;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = node->ReturnType->Symbol;
	anonymousMethod->IsStatic = true;

	std::wstring delegateName = node->IdentifierToken.Word;
	DelegateTypeSymbol* symbol = new DelegateTypeSymbol(delegateName);
	symbol->ReturnType = node->ReturnType->Symbol;
	symbol->AnonymousSymbol = anonymousMethod;
	SetAccesibility(symbol, node->Modifiers);

	return symbol;
}

DelegateTypeSymbol* SymbolFactory::Delegate(DelegateTypeSyntax* node)
{
	// Anonymous method symbol
	MethodSymbol* anonymousMethod = new MethodSymbol(L"Delegate");
	anonymousMethod->HandleType = MethodHandleType::Lambda;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = node->ReturnType->Symbol;
	anonymousMethod->IsStatic = true;

	// Delegate symbol
	DelegateTypeSymbol* symbol = new DelegateTypeSymbol(L"Delegate");
	symbol->ReturnType = node->ReturnType->Symbol;
	symbol->AnonymousSymbol = anonymousMethod;

	node->Symbol = symbol;
	return symbol;
}

DelegateTypeSymbol* SymbolFactory::Delegate(MethodSymbol* method)
{
	DelegateTypeSymbol* delegate = new DelegateTypeSymbol(method->Name);
	delegate->Accesibility = SymbolAccesibility::Public;
	delegate->AnonymousSymbol = method;
	delegate->Parameters = method->Parameters;
	delegate->ReturnType = method->ReturnType;

	return delegate;
}
