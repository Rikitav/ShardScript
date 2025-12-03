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

#include <shard/syntax/symbols/DelegateTypeSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>

namespace shard::syntax
{
	class SHARD_API SymbolFactory
	{
	public:
		static shard::syntax::symbols::StructSymbol* Struct(shard::syntax::nodes::StructDeclarationSyntax* node);

		static shard::syntax::symbols::ClassSymbol* Class(shard::syntax::nodes::ClassDeclarationSyntax* node);

		static shard::syntax::symbols::FieldSymbol* Field(shard::syntax::nodes::FieldDeclarationSyntax* node);

		static shard::syntax::symbols::PropertySymbol* Property(shard::syntax::nodes::PropertyDeclarationSyntax* node);

		static shard::syntax::symbols::AccessorSymbol* Accessor(shard::syntax::nodes::AccessorDeclarationSyntax* node, shard::syntax::symbols::PropertySymbol* propertySymbol, bool setProperty = true);

		static shard::syntax::symbols::MethodSymbol* Method(shard::syntax::nodes::MethodDeclarationSyntax* node);

		static shard::syntax::symbols::MethodSymbol* Constructor(shard::syntax::nodes::ConstructorDeclarationSyntax* node);

		static shard::syntax::symbols::DelegateTypeSymbol* Delegate(shard::syntax::nodes::DelegateDeclarationSyntax* node);
		static shard::syntax::symbols::DelegateTypeSymbol* Delegate(shard::syntax::nodes::DelegateTypeSyntax* node);
		static shard::syntax::symbols::DelegateTypeSymbol* Delegate(shard::syntax::symbols::MethodSymbol* method);
	};
}