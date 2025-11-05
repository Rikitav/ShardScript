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

void DeclarationCollector::pushScope(SyntaxSymbol* symbol)
{
    SemanticScope* newScope = new SemanticScope(symbol, scopeStack.top());
    scopeStack.push(newScope);
}

void DeclarationCollector::VisitCompilationUnit(CompilationUnitSyntax* node)
{
    pushScope(nullptr);
    if (node->Imports.size() > 0)
    {
        FFISymbol* symbol = new FFISymbol();
        pushScope(symbol);

        for (ImportDirectiveSyntax* directive : node->Imports)
            VisitImportDirective(directive);
    }

    for (MemberDeclarationSyntax* member : node->Members)
        VisitTypeDeclaration(member);

    pushScope(nullptr);
}

void DeclarationCollector::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
    wstring namespaceName = node->IdentifierToken.Word;
    NamespaceSymbol* symbol = new NamespaceSymbol(namespaceName);

    symbolTable->BindSymbol(node, symbol);

    NamespaceSymbol* parent = static_cast<NamespaceSymbol*>((SyntaxSymbol*)scopeStack.top()->Owner);
    if (parent != nullptr)
        symbol->Parent = parent;
   
    scopeStack.top()->DeclareSymbol(symbol);
    pushScope(symbol);
    
    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    scopeStack.pop();
}

void DeclarationCollector::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
    wstring className = node->IdentifierToken.Word;
    ClassSymbol* symbol = new ClassSymbol(className);
    SetAccesibility(symbol, node->Modifiers);

    symbolTable->BindSymbol(node, symbol);
    scopeStack.top()->DeclareSymbol(symbol);
    pushScope(symbol);

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    scopeStack.pop();
}

void DeclarationCollector::VisitStructDeclaration(StructDeclarationSyntax* node)
{
    wstring structName = node->IdentifierToken.Word;
    StructSymbol* symbol = new StructSymbol(structName);
    SetAccesibility(symbol, node->Modifiers);

    symbolTable->BindSymbol(node, symbol);
    scopeStack.top()->DeclareSymbol(symbol);
    pushScope(symbol);

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    scopeStack.pop();
}

void DeclarationCollector::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
    wstring fieldName = node->IdentifierToken.Word;
    FieldSymbol* symbol = new FieldSymbol(fieldName);
    SetAccesibility(symbol, node->Modifiers);

    TypeSymbol* ownerType = static_cast<TypeSymbol*>((SyntaxSymbol*)scopeStack.top()->Owner);
    if (ownerType == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, "Cannot resolve method's owner type");
    }
    else
    {
        // Validate: static class cannot have instance fields
        if (!symbol->IsStatic && ownerType->IsStatic)
        {
            Diagnostics.ReportError(node->IdentifierToken, "Static class cannot have instance fields");
        }
        
        ownerType->Fields.push_back(symbol);
    }

    scopeStack.top()->DeclareSymbol(symbol);
    symbolTable->BindSymbol(node, symbol);
}

void DeclarationCollector::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
    wstring methodName = node->IdentifierToken.Word;
    MethodSymbol* symbol = new MethodSymbol(methodName, node->Body);
    SetAccesibility(symbol, node->Modifiers);

    for (ParameterSyntax* parameter : node->Params->Parameters)
    {
        ParameterSymbol* paramSymbol = new ParameterSymbol(parameter->Identifier.Word);
        symbol->Parameters.push_back(paramSymbol);
    }

    SyntaxSymbol* ownerSymbol = const_cast<SyntaxSymbol*>(scopeStack.top()->Owner);
    if (ownerSymbol->Kind != SyntaxKind::ClassDeclaration && ownerSymbol->Kind != SyntaxKind::StructDeclaration)
    {
        Diagnostics.ReportError(node->IdentifierToken, "Method cannot be declared within types");
        return;
    }

    TypeSymbol* ownerType = static_cast<TypeSymbol*>(ownerSymbol);
    if (ownerType == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, "Cannot resolve method's owner type");
        return;
    }

    ownerType->Methods.push_back(symbol);
    scopeStack.top()->DeclareSymbol(symbol);
    symbolTable->BindSymbol(node, symbol);

    if (node->Body != nullptr)
    {
        pushScope(symbol);
        if (!symbol->IsStatic)
        {
            if (ownerType->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, "Cannot declare a non static method's in static type");
        }

        VisitStatementsBlock(node->Body);
        scopeStack.pop();
    }
}

void DeclarationCollector::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
    wstring propertyName = node->IdentifierToken.Word;
    PropertySymbol* symbol = new PropertySymbol(propertyName);
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

    SyntaxSymbol* ownerSymbol = const_cast<SyntaxSymbol*>(scopeStack.top()->Owner);
    if (ownerSymbol->Kind != SyntaxKind::ClassDeclaration && ownerSymbol->Kind != SyntaxKind::StructDeclaration)
    {
        Diagnostics.ReportError(node->IdentifierToken, "Property cannot be declared outside of types");
        return;
    }

    TypeSymbol* ownerType = static_cast<TypeSymbol*>(ownerSymbol);
    if (ownerType == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, "Cannot resolve property's owner type");
        return;
    }

    // Validate: static class cannot have instance properties
    if (!symbol->IsStatic && ownerType->IsStatic)
    {
        Diagnostics.ReportError(node->IdentifierToken, "Static class cannot have instance properties");
    }

    // Check if this is an auto-property (has get/set but no body)
    bool isAutoProperty = (node->HasGet && node->GetBody == nullptr) || 
                          (node->HasSet && node->SetBody == nullptr);
    
    // Create backing field for auto-properties
    if (isAutoProperty)
    {
        wstring backingFieldName = L"<" + propertyName + L">k__BackingField"; // C#-like naming
        FieldSymbol* backingField = new FieldSymbol(backingFieldName);
        backingField->Accesibility = SymbolAccesibility::Private;
        backingField->IsStatic = symbol->IsStatic;
        backingField->ReturnType = nullptr; // Will be set in TypeBinder
        symbol->BackingField = backingField;
        ownerType->Fields.push_back(backingField);
        scopeStack.top()->DeclareSymbol(backingField);
    }

    // Create get and set methods if they exist
    if (node->HasGet)
    {
        wstring getMethodName = propertyName + L"_get";
        MethodSymbol* getMethod = new MethodSymbol(getMethodName, node->GetBody);
        getMethod->Accesibility = symbol->Accesibility;
        getMethod->IsStatic = symbol->IsStatic;
        getMethod->ReturnType = nullptr; // Will be set in TypeBinder
        symbol->GetMethod = getMethod;
        ownerType->Methods.push_back(getMethod);
    }

    if (node->HasSet)
    {
        wstring setMethodName = propertyName + L"_set";
        MethodSymbol* setMethod = new MethodSymbol(setMethodName, node->SetBody);
        setMethod->Accesibility = symbol->Accesibility;
        setMethod->IsStatic = symbol->IsStatic;
        setMethod->ReturnType = SymbolTable::Primitives::Void;
        
        // Add 'value' parameter for setter
        ParameterSymbol* valueParam = new ParameterSymbol(L"value");
        setMethod->Parameters.push_back(valueParam);
        
        symbol->SetMethod = setMethod;
        ownerType->Methods.push_back(setMethod);
    }

    ownerType->Properties.push_back(symbol);
    scopeStack.top()->DeclareSymbol(symbol);
    symbolTable->BindSymbol(node, symbol);

    // Visit accessor bodies
    if (node->GetBody != nullptr)
    {
        pushScope(symbol->GetMethod);
        VisitStatementsBlock(node->GetBody);
        scopeStack.pop();
    }

    if (node->SetBody != nullptr)
    {
        pushScope(symbol->SetMethod);
        VisitStatementsBlock(node->SetBody);
        scopeStack.pop();
    }

    if (node->InitializerExpression != nullptr)
        VisitExpression(node->InitializerExpression);
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax* node)
{
    wstring varName = node->IdentifierToken.Word;
    VariableSymbol* symbol = new VariableSymbol(varName);

    scopeStack.top()->DeclareSymbol(symbol);
    symbolTable->BindSymbol(node, symbol);

    if (node->Expression != nullptr)
        VisitExpression(node->Expression);
}