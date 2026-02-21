#include <shard/syntax/SymbolFactory.hpp>
#include <shard/syntax/SyntaxHelpers.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <sstream>
#include <algorithm>

using namespace shard;

void SymbolFactory::SetAccesibility(SyntaxSymbol* symbol, std::vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}

			case TokenType::ExternKeyword:
			{
				symbol->IsExtern = true;
				break;
			}
		}
	}
}

void SymbolFactory::SetAccesibility(TypeSymbol* symbol, std::vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}

			case TokenType::StaticKeyword:
			{
				symbol->IsStatic = true;
				break;
			}
			
			/*
			case TokenType::AbstractKeyword:
			{
				symbol->IsAbstract = true;
				break;
			}

			case TokenType::SealedKeyword:
			{
				symbol->IsSealed = true;
				break;
			}
			*/

			case TokenType::ExternKeyword:
			{
				symbol->IsExtern = true;
				break;
			}
		}
	}
}

void SymbolFactory::SetAccesibility(MethodSymbol* symbol, std::vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}

			case TokenType::StaticKeyword:
			{
				symbol->IsStatic = true;
				break;
			}

			/*
			case TokenType::AbstractKeyword:
			{
				symbol->IsAbstract = true;
				break;
			}

			case TokenType::OverrideKeyword:
			{
				symbol->IsOverride = true;
				break;
			}

			case TokenType::VirtualKeyword:
			{
				symbol->IsVirtual = true;
				break;
			}
			*/

			case TokenType::ExternKeyword:
			{
				symbol->IsExtern = true;
				break;
			}
		}
	}
}

void SymbolFactory::SetAccesibility(PropertySymbol* symbol, std::vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}

			case TokenType::StaticKeyword:
			{
				symbol->IsStatic = true;
				break;
			}

			case TokenType::ExternKeyword:
			{
				symbol->IsExtern = true;
				break;
			}

			/*
			case TokenType::OverrideKeyword:
			{
				symbol->IsOverride = true;
				break;
			}

			case TokenType::VirtualKeyword:
			{
				symbol->IsVirtual = true;
				break;
			}
			*/
		}
	}
}

void SymbolFactory::SetAccesibility(FieldSymbol* symbol, std::vector<SyntaxToken> modifiers)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Private;
				break;
			}

			/*
			case TokenType::ProtectedKeyword:
			{
				symbol->Accesibility = SymbolAccesibility::Protected;
				break;
			}
			*/

			case TokenType::StaticKeyword:
			{
				symbol->IsStatic = true;
				break;
			}

			case TokenType::ExternKeyword:
			{
				symbol->IsExtern = true;
				break;
			}
		}
	}
}

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
	else
		symbol->HandleType = MethodHandleType::Body;

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
    MethodSymbol* symbol = new MethodSymbol(methodName);
    SetAccesibility(symbol, node->Modifiers);

	if (symbol->IsExtern)
		symbol->HandleType = MethodHandleType::External;
	else
		symbol->HandleType = MethodHandleType::Body;

	if (!symbol->IsStatic)
	{
		// implicit 'this' parameter
		symbol->EvalStackLocalsCount += 1;
	}

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
	ConstructorSymbol* symbol = new ConstructorSymbol(methodName);
	symbol->ReturnType = shard::SymbolTable::Primitives::Void;
	SetAccesibility(symbol, node->Modifiers);

	if (symbol->IsExtern)
		symbol->HandleType = MethodHandleType::External;
	else
		symbol->HandleType = MethodHandleType::Body;

	if (!symbol->IsStatic)
	{
		// implicit 'this' parameter
		symbol->EvalStackLocalsCount += 1;
	}

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

NamespaceSymbol* SymbolFactory::Namespace(NamespaceDeclarationSyntax* node)
{
	std::wstring namespaceName = node->IdentifierTokens.at(0).Word;
	for (int i = 1; i < node->IdentifierTokens.size(); i++)
		namespaceName += L"." + node->IdentifierTokens.at(i).Word;

	NamespaceSymbol* symbol = new NamespaceSymbol(namespaceName);
	SetAccesibility(symbol, node->Modifiers);
	return symbol;
}

NamespaceSymbol* SymbolFactory::Namespace(const std::wstring& name)
{
	return new NamespaceSymbol(name);
}

FieldSymbol* SymbolFactory::Field(const std::wstring& name, TypeSymbol* type, bool isStatic)
{
	FieldSymbol* symbol = new FieldSymbol(name);
	symbol->ReturnType = type;
	symbol->IsStatic = isStatic;
	symbol->Accesibility = SymbolAccesibility::Private;
	return symbol;
}

PropertySymbol* SymbolFactory::Property(const std::wstring& name, TypeSymbol* returnType, bool isStatic)
{
	PropertySymbol* symbol = new PropertySymbol(name);
	symbol->ReturnType = returnType;
	symbol->IsStatic = isStatic;
	symbol->Accesibility = SymbolAccesibility::Private;
	return symbol;
}

MethodSymbol* SymbolFactory::Method(const std::wstring& name, TypeSymbol* returnType, bool isStatic)
{
	MethodSymbol* symbol = new MethodSymbol(name);
	symbol->ReturnType = returnType;
	symbol->IsStatic = isStatic;
	symbol->Accesibility = SymbolAccesibility::Private;
	return symbol;
}

ConstructorSymbol* SymbolFactory::Constructor(const std::wstring& name)
{
	ConstructorSymbol* symbol = new ConstructorSymbol(name);
	symbol->ReturnType = shard::SymbolTable::Primitives::Void;
	symbol->Accesibility = SymbolAccesibility::Public;
	return symbol;
}

AccessorSymbol* SymbolFactory::Accessor(const std::wstring& name, PropertySymbol* property, bool isGetter)
{
	AccessorSymbol* symbol = new AccessorSymbol(name);
	symbol->HandleType = MethodHandleType::Body;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsStatic = property->IsStatic;
	symbol->ReturnType = isGetter ? property->ReturnType : shard::SymbolTable::Primitives::Void;

	if (!symbol->IsStatic)
	{
		// implicit 'this' parameter
		symbol->EvalStackLocalsCount += 1;
	}

	if (isGetter)
		property->Getter = symbol;
	else
		property->Setter = symbol;
	
	return symbol;
}

AccessorSymbol* SymbolFactory::Getter(const std::wstring& propertyName, PropertySymbol* property)
{
	std::wstring getterName = L"get_" + propertyName;
	return Accessor(getterName, property, true);
}

AccessorSymbol* SymbolFactory::Setter(const std::wstring& propertyName, PropertySymbol* property)
{
	std::wstring setterName = L"set_" + propertyName;
	return Accessor(setterName, property, false);
}

IndexatorSymbol* SymbolFactory::Indexator(IndexatorDeclarationSyntax* node)
{
	IndexatorSymbol* symbol = new IndexatorSymbol(L"index"); // Name is always "index"
	SetAccesibility(symbol, node->Modifiers);

	for (ParameterSyntax* parameter : node->Parameters->Parameters)
	{
		ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
		symbol->Parameters.push_back(paramSymbol);
	}

	return symbol;
}

IndexatorSymbol* SymbolFactory::Indexator(const std::wstring& name, TypeSymbol* returnType)
{
	IndexatorSymbol* symbol = new IndexatorSymbol(name);
	symbol->ReturnType = returnType;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsStatic = false;
	return symbol;
}

IndexatorSymbol* SymbolFactory::Indexator(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*> parameters)
{
	IndexatorSymbol* symbol = Indexator(name, returnType);
	symbol->Parameters = parameters;
	return symbol;
}

ParameterSymbol* SymbolFactory::Parameter(const std::wstring& name)
{
	return new ParameterSymbol(name);
}

ParameterSymbol* SymbolFactory::Parameter(const std::wstring& name, TypeSymbol* type)
{
	return new ParameterSymbol(name, type);
}

ParameterSymbol* SymbolFactory::Parameter(const std::wstring& name, TypeSymbol* type, bool isOptional)
{
	ParameterSymbol* symbol = new ParameterSymbol(name, type);
	symbol->IsOptional = isOptional;
	return symbol;
}

VariableSymbol* SymbolFactory::Variable(const std::wstring& name, TypeSymbol* type)
{
	return new VariableSymbol(name, type);
}

VariableSymbol* SymbolFactory::Variable(const std::wstring& name, TypeSymbol* type, bool isConst)
{
	VariableSymbol* symbol = new VariableSymbol(name, type);
	symbol->IsConst = isConst;
	return symbol;
}

TypeParameterSymbol* SymbolFactory::TypeParameter(const std::wstring& name)
{
	return new TypeParameterSymbol(name);
}

DelegateTypeSymbol* SymbolFactory::Delegate(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*> parameters)
{
	MethodSymbol* anonymousMethod = new MethodSymbol(L"");
	anonymousMethod->HandleType = MethodHandleType::Lambda;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = returnType;
	anonymousMethod->IsStatic = true;
	anonymousMethod->Parameters = parameters;

	DelegateTypeSymbol* symbol = new DelegateTypeSymbol(name);
	symbol->ReturnType = returnType;
	symbol->AnonymousSymbol = anonymousMethod;
	symbol->Parameters = parameters;
	symbol->Accesibility = SymbolAccesibility::Public;

	return symbol;
}

ArrayTypeSymbol* SymbolFactory::Array(ArrayTypeSyntax* node)
{
	if (node == nullptr || node->UnderlayingType == nullptr || node->UnderlayingType->Symbol == nullptr)
		return nullptr;

	ArrayTypeSymbol* symbol = new ArrayTypeSymbol(node->UnderlayingType->Symbol);
	symbol->Rank = node->Rank;
	return symbol;
}

ArrayTypeSymbol* SymbolFactory::Array(TypeSymbol* underlayingType)
{
	return new ArrayTypeSymbol(underlayingType);
}

ArrayTypeSymbol* SymbolFactory::Array(TypeSymbol* underlayingType, size_t size)
{
	ArrayTypeSymbol* symbol = new ArrayTypeSymbol(underlayingType);
	symbol->Size = size;
	return symbol;
}

ArrayTypeSymbol* SymbolFactory::Array(TypeSymbol* underlayingType, size_t size, int rank)
{
	ArrayTypeSymbol* symbol = new ArrayTypeSymbol(underlayingType);
	symbol->Size = size;
	symbol->Rank = rank;
	return symbol;
}

GenericTypeSymbol* SymbolFactory::GenericType(TypeSymbol* underlayingType)
{
	return new GenericTypeSymbol(underlayingType);
}

GenericTypeSymbol* SymbolFactory::GenericType(TypeSymbol* underlayingType, std::unordered_map<std::wstring, TypeSymbol*> typeArguments)
{
	GenericTypeSymbol* symbol = new GenericTypeSymbol(underlayingType);
	
	// Заполняем маппинг type parameters -> type arguments
	for (size_t i = 0; i < underlayingType->TypeParameters.size(); i++)
	{
		TypeParameterSymbol* typeParam = underlayingType->TypeParameters[i];
		auto it = typeArguments.find(typeParam->Name);
		if (it != typeArguments.end())
		{
			symbol->AddTypeParameter(typeParam, it->second);
		}
	}
	
	return symbol;
}

std::wstring SymbolFactory::FormatFullName(SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		return L"";

	if (symbol->FullName.empty())
	{
		if (symbol->Parent != nullptr)
		{
			std::wstring parentName = FormatFullName(symbol->Parent);
			return parentName.empty() ? symbol->Name : parentName + L"." + symbol->Name;
		}
		return symbol->Name;
	}

	return symbol->FullName;
}

std::wstring SymbolFactory::FormatFullName(SyntaxSymbol* symbol, SyntaxSymbol* parent)
{
	if (symbol == nullptr)
		return L"";

	if (parent != nullptr)
	{
		std::wstring parentName = FormatFullName(parent);
		return parentName.empty() ? symbol->Name : parentName + L"." + symbol->Name;
	}

	return symbol->Name;
}

std::wstring SymbolFactory::FormatMethodSignature(MethodSymbol* method)
{
	if (method == nullptr)
		return L"";

	std::wostringstream signature;
	signature << method->Name << L"(";

	for (size_t i = 0; i < method->Parameters.size(); i++)
	{
		if (i > 0)
			signature << L", ";

		ParameterSymbol* param = method->Parameters[i];
		if (param->Type != nullptr)
			signature << param->Type->Name;
		else
			signature << L"<unknown>";
	}

	signature << L")";
	return signature.str();
}

std::wstring SymbolFactory::FormatTypeName(TypeSymbol* type)
{
	if (type == nullptr)
		return L"<unknown>";

	if (type->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(type);
		std::wostringstream name;
		name << genericType->UnderlayingType->Name << L"<";

		// Добавляем type arguments если есть
		bool first = true;
		for (TypeParameterSymbol* typeParam : genericType->UnderlayingType->TypeParameters)
		{
			if (!first)
				name << L", ";
			first = false;

			TypeSymbol* argType = genericType->SubstituteTypeParameters(typeParam);
			if (argType != nullptr)
				name << argType->Name;
			else
				name << typeParam->Name;
		}

		name << L">";
		return name.str();
	}

	return type->Name;
}

MethodSymbol* SymbolFactory::CreateAnonymousMethod(const std::wstring& name, TypeSymbol* returnType)
{
	MethodSymbol* symbol = new MethodSymbol(name);
	symbol->ReturnType = returnType;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsStatic = true;
	symbol->HandleType = MethodHandleType::Lambda;
	return symbol;
}

MethodSymbol* SymbolFactory::CreateLambdaMethod(StatementsBlockSyntax* body)
{
	MethodSymbol* symbol = new MethodSymbol(std::wstring(L"Lambda"));
	symbol->ReturnType = shard::SymbolTable::Primitives::Any;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsStatic = true;
	symbol->HandleType = MethodHandleType::Lambda;
	return symbol;
}
