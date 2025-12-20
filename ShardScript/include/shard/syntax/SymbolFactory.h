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

namespace shard::syntax
{
	class SHARD_API SymbolFactory
	{
	public:
		static void SetAccesibility(shard::syntax::SyntaxSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
		static void SetAccesibility(shard::syntax::symbols::TypeSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
		static void SetAccesibility(shard::syntax::symbols::FieldSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
		static void SetAccesibility(shard::syntax::symbols::PropertySymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);
		static void SetAccesibility(shard::syntax::symbols::MethodSymbol* node, std::vector<shard::syntax::SyntaxToken> modifiers);

		static shard::syntax::symbols::StructSymbol* Struct(shard::syntax::nodes::StructDeclarationSyntax* node);
		static shard::syntax::symbols::ClassSymbol* Class(shard::syntax::nodes::ClassDeclarationSyntax* node);
		static shard::syntax::symbols::NamespaceSymbol* Namespace(shard::syntax::nodes::NamespaceDeclarationSyntax* node);
		static shard::syntax::symbols::NamespaceSymbol* Namespace(const std::wstring& name);

		static shard::syntax::symbols::FieldSymbol* Field(shard::syntax::nodes::FieldDeclarationSyntax* node);
		static shard::syntax::symbols::FieldSymbol* Field(const std::wstring& name, shard::syntax::symbols::TypeSymbol* type, bool isStatic = false);

		static shard::syntax::symbols::PropertySymbol* Property(shard::syntax::nodes::PropertyDeclarationSyntax* node);
		static shard::syntax::symbols::PropertySymbol* Property(const std::wstring& name, shard::syntax::symbols::TypeSymbol* returnType, bool isStatic = false);

		static shard::syntax::symbols::MethodSymbol* Method(shard::syntax::nodes::MethodDeclarationSyntax* node);
		static shard::syntax::symbols::MethodSymbol* Method(const std::wstring& name, shard::syntax::symbols::TypeSymbol* returnType, bool isStatic = false);

		static shard::syntax::symbols::ConstructorSymbol* Constructor(shard::syntax::nodes::ConstructorDeclarationSyntax* node);
		static shard::syntax::symbols::ConstructorSymbol* Constructor(const std::wstring& name);

		static shard::syntax::symbols::AccessorSymbol* Accessor(shard::syntax::nodes::AccessorDeclarationSyntax* node, shard::syntax::symbols::PropertySymbol* propertySymbol, bool setProperty = true);
		static shard::syntax::symbols::AccessorSymbol* Accessor(const std::wstring& name, shard::syntax::symbols::PropertySymbol* property, bool isGetter);
		static shard::syntax::symbols::AccessorSymbol* Getter(const std::wstring& propertyName, shard::syntax::symbols::PropertySymbol* property);
		static shard::syntax::symbols::AccessorSymbol* Setter(const std::wstring& propertyName, shard::syntax::symbols::PropertySymbol* property);

		static shard::syntax::symbols::IndexatorSymbol* Indexator(const std::wstring& name, shard::syntax::symbols::TypeSymbol* returnType);
		static shard::syntax::symbols::IndexatorSymbol* Indexator(const std::wstring& name, shard::syntax::symbols::TypeSymbol* returnType, std::vector<shard::syntax::symbols::ParameterSymbol*> parameters);

		static shard::syntax::symbols::ParameterSymbol* Parameter(const std::wstring& name);
		static shard::syntax::symbols::ParameterSymbol* Parameter(const std::wstring& name, shard::syntax::symbols::TypeSymbol* type);
		static shard::syntax::symbols::ParameterSymbol* Parameter(const std::wstring& name, shard::syntax::symbols::TypeSymbol* type, bool isOptional);

		static shard::syntax::symbols::VariableSymbol* Variable(const std::wstring& name, shard::syntax::symbols::TypeSymbol* type);
		static shard::syntax::symbols::VariableSymbol* Variable(const std::wstring& name, shard::syntax::symbols::TypeSymbol* type, bool isConst);

		static shard::syntax::symbols::TypeParameterSymbol* TypeParameter(const std::wstring& name);

		static shard::syntax::symbols::DelegateTypeSymbol* Delegate(shard::syntax::nodes::DelegateDeclarationSyntax* node);
		static shard::syntax::symbols::DelegateTypeSymbol* Delegate(shard::syntax::nodes::DelegateTypeSyntax* node);
		static shard::syntax::symbols::DelegateTypeSymbol* Delegate(shard::syntax::symbols::MethodSymbol* method);
		static shard::syntax::symbols::DelegateTypeSymbol* Delegate(const std::wstring& name, shard::syntax::symbols::TypeSymbol* returnType, std::vector<shard::syntax::symbols::ParameterSymbol*> parameters);

		static shard::syntax::symbols::ArrayTypeSymbol* Array(shard::syntax::nodes::ArrayTypeSyntax* node);
		static shard::syntax::symbols::ArrayTypeSymbol* Array(shard::syntax::symbols::TypeSymbol* underlayingType);
		static shard::syntax::symbols::ArrayTypeSymbol* Array(shard::syntax::symbols::TypeSymbol* underlayingType, size_t size);
		static shard::syntax::symbols::ArrayTypeSymbol* Array(shard::syntax::symbols::TypeSymbol* underlayingType, size_t size, int rank);

		static shard::syntax::symbols::GenericTypeSymbol* GenericType(shard::syntax::symbols::TypeSymbol* underlayingType);
		static shard::syntax::symbols::GenericTypeSymbol* GenericType(shard::syntax::symbols::TypeSymbol* underlayingType, std::unordered_map<std::wstring, shard::syntax::symbols::TypeSymbol*> typeArguments);

		static std::wstring FormatFullName(shard::syntax::SyntaxSymbol* symbol);
		static std::wstring FormatFullName(shard::syntax::SyntaxSymbol* symbol, shard::syntax::SyntaxSymbol* parent);
		static std::wstring FormatMethodSignature(shard::syntax::symbols::MethodSymbol* method);
		static std::wstring FormatTypeName(shard::syntax::symbols::TypeSymbol* type);

		static bool IsAccessible(shard::syntax::SyntaxSymbol* symbol, shard::syntax::SymbolAccesibility requiredAccessibility);
		static bool CanOverride(shard::syntax::symbols::MethodSymbol* method, shard::syntax::symbols::MethodSymbol* baseMethod);
		static bool IsCompatible(shard::syntax::symbols::MethodSymbol* method1, shard::syntax::symbols::MethodSymbol* method2);

		static bool ValidateModifiers(std::vector<shard::syntax::SyntaxToken> modifiers, shard::syntax::SyntaxKind symbolKind);
		static shard::syntax::SymbolAccesibility GetDefaultAccessibility(shard::syntax::SyntaxKind symbolKind);
		static bool IsValidModifierCombination(std::vector<shard::syntax::SyntaxToken> modifiers);

		static shard::syntax::symbols::MethodSymbol* CreateAnonymousMethod(const std::wstring& name, shard::syntax::symbols::TypeSymbol* returnType);
		static shard::syntax::symbols::MethodSymbol* CreateLambdaMethod(shard::syntax::nodes::StatementsBlockSyntax* body);
	};
}