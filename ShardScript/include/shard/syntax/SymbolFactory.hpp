#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForEachStatementSyntax.hpp>

#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
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
#include <shard/syntax/SymbolAccesibility.hpp>

#include <string>
#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API SymbolFactory
	{
		SymbolTable* Table;

		void SetAccesibility(SyntaxSymbol* node, std::vector<SyntaxToken> modifiers);
		void SetAccesibility(TypeSymbol* node, std::vector<SyntaxToken> modifiers);
		void SetAccesibility(FieldSymbol* node, std::vector<SyntaxToken> modifiers);
		void SetAccesibility(PropertySymbol* node, std::vector<SyntaxToken> modifiers);
		void SetAccesibility(MethodSymbol* node, std::vector<SyntaxToken> modifiers);

		std::wstring FormatFullName(SyntaxSymbol* symbol);
		std::wstring FormatFullName(SyntaxSymbol* symbol, SyntaxSymbol* parent);
		std::wstring FormatMethodSignature(MethodSymbol* method);
		std::wstring FormatTypeName(TypeSymbol* type);

	public:
		SymbolFactory(SymbolTable* table)
			: Table(table) { }

		NamespaceSymbol* Namespace(NamespaceDeclarationSyntax* node);
		NamespaceSymbol* Namespace(const std::wstring& name);
		StructSymbol* Struct(StructDeclarationSyntax* node);
		ClassSymbol* Class(ClassDeclarationSyntax* node);

		FieldSymbol* Field(FieldDeclarationSyntax* node);
		FieldSymbol* Field(const std::wstring& name, TypeSymbol* type, bool isStatic = false);

		PropertySymbol* Property(PropertyDeclarationSyntax* node);
		PropertySymbol* Property(const std::wstring& name, TypeSymbol* returnType, bool isStatic = false);

		MethodSymbol* Method(MethodDeclarationSyntax* node);
		MethodSymbol* Method(const wchar_t* name, TypeSymbol* returnType, bool isStatic = false);
		MethodSymbol* Method(const std::wstring& name, TypeSymbol* returnType, bool isStatic = false);
		MethodSymbol* Method(SymbolAccesibility accessibility, bool isStatic, TypeSymbol* returnType, const wchar_t* name, MethodSymbolDelegate function);
		MethodSymbol* Method(SymbolAccesibility accessibility, bool isStatic, TypeSymbol* returnType, const std::wstring& name, MethodSymbolDelegate function);

		MethodSymbol* CreateAnonymousMethod(const std::wstring& name, TypeSymbol* returnType);
		MethodSymbol* CreateLambdaMethod(StatementsBlockSyntax* body);

		ConstructorSymbol* Constructor(ConstructorDeclarationSyntax* node);
		ConstructorSymbol* Constructor(const std::wstring& name);

		AccessorSymbol* Accessor(AccessorDeclarationSyntax* node, PropertySymbol* propertySymbol, bool setProperty = true);
		AccessorSymbol* Accessor(const std::wstring& name, PropertySymbol* property, bool isGetter);
		AccessorSymbol* Getter(const std::wstring& propertyName, PropertySymbol* property);
		AccessorSymbol* Setter(const std::wstring& propertyName, PropertySymbol* property);

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

		TypeParameterSymbol* TypeParameter(const std::wstring& name);

		DelegateTypeSymbol* Delegate(DelegateDeclarationSyntax* node);
		DelegateTypeSymbol* Delegate(DelegateTypeSyntax* node);
		DelegateTypeSymbol* Delegate(MethodSymbol* method);
		DelegateTypeSymbol* Delegate(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*>& parameters);

		ArrayTypeSymbol* Array(ArrayTypeSyntax* node);
		ArrayTypeSymbol* Array(TypeSymbol* underlayingType);
		ArrayTypeSymbol* Array(TypeSymbol* underlayingType, size_t size);
		ArrayTypeSymbol* Array(TypeSymbol* underlayingType, size_t size, int rank);

		GenericTypeSymbol* GenericType(GenericTypeSyntax* node);
		GenericTypeSymbol* GenericType(TypeSymbol* underlayingType);
		GenericTypeSymbol* GenericType(TypeSymbol* underlayingType, std::unordered_map<std::wstring, TypeSymbol*> typeArguments);
	};
}