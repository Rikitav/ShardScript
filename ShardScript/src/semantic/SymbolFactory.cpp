#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/lexical/TokenType.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/parsing/nodes/Types/DelegateTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>

#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <memory>
#include <sstream>
#include <algorithm>

using namespace shard;

void SymbolFactory::SetAccesibility(std::vector<SyntaxToken> modifiers, SymbolAccesibility& accesibility, SymbolLinking& linking)
{
	for (SyntaxToken modifier : modifiers)
	{
		switch (modifier.Type)
		{
			case TokenType::PublicKeyword:
			{
				accesibility = SymbolAccesibility::Public;
				break;
			}

			case TokenType::PrivateKeyword:
			{
				accesibility = SymbolAccesibility::Private;
				break;
			}

			case TokenType::StaticKeyword:
			{
				linking = LINK_STATIC;
				break;
			}
		}
	}
}

StructSymbol* SymbolFactory::Struct(StructDeclarationSyntax* node)
{
	std::wstring structName = node->IdentifierToken.Word;
	auto symbol = std::make_unique<StructSymbol>(structName);
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);

	return static_cast<StructSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

ClassSymbol* SymbolFactory::Class(ClassDeclarationSyntax* node)
{
	std::wstring className = node->IdentifierToken.Word;
	auto symbol = std::make_unique<ClassSymbol>(className);
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);

	return static_cast<ClassSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}


ClassSymbol* SymbolFactory::Class(const std::wstring& name)
{
	auto symbol = std::make_unique<ClassSymbol>(name);
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<ClassSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ClassSymbol* SymbolFactory::Class(const wchar_t* name)
{
	auto symbol = std::make_unique<ClassSymbol>(name);
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<ClassSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

EnumSymbol* SymbolFactory::Enum(EnumDeclarationSyntax* node, bool isFlags)
{
	std::wstring enumName = node->IdentifierToken.Word;
	auto symbol = std::make_unique<EnumSymbol>(enumName);
	symbol->IsFlags = isFlags;
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);

	return static_cast<EnumSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

EnumSymbol* SymbolFactory::Enum(const std::wstring& name, bool isFlags)
{
	auto symbol = std::make_unique<EnumSymbol>(name);
	symbol->IsFlags = isFlags;
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<EnumSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

EnumSymbol* SymbolFactory::Enum(const wchar_t* name, bool isFlags)
{
	auto symbol = std::make_unique<EnumSymbol>(name);
	symbol->IsFlags = isFlags;
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<EnumSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

StructSymbol* SymbolFactory::Struct(const std::wstring& name)
{
	auto symbol = std::make_unique<StructSymbol>(name);
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<StructSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

StructSymbol* SymbolFactory::Struct(const wchar_t* name)
{
	auto symbol = std::make_unique<StructSymbol>(name);
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<StructSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

InterfaceSymbol* SymbolFactory::Interface(InterfaceDeclarationSyntax* node)
{
	std::wstring interfaceName = node->IdentifierToken.Word;
	auto symbol = std::make_unique<InterfaceSymbol>(interfaceName);
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);

	return static_cast<InterfaceSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

InterfaceSymbol* SymbolFactory::Interface(const std::wstring& name, SymbolAccesibility accesibility, SyntaxSymbol* parent)
{
	auto symbol = std::make_unique<InterfaceSymbol>(name);
	symbol->Accesibility = accesibility;
	symbol->Parent = parent;
	symbol->Inlining = TypeInlining::ByReference;
	return static_cast<InterfaceSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

InterfaceSymbol* SymbolFactory::Interface(const wchar_t* name, SymbolAccesibility accesibility, SyntaxSymbol* parent)
{
	auto symbol = std::make_unique<InterfaceSymbol>(name);
	symbol->Accesibility = accesibility;
	symbol->Parent = parent;
	symbol->Inlining = TypeInlining::ByReference;
	return static_cast<InterfaceSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

FieldSymbol* SymbolFactory::Field(FieldDeclarationSyntax* node)
{
	std::wstring fieldName = node->IdentifierToken.Word;
	auto symbol = std::make_unique<FieldSymbol>(fieldName);
	symbol->DefaultValueExpression = node->InitializerExpression.get();
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);

	return static_cast<FieldSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

PropertySymbol* SymbolFactory::Property(PropertyDeclarationSyntax* node)
{
	std::wstring propertyName = node->IdentifierToken.Word;
	auto symbol = std::make_unique<PropertySymbol>(propertyName);
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);
	symbol->DefaultValueExpression = node->InitializerExpression.get();

	return static_cast<PropertySymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

AccessorSymbol* SymbolFactory::Accessor(AccessorDeclarationSyntax* node, PropertySymbol* propertySymbol, bool setProperty)
{
	std::wstring accessorName = propertySymbol->Name + L"_" + node->KeywordToken.Word;
	auto symbol = std::make_unique<AccessorSymbol>(accessorName);

	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);
	symbol->Linking = propertySymbol->Linking;
	symbol->HandleType = MethodHandleType::Body;

	switch (node->KeywordToken.Type)
	{
		case TokenType::GetKeyword:
		{
			symbol->ReturnType = propertySymbol->ReturnType;
			if (setProperty)
				propertySymbol->Getter = symbol.get();
			break;
		}

		case TokenType::SetKeyword:
		{
			symbol->ReturnType = SymbolTable::Primitives::Void;

			auto valueParam = std::make_unique<ParameterSymbol>(L"value");
			valueParam->Type = propertySymbol->ReturnType;
			symbol->Parameters.push_back(valueParam.get());
			Table->ImplicitSymbol(std::move(valueParam));

			if (setProperty)
				propertySymbol->Setter = symbol.get();
			break;
		}
	}

	return static_cast<AccessorSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

MethodSymbol* SymbolFactory::Method(MethodDeclarationSyntax* node)
{
    std::wstring methodName = node->IdentifierToken.Word;
    auto symbol = std::make_unique<MethodSymbol>(methodName);

    SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);
	symbol->HandleType = MethodHandleType::Body;

	return static_cast<MethodSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

OperatorSymbol* SymbolFactory::Operator(OperatorDeclarationSyntax* node)
{
    std::wstring methodName = GetOperatorMethodName(node->OperatorToken.Type);
    auto symbol = std::make_unique<OperatorSymbol>(methodName, node->OperatorToken.Type);

    SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);
	symbol->HandleType = MethodHandleType::Body;

	return static_cast<OperatorSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

OperatorSymbol* SymbolFactory::Operator(const std::wstring& name, TokenType opToken, TypeSymbol* returnType, MethodSymbolDelegate callback, const std::vector<TypeSymbol*>& paramTypes)
{
	auto symbol = std::make_unique<OperatorSymbol>(name, opToken, callback);
	symbol->ReturnType = returnType;
	symbol->Accesibility = ACS_PUBLIC;
	symbol->Linking = LINK_STATIC;
	symbol->HandleType = MethodHandleType::External;

	for (TypeSymbol* paramType : paramTypes)
	{
		ParameterSymbol* param = Parameter(L"", paramType);
		param->Parent = symbol.get();
		symbol->Parameters.push_back(param);
	}

	return static_cast<OperatorSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ConstructorSymbol* SymbolFactory::Constructor(ConstructorDeclarationSyntax* node)
{
	std::wstring methodName = node->IdentifierToken.Word;
	auto symbol = std::make_unique<ConstructorSymbol>(methodName);
	symbol->ReturnType = SymbolTable::Primitives::Void;

	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);
	symbol->HandleType = MethodHandleType::Body;

	return static_cast<ConstructorSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

ConstructorSymbol* SymbolFactory::Constructor(TypeSymbol* owner, SymbolAccesibility accessibility)
{
	auto symbol = std::make_unique<ConstructorSymbol>(L"init");
	symbol->ReturnType = SymbolTable::Primitives::Void;
	symbol->Accesibility = accessibility;
	symbol->HandleType = MethodHandleType::Body;
	return static_cast<ConstructorSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

DelegateTypeSymbol* SymbolFactory::Delegate(DelegateDeclarationSyntax* node)
{
	// Anonymous method symbol
	auto anonymousMethod = std::make_unique<MethodSymbol>(L"Delegate");
	anonymousMethod->HandleType = MethodHandleType::Lambda;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = SymbolTable::Primitives::Void;
	anonymousMethod->Linking = LINK_STATIC;

	auto symbol = std::make_unique<DelegateTypeSymbol>(node->IdentifierToken.Word);
	symbol->AnonymousSymbol = anonymousMethod.get();
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);

	if (node->ParametersList != nullptr)
	{
		for (const auto& parameter : node->ParametersList->Parameters)
		{
			ParameterSymbol* paramSymbol = Parameter(parameter->Identifier.Word, SymbolTable::Primitives::Any);
			symbol->Parameters.push_back(paramSymbol);
			anonymousMethod->Parameters.push_back(paramSymbol);
		}
	}

	Table->ImplicitSymbol(std::move(anonymousMethod));
	return static_cast<DelegateTypeSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

DelegateTypeSymbol* SymbolFactory::Delegate(DelegateTypeSyntax* node)
{
	// Anonymous method symbol
	auto anonymousMethod = std::make_unique<MethodSymbol>(L"Delegate");
	anonymousMethod->HandleType = MethodHandleType::Lambda;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = node->ReturnType->Symbol;
	anonymousMethod->Linking = LINK_STATIC;

	// Delegate symbol
	auto symbol = std::make_unique<DelegateTypeSymbol>(L"Delegate");
	symbol->ReturnType = node->ReturnType->Symbol;
	symbol->AnonymousSymbol = anonymousMethod.get();

	node->Symbol = symbol.get();
	Table->ImplicitSymbol(std::move(anonymousMethod));
	return static_cast<DelegateTypeSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

DelegateTypeSymbol* SymbolFactory::Delegate(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*>& parameters)
{
	auto anonymousMethod = std::make_unique<MethodSymbol>(L"");
	anonymousMethod->HandleType = MethodHandleType::Lambda;
	anonymousMethod->Accesibility = SymbolAccesibility::Public;
	anonymousMethod->ReturnType = returnType;
	anonymousMethod->Linking = LINK_STATIC;
	anonymousMethod->Parameters = parameters;

	auto symbol = std::make_unique<DelegateTypeSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->AnonymousSymbol = anonymousMethod.get();
	symbol->Parameters = parameters;
	symbol->Accesibility = SymbolAccesibility::Public;

	Table->ImplicitSymbol(std::move(anonymousMethod));
	return static_cast<DelegateTypeSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

DelegateTypeSymbol* SymbolFactory::Delegate(MethodSymbol* method)
{
	auto symbol = std::make_unique<DelegateTypeSymbol>(method->Name);
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->AnonymousSymbol = method;
	symbol->Parameters = method->Parameters;
	symbol->ReturnType = method->ReturnType;
	symbol->State = TypeLayoutingState::Visited;
	symbol->MemoryBytesSize = 0;
	symbol->FullName = method->FullName;

	return static_cast<DelegateTypeSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

NamespaceSymbol* SymbolFactory::Namespace(NamespaceDeclarationSyntax* node)
{
	std::wstring namespaceName = node->IdentifierTokens.at(0).Word;
	for (int i = 1; i < node->IdentifierTokens.size(); i++)
		namespaceName += L"." + node->IdentifierTokens.at(i).Word;

	auto symbol = std::make_unique<NamespaceSymbol>(namespaceName);
	return static_cast<NamespaceSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

NamespaceSymbol* SymbolFactory::Namespace(const std::wstring& name)
{
	auto symbol = std::make_unique<NamespaceSymbol>(name);
	return static_cast<NamespaceSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}


NamespaceSymbol* SymbolFactory::Namespace(const wchar_t* name)
{
	auto symbol = std::make_unique<NamespaceSymbol>(name);
	return static_cast<NamespaceSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

FieldSymbol* SymbolFactory::Field(const std::wstring& name, TypeSymbol* type, SymbolLinking linking)
{
	auto symbol = std::make_unique<FieldSymbol>(name);
	symbol->ReturnType = type;
	symbol->Linking = linking;
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<FieldSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

FieldSymbol* SymbolFactory::EnumField(const std::wstring& name, TypeSymbol* enumType, std::int64_t value)
{
	auto symbol = std::make_unique<FieldSymbol>(name);
	symbol->ReturnType = enumType;
	symbol->Linking = LINK_STATIC;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->IsEnumValue = true;
	symbol->EnumValue = value;
	return static_cast<FieldSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

FieldSymbol* SymbolFactory::BackingField(PropertySymbol* symbol)
{
	FieldSymbol* backingField = Field(L"<" + symbol->Name + L">k__BackingField", symbol->ReturnType, symbol->Linking);
	backingField->Accesibility = SymbolAccesibility::Private;
	backingField->DefaultValueExpression = symbol->DefaultValueExpression;

	symbol->BackingField = backingField;
	return backingField;
}

PropertySymbol* SymbolFactory::Property(const std::wstring& name, TypeSymbol* returnType, SymbolLinking linking)
{
	auto symbol = std::make_unique<PropertySymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Linking = linking;
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<PropertySymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

MethodSymbol* SymbolFactory::Method(const wchar_t* name, TypeSymbol* returnType, SymbolLinking linking)
{
	auto symbol = std::make_unique<MethodSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Linking = linking;
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<MethodSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

MethodSymbol* SymbolFactory::Method(const std::wstring& name, TypeSymbol* returnType, SymbolLinking linking)
{
	auto symbol = std::make_unique<MethodSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Linking = linking;
	symbol->Accesibility = SymbolAccesibility::Private;
	return static_cast<MethodSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

MethodSymbol* SymbolFactory::Method(SymbolAccesibility accessibility, SymbolLinking linking, TypeSymbol* returnType, const wchar_t* name, MethodSymbolDelegate function)
{
	auto symbol = std::make_unique<MethodSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Linking = linking;
	symbol->Accesibility = accessibility;
	symbol->FunctionPointer = function;
	symbol->HandleType = MethodHandleType::External;
	return static_cast<MethodSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

MethodSymbol* SymbolFactory::Method(SymbolAccesibility accessibility, SymbolLinking linking, TypeSymbol* returnType, const std::wstring& name, MethodSymbolDelegate function)
{
	auto symbol = std::make_unique<MethodSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Linking = linking;
	symbol->Accesibility = accessibility;
	symbol->FunctionPointer = function;
	symbol->HandleType = MethodHandleType::External;
	return static_cast<MethodSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

AccessorSymbol* SymbolFactory::Accessor(const std::wstring& name, PropertySymbol* property, bool isGetter)
{
	auto symbol = std::make_unique<AccessorSymbol>(name);
	symbol->HandleType = MethodHandleType::Body;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->Linking = property->Linking;
	symbol->ReturnType = isGetter ? property->ReturnType : SymbolTable::Primitives::Void;

	if (!isGetter)
	{
		auto valueParam = std::make_unique<ParameterSymbol>(L"value");
		symbol->Parameters.push_back(valueParam.get());
		Table->ImplicitSymbol(std::move(valueParam));
	}

	AccessorSymbol* raw = symbol.get();
	if (isGetter)
		property->Getter = raw;
	else
		property->Setter = raw;
	
	return static_cast<AccessorSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

AccessorSymbol* SymbolFactory::Getter(PropertySymbol* property)
{
	std::wstring getterName = L"get_" + property->Name;
	return Accessor(getterName, property, true);
}

AccessorSymbol* SymbolFactory::Setter(PropertySymbol* property)
{
	std::wstring setterName = L"set_" + property->Name;
	return Accessor(setterName, property, false);
}

IndexatorSymbol* SymbolFactory::Indexator(IndexatorDeclarationSyntax* node)
{
	auto symbol = std::make_unique<IndexatorSymbol>(L"index"); // Name is always "index"
	SetAccesibility(node->Modifiers, symbol.get()->Accesibility, symbol.get()->Linking);

	return static_cast<IndexatorSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

IndexatorSymbol* SymbolFactory::Indexator(const std::wstring& name, TypeSymbol* returnType)
{
	auto symbol = std::make_unique<IndexatorSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->Linking = LINK_INSTANCE;
	return static_cast<IndexatorSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

IndexatorSymbol* SymbolFactory::Indexator(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*>& parameters)
{
	auto symbol = std::make_unique<IndexatorSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->Linking = LINK_INSTANCE;
	symbol->Parameters = parameters;
	return static_cast<IndexatorSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ParameterSymbol* SymbolFactory::Parameter(ParameterSyntax* node)
{
	auto symbol = std::make_unique<ParameterSymbol>(node->Identifier.Word);
	return static_cast<ParameterSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

ParameterSymbol* SymbolFactory::Parameter(const std::wstring& name)
{
	auto symbol = std::make_unique<ParameterSymbol>(name);
	return static_cast<ParameterSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ParameterSymbol* SymbolFactory::Parameter(const std::wstring& name, TypeSymbol* type)
{
	auto symbol = std::make_unique<ParameterSymbol>(name, type);
	return static_cast<ParameterSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ParameterSymbol* SymbolFactory::Parameter(const std::wstring& name, TypeSymbol* type, bool isOptional)
{
	auto symbol = std::make_unique<ParameterSymbol>(name, type);
	symbol->IsOptional = isOptional;
	return static_cast<ParameterSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

VariableSymbol* SymbolFactory::Variable(VariableStatementSyntax* node)
{
	auto symbol = std::make_unique<VariableSymbol>(node->IdentifierToken.Word, SymbolTable::Primitives::Integer);
	return static_cast<VariableSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

VariableSymbol* SymbolFactory::Variable(ForEachStatementSyntax* node)
{
	auto symbol = std::make_unique<VariableSymbol>(node->IdentifierToken.Word);
	return static_cast<VariableSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

VariableSymbol* SymbolFactory::Variable(ForInStatementSyntax* node)
{
	auto symbol = std::make_unique<VariableSymbol>(node->IdentifierToken.Word);
	return static_cast<VariableSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

VariableSymbol* SymbolFactory::Variable(const std::wstring& name)
{
	auto symbol = std::make_unique<VariableSymbol>(name);
	return static_cast<VariableSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

VariableSymbol* SymbolFactory::Variable(const std::wstring& name, TypeSymbol* type)
{
	auto symbol = std::make_unique<VariableSymbol>(name, type);
	return static_cast<VariableSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

VariableSymbol* SymbolFactory::Variable(const std::wstring& name, TypeSymbol* type, bool isConst)
{
	auto symbol = std::make_unique<VariableSymbol>(name, type);
	symbol->IsConst = isConst;
	return static_cast<VariableSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

TypeParameterSymbol* SymbolFactory::TypeParameter(const std::wstring& name, TypeSymbol* parent)
{
	auto symbol = std::make_unique<TypeParameterSymbol>(name);
	symbol->TypeArgumentIndex = parent->TypeParameters.size();
	symbol->TypeParameters.push_back(symbol.get());
	symbol->Parent = parent;
	return static_cast<TypeParameterSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

TypeParameterSymbol* SymbolFactory::TypeParameter(const std::wstring& name, MethodSymbol* parent)
{
	auto symbol = std::make_unique<TypeParameterSymbol>(name);
	symbol->TypeParameters.push_back(symbol.get());
	symbol->Parent = parent;

	if (parent != nullptr)
		symbol->TypeArgumentIndex = parent->TypeParameters.size();

	return static_cast<TypeParameterSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ArrayTypeSymbol* SymbolFactory::Array(ArrayTypeSyntax* node)
{
	if (node == nullptr || node->UnderlayingType == nullptr || node->UnderlayingType->Symbol == nullptr)
		return nullptr;

	auto symbol = std::make_unique<ArrayTypeSymbol>(node->UnderlayingType->Symbol);
	return static_cast<ArrayTypeSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

ArrayTypeSymbol* SymbolFactory::Array(TypeSymbol* underlayingType)
{
	auto symbol = std::make_unique<ArrayTypeSymbol>(underlayingType);
	return static_cast<ArrayTypeSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ArrayTypeSymbol* SymbolFactory::Array(TypeSymbol* underlayingType, std::size_t size)
{
	auto symbol = std::make_unique<ArrayTypeSymbol>(underlayingType);
	symbol->Length = size;
	return static_cast<ArrayTypeSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

ArrayTypeSymbol* SymbolFactory::Array(TypeSymbol* underlayingType, std::size_t size, int rank)
{
	auto symbol = std::make_unique<ArrayTypeSymbol>(underlayingType);
	symbol->Length = size;
	return static_cast<ArrayTypeSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

GenericTypeSymbol* SymbolFactory::GenericType(GenericTypeSyntax* node)
{
	auto symbol = std::make_unique<GenericTypeSymbol>(node->UnderlayingType->Symbol);
	return static_cast<GenericTypeSymbol*>(Table->BindSymbol(node, std::move(symbol)));
}

GenericTypeSymbol* SymbolFactory::GenericType(TypeSymbol* underlayingType)
{
	auto symbol = std::make_unique<GenericTypeSymbol>(underlayingType);
	return static_cast<GenericTypeSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

GenericTypeSymbol* SymbolFactory::GenericType(TypeSymbol* underlayingType, std::unordered_map<std::wstring, TypeSymbol*> typeArguments)
{
	auto symbol = std::make_unique<GenericTypeSymbol>(underlayingType);
	
	// Заполняем маппинг type parameters -> type arguments
	for (std::size_t i = 0; i < underlayingType->TypeParameters.size(); i++)
	{
		TypeParameterSymbol* typeParam = underlayingType->TypeParameters[i];
		auto it = typeArguments.find(typeParam->Name);
		if (it != typeArguments.end())
		{
			symbol->AddTypeParameter(typeParam, it->second);
		}
	}
	
	return static_cast<GenericTypeSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
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

	for (std::size_t i = 0; i < method->Parameters.size(); i++)
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
	auto symbol = std::make_unique<MethodSymbol>(name);
	symbol->ReturnType = returnType;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->Linking = LINK_STATIC;
	symbol->HandleType = MethodHandleType::Lambda;
	return static_cast<MethodSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}

MethodSymbol* SymbolFactory::CreateLambdaMethod(StatementsBlockSyntax* body)
{
	auto symbol = std::make_unique<MethodSymbol>(L"Lambda");
	symbol->ReturnType = SymbolTable::Primitives::Any;
	symbol->Accesibility = SymbolAccesibility::Public;
	symbol->Linking = LINK_STATIC;
	symbol->HandleType = MethodHandleType::Lambda;
	return static_cast<MethodSymbol*>(Table->ImplicitSymbol(std::move(symbol)));
}
