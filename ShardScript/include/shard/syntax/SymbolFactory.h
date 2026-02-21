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
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h>

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
		static void SetAccesibility(SyntaxSymbol* node, std::vector<SyntaxToken> modifiers);
		static void SetAccesibility(TypeSymbol* node, std::vector<SyntaxToken> modifiers);
		static void SetAccesibility(FieldSymbol* node, std::vector<SyntaxToken> modifiers);
		static void SetAccesibility(PropertySymbol* node, std::vector<SyntaxToken> modifiers);
		static void SetAccesibility(MethodSymbol* node, std::vector<SyntaxToken> modifiers);

		static NamespaceSymbol* Namespace(NamespaceDeclarationSyntax* node);
		static NamespaceSymbol* Namespace(const std::wstring& name);
		static StructSymbol* Struct(StructDeclarationSyntax* node);
		static ClassSymbol* Class(ClassDeclarationSyntax* node);

		static FieldSymbol* Field(FieldDeclarationSyntax* node);
		static FieldSymbol* Field(const std::wstring& name, TypeSymbol* type, bool isStatic = false);

		static PropertySymbol* Property(PropertyDeclarationSyntax* node);
		static PropertySymbol* Property(const std::wstring& name, TypeSymbol* returnType, bool isStatic = false);

		static MethodSymbol* Method(MethodDeclarationSyntax* node);
		static MethodSymbol* Method(const std::wstring& name, TypeSymbol* returnType, bool isStatic = false);

		static ConstructorSymbol* Constructor(ConstructorDeclarationSyntax* node);
		static ConstructorSymbol* Constructor(const std::wstring& name);

		static AccessorSymbol* Accessor(AccessorDeclarationSyntax* node, PropertySymbol* propertySymbol, bool setProperty = true);
		static AccessorSymbol* Accessor(const std::wstring& name, PropertySymbol* property, bool isGetter);
		static AccessorSymbol* Getter(const std::wstring& propertyName, PropertySymbol* property);
		static AccessorSymbol* Setter(const std::wstring& propertyName, PropertySymbol* property);

		static IndexatorSymbol* Indexator(IndexatorDeclarationSyntax* node);
		static IndexatorSymbol* Indexator(const std::wstring& name, TypeSymbol* returnType);
		static IndexatorSymbol* Indexator(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*> parameters);

		static ParameterSymbol* Parameter(const std::wstring& name);
		static ParameterSymbol* Parameter(const std::wstring& name, TypeSymbol* type);
		static ParameterSymbol* Parameter(const std::wstring& name, TypeSymbol* type, bool isOptional);

		static VariableSymbol* Variable(const std::wstring& name, TypeSymbol* type);
		static VariableSymbol* Variable(const std::wstring& name, TypeSymbol* type, bool isConst);

		static TypeParameterSymbol* TypeParameter(const std::wstring& name);

		static DelegateTypeSymbol* Delegate(DelegateDeclarationSyntax* node);
		static DelegateTypeSymbol* Delegate(DelegateTypeSyntax* node);
		static DelegateTypeSymbol* Delegate(MethodSymbol* method);
		static DelegateTypeSymbol* Delegate(const std::wstring& name, TypeSymbol* returnType, std::vector<ParameterSymbol*> parameters);

		static ArrayTypeSymbol* Array(ArrayTypeSyntax* node);
		static ArrayTypeSymbol* Array(TypeSymbol* underlayingType);
		static ArrayTypeSymbol* Array(TypeSymbol* underlayingType, size_t size);
		static ArrayTypeSymbol* Array(TypeSymbol* underlayingType, size_t size, int rank);

		static GenericTypeSymbol* GenericType(TypeSymbol* underlayingType);
		static GenericTypeSymbol* GenericType(TypeSymbol* underlayingType, std::unordered_map<std::wstring, TypeSymbol*> typeArguments);

		static std::wstring FormatFullName(SyntaxSymbol* symbol);
		static std::wstring FormatFullName(SyntaxSymbol* symbol, SyntaxSymbol* parent);
		static std::wstring FormatMethodSignature(MethodSymbol* method);
		static std::wstring FormatTypeName(TypeSymbol* type);

		static MethodSymbol* CreateAnonymousMethod(const std::wstring& name, TypeSymbol* returnType);
		static MethodSymbol* CreateLambdaMethod(StatementsBlockSyntax* body);
	};
}