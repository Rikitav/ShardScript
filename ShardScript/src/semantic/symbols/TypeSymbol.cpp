#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxFacts.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <algorithm>
#include <vector>
#include <string>

using namespace shard;

std::size_t TypeSymbol::GetInlineSize() const
{
	return Inlining == TypeInlining::ByReference ? sizeof(void*) : MemoryBytesSize;
}

static bool paramPredicate(ParameterSymbol* left, TypeSymbol* right)
{
	if (left->Type == SymbolTable::Primitives::Any)
		return true;

	return SemanticModel::IsAssignableTo(left->Type, right);
}

void TypeSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::ConstructorDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			ConstructorSymbol* ctor = static_cast<ConstructorSymbol*>(symbol);
			Constructors.push_back(ctor);
			break;
		}

		case SyntaxKind::MethodDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			MethodSymbol* method = static_cast<MethodSymbol*>(symbol);
			Methods.push_back(method);
			break;
		}

		case SyntaxKind::OperatorDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			OperatorSymbol* op = static_cast<OperatorSymbol*>(symbol);
			Operators.push_back(op);
			break;
		}

		case SyntaxKind::FieldDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			FieldSymbol* field = static_cast<FieldSymbol*>(symbol);
			Fields.push_back(field);
			break;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			PropertySymbol* prop = static_cast<PropertySymbol*>(symbol);
			Properties.push_back(prop);
			break;
		}

		case SyntaxKind::IndexatorDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			IndexatorSymbol* index = static_cast<IndexatorSymbol*>(symbol);
			Indexators.push_back(index);
			break;
		}

		case SyntaxKind::TypeParameter:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(symbol);
			TypeParameters.push_back(typeParam);
			break;
		}
	}
}

bool TypeSymbol::IsType() const
{
	return true;
}

bool TypeSymbol::IsMember() const
{
	return false;
}

ConstructorSymbol* TypeSymbol::FindConstructor(const std::vector<TypeSymbol*>& parameterTypes)
{
	for (ConstructorSymbol* symbol : Constructors)
	{
		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

MethodSymbol* TypeSymbol::FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes)
{
	for (MethodSymbol* symbol : Methods)
	{
		if (symbol->Name != name)
			continue;

		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

OperatorSymbol* TypeSymbol::FindOperator(TokenType opToken, const std::vector<TypeSymbol*>& parameterTypes)
{
	std::wstring opName = GetOperatorMethodName(opToken);
	if (opName.empty())
		return nullptr;

	for (OperatorSymbol* symbol : Operators)
	{
		if (symbol->Name != opName)
			continue;

		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

IndexatorSymbol* TypeSymbol::FindIndexator(const std::vector<TypeSymbol*>& parameterTypes)
{
	for (IndexatorSymbol* symbol : Indexators)
	{
		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

FieldSymbol* TypeSymbol::FindField(std::wstring& name)
{
	for (FieldSymbol* symbol : Fields)
	{
		if (symbol->Name != name)
			continue;

		return symbol;
	}

	return nullptr;
}

PropertySymbol* TypeSymbol::FindProperty(std::wstring& name)
{
	for (PropertySymbol* symbol : Properties)
	{
		if (symbol->Name != name)
			continue;

		return symbol;
	}

	return nullptr;
}

MethodSymbol* TypeSymbol::FindInterfaceImplementation(MethodSymbol* interfaceMethod)
{
	auto it = InterfaceMethodMap.find(interfaceMethod);
	if (it != InterfaceMethodMap.end())
		return it->second;

	for (TypeSymbol* iface : Interfaces)
	{
		if (iface == nullptr || iface->Kind != SyntaxKind::InterfaceDeclaration)
			continue;

		MethodSymbol* baseImplementation = iface->FindInterfaceImplementation(interfaceMethod);
		if (baseImplementation != nullptr)
			return baseImplementation;
	}

	return nullptr;
}
