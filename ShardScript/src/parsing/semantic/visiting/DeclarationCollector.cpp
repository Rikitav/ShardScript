#include <shard/parsing/semantic/visiting/DeclarationCollector.h>
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
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/VariableSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>
#include <shard/syntax/symbols/TypeParameterSymbol.h>
#include <shard/syntax/nodes/TypeParametersListSyntax.h>

#include <string>

using namespace shard;

static std::wstring FormatFullNameOf(SyntaxSymbol* symbol)
{
    if (symbol->Parent == nullptr)
        return symbol->Name;

    return symbol->Parent->FullName + L"." + symbol->Name;
}

void DeclarationCollector::Declare(SyntaxSymbol *const symbol)
{
    SemanticScope* current = CurrentScope();
    current->DeclareSymbol(symbol);

    SyntaxSymbol* owner = const_cast<SyntaxSymbol*>(current->Owner);
    if (owner != nullptr)
        owner->OnSymbolDeclared(symbol);
}

void DeclarationCollector::VisitCompilationUnit(CompilationUnitSyntax *const node)
{
    // The beginning of each script file in compilation
    // First scope is declared as nullptr symbol
    PushScope(nullptr);

    // Visiting members of unit
    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitNamespaceDeclaration(NamespaceDeclarationSyntax *const node)
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

void DeclarationCollector::VisitClassDeclaration(ClassDeclarationSyntax *const node)
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
    if (node->TypeParameters != nullptr)
    {
        for (const SyntaxToken& typeParamToken : node->TypeParameters->Types)
        {
            TypeParameterSymbol* typeParamSymbol = new TypeParameterSymbol(typeParamToken.Word);
            typeParamSymbol->Parent = symbol;
            symbol->TypeParameters.push_back(typeParamSymbol);
            Declare(typeParamSymbol);
        }
    }

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    if (!symbol->IsStatic && symbol->Constructors.empty())
    {
        ConstructorSymbol* ctor = new ConstructorSymbol(L"default");
        ctor->HandleType = MethodHandleType::Body;
        ctor->Accesibility = SymbolAccesibility::Public;
        ctor->Parent = symbol;
        ctor->FullName = FormatFullNameOf(ctor);
        symbol->Constructors.push_back(ctor);
    }

    PopScope();
}

void DeclarationCollector::VisitStructDeclaration(StructDeclarationSyntax *const node)
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
    if (node->TypeParameters != nullptr)
    {
        for (const SyntaxToken& typeParamToken : node->TypeParameters->Types)
        {
            TypeParameterSymbol* typeParamSymbol = new TypeParameterSymbol(typeParamToken.Word);
            typeParamSymbol->Parent = symbol;
            symbol->TypeParameters.push_back(typeParamSymbol);
            Declare(typeParamSymbol);
        }
    }

    for (MemberDeclarationSyntax* member : node->Members)
        VisitMemberDeclaration(member);

    PopScope();
}

void DeclarationCollector::VisitDelegateDeclaration(DelegateDeclarationSyntax *const node)
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

void DeclarationCollector::VisitFieldDeclaration(FieldDeclarationSyntax *const node)
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

void DeclarationCollector::VisitMethodDeclaration(MethodDeclarationSyntax *const node)
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

    for (ParameterSymbol* const param : symbol->Parameters)
    {
        param->SlotIndex = symbol->EvalStackLocalsCount;
        symbol->EvalStackLocalsCount += 1;
    }

    PushScope(symbol);
    VisitType(node->ReturnType);
    VisitParametersList(node->Params);
    VisitStatementsBlock(node->Body);
    PopScope();
}

void DeclarationCollector::VisitConstructorDeclaration(ConstructorDeclarationSyntax *const node)
{
    // Creating symbol
    ConstructorSymbol* symbol = SymbolFactory::Constructor(node);
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

    for (ParameterSymbol* const param : symbol->Parameters)
    {
        param->SlotIndex = symbol->EvalStackLocalsCount;
        symbol->EvalStackLocalsCount += 1;
    }

    PushScope(symbol);
    VisitParametersList(node->Params);
    VisitStatementsBlock(node->Body);
    PopScope();
}

void DeclarationCollector::VisitPropertyDeclaration(PropertyDeclarationSyntax *const node)
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
    if (node->Getter != nullptr)
        VisitAccessorDeclaration(node->Getter);

    if (node->Setter != nullptr)
        VisitAccessorDeclaration(node->Setter);

    VisitExpression(node->InitializerExpression);
    PopScope();

    if (symbol->Getter != nullptr)
    {
        // Assert: extern Method cannot have body
        if (symbol->Getter->IsExtern && node->Getter->Body != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Get Accessors' marked as 'extern' cannot have Body");
    }

    if (symbol->Setter != nullptr)
    {
        // Assert: extern Method cannot have body
        if (symbol->Setter->IsExtern && node->Setter->Body != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Set Accessors' marked as 'extern' cannot have Body");
    }
}

void DeclarationCollector::VisitIndexatorDeclaration(IndexatorDeclarationSyntax *const node)
{
    // Creating symbol
    IndexatorSymbol* symbol = SymbolFactory::Indexator(node);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    if (symbol->BackingField != nullptr)
        Declare(symbol->BackingField);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        // Failed
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Indexators' owner Type");
    }
    else
    {
        // Resolving Indexators' full name
        symbol->FullName = FormatFullNameOf(symbol);

        // Checking if owner is type
        if (!symbol->Parent->IsType())
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Indexators cannot be declared outside of Classes or Structures");
        }
        else
        {
            TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

            // Assert: static Class cannot have instance Indexators
            if (!symbol->IsStatic && ownerType->IsStatic)
                Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Indexator in static Type");
        }
    }

    PushScope(symbol);
    if (node->Parameters != nullptr)
        VisitParametersList(node->Parameters);

    if (node->Getter != nullptr)
        VisitAccessorDeclaration(node->Getter);

    if (node->Setter != nullptr)
        VisitAccessorDeclaration(node->Setter);

    PopScope();

    for (ParameterSymbol* const param : symbol->Parameters)
    {
        param->SlotIndex = symbol->Getter->EvalStackLocalsCount;
        if (symbol->Getter != nullptr)
            symbol->Getter->EvalStackLocalsCount += 1;

        if (symbol->Setter != nullptr)
            symbol->Setter->EvalStackLocalsCount += 1;
    }

    if (symbol->Getter != nullptr)
    {
        // Assert: extern Method cannot have body
        if (symbol->Getter->IsExtern && node->Getter->Body != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Get Accessors' marked as 'extern' cannot have Body");
    }

    if (symbol->Setter != nullptr)
    {
        // Assert: extern Method cannot have body
        symbol->Setter->EvalStackLocalsCount += 1;
        if (symbol->Setter->IsExtern && node->Setter->Body != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Set Accessors' marked as 'extern' cannot have Body");
    }

    if (node->Getter == nullptr && node->Setter == nullptr)
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Indexator should have at least one accessor");
    }
}

void DeclarationCollector::VisitAccessorDeclaration(AccessorDeclarationSyntax *const node)
{
    // Expecting parent node to be property
    if (node->Parent->Kind != SyntaxKind::PropertyDeclaration && node->Parent->Kind != SyntaxKind::IndexatorDeclaration)
    {
        Diagnostics.ReportError(node->KeywordToken, L"Accessors cannot be declared outside of Properties or Indexers");
        return;
    }

    PropertySymbol* propertySymbol = nullptr;
    if(node->Parent->Kind == SyntaxKind::PropertyDeclaration)
	{
		PropertyDeclarationSyntax* propertyNode = static_cast<PropertyDeclarationSyntax*>(node->Parent);
		propertySymbol = LookupSymbol<PropertySymbol>(propertyNode);
	}
    else if(node->Parent->Kind == SyntaxKind::IndexatorDeclaration)
    {
        IndexatorDeclarationSyntax* indexerNode = static_cast<IndexatorDeclarationSyntax*>(node->Parent);
        propertySymbol = LookupSymbol<IndexatorSymbol>(indexerNode);
    }
    else
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Accessors cannot be declared outside of Properties or Indexators");
        return;
    }

    // Creating symbol
    AccessorSymbol* symbol = SymbolFactory::Accessor(node, propertySymbol);
    Table->BindSymbol(node, symbol);
    Declare(symbol);

    // Resolving owner symbol
    symbol->Parent = OwnerSymbol();
    if (symbol->Parent == nullptr)
    {
        // Failed
        Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Accessors' owner Type");
        return;
    }

    // Resolving Methods' full name
    symbol->FullName = FormatFullNameOf(symbol);
    symbol->IsStatic = propertySymbol->IsStatic;
    
    /*
    // Checking if owner is type
    if (!symbol->Parent->IsType())
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Accessors cannot be declared outside of Classes or Structures");
    }
    else
    {
        TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

        // Assert: static Class cannot have instance Methods
        if (!symbol->IsStatic && ownerType->IsStatic)
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Method in static Type");
    }
    */

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
                symbol->EvalStackLocalsCount += 1;
                symbol->ReturnType = SymbolTable::Primitives::Void;
                break;
            }
        }
    }
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax *const node)
{
    std::wstring varName = node->IdentifierToken.Word;
    VariableSymbol* symbol = new VariableSymbol(varName, nullptr);

    MethodSymbol *const hostMethod = FindHostMethodSymbol();
    symbol->SlotIndex = hostMethod->EvalStackLocalsCount;
    hostMethod->EvalStackLocalsCount += 1;

    Table->BindSymbol(node, symbol);
    Declare(symbol);
    PushScope(symbol);

    if (node->Expression != nullptr)
        VisitExpression(node->Expression);

    PopScope();
}
