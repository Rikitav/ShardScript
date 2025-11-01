#include <shard/parsing/visiting/DeclarationCollector.h>
#include <shard/parsing/semantic/SemanticScope.h>

#include <shard/syntax/SyntaxHelpers.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>

#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>

#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
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
    if (node->Imports.size() > 0)
    {
        FFISymbol* symbol = new FFISymbol();
        pushScope(symbol);

        for (ImportDirectiveSyntax* directive : node->Imports)
            VisitImportDirective(directive);
    }

    for (MemberDeclarationSyntax* member : node->Members)
        VisitTypeDeclaration(member);
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

    scopeStack.top()->DeclareSymbol(symbol);
    symbolTable->BindSymbol(node, symbol);

    if (node->Body != nullptr)
    {
        pushScope(symbol);
        for (ParameterSymbol* parameter : symbol->Parameters)
        {
            scopeStack.top()->DeclareSymbol(parameter);
        }

        VisitStatementsBlock(node->Body);
        scopeStack.pop();
    }
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax* node)
{
    wstring varName = node->IdentifierToken.Word;
    VariableSymbol* symbol = new VariableSymbol(varName);

    scopeStack.top()->DeclareSymbol(symbol);
    symbolTable->BindSymbol(node, symbol);
}