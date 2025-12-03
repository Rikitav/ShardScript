#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <shard/syntax/SyntaxHelpers.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/SymbolFactory.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>

#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>
#include <shard/syntax/symbols/FFISymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>

#include <string>

using namespace shard::parsing;
using namespace shard::parsing::semantic;
using namespace shard::parsing::analysis;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

static std::wstring FormatFullNameOf(SyntaxSymbol* symbol)
{
    if (symbol->Parent == nullptr)
        return symbol->Name;

    return symbol->Parent->FullName + L"." + symbol->Name;
}

void DeclarationCollector::Declare(SyntaxSymbol* symbol)
{
    SemanticScope* current = CurrentScope();
    current->DeclareSymbol(symbol);

    SyntaxSymbol* owner = const_cast<SyntaxSymbol*>(current->Owner);
    if (owner != nullptr)
        owner->OnSymbolDeclared(symbol);
}

void DeclarationCollector::VisitCompilationUnit(CompilationUnitSyntax* node)
{
    // The beginning of each script file in compilation
    // First scope is declared as nullptr symbol
    PushScope(nullptr);

    // FFI imports declarations collecting
    // Soon...
    /* 
    if (node->Imports.size() > 0)
    {
        FFISymbol* symbol = new FFISymbol();
        PushScope(symbol);

        for (ImportDirectiveSyntax* directive : node->Imports)
            VisitImportDirective(directive);
    }
    */

    // Visiting members of unit
    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitImportDirective(ImportDirectiveSyntax* node)
{
    /* Soon...
    std::wstring methodName = node->IdentifierToken.Word;
    MethodSymbol* symbol = new MethodSymbol(methodName);
    symbol->Accesibility = SymbolAccesibility::Public;
    symbol->HandleType = MethodHandleType::ForeignInterface;
    symbol->ForeighInterfacePath = node->LibPathToken.Word;

    if (node->Params == nullptr)
        return;

    for (ParameterSyntax* parameter : node->Params->Parameters)
    {
        ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
        symbol->Parameters.push_back(paramSymbol);
    }

    Table->BindSymbol(node, symbol);
    Declare(symbol);
    */
}

void DeclarationCollector::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
    std::wstring namespaceName = node->IdentifierToken.Word;
    NamespaceSymbol* symbol = new NamespaceSymbol(namespaceName);
    Table->BindSymbol(node, symbol);

    SyntaxSymbol* parent = OwnerSymbol();
    symbol->Parent = parent;
    symbol->FullName = parent == nullptr ? symbol->Name : parent->FullName + L"." + symbol->Name;
    
    if (!node->IdentifierTokens.empty())
    {
        NamespaceNode* nsNode = Namespaces->Root;
        for (SyntaxToken token : node->IdentifierTokens)
        {
            nsNode = nsNode->LookupOrCreate(token.Word, symbol);
            continue;
        }

        CurrentScope()->Namespace = nsNode;
        symbol->Node = nsNode;
    }

    Declare(symbol);
    PushScope(symbol);
    
    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
    ClassSymbol* symbol = SymbolFactory::Class(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Delegates' owner type");
    }
    else
    {
        // Resolving Methods' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Classes can only be declared inside Namespace");
        }
    }

    PushScope(symbol);
    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitStructDeclaration(StructDeclarationSyntax* node)
{
    StructSymbol* symbol = SymbolFactory::Struct(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Delegates' owner type");
    }
    else
    {
        // Resolving Methods' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Structs can only be declared inside Namespace");
        }
    }

    PushScope(symbol);
    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitDelegateDeclaration(DelegateDeclarationSyntax* node)
{
    // Creating symbol
    DelegateTypeSymbol* symbol = SymbolFactory::Delegate(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Delegates' owner type");
    }
    else
    {
        // Resolving Methods' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Delegates cannot be declared outside of Namespaces");
        }
    }

    // Resolving Methods' full name
    PushScope(symbol);
    VisitType(node->ReturnType);
    VisitParametersList(node->Params);
    PopScope();
}

void DeclarationCollector::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
    // Creating symbol
    FieldSymbol* symbol = SymbolFactory::Field(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Fields' owner type");
    }
    else
    {
        // Resolving Methods' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (!symbol->Parent->IsType())
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Methods cannot be declared outside of Classes or Structures");
        }
        else
        {
            TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

            // Assert: static Class cannot have instance Fields
            if (!symbol->IsStatic && ownerType->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Field in static Type");
        }
    }

    PushScope(symbol);
    VisitType(node->ReturnType);
    VisitExpression(node->InitializerExpression);
    PopScope();
}

void DeclarationCollector::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
    // Creating symbol
    MethodSymbol* symbol = SymbolFactory::Method(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        // Failed
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Methods' owner Type");
    }
    else
    {
        // Resolving Methods' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (!symbol->Parent->IsType())
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Methods cannot be declared outside of Classes or Structures");
        }
        else
        {
            TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

            // Assert: static Class cannot have instance Methods
            if (!symbol->IsStatic && ownerType->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Method in static Type");

            // Assert: extern Method cannot have body
            if (symbol->IsExtern && node->Body != nullptr)
                Diagnostics.ReportError(node->IdentifierToken, L"Methods marked as 'extern' cannot have Body");

            // Assert: Method should have body
            if (!symbol->IsExtern && node->Body == nullptr)
                Diagnostics.ReportError(node->IdentifierToken, L"Method should have a Body, as it's not marked as 'extern' or 'abstract'");
        }
    }

    PushScope(symbol);
    VisitType(node->ReturnType);
    VisitParametersList(node->Params);
    VisitStatementsBlock(node->Body);
    PopScope();
}

void DeclarationCollector::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
    // Creating symbol
    MethodSymbol* symbol = SymbolFactory::Constructor(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        // Failed
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Methods' owner Type");
    }
    else
    {
        // Resolving Methods' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (!symbol->Parent->IsType())
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Methods cannot be declared outside of Classes or Structures");
        }
        else
        {
            TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

            // Assert: Constructors' name should match owners' Type name
            if (symbol->Name != ownerType->Name)
                Diagnostics.ReportError(node->IdentifierToken, L"Constructor should have same name as containing Type");

            // Assert: Type cannot have static Constructors'
            if (symbol->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, L"Type Constructors' cannot be static");

            // Assert: extern Method cannot have body
            if (symbol->IsExtern && node->Body != nullptr)
                Diagnostics.ReportError(node->IdentifierToken, L"Constructors' marked as 'extern' cannot have Body");

            // Assert: Method should have body
            if (!symbol->IsExtern && node->Body == nullptr)
                Diagnostics.ReportError(node->IdentifierToken, L"Constructor should have a Body, as it's not marked as 'extern' or 'abstract'");
        }
    }

    PushScope(symbol);
    VisitParametersList(node->Params);
    VisitStatementsBlock(node->Body);
    PopScope();
}

void DeclarationCollector::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
    // Creating symbol
    PropertySymbol* symbol = SymbolFactory::Property(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    if (symbol->BackingField != nullptr)
        Declare(symbol->BackingField);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        // Failed
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Methods' owner Type");
    }
    else
    {
        // Resolving Methods' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (!symbol->Parent->IsType())
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Methods cannot be declared outside of Classes or Structures");
        }
        else
        {
            TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

            // Assert: static Class cannot have instance Methods
            if (!symbol->IsStatic && ownerType->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Method in static Type");
        }
    }

    PushScope(symbol);
    VisitAccessorDeclaration(node->Getter);
    VisitAccessorDeclaration(node->Setter);
    VisitExpression(node->InitializerExpression);
    PopScope();

    if (symbol->Getter != nullptr)
    {
        // Assert: extern Method cannot have body
        if (symbol->Getter->IsExtern && symbol->Getter->Body != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Get Accessors' marked as 'extern' cannot have Body");
    }

    if (symbol->Setter != nullptr)
    {
        // Assert: extern Method cannot have body
        if (symbol->Setter->IsExtern && symbol->Setter->Body != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Set Accessors' marked as 'extern' cannot have Body");
    }
}

void DeclarationCollector::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
    // Expecting parent node to be property
    if (node->Parent->Kind != SyntaxKind::PropertyDeclaration)
    {
        Diagnostics.ReportError(node->KeywordToken, L"Accessors cannot be declared outside of Properties");
        return;
    }

    PropertyDeclarationSyntax* propertyNode = static_cast<PropertyDeclarationSyntax*>(node->Parent);
    PropertySymbol* propertySymbol = LookupSymbol<PropertySymbol>(propertyNode);

    // Creating symbol
    AccessorSymbol* symbol = SymbolFactory::Accessor(node, propertySymbol);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    if (node->Body != nullptr)
    {
        switch (node->KeywordToken.Type)
        {
            case TokenType::GetKeyword:
            {
                symbol->ReturnType = propertySymbol->ReturnType;
                break;
            }

            case TokenType::SetKeyword:
            {
                ParameterSymbol* valueParam = new ParameterSymbol(L"value");
                symbol->Parameters.push_back(valueParam);
                symbol->ReturnType = SymbolTable::Primitives::Void;
                break;
            }
        }
    }
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax* node)
{
    std::wstring varName = node->IdentifierToken.Word;
    VariableSymbol* symbol = new VariableSymbol(varName, nullptr);

    Table->BindSymbol(node, symbol);
    Declare(symbol);
    PushScope(symbol);

    if (node->Expression != nullptr)
        VisitExpression(node->Expression);

    PopScope();
}
