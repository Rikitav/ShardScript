#include <shard/syntax/SymbolFactory.h>
#include <shard/syntax/SyntaxHelpers.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/nodes/Types/DelegateTypeSyntax.h>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/syntax/symbols/DelegateTypeSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/TypeParameterSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <sstream>
#include <algorithm>

using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

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

NamespaceSymbol* SymbolFactory::Namespace(NamespaceDeclarationSyntax* node)
{
	std::wstring namespaceName = node->IdentifierToken.Word;
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
	symbol->ReturnType = shard::parsing::semantic::SymbolTable::Primitives::Void;
	symbol->Accesibility = SymbolAccesibility::Public;
	return symbol;
}

AccessorSymbol* SymbolFactory::Accessor(const std::wstring& name, PropertySymbol* property, bool isGetter)
{
	AccessorSymbol* symbol = new AccessorSymbol(name);
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsStatic = property->IsStatic;
	symbol->ReturnType = isGetter ? property->ReturnType : shard::parsing::semantic::SymbolTable::Primitives::Void;
	
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

bool SymbolFactory::IsAccessible(SyntaxSymbol* symbol, SymbolAccesibility requiredAccessibility)
{
	if (symbol == nullptr)
		return false;

	// Public всегда доступен
	if (symbol->Accesibility == SymbolAccesibility::Public)
		return true;

	// Проверяем требуемый уровень доступа
	switch (requiredAccessibility)
	{
		case SymbolAccesibility::Public:
			return symbol->Accesibility == SymbolAccesibility::Public;

		case SymbolAccesibility::Protected:
			return symbol->Accesibility == SymbolAccesibility::Public ||
				   symbol->Accesibility == SymbolAccesibility::Protected;

		case SymbolAccesibility::Private:
			return true; // Private доступен в том же контексте

		default:
			return false;
	}
}

bool SymbolFactory::CanOverride(MethodSymbol* method, MethodSymbol* baseMethod)
{
	if (method == nullptr || baseMethod == nullptr)
		return false;

	// Метод должен быть виртуальным или абстрактным в базовом классе
	if (!baseMethod->IsVirtual && !baseMethod->IsAbstract)
		return false;

	// Сигнатуры должны совпадать
	if (method->Name != baseMethod->Name)
		return false;

	if (method->Parameters.size() != baseMethod->Parameters.size())
		return false;

	for (size_t i = 0; i < method->Parameters.size(); i++)
	{
		if (method->Parameters[i]->Type != baseMethod->Parameters[i]->Type)
			return false;
	}

	// Возвращаемый тип должен совпадать
	if (method->ReturnType != baseMethod->ReturnType)
		return false;

	return true;
}

bool SymbolFactory::IsCompatible(MethodSymbol* method1, MethodSymbol* method2)
{
	if (method1 == nullptr || method2 == nullptr)
		return false;

	if (method1->Name != method2->Name)
		return false;

	if (method1->Parameters.size() != method2->Parameters.size())
		return false;

	for (size_t i = 0; i < method1->Parameters.size(); i++)
	{
		if (method1->Parameters[i]->Type != method2->Parameters[i]->Type)
			return false;
	}

	return true;
}

bool SymbolFactory::ValidateModifiers(std::vector<SyntaxToken> modifiers, SyntaxKind symbolKind)
{
	// Базовая валидация - проверяем конфликтующие модификаторы
	bool hasPublic = false, hasPrivate = false, hasProtected = false;

	for (const SyntaxToken& modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
				hasPublic = true;
				break;
			case TokenType::PrivateKeyword:
				hasPrivate = true;
				break;
			case TokenType::ProtectedKeyword:
				hasProtected = true;
				break;
		}
	}

	// Нельзя иметь несколько модификаторов доступа одновременно
	int accessModifiers = (hasPublic ? 1 : 0) + (hasPrivate ? 1 : 0) + (hasProtected ? 1 : 0);
	if (accessModifiers > 1)
		return false;

	// Дополнительные проверки в зависимости от типа символа
	switch (symbolKind)
	{
		case SyntaxKind::MethodDeclaration:
		{
			bool hasStatic = false, hasAbstract = false, hasVirtual = false, hasOverride = false;
			for (const SyntaxToken& modifier : modifiers)
			{
				if (modifier.Type == TokenType::StaticKeyword) hasStatic = true;
				if (modifier.Type == TokenType::AbstractKeyword) hasAbstract = true;
				if (modifier.Type == TokenType::VirtualKeyword) hasVirtual = true;
				if (modifier.Type == TokenType::OverrideKeyword) hasOverride = true;
			}

			// Static и abstract/virtual/override несовместимы
			if (hasStatic && (hasAbstract || hasVirtual || hasOverride))
				return false;

			// Abstract и virtual/override несовместимы
			if (hasAbstract && (hasVirtual || hasOverride))
				return false;

			// Virtual и override несовместимы
			if (hasVirtual && hasOverride)
				return false;

			break;
		}

		case SyntaxKind::ClassDeclaration:
		case SyntaxKind::StructDeclaration:
		{
			bool hasStatic = false, hasAbstract = false, hasSealed = false;
			for (const SyntaxToken& modifier : modifiers)
			{
				if (modifier.Type == TokenType::StaticKeyword) hasStatic = true;
				if (modifier.Type == TokenType::AbstractKeyword) hasAbstract = true;
				if (modifier.Type == TokenType::SealedKeyword) hasSealed = true;
			}

			// Static и abstract/sealed несовместимы
			if (hasStatic && (hasAbstract || hasSealed))
				return false;

			// Abstract и sealed несовместимы
			if (hasAbstract && hasSealed)
				return false;

			break;
		}
	}

	return true;
}

SymbolAccesibility SymbolFactory::GetDefaultAccessibility(SyntaxKind symbolKind)
{
	switch (symbolKind)
	{
		case SyntaxKind::ClassDeclaration:
		case SyntaxKind::StructDeclaration:
		case SyntaxKind::NamespaceDeclaration:
		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::FieldDeclaration:
		case SyntaxKind::PropertyDeclaration:
		case SyntaxKind::ConstructorDeclaration:
			return SymbolAccesibility::Private;

		case SyntaxKind::Parameter:
		case SyntaxKind::VariableStatement:
		case SyntaxKind::TypeParameter:
			return SymbolAccesibility::Public;

		default:
			return SymbolAccesibility::Private;
	}
}

bool SymbolFactory::IsValidModifierCombination(std::vector<SyntaxToken> modifiers)
{
	// Проверяем базовые конфликты
	bool hasStatic = false, hasAbstract = false, hasVirtual = false, hasOverride = false, hasSealed = false;

	for (const SyntaxToken& modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::StaticKeyword:
				hasStatic = true;
				break;
			case TokenType::AbstractKeyword:
				hasAbstract = true;
				break;
			case TokenType::VirtualKeyword:
				hasVirtual = true;
				break;
			case TokenType::OverrideKeyword:
				hasOverride = true;
				break;
			case TokenType::SealedKeyword:
				hasSealed = true;
				break;
		}
	}

	// Static несовместим с abstract, virtual, override
	if (hasStatic && (hasAbstract || hasVirtual || hasOverride))
		return false;

	// Abstract несовместим с virtual, override
	if (hasAbstract && (hasVirtual || hasOverride))
		return false;

	// Virtual несовместим с override
	if (hasVirtual && hasOverride)
		return false;

	return true;
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
	MethodSymbol* symbol = new MethodSymbol(L"Lambda", body);
	symbol->ReturnType = shard::parsing::semantic::SymbolTable::Primitives::Any;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsStatic = true;
	symbol->HandleType = MethodHandleType::Lambda;
	return symbol;
}
