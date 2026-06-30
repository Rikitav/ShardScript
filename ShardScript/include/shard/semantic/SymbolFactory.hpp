#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/semantic/SymbolTable.hpp>

#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/DelegateTypeSyntax.hpp>

#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForInStatementSyntax.hpp>

#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>

#include <string>
#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API SymbolFactory
	{
		SymbolTable* Table;

		void SetAccesibility(std::vector<SyntaxToken> modifiers, SymbolAccesibility& accesibility, SymbolLinking& linking);

		std::wstring FormatFullName(SyntaxSymbol* symbol);
		std::wstring FormatFullName(SyntaxSymbol* symbol, SyntaxSymbol* parent);
		std::wstring FormatMethodSignature(MethodSymbol* method);
		std::wstring FormatTypeName(TypeSymbol* type);

	public:
		SymbolFactory(SymbolTable* table)
			: Table(table) { }

		NamespaceSymbol* Namespace(NamespaceDeclarationSyntax* node);
		NamespaceSymbol* Namespace(const std::wstring& name);
		NamespaceSymbol* Namespace(const wchar_t* name);
		StructSymbol* Struct(StructDeclarationSyntax* node);
		StructSymbol* Struct(const std::wstring& name);
		StructSymbol* Struct(const wchar_t* name);
		ClassSymbol* Class(ClassDeclarationSyntax* node);
		ClassSymbol* Class(const std::wstring& name);
		ClassSymbol* Class(const wchar_t* name);

		EnumSymbol* Enum(EnumDeclarationSyntax* node, bool isFlags = false);
		EnumSymbol* Enum(const std::wstring& name, bool isFlags = false);
		EnumSymbol* Enum(const wchar_t* name, bool isFlags = false);

		InterfaceSymbol* Interface(InterfaceDeclarationSyntax* node);
		InterfaceSymbol* Interface(const std::wstring& name, SymbolAccesibility accesibility = ACS_PUBLIC, SyntaxSymbol* parent = nullptr);
		InterfaceSymbol* Interface(const wchar_t* name, SymbolAccesibility accesibility = ACS_PUBLIC, SyntaxSymbol* parent = nullptr);

		FieldSymbol* Field(FieldDeclarationSyntax* node);
		FieldSymbol* Field(const std::wstring& name, TypeSymbol* type, SymbolLinking linking = LINK_INSTANCE);
		FieldSymbol* EnumField(const std::wstring& name, TypeSymbol* enumType, std::int64_t value);
		FieldSymbol* BackingField(PropertySymbol* symbol);

		PropertySymbol* Property(PropertyDeclarationSyntax* node);
		PropertySymbol* Property(const std::wstring& name, TypeSymbol* returnType, SymbolLinking linking = LINK_INSTANCE);

		MethodSymbol* Method(MethodDeclarationSyntax* node);
		OperatorSymbol* Operator(OperatorDeclarationSyntax* node);
		MethodSymbol* Method(const wchar_t* name, TypeSymbol* returnType, SymbolLinking linking = LINK_INSTANCE);
		MethodSymbol* Method(const std::wstring& name, TypeSymbol* returnType, SymbolLinking linking = LINK_INSTANCE);
		MethodSymbol* Method(SymbolAccesibility accessibility, SymbolLinking linking, TypeSymbol* returnType, const wchar_t* name, MethodSymbolDelegate function);
		MethodSymbol* Method(SymbolAccesibility accessibility, SymbolLinking linking, TypeSymbol* returnType, const std::wstring& name, MethodSymbolDelegate function);

		OperatorSymbol* Operator(const std::wstring& name, TokenType opToken, TypeSymbol* returnType, MethodSymbolDelegate callback, const std::vector<TypeSymbol*>& paramTypes);

		MethodSymbol* CreateAnonymousMethod(const std::wstring& name, TypeSymbol* returnType);
		MethodSymbol* CreateLambdaMethod(StatementsBlockSyntax* body);

		ConstructorSymbol* Constructor(ConstructorDeclarationSyntax* node);
		ConstructorSymbol* Constructor(TypeSymbol* owner, SymbolAccesibility accessibility);

		AccessorSymbol* Accessor(AccessorDeclarationSyntax* node, PropertySymbol* propertySymbol, bool setProperty = true);
		AccessorSymbol* Accessor(const std::wstring& name, PropertySymbol* property, bool isGetter);
		AccessorSymbol* Getter(PropertySymbol* property);
		AccessorSymbol* Setter(PropertySymbol* property);

		IndexatorSymbol* Indexator(IndexatorDeclarationSyntax* node);
		IndexatorSymbol* Indexator(const std::wstring& name, TypeSymbol* returnType);
		IndexatorSymbol* Indexator(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*>& parameters);

		ParameterSymbol* Parameter(ParameterSyntax* node);
		ParameterSymbol* Parameter(const std::wstring& name);
		ParameterSymbol* Parameter(const std::wstring& name, TypeSymbol* type);
		ParameterSymbol* Parameter(const std::wstring& name, TypeSymbol* type, bool isOptional);

		VariableSymbol* Variable(ForEachStatementSyntax* node);
		VariableSymbol* Variable(ForInStatementSyntax* node);
		VariableSymbol* Variable(VariableStatementSyntax* node);
		VariableSymbol* Variable(const std::wstring& name);
		VariableSymbol* Variable(const std::wstring& name, TypeSymbol* type);
		VariableSymbol* Variable(const std::wstring& name, TypeSymbol* type, bool isConst);

		TypeParameterSymbol* TypeParameter(const std::wstring& name, TypeSymbol* parent);
		TypeParameterSymbol* TypeParameter(const std::wstring& name, MethodSymbol* parent);

		DelegateTypeSymbol* Delegate(DelegateDeclarationSyntax* node);
		DelegateTypeSymbol* Delegate(DelegateTypeSyntax* node);
		DelegateTypeSymbol* Delegate(MethodSymbol* method);
		DelegateTypeSymbol* Delegate(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*>& parameters);

		ArrayTypeSymbol* Array(ArrayTypeSyntax* node);
		ArrayTypeSymbol* Array(TypeSymbol* underlayingType);
		ArrayTypeSymbol* Array(TypeSymbol* underlayingType, std::size_t size);
		ArrayTypeSymbol* Array(TypeSymbol* underlayingType, std::size_t size, int rank);

		GenericTypeSymbol* GenericType(GenericTypeSyntax* node);
		GenericTypeSymbol* GenericType(TypeSymbol* underlayingType);
		GenericTypeSymbol* GenericType(TypeSymbol* underlayingType, std::unordered_map<std::wstring, TypeSymbol*> typeArguments);
	};
}
