#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>

#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/InterfaceSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/OperatorSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>

#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>

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

		InterfaceSymbol* Interface(InterfaceDeclarationSyntax* node);
		InterfaceSymbol* Interface(const std::wstring& name, SymbolAccesibility accesibility = ACS_PUBLIC, SyntaxSymbol* parent = nullptr);
		InterfaceSymbol* Interface(const wchar_t* name, SymbolAccesibility accesibility = ACS_PUBLIC, SyntaxSymbol* parent = nullptr);

		FieldSymbol* Field(FieldDeclarationSyntax* node);
		FieldSymbol* Field(const std::wstring& name, TypeSymbol* type, SymbolLinking linking = LINK_INSTANCE);
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
