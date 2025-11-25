#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <shard/syntax/SyntaxHelpers.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SymbolAccesibility.h>

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

#include <string>

using namespace shard::parsing;
using namespace shard::parsing::semantic;
using namespace shard::parsing::analysis;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;

void DeclarationCollector::VisitCompilationUnit(CompilationUnitSyntax* node)
{
    PushScope(nullptr);
    if (node->Imports.size() > 0)
    {
        FFISymbol* symbol = new FFISymbol();
        PushScope(symbol);

        for (ImportDirectiveSyntax* directive : node->Imports)
            VisitImportDirective(directive);
    }

    for (MemberDeclarationSyntax* member : node->Members)
        VisitTypeDeclaration(member);

    if (node->Imports.size() > 0)
        PopScope();

    PopScope();
}

void DeclarationCollector::VisitImportDirective(ImportDirectiveSyntax* node)
{

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
    std::wstring className = node->IdentifierToken.Word;
    ClassSymbol* symbol = new ClassSymbol(className);
    SetAccesibility(symbol, node->Modifiers);
    OwnerNamespaceNode()->Types.push_back(symbol);

    SyntaxSymbol* parent = OwnerSymbol();
    symbol->Parent = parent;
    symbol->FullName = parent == nullptr ? symbol->Name : parent->FullName + L"." + symbol->Name;

    Table->BindSymbol(node, symbol);
    Declare(symbol);
    PushScope(symbol);

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitStructDeclaration(StructDeclarationSyntax* node)
{
    std::wstring structName = node->IdentifierToken.Word;
    StructSymbol* symbol = new StructSymbol(structName);
    symbol->Parent = OwnerSymbol();
    SetAccesibility(symbol, node->Modifiers);
    OwnerNamespaceNode()->Types.push_back(symbol);

    SyntaxSymbol* parent = OwnerSymbol();
    symbol->Parent = parent;
    symbol->FullName = parent == nullptr ? symbol->Name : parent->FullName + L"." + symbol->Name;

    Table->BindSymbol(node, symbol);
    Declare(symbol);
    PushScope(symbol);

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
    std::wstring fieldName = node->IdentifierToken.Word;
    FieldSymbol* symbol = new FieldSymbol(fieldName);
    SetAccesibility(symbol, node->Modifiers);
    symbol->DefaultValueExpression = node->InitializerExpression;

    TypeSymbol* ownerType = OwnerType();
    symbol->Parent = ownerType;
    symbol->FullName = ownerType == nullptr ? symbol->Name : ownerType->FullName + L"." + symbol->Name;

    if (symbol->Parent == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve method's owner type");
    }
    else
    {
        // Validate: static class cannot have instance fields
        if (!symbol->IsStatic && ownerType->IsStatic)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Static class cannot have instance fields");
        }
        
        ownerType->Fields.push_back(symbol);
    }

    Declare(symbol);
    Table->BindSymbol(node, symbol);
}

void DeclarationCollector::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
    std::wstring methodName = node->IdentifierToken.Word;
    MethodSymbol* symbol = new MethodSymbol(methodName, node->Body);
    SetAccesibility(symbol, node->Modifiers);

    for (ParameterSyntax* parameter : node->Params->Parameters)
    {
        ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
        symbol->Parameters.push_back(paramSymbol);
    }

    SyntaxSymbol* ownerSymbol = OwnerType();
    if (!ownerSymbol->IsType())
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Method cannot be declared within types");
        return;
    }

    TypeSymbol* ownerType = static_cast<TypeSymbol*>(ownerSymbol);
    if (ownerType == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve method's owner type");
        return;
    }

    symbol->Parent = ownerType;
    symbol->FullName = ownerType == nullptr ? symbol->Name : ownerType->FullName + L"." + symbol->Name;

    ownerType->Methods.push_back(symbol);
    Declare(symbol);
    Table->BindSymbol(node, symbol);

    if (node->Body != nullptr)
    {
        PushScope(symbol);
        if (!symbol->IsStatic)
        {
            if (ownerType->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static method's in static type");
        }

        VisitStatementsBlock(node->Body);
        PopScope();
    }
}

void DeclarationCollector::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
    std::wstring methodName = node->IdentifierToken.Word;
    MethodSymbol* symbol = new MethodSymbol(methodName, node->Body);
    symbol->ReturnType = SymbolTable::Primitives::Void;
    SetAccesibility(symbol, node->Modifiers);

    if (symbol->IsStatic)
        Diagnostics.ReportError(node->IdentifierToken, L"Type constructor cannot be static");

    for (ParameterSyntax* parameter : node->Params->Parameters)
    {
        ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
        symbol->Parameters.push_back(paramSymbol);
    }

    SyntaxSymbol* ownerSymbol = OwnerType();
    if (!ownerSymbol->IsType())
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Method cannot be declared within types");
        //return;
    }

    TypeSymbol* ownerType = static_cast<TypeSymbol*>(ownerSymbol);
    if (ownerType == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve method's owner type");
        return;
    }

    if (ownerType->Name != methodName)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Name of constructor should match class's name");
        //return;
    }

    symbol->Parent = ownerType;
    symbol->FullName = ownerType == nullptr ? symbol->Name : ownerType->FullName + L"." + symbol->Name;

    ownerType->Constructors.push_back(symbol);
    Declare(symbol);
    Table->BindSymbol(node, symbol);

    if (node->Body != nullptr)
    {
        PushScope(symbol);
        if (!symbol->IsStatic)
        {
            if (ownerType->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static method's in static type");
        }

        VisitStatementsBlock(node->Body);
        PopScope();
    }
}

void DeclarationCollector::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
    std::wstring propertyName = node->IdentifierToken.Word;
    PropertySymbol* symbol = new PropertySymbol(propertyName);
    SetAccesibility(symbol, node->Modifiers);
    symbol->DefaultValueExpression = node->InitializerExpression;

    SyntaxSymbol* ownerSymbol = OwnerSymbol();
    if (!ownerSymbol->IsType())
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Property cannot be declared outside of types");
        return;
    }

    // Validate: static class cannot have instance properties
    TypeSymbol* ownerType = static_cast<TypeSymbol*>(ownerSymbol);
    if (ownerType == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve method's owner type");
        return;
    }

    symbol->Parent = ownerType;
    symbol->FullName = ownerType == nullptr ? symbol->Name : ownerType->FullName + L"." + symbol->Name;

    if (!symbol->IsStatic && ownerType->IsStatic)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Static class cannot have instance properties");
    }

    bool isAutoProperty =
        (node->Getter != nullptr && node->Getter->Body == nullptr) ||
        (node->Setter != nullptr && node->Setter->Body == nullptr);
    
    // Create backing field for auto-properties
    if (isAutoProperty)
    {
        symbol->GenerateBackingField();
        ownerType->Fields.push_back(symbol->BackingField);
        Declare(symbol->BackingField);
    }

    ownerType->Properties.push_back(symbol);
    Table->BindSymbol(node, symbol);
    Declare(symbol);
    PushScope(symbol);

    // Visit accessor bodies
    if (node->Getter != nullptr)
        VisitAccessorDeclaration(node->Getter);

    if (node->Setter != nullptr)
        VisitAccessorDeclaration(node->Getter);

    if (node->InitializerExpression != nullptr)
        VisitExpression(node->InitializerExpression);
    
    PopScope();
}

void DeclarationCollector::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
    SyntaxSymbol* owner = OwnerSymbol();
    if (owner->Kind != SyntaxKind::PropertyDeclaration)
        Diagnostics.ReportError(node->KeywordToken, L"Accessors cannot be declared outside of properties");

    TypeSymbol* ownerType = OwnerType();
    if (ownerType == nullptr)
        return; // diagnostic already generated in property
    
    PropertySymbol* ownerProp = static_cast<PropertySymbol*>(owner);
    AccessorSymbol* symbol = new AccessorSymbol(ownerProp->Name + L"_" + node->KeywordToken.Word);
    symbol->Accesibility = SymbolAccesibility::Public;
    SetAccesibility(symbol, node->Modifiers);

    if (node->Body != nullptr)
    {
        MethodSymbol* method = symbol->Method = new MethodSymbol(symbol->Name, node->Body);
        SetAccesibility(method, node->Modifiers);
        method->IsStatic = ownerProp->IsStatic;
        ownerType->Methods.push_back(method);

        switch (node->KeywordToken.Type)
        {
            case TokenType::GetKeyword:
            {
                method->ReturnType = ownerProp->ReturnType;
                break;
            }

            case TokenType::SetKeyword:
            {
                method->ReturnType = SymbolTable::Primitives::Void;
                ParameterSymbol* valueParam = new ParameterSymbol(L"value");
                method->Parameters.push_back(valueParam);
                break;
            }
        }
    }

    switch (node->KeywordToken.Type)
    {
        case TokenType::GetKeyword:
        {
            ownerProp->Getter = symbol;
            break;
        }

        case TokenType::SetKeyword:
        {
            ownerProp->Setter = symbol;
            break;
        }
    }
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax* node)
{
    std::wstring varName = node->IdentifierToken.Word;
    VariableSymbol* symbol = new VariableSymbol(varName, nullptr);

    Declare(symbol);
    Table->BindSymbol(node, symbol);

    if (node->Expression != nullptr)
        VisitExpression(node->Expression);
}