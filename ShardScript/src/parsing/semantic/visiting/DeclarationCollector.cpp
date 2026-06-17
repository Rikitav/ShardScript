#include <shard/parsing/semantic/visiting/DeclarationCollector.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/SyntaxHelpers.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/syntax/SymbolFactory.hpp>

#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>
#include <shard/syntax/nodes/AttributeSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>

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
}

void DeclarationCollector::VisitCompilationUnit(CompilationUnitSyntax *const node)
{
    // The beginning of each script file in compilation
    // First scope is declared as nullptr symbol
    PushScope(nullptr);

    if (node->Namespace != nullptr)
    {
        NamespaceSymbol* symbol = LookupSymbol<NamespaceSymbol>(node->Namespace.get()).value_or(nullptr);
        if (symbol == nullptr)
        {
			// Creating symbol
			symbol = Factory.Namespace(node->Namespace.get());

            symbol->Parent = OwnerSymbol();
            if (symbol->Parent != nullptr)
            {
                symbol->FullName = symbol->Parent->FullName + L"." + symbol->Name;
                symbol->Parent->OnSymbolDeclared(symbol);
            }

            if (!node->Namespace->IdentifierTokens.empty())
            {
                NamespaceNode* nsNode = Namespaces->Root;
                for (SyntaxToken token : node->Namespace->IdentifierTokens)
                {
                    nsNode = nsNode->LookupOrCreate(token.Word, symbol);
                }

                CurrentScope()->Namespace = nsNode;
                symbol->Node = nsNode;
            }
        }

        Declare(symbol);
        PushScope(symbol);
    }

    // Visiting members of unit
    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    if (node->Namespace != nullptr)
        PopScope();

    PopScope();
}

void DeclarationCollector::VisitNamespaceDeclaration(NamespaceDeclarationSyntax *const node)
{
    // Namespace declarations are now handled inline in VisitCompilationUnit
    // This method is kept for SyntaxVisitor compatibility only
}

void DeclarationCollector::VisitClassDeclaration(ClassDeclarationSyntax *const node)
{
    ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		// Creating symbol
		symbol = Factory.Class(node);

        // Resolving owner symbol
        symbol->Parent = OwnerSymbol();
        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Classes' owner type");
        }
        else
        {
            // Resolving Methods' full name
            symbol->FullName = FormatFullNameOf(symbol);
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Classes can only be declared inside Namespace");
            }
        }

        if (node->TypeParameters != nullptr)
        {
            for (std::size_t i = 0; i < node->TypeParameters->Types.size(); i++)
            {
                SyntaxToken& param = node->TypeParameters->Types.at(i);
                TypeParameterSymbol* typeParamSymbol = Factory.TypeParameter(param.Word);
                typeParamSymbol->Parent = symbol;
                typeParamSymbol->TypeArgumentIndex = static_cast<std::uint16_t>(i);

                symbol->TypeParameters.push_back(typeParamSymbol);
            }
        }
    }

    Declare(symbol);
    PushScope(symbol);

    for (const auto& typeParam : symbol->TypeParameters)
        Declare(typeParam);

    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    if (symbol->Linking == LINK_INSTANCE && symbol->Constructors.empty())
    {
        ConstructorSymbol* ctor = Factory.Constructor(symbol, ACS_PUBLIC);
        ctor->FullName = FormatFullNameOf(ctor);

        symbol->Constructors.push_back(ctor);
    }

    PopScope();
}

void DeclarationCollector::VisitStructDeclaration(StructDeclarationSyntax *const node)
{
    StructSymbol* symbol = LookupSymbol<StructSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		// Creating symbol
		symbol = Factory.Struct(node);

        // Resolving owner symbol
        symbol->Parent = OwnerSymbol();
        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Structs' owner type");
        }
        else
        {
            // Resolving Methods' full name
            symbol->FullName = FormatFullNameOf(symbol);
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Structs can only be declared inside Namespace");
            }
        }

        if (node->TypeParameters != nullptr)
        {
            for (std::size_t i = 0; i < node->TypeParameters->Types.size(); i++)
            {
                SyntaxToken& param = node->TypeParameters->Types.at(i);
                TypeParameterSymbol* typeParamSymbol = Factory.TypeParameter(param.Word);
                typeParamSymbol->Parent = symbol;
                typeParamSymbol->TypeArgumentIndex = static_cast<std::uint16_t>(i);

                symbol->TypeParameters.push_back(typeParamSymbol);
            }
        }
    }

    Declare(symbol);
    PushScope(symbol);

    for (const auto& typeParam : symbol->TypeParameters)
        Declare(typeParam);

    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    if (symbol->Linking == LINK_INSTANCE && symbol->Constructors.empty())
    {
        ConstructorSymbol* ctor = Factory.Constructor(symbol, ACS_PUBLIC);
        ctor->FullName = FormatFullNameOf(ctor);

        symbol->Constructors.push_back(ctor);
    }

    PopScope();
}

void DeclarationCollector::VisitDelegateDeclaration(DelegateDeclarationSyntax *const node)
{
    // Creating symbol
    DelegateTypeSymbol* symbol = LookupSymbol<DelegateTypeSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		// Creating symbol
		symbol = Factory.Delegate(node);

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
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Delegates cannot be declared outside of Namespaces");
            }
        }
    }

    Declare(symbol);
    PushScope(symbol);

    VisitType(node->ReturnType.get());
    VisitParametersList(node->ParametersList.get());

    PopScope();
}

void DeclarationCollector::VisitFieldDeclaration(FieldDeclarationSyntax *const node)
{
    // Creating symbol
    FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        // Creating symbol
		symbol = Factory.Field(node);

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
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (!symbol->Parent->IsType())
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Fields cannot be declared outside of Classes or Structures");
            }
            else
            {
                TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

                // Assert: static Class cannot have instance Fields
                if (symbol->Linking == LINK_INSTANCE && ownerType->Linking == LINK_STATIC)
                    Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Field in static Type");
            }
        }
    }

    Declare(symbol);
    PushScope(symbol);

    VisitType(node->ReturnType.get());
    VisitExpression(node->InitializerExpression.get());
    
    PopScope();
}

void DeclarationCollector::VisitMethodDeclaration(MethodDeclarationSyntax *const node)
{
    // Creating symbol
    MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		// Creating symbol
		symbol = Factory.Method(node);

        // Creating parameters symbols
        for (const auto& parameter : node->ParametersList->Parameters)
        {
            auto paramSymbol = Factory.Parameter(parameter.get());
            symbol->Parameters.push_back(paramSymbol);
        }

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
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (symbol->Parent->Kind == SyntaxKind::NamespaceDeclaration)
            {
                // Assert: namespace cannot have instance Methods
                if (symbol->Linking == LINK_INSTANCE)
                    Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Method in namespace");
            }
            else if (symbol->Parent->IsType())
            {
                TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

                // Assert: static Class cannot have instance Methods
                if (symbol->Linking == LINK_INSTANCE && ownerType->Linking == LINK_STATIC)
                    Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Method in static Type");
            }
            else if (symbol->Parent->IsMember())
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Methods cannot be declared inside of members");
            }
            else
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Methods cannot be declared inside of UNKNOWN");
            }

            // Assert: extern Method cannot have body
            if (symbol->IsExtern && node->Body != nullptr)
                Diagnostics.ReportError(node->IdentifierToken, L"Methods marked as 'extern' cannot have Body");

            // Assert: Method should have body
            if (!symbol->IsExtern && node->Body == nullptr)
                Diagnostics.ReportError(node->IdentifierToken, L"Method should have a Body, as it's not marked as 'extern' or 'abstract'");
        }

        std::uint16_t baseIndex = symbol->Linking == LINK_STATIC ? 0 : 1;
        for (std::size_t i = 0; i < symbol->Parameters.size(); ++i)
            symbol->Parameters[i]->SlotIndex = baseIndex + i;

        ApplyMethodAttributes(symbol, node->Attributes);
    }

    Declare(symbol);
    PushScope(symbol);

    VisitType(node->ReturnType.get());
    VisitParametersList(node->ParametersList.get());
    VisitStatementsBlock(node->Body.get());

    PopScope();
}

void DeclarationCollector::VisitConstructorDeclaration(ConstructorDeclarationSyntax *const node)
{
    // Creating symbol
    ConstructorSymbol* symbol = LookupSymbol<ConstructorSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		// Creating symbol
		symbol = Factory.Constructor(node);

		// Creating parameters symbols
        for (const auto& parameter : node->ParametersList->Parameters)
        {
            auto paramSymbol = Factory.Parameter(parameter.get());
            symbol->Parameters.push_back(paramSymbol);
        }

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
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (!symbol->Parent->IsType())
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Constructors cannot be declared outside of Classes or Structures");
            }
            else
            {
                TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

                // Assert: Constructors' name should match owners' Type name
                // (skip for 'init' keyword which is the new constructor syntax)
                if (node->IdentifierToken.Type != TokenType::InitKeyword && symbol->Name != ownerType->Name)
                    Diagnostics.ReportError(node->IdentifierToken, L"Constructor should have same name as containing Type");

                // Assert: Type cannot have static Constructors'
                if (symbol->Linking == LINK_STATIC)
                    Diagnostics.ReportError(node->IdentifierToken, L"Type Constructors' cannot be static");

                // Assert: extern Method cannot have body
                if (symbol->IsExtern && node->Body != nullptr)
                    Diagnostics.ReportError(node->IdentifierToken, L"Constructors' marked as 'extern' cannot have Body");

                // Assert: Method should have body
                if (!symbol->IsExtern && node->Body == nullptr)
                    Diagnostics.ReportError(node->IdentifierToken, L"Constructor should have a Body, as it's not marked as 'extern' or 'abstract'");
            }
        }

        std::uint16_t baseIndex = symbol->Linking == LINK_STATIC ? 0 : 1;
        for (std::size_t i = 0; i < symbol->Parameters.size(); ++i)
            symbol->Parameters[i]->SlotIndex = baseIndex + i;

        ApplyMethodAttributes(symbol, node->Attributes);
    }

    Declare(symbol);
    PushScope(symbol);

    VisitParametersList(node->ParametersList.get());
    VisitStatementsBlock(node->Body.get());
    
    PopScope();
}

void DeclarationCollector::VisitPropertyDeclaration(PropertyDeclarationSyntax *const node)
{
    // Creating symbol
    PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        // Creating symbol
        symbol = Factory.Property(node);

        // Create backing field for auto-properties
        bool isAutoProperty =
            (node->Getter != nullptr && node->Getter->Body == nullptr) ||
            (node->Setter != nullptr && node->Setter->Body == nullptr);

        if (isAutoProperty)
            Factory.BackingField(symbol);

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
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (!symbol->Parent->IsType())
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Properties cannot be declared outside of Classes or Structures");
            }
            else
            {
                TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

                // Assert: static Class cannot have instance Methods
                if (symbol->Linking == LINK_INSTANCE && ownerType->Linking == LINK_STATIC)
                    Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Method in static Type");
            }
        }
    }

    if (symbol->BackingField != nullptr)
        Declare(symbol->BackingField);

    Declare(symbol);
    PushScope(symbol);

    if (node->Getter != nullptr)
        VisitAccessorDeclaration(node->Getter.get());

    if (node->Setter != nullptr)
        VisitAccessorDeclaration(node->Setter.get());

    VisitExpression(node->InitializerExpression.get());

    PopScope();
}

void DeclarationCollector::VisitIndexatorDeclaration(IndexatorDeclarationSyntax *const node)
{
    // Creating symbol
    IndexatorSymbol* symbol = LookupSymbol<IndexatorSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        // Creating symbol
        symbol = Factory.Indexator(node);

        // Creating parameters symbols
        for (const auto& parameter : node->ParametersList->Parameters)
        {
            auto paramSymbol = Factory.Parameter(parameter.get());
            symbol->Parameters.push_back(paramSymbol);
        }

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
            symbol->Parent->OnSymbolDeclared(symbol);

            // Checking if owner is type
            if (!symbol->Parent->IsType())
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Indexators cannot be declared outside of Classes or Structures");
            }
            else
            {
                TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

                // Assert: static Class cannot have instance Indexators
                if (symbol->Linking == LINK_INSTANCE && ownerType->Linking == LINK_STATIC)
                    Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Indexator in static Type");
            }
        }
    }

    Declare(symbol);

    if (symbol->BackingField != nullptr)
        Declare(symbol->BackingField);

    PushScope(symbol);
    if (node->ParametersList != nullptr)
        VisitParametersList(node->ParametersList.get());

    if (node->Getter != nullptr)
        VisitAccessorDeclaration(node->Getter.get());

    if (node->Setter != nullptr)
        VisitAccessorDeclaration(node->Setter.get());

    PopScope();

    // TODO: fix
    /*
    for (ParameterSymbol* const param : symbol->Parameters)
    {
        if (symbol->Getter != nullptr)
        {
            param->SlotIndex = symbol->Getter->GetEvalStackArgumentsCount() + symbol->Getter->AddVariableCount();
        }

        if (symbol->Setter != nullptr)
        {
            symbol->Setter->AddVariableCount();
        }
    }
    */

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
		propertySymbol = LookupSymbol<PropertySymbol>(propertyNode).value_or(nullptr);
	}
    else if(node->Parent->Kind == SyntaxKind::IndexatorDeclaration)
    {
        IndexatorDeclarationSyntax* indexerNode = static_cast<IndexatorDeclarationSyntax*>(node->Parent);
        propertySymbol = LookupSymbol<IndexatorSymbol>(indexerNode).value_or(nullptr);
    }
    else
    {
        Diagnostics.ReportError(node->IdentifierToken, L"Accessors cannot be declared outside of Properties or Indexators");
        return;
    }

    AccessorSymbol* symbol = LookupSymbol<AccessorSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        // Creating symbol
        symbol = Factory.Accessor(node, propertySymbol);

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
        symbol->Linking = propertySymbol->Linking;
        symbol->Parent->OnSymbolDeclared(symbol);

        ApplyMethodAttributes(symbol, node->Attributes);

        // Assert: extern Method cannot have body
        if (symbol->IsExtern && node->Body != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Set Accessors' marked as 'extern' cannot have Body");

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
                    symbol->ReturnType = SymbolTable::Primitives::Void;
                    break;
                }
            }
        }
    }

    // Assign slot indices for accessor parameters (e.g. setter's 'value')
    {
        std::uint16_t baseIndex = symbol->Linking == LINK_STATIC ? 0 : 1;
        for (std::size_t i = 0; i < symbol->Parameters.size(); ++i)
            symbol->Parameters[i]->SlotIndex = baseIndex + i;
    }

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
                symbol->ReturnType = SymbolTable::Primitives::Void;
                break;
            }
        }
    }
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax *const node)
{
    VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        std::wstring varName = node->IdentifierToken.Word;
        symbol = Factory.Variable(node);

        MethodSymbol *const hostMethod = FindHostMethodSymbol();
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();


        symbol->Parent = OwnerSymbol();
        if (symbol->Parent != nullptr)
        {
            symbol->FullName = symbol->Parent->FullName + L"." + symbol->Name;
            symbol->Parent->OnSymbolDeclared(symbol);
        }
    }

    Declare(symbol);
    PushScope(symbol);

    if (node->Expression != nullptr)
        VisitExpression(node->Expression.get());

    PopScope();
}

void DeclarationCollector::VisitForEachStatement(ForEachStatementSyntax *const node)
{
    VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		symbol = Factory.Variable(node);

        MethodSymbol *const hostMethod = FindHostMethodSymbol();
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();

        symbol->Parent = OwnerSymbol();
        if (symbol->Parent != nullptr)
        {
            symbol->FullName = symbol->Parent->FullName + L"." + symbol->Name;
            symbol->Parent->OnSymbolDeclared(symbol);
        }
    }

    Declare(symbol);
    PushScope(symbol);

    if (node->RangeExpression != nullptr)
        VisitExpression(node->RangeExpression.get());

    if (node->StatementsBlock != nullptr)
        VisitStatementsBlock(node->StatementsBlock.get());

    PopScope();
}

void DeclarationCollector::ApplyMethodAttributes(MethodSymbol* symbol, const std::vector<std::unique_ptr<AttributeSyntax>>& attributes)
{
    for (const auto& attr : attributes)
    {
        if (attr->NameToken.Word == L"link")
        {
            if (attr->Arguments.size() == 1)
            {
                symbol->LinkSymbol = attr->Arguments[0].Word;
            }
            else if (attr->Arguments.size() == 2)
            {
                symbol->LinkLibrary = attr->Arguments[0].Word;
                symbol->LinkSymbol = attr->Arguments[1].Word;
            }
        }
    }
}
