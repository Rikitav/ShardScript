#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/semantic/SemanticScope.h>

#include <shard/syntax/SyntaxHelpers.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>
#include <shard/syntax/symbols/FFISymbol.h>

#include <string>

using namespace std;
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

        PopScope();
    }

    for (MemberDeclarationSyntax* member : node->Members)
        VisitTypeDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
    wstring namespaceName = node->IdentifierToken.Word;
    NamespaceSymbol* symbol = new NamespaceSymbol(namespaceName);
    symbol->Parent = OwnerType();

    Table->BindSymbol(node, symbol);

    if (symbol->Parent != nullptr)
    {
        if (symbol->Parent->Kind == SyntaxKind::NamespaceDeclaration)
        {
            NamespaceSymbol* parent = static_cast<NamespaceSymbol*>(symbol->Parent);
            parent->Members.push_back(symbol);
        }
    }

    Declare(symbol);
    PushScope(symbol);
    
    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
    wstring className = node->IdentifierToken.Word;
    ClassSymbol* symbol = new ClassSymbol(className);
    symbol->Parent = OwnerType();
    SetAccesibility(symbol, node->Modifiers);

    Table->BindSymbol(node, symbol);
    Declare(symbol);
    PushScope(symbol);

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitStructDeclaration(StructDeclarationSyntax* node)
{
    wstring structName = node->IdentifierToken.Word;
    StructSymbol* symbol = new StructSymbol(structName);
    symbol->Parent = OwnerType();
    SetAccesibility(symbol, node->Modifiers);

    Table->BindSymbol(node, symbol);
    Declare(symbol);
    PushScope(symbol);

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
    wstring fieldName = node->IdentifierToken.Word;
    FieldSymbol* symbol = new FieldSymbol(fieldName);
    SetAccesibility(symbol, node->Modifiers);
    symbol->DefaultValueExpression = node->InitializerExpression;

    TypeSymbol* ownerType = OwnerType();
    symbol->Parent = ownerType;

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
    wstring methodName = node->IdentifierToken.Word;
    MethodSymbol* symbol = new MethodSymbol(methodName, node->Body);
    symbol->Parent = OwnerType();
    SetAccesibility(symbol, node->Modifiers);

    for (ParameterSyntax* parameter : node->Params->Parameters)
    {
        ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
        symbol->Parameters.push_back(paramSymbol);
    }

    SyntaxSymbol* ownerSymbol = OwnerType();
    if (ownerSymbol->Kind != SyntaxKind::ClassDeclaration && ownerSymbol->Kind != SyntaxKind::StructDeclaration)
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

void DeclarationCollector::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
    wstring propertyName = node->IdentifierToken.Word;
    PropertySymbol* symbol = new PropertySymbol(propertyName);
    symbol->Parent = OwnerType();
    SetAccesibility(symbol, node->Modifiers);
    
    // Check if property is static
    for (const SyntaxToken& modifier : node->Modifiers)
    {
        if (modifier.Type == TokenType::StaticKeyword)
        {
            symbol->IsStatic = true;
            break;
        }
    }

    SyntaxSymbol* ownerSymbol = OwnerType();
    if (ownerSymbol->Kind != SyntaxKind::ClassDeclaration && ownerSymbol->Kind != SyntaxKind::StructDeclaration)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Property cannot be declared outside of types");
        return;
    }

    TypeSymbol* ownerType = static_cast<TypeSymbol*>(ownerSymbol);
    if (ownerType == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve property's owner type");
        return;
    }

    // Validate: static class cannot have instance properties
    if (!symbol->IsStatic && ownerType->IsStatic)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Static class cannot have instance properties");
    }

    // Check if this is an auto-property (has get/set but no body)
    bool isAutoProperty = (node->HasGet && node->GetBody == nullptr) || (node->HasSet && node->SetBody == nullptr);
    
    // Create backing field for auto-properties
    if (isAutoProperty)
    {
        symbol->GenerateBackingField();
        ownerType->Fields.push_back(symbol->BackingField);
        Declare(symbol->BackingField);
    }

    /*
    // Create get and set methods if they exist
    if (node->HasGet)
    {
        wstring getMethodName = propertyName + L"_get";
        MethodSymbol* getMethod = new MethodSymbol(getMethodName, node->GetBody);
        getMethod->Accesibility = symbol->Accesibility;
        getMethod->IsStatic = symbol->IsStatic;
        getMethod->ReturnType = nullptr;
        symbol->GetMethod = getMethod;
        ownerType->Methods.push_back(getMethod);
    }

    if (node->HasSet)
    {
        wstring setMethodName = propertyName + L"_set";
        MethodSymbol* setMethod = new MethodSymbol(setMethodName, node->SetBody);
        setMethod->Accesibility = symbol->Accesibility;
        setMethod->IsStatic = symbol->IsStatic;
        setMethod->ReturnType = Table::Primitives::Void;
        
        // Add 'value' parameter for setter
        ParameterSymbol* valueParam = new ParameterSymbol(L"value");
        setMethod->Parameters.push_back(valueParam);
        
        symbol->SetMethod = setMethod;
        ownerType->Methods.push_back(setMethod);
    }
    */

    ownerType->Properties.push_back(symbol);
    Declare(symbol);
    Table->BindSymbol(node, symbol);

    // Visit accessor bodies
    if (node->GetBody != nullptr)
    {
        PushScope(symbol->GetMethod);
        VisitStatementsBlock(node->GetBody);
        PopScope();
    }

    if (node->SetBody != nullptr)
    {
        PushScope(symbol->SetMethod);
        VisitStatementsBlock(node->SetBody);
        PopScope();
    }

    if (node->InitializerExpression != nullptr)
        VisitExpression(node->InitializerExpression);
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax* node)
{
    wstring varName = node->IdentifierToken.Word;
    VariableSymbol* symbol = new VariableSymbol(varName, nullptr);

    Declare(symbol);
    Table->BindSymbol(node, symbol);

    if (node->Expression != nullptr)
        VisitExpression(node->Expression);
}