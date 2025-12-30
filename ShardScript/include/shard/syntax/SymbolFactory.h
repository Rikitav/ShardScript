#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.h>

#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>
#include <shard/syntax/symbols/TypeParameterSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>

#include <shard/syntax/symbols/DelegateTypeSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API SymbolFactory
	{
	public:
		static void SetAccesibility(shard::SyntaxSymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::TypeSymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::FieldSymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::PropertySymbol* node, std::vector<shard::SyntaxToken> modifiers);
		static void SetAccesibility(shard::MethodSymbol* node, std::vector<shard::SyntaxToken> modifiers);

		static shard::StructSymbol* Struct(shard::StructDeclarationSyntax* node);
		static shard::ClassSymbol* Class(shard::ClassDeclarationSyntax* node);
		static shard::NamespaceSymbol* Namespace(shard::NamespaceDeclarationSyntax* node);
		static shard::NamespaceSymbol* Namespace(const std::wstring& name);

		static shard::FieldSymbol* Field(shard::FieldDeclarationSyntax* node);
		static shard::FieldSymbol* Field(const std::wstring& name, shard::TypeSymbol* type, bool isStatic = false);

		static shard::PropertySymbol* Property(shard::PropertyDeclarationSyntax* node);
		static shard::PropertySymbol* Property(const std::wstring& name, shard::TypeSymbol* returnType, bool isStatic = false);

		static shard::MethodSymbol* Method(shard::MethodDeclarationSyntax* node);
		static shard::MethodSymbol* Method(const std::wstring& name, shard::TypeSymbol* returnType, bool isStatic = false);

		static shard::ConstructorSymbol* Constructor(shard::ConstructorDeclarationSyntax* node);
		static shard::ConstructorSymbol* Constructor(const std::wstring& name);

		static shard::AccessorSymbol* Accessor(shard::AccessorDeclarationSyntax* node, shard::PropertySymbol* propertySymbol, bool setProperty = true);
		static shard::AccessorSymbol* Accessor(const std::wstring& name, shard::PropertySymbol* property, bool isGetter);
		static shard::AccessorSymbol* Getter(const std::wstring& propertyName, shard::PropertySymbol* property);
		static shard::AccessorSymbol* Setter(const std::wstring& propertyName, shard::PropertySymbol* property);

		static shard::IndexatorSymbol* Indexator(const std::wstring& name, shard::TypeSymbol* returnType);
		static shard::IndexatorSymbol* Indexator(const std::wstring& name, shard::TypeSymbol* returnType, std::vector<shard::ParameterSymbol*> parameters);

		static shard::ParameterSymbol* Parameter(const std::wstring& name);
		static shard::ParameterSymbol* Parameter(const std::wstring& name, shard::TypeSymbol* type);
		static shard::ParameterSymbol* Parameter(const std::wstring& name, shard::TypeSymbol* type, bool isOptional);

		static shard::VariableSymbol* Variable(const std::wstring& name, shard::TypeSymbol* type);
		static shard::VariableSymbol* Variable(const std::wstring& name, shard::TypeSymbol* type, bool isConst);

		static shard::TypeParameterSymbol* TypeParameter(const std::wstring& name);

		static shard::DelegateTypeSymbol* Delegate(shard::DelegateDeclarationSyntax* node);
		static shard::DelegateTypeSymbol* Delegate(shard::DelegateTypeSyntax* node);
		static shard::DelegateTypeSymbol* Delegate(shard::MethodSymbol* method);
		static shard::DelegateTypeSymbol* Delegate(const std::wstring& name, shard::TypeSymbol* returnType, std::vector<shard::ParameterSymbol*> parameters);

		static shard::ArrayTypeSymbol* Array(shard::ArrayTypeSyntax* node);
		static shard::ArrayTypeSymbol* Array(shard::TypeSymbol* underlayingType);
		static shard::ArrayTypeSymbol* Array(shard::TypeSymbol* underlayingType, size_t size);
		static shard::ArrayTypeSymbol* Array(shard::TypeSymbol* underlayingType, size_t size, int rank);

		static shard::GenericTypeSymbol* GenericType(shard::TypeSymbol* underlayingType);
		static shard::GenericTypeSymbol* GenericType(shard::TypeSymbol* underlayingType, std::unordered_map<std::wstring, shard::TypeSymbol*> typeArguments);

		static std::wstring FormatFullName(shard::SyntaxSymbol* symbol);
		static std::wstring FormatFullName(shard::SyntaxSymbol* symbol, shard::SyntaxSymbol* parent);
		static std::wstring FormatMethodSignature(shard::MethodSymbol* method);
		static std::wstring FormatTypeName(shard::TypeSymbol* type);

		static bool IsAccessible(shard::SyntaxSymbol* symbol, shard::SymbolAccesibility requiredAccessibility);
		static bool CanOverride(shard::MethodSymbol* method, shard::MethodSymbol* baseMethod);
		static bool IsCompatible(shard::MethodSymbol* method1, shard::MethodSymbol* method2);

		static bool ValidateModifiers(std::vector<shard::SyntaxToken> modifiers, shard::SyntaxKind symbolKind);
		static shard::SymbolAccesibility GetDefaultAccessibility(shard::SyntaxKind symbolKind);
		static bool IsValidModifierCombination(std::vector<shard::SyntaxToken> modifiers);

		static shard::MethodSymbol* CreateAnonymousMethod(const std::wstring& name, shard::TypeSymbol* returnType);
		static shard::MethodSymbol* CreateLambdaMethod(shard::StatementsBlockSyntax* body);
	};
}