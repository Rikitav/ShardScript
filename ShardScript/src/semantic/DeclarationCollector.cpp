#include <shard/semantic/DeclarationCollector.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/NamespaceTree.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/lexical/TokenType.hpp>

#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/AttributeSyntax.hpp>

#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/CompilationUnit.hpp>

#include <string>
#include <iostream>

using namespace shard;

static bool HasAccessOperator(TypeSymbol* type)
{
    if (type == nullptr)
        return false;

    for (OperatorSymbol* op : type->Operators)
    {
        if (op->OperatorToken == TokenType::Delimeter)
            return true;
    }

    return false;
}

void DeclarationCollector::VisitCompilationUnit(CompilationUnitSyntax* node)
{
    CompilationUnit* symbol = LookupSymbol<CompilationUnit>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        symbol = static_cast<CompilationUnit*>(Table->BindSymbol(node, std::make_unique<CompilationUnit>()));
    }
    
    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        NamespaceSymbol* namespaceSymbol = nullptr;
        if (node->Namespace != nullptr)
        {
            namespaceSymbol = LookupSymbol<NamespaceSymbol>(node->Namespace.get()).value_or(nullptr);
            if (namespaceSymbol == nullptr)
            {
                // Creating symbol
                namespaceSymbol = Factory.Namespace(node->Namespace.get());
                Declare(namespaceSymbol);

                if (!node->Namespace->IdentifierTokens.empty())
                {
                    NamespaceNode* nsNode = Namespaces->Root;
                    for (SyntaxToken token : node->Namespace->IdentifierTokens)
                        nsNode = nsNode->LookupOrCreate(token.Word, namespaceSymbol);

                    namespaceSymbol->Node = nsNode;
                    CurrentScope()->Namespace = nsNode;
                }
            }

            PushScope(namespaceSymbol);
        }

        // Visiting members of unit
        for (const auto& member : node->Members)
            VisitMemberDeclaration(member.get());

        if (node->Namespace != nullptr)
        {
            namespaceSymbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
            PopScope();
        }

		symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
    // Namespace declarations are now handled inline in VisitCompilationUnit
    // This method is kept for SyntaxVisitor compatibility only
}

void DeclarationCollector::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
    ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		// Creating symbol
		symbol = Factory.Class(node);
        Declare(symbol);

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Classes' owner type");
        }
        else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration && symbol->Parent->Kind != SyntaxKind::CompilationUnit)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Classes can only be declared inside Namespace or CompilationUnit");
        }
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->TypeParameters != nullptr)
        {
            if (symbol->TypeParameters.empty())
            {
                for (const auto& param : node->TypeParameters->Types)
                    Declare(Factory.TypeParameter(param.Word, symbol));
            }
            else
            {
                for (const auto& typeParam : symbol->TypeParameters)
                {
                    if (CurrentScope()->_symbols.find(typeParam->Name) == CurrentScope()->_symbols.end())
                        CurrentScope()->DeclareSymbol(typeParam);
                }
            }
        }

        for (const auto& member : node->Members)
            VisitMemberDeclaration(member.get());

        if (symbol->Linking == LINK_INSTANCE && symbol->Constructors.empty())
            Declare(Factory.Constructor(symbol, ACS_PUBLIC));

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitInterfaceDeclaration(InterfaceDeclarationSyntax* node)
{
    InterfaceSymbol* symbol = LookupSymbol<InterfaceSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        symbol = Factory.Interface(node);
        Declare(symbol);

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Interfaces' owner type");
        }
        else if(symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration && symbol->Parent->Kind != SyntaxKind::CompilationUnit)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Interfaces can only be declared inside Namespace or CompilationUnit");
        }
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->TypeParameters != nullptr)
        {
            if (symbol->TypeParameters.empty())
            {
                for (const auto& param : node->TypeParameters->Types)
                    Declare(Factory.TypeParameter(param.Word, symbol));
            }
            else
            {
                for (const auto& typeParam : symbol->TypeParameters)
                {
                    if (CurrentScope()->_symbols.find(typeParam->Name) == CurrentScope()->_symbols.end())
                        CurrentScope()->DeclareSymbol(typeParam);
                }
            }
        }

        for (const auto& member : node->Members)
            VisitMemberDeclaration(member.get());

        for (const auto& member : node->Members)
        {
            SyntaxSymbol* memberSymbol = Table->LookupSymbol(member.get()).value_or(nullptr);
            if (memberSymbol != nullptr)
            {
                memberSymbol->Accesibility = SymbolAccesibility::Public;
                if (memberSymbol->IsMethod())
                {
                    MethodSymbol* methodSymbol = static_cast<MethodSymbol*>(memberSymbol);
                    methodSymbol->IsAbstract = true;
                }
            }
        }

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitStructDeclaration(StructDeclarationSyntax* node)
{
    auto symbolOpt = LookupSymbol<StructSymbol>(node);
    StructSymbol* symbol = symbolOpt.value_or(nullptr);

    // Checking, is table had symbol for that node already
    if (!symbolOpt.has_value())
    {
		// If not, we should create it
		symbol = Factory.Struct(node);
        Declare(symbol);

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Structs' owner type");
        }
        else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration && symbol->Parent->Kind != SyntaxKind::CompilationUnit)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Structs can only be declared inside Namespace or CompilationUnit");
        }
    }

    // Pushing new semantic scope, with symbol, as owner
    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->TypeParameters != nullptr)
        {
            if (symbol->TypeParameters.empty())
            {
                for (const auto& param : node->TypeParameters->Types)
                    Declare(Factory.TypeParameter(param.Word, symbol));
            }
            else
            {
                for (const auto& typeParam : symbol->TypeParameters)
                {
                    if (CurrentScope()->_symbols.find(typeParam->Name) == CurrentScope()->_symbols.end())
                        CurrentScope()->DeclareSymbol(typeParam);
                }
            }
        }

        for (const auto& member : node->Members)
            VisitMemberDeclaration(member.get());

        if (!symbolOpt.has_value())
        {
            if (symbol->Linking == LINK_INSTANCE && symbol->Constructors.empty())
                Declare(Factory.Constructor(symbol, ACS_PUBLIC));
        }

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitDelegateDeclaration(DelegateDeclarationSyntax* node)
{
    // Creating symbol
    DelegateTypeSymbol* symbol = LookupSymbol<DelegateTypeSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		// Creating symbol
		symbol = Factory.Delegate(node);
        Declare(symbol);

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Delegates' owner type");
        }
        else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration && symbol->Parent->Kind != SyntaxKind::CompilationUnit)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Delegates can only be declared inside Namespace or CompilationUnit");
        }
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->TypeParameters != nullptr)
        {
            if (symbol->TypeParameters.empty())
            {
                for (const auto& param : node->TypeParameters->Types)
                    Declare(Factory.TypeParameter(param.Word, symbol));
            }
            else
            {
                for (const auto& typeParam : symbol->TypeParameters)
                {
                    if (CurrentScope()->_symbols.find(typeParam->Name) == CurrentScope()->_symbols.end())
                        CurrentScope()->DeclareSymbol(typeParam);
                }
            }
        }

        VisitType(node->ReturnType.get());
        VisitParametersList(node->ParametersList.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

static std::optional<std::int64_t> ReadEnumFieldValue(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return std::nullopt;

	if (expression->Kind == SyntaxKind::LiteralExpression)
	{
		const LiteralExpressionSyntax* literal = static_cast<const LiteralExpressionSyntax*>(expression);
		if (literal->LiteralToken.Type == TokenType::NumberLiteral)
		{
			try
			{
				return static_cast<std::int64_t>(std::stoll(literal->LiteralToken.Word));
			}
			catch (...)
			{
				return std::nullopt;
			}
		}
	}

	return std::nullopt;
}

void DeclarationCollector::VisitEnumDeclaration(EnumDeclarationSyntax* node)
{
	EnumSymbol* symbol = LookupSymbol<EnumSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
	{
		symbol = Factory.Enum(node, node->IsFlags);
		Declare(symbol);

		if (symbol->Parent == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Enums' owner type");
		}
		else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration && symbol->Parent->Kind != SyntaxKind::CompilationUnit)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Enums can only be declared inside Namespace or CompilationUnit");
		}
	}

	PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        std::int64_t previousValue = 0;
        for (std::size_t i = 0; i < node->Fields.size(); i++)
        {
            EnumFieldDeclarationSyntax* fieldNode = node->Fields[i].get();
            std::int64_t value = 0;

            if (fieldNode->InitializerExpression != nullptr)
            {
                auto valueOpt = ReadEnumFieldValue(fieldNode->InitializerExpression.get());
                if (!valueOpt.has_value())
                {
                    // TODO
                }

                previousValue = valueOpt.value_or(0);
            }
            else if (symbol->IsFlags)
            {
                value = 1LL << static_cast<std::int64_t>(i);
                previousValue = value;
            }
            else
            {
                if (i == 0)
                    value = 0;
                else
                    value = previousValue + 1;
                previousValue = value;
            }

            FieldSymbol* fieldSymbol = Factory.EnumField(fieldNode->IdentifierToken.Word, symbol, value);
            Declare(fieldSymbol);

            fieldNode->Symbol = fieldSymbol;
            fieldSymbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
        }

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

	PopScope();
}

void DeclarationCollector::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
    // Creating symbol
    FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        // Creating symbol
		symbol = Factory.Field(node);
        Declare(symbol);

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Fields' owner type");
        }
        else
        {
            // Checking if owner is type
            if (!symbol->Parent->IsType())
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Fields cannot be declared outside of Classes or Structures");
            }
            else if (symbol->Parent->Kind == SyntaxKind::InterfaceDeclaration)
            {
                Diagnostics.ReportError(node->IdentifierToken, L"Interfaces cannot contain fields");
            }
            else
            {
                TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);

                // Assert: static Class cannot have instance Fields
                if (symbol->Linking == LINK_INSTANCE && ownerType->Linking == LINK_STATIC)
                    Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Field in static Type");

                if (HasAccessOperator(ownerType) && symbol->Accesibility == SymbolAccesibility::Public)
                    Diagnostics.ReportError(node->IdentifierToken, L"Type declaring access operator ('.') cannot have public fields");
            }
        }
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        VisitType(node->ReturnType.get());
        VisitExpression(node->InitializerExpression.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
    // Creating symbol
    MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        // Creating symbol
        symbol = Factory.Method(node);
        Declare(symbol);
        ApplyMethodAttributes(symbol, node->Attributes);
    }

    if (node->AsyncModifierToken.Type == TokenType::AsyncKeyword)
        symbol->IsAsync = true;

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->TypeParameters != nullptr && !node->TypeParameters->Types.empty())
        {
            std::uint16_t ownerParamOffset = 0;
            if (symbol->Parent != nullptr && symbol->Parent->IsType())
                ownerParamOffset = static_cast<TypeSymbol*>(symbol->Parent)->TypeParameters.size();

            for (const auto& typeParamToken : node->TypeParameters->Types)
            {
                TypeParameterSymbol* typeParam = Factory.TypeParameter(typeParamToken.Word, symbol);
                typeParam->TypeArgumentIndex = ownerParamOffset + typeParam->TypeArgumentIndex;
                Declare(typeParam);
            }
        }

        if (node->ParametersList != nullptr)
        {
            // Creating parameters symbols
            std::uint16_t baseIndex = symbol->Linking == LINK_STATIC ? 0 : 1;
            for (const auto& parameter : node->ParametersList->Parameters)
            {
                ParameterSymbol* param = Factory.Parameter(parameter.get());
                param->SlotIndex = baseIndex++;
                Declare(param);
            }
        }

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            // Failed
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Methods' owner Type");
        }
        else
        {
            // Checking if owner is type
            if (symbol->Parent->Kind == SyntaxKind::NamespaceDeclaration || symbol->Parent->Kind == SyntaxKind::CompilationUnit)
            {
                // Assert: namespace cannot have instance Methods
                if (symbol->Linking == LINK_INSTANCE)
                {
                    symbol->Linking = LINK_STATIC;
                    //Diagnostics.ReportError(node->IdentifierToken, L"Cannot declare a non static Method in namespace");
                }
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

            // Assert: Method should have body (interfaces may declare abstract methods)
            if (node->Body == nullptr && symbol->Parent->Kind != SyntaxKind::InterfaceDeclaration)
                Diagnostics.ReportError(node->IdentifierToken, L"Method should have a Body, as it's not marked as 'abstract'");
        }

        VisitType(node->ReturnType.get());
        VisitParametersList(node->ParametersList.get());
        VisitStatementsBlock(node->Body.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}
void DeclarationCollector::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
    OperatorSymbol* symbol = LookupSymbol<OperatorSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        symbol = Factory.Operator(node);
        Declare(symbol);

        if (symbol->Accesibility != SymbolAccesibility::Public)
            Diagnostics.ReportError(node->OperatorToken, L"Operator overloads must be declared public");

        if (node->OperatorToken.Type == TokenType::Delimeter)
        {
            /*
            if (symbol->Linking == LINK_STATIC)
                Diagnostics.ReportError(node->OperatorToken, L"Access operator ('.') cannot be static; it must be an instance member");
            */
        }
        else
        {
            if (symbol->Linking != LINK_STATIC)
                Diagnostics.ReportError(node->OperatorToken, L"Operator overloads must be declared static");
        }

        ApplyMethodAttributes(symbol, node->Attributes);
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        TypeSymbol* ownerType = nullptr;
        if (symbol->Parent != nullptr && symbol->Parent->IsType())
            ownerType = static_cast<TypeSymbol*>(symbol->Parent);

        std::uint16_t baseIndex = symbol->Linking == LINK_STATIC ? 0 : 1;
        for (const auto& parameter : node->ParametersList->Parameters)
        {
            ParameterSymbol* param = Factory.Parameter(parameter.get());
            param->SlotIndex = baseIndex++;
            Declare(param);
        }

        if (symbol->Parent == nullptr)
        {
            Diagnostics.ReportError(node->OperatorToken, L"Cannot resolve Operators' owner Type");
        }
        else
        {
            if (!symbol->Parent->IsType())
            {
                Diagnostics.ReportError(node->OperatorToken, L"Operators cannot be declared outside of Classes or Structures");
            }
            else
            {
                if (ownerType->Linking == LINK_STATIC && node->OperatorToken.Type != TokenType::Delimeter)
                    Diagnostics.ReportError(node->OperatorToken, L"Cannot declare an operator overload in a static Type");
            }

            if (node->Body == nullptr)
                Diagnostics.ReportError(node->OperatorToken, L"Operator overload should have a Body");
        }

        VisitType(node->ReturnType.get());
        VisitParametersList(node->ParametersList.get());

        if (ownerType != nullptr)
        {
            if (node->OperatorToken.Type == TokenType::Delimeter)
            {
                std::size_t dotCount = 0;
                for (OperatorSymbol* op : ownerType->Operators)
                {
                    if (op->OperatorToken == TokenType::Delimeter)
                        dotCount++;
                }

                if (dotCount > 1)
                    Diagnostics.ReportError(node->OperatorToken, L"Type can only declare one access operator ('.') overload");

                if (symbol->ReturnType == SymbolTable::Primitives::Void || symbol->ReturnType == SymbolTable::Primitives::Any)
                    Diagnostics.ReportError(node->OperatorToken, L"Access operator ('.') cannot return 'void' or 'any'");

                for (FieldSymbol* field : ownerType->Fields)
                {
                    if (field->Accesibility == SymbolAccesibility::Public)
                    {
                        Diagnostics.ReportError(node->OperatorToken, L"Type declaring access operator ('.') cannot have public fields");
                        break;
                    }
                }

                if (node->ParametersList == nullptr || node->ParametersList->Parameters.size() != 1)
                    Diagnostics.ReportError(node->OperatorToken, L"Access operator ('.') must take exactly one parameter");
            }
        }

        VisitStatementsBlock(node->Body.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}
void DeclarationCollector::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
    // Creating symbol
    ConstructorSymbol* symbol = LookupSymbol<ConstructorSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        symbol = Factory.Constructor(node);
        Declare(symbol);
        ApplyMethodAttributes(symbol, node->Attributes);
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        // Creating parameters symbols
        std::uint16_t baseIndex = symbol->Linking == LINK_STATIC ? 0 : 1;
        for (const auto& parameter : node->ParametersList->Parameters)
        {
            ParameterSymbol* param = Factory.Parameter(parameter.get());
            param->SlotIndex = baseIndex++;
            Declare(param);
        }

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            // Failed
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Methods' owner Type");
        }
        else
        {
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

                // Assert: Method should have body
                if (node->Body == nullptr)
                    Diagnostics.ReportError(node->IdentifierToken, L"Constructor should have a Body, as it's not marked as 'extern' or 'abstract'");
            }
        }

        VisitParametersList(node->ParametersList.get());
        VisitStatementsBlock(node->Body.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}
void DeclarationCollector::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
    // Creating symbol
    PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        // Creating symbol
        symbol = Factory.Property(node);
        Declare(symbol);

        // Create backing field for auto-properties
        bool isInterfaceProperty = symbol->Parent != nullptr && symbol->Parent->Kind == SyntaxKind::InterfaceDeclaration;
        bool isAutoProperty = !isInterfaceProperty &&
            ((node->Getter != nullptr && node->Getter->Body == nullptr) ||
             (node->Setter != nullptr && node->Setter->Body == nullptr));

        if (isAutoProperty)
        {
            Factory.BackingField(symbol);
            Declare(symbol->BackingField);
        }

        if (symbol->Parent == nullptr)
        {
            // Failed
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Methods' owner Type");
        }
        else
        {
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

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->Getter != nullptr)
            VisitAccessorDeclaration(node->Getter.get());

        if (node->Setter != nullptr)
            VisitAccessorDeclaration(node->Setter.get());

        VisitExpression(node->InitializerExpression.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
        if (symbol->BackingField != nullptr)
            symbol->BackingField->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
    // Creating symbol
    IndexatorSymbol* symbol = LookupSymbol<IndexatorSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        symbol = Factory.Indexator(node);
        Declare(symbol);
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        // Creating parameters symbols
        std::uint16_t baseIndex = symbol->Linking == LINK_STATIC ? 1 : 0;
        for (const auto& parameter : node->ParametersList->Parameters)
        {
            ParameterSymbol* param = Factory.Parameter(parameter.get());
            param->SlotIndex = baseIndex++;
            Declare(param);
        }

        // Checking if indexator has backing field
        if (symbol->BackingField != nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Indexators' should not have backing fields.");

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            // Failed
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Indexators' owner Type");
        }
        else
        {
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

        if (node->ParametersList != nullptr)
            VisitParametersList(node->ParametersList.get());

        if (node->Getter != nullptr)
            VisitAccessorDeclaration(node->Getter.get());

        if (node->Setter != nullptr)
            VisitAccessorDeclaration(node->Setter.get());

        if (node->Getter == nullptr && node->Setter == nullptr)
            Diagnostics.ReportError(node->IdentifierToken, L"Indexator should have at least one accessor");

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
    // Expecting parent node to be property
    if (node->Parent->Kind != SyntaxKind::PropertyDeclaration && node->Parent->Kind != SyntaxKind::IndexatorDeclaration)
    {
        Diagnostics.ReportError(node->KeywordToken, L"Accessors cannot be declared outside of Properties or Indexers");
        return;
    }

    PropertySymbol* propertySymbol = nullptr;
    if (node->Parent->Kind == SyntaxKind::PropertyDeclaration)
    {
        PropertyDeclarationSyntax* propertyNode = static_cast<PropertyDeclarationSyntax*>(node->Parent);
        propertySymbol = LookupSymbol<PropertySymbol>(propertyNode).value_or(nullptr);
    }
    else if (node->Parent->Kind == SyntaxKind::IndexatorDeclaration)
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
        Declare(symbol);

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            // Failed
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Accessors' owner Type");
            return;
        }

        ApplyMethodAttributes(symbol, node->Attributes);
    }

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
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

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }
}

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax* node)
{
    VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        std::wstring varName = node->IdentifierToken.Word;
        symbol = Factory.Variable(node);
        Declare(symbol);

        MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();

        symbol->Parent = OwnerSymbol().value_or(nullptr);
        if (symbol->Parent != nullptr)
        {
            symbol->FullName = symbol->Parent->FullName + L"." + symbol->Name;
            symbol->Parent->OnSymbolDeclared(symbol);
        }
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->Expression != nullptr)
            VisitExpression(node->Expression.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitDeferStatement(DeferStatementSyntax* node)
{
    if (node->Statement != nullptr)
        VisitStatement(node->Statement.get());
}

void DeclarationCollector::VisitForEachStatement(ForEachStatementSyntax* node)
{
    VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		symbol = Factory.Variable(node);
        Declare(symbol);

        MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();

        symbol->Parent = OwnerSymbol().value_or(nullptr);
        if (symbol->Parent != nullptr)
        {
            symbol->FullName = symbol->Parent->FullName + L"." + symbol->Name;
            symbol->Parent->OnSymbolDeclared(symbol);
        }
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->RangeExpression != nullptr)
            VisitExpression(node->RangeExpression.get());

        if (node->StatementsBlock != nullptr)
            VisitStatementsBlock(node->StatementsBlock.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitForInStatement(ForInStatementSyntax* node)
{
    VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		symbol = Factory.Variable(node);
        Declare(symbol);

        MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();

        symbol->Parent = OwnerSymbol().value_or(nullptr);
        if (symbol->Parent != nullptr)
        {
            symbol->FullName = symbol->Parent->FullName + L"." + symbol->Name;
            symbol->Parent->OnSymbolDeclared(symbol);
        }
    }

    PushScope(symbol);

    if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
    {
        if (node->RangeExpression != nullptr)
            VisitExpression(node->RangeExpression.get());

        if (node->StatementsBlock != nullptr)
            VisitStatementsBlock(node->StatementsBlock.get());

        symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
    }

    PopScope();
}

void DeclarationCollector::VisitTryStatement(TryStatementSyntax* node)
{
    if (node->TryBlock != nullptr)
        VisitStatementsBlock(node->TryBlock.get());

    for (const auto& clause : node->CatchClauses)
    {
        if (clause->IdentifierToken.Type == TokenType::Unknown)
            continue;

        VariableSymbol* symbol = clause->Symbol = symbol = LookupSymbol<VariableSymbol>(clause.get()).value_or(nullptr);
        if (symbol == nullptr)
        {
            symbol = Factory.Variable(clause->IdentifierToken.Word, SymbolTable::Primitives::Any);
            Declare(symbol);

            MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
            symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();
            symbol->AdvanceAnalysisState(SymbolAnalysisState::Collected);
        }

        PushScope(symbol);

        if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
        {
            if (clause->Body != nullptr)
                VisitStatementsBlock(clause->Body.get());
        }

        PopScope();
    }
}

void DeclarationCollector::ApplyMethodAttributes(MethodSymbol* symbol, const std::vector<std::unique_ptr<AttributeSyntax>>& attributes)
{
    for (const auto& attr : attributes)
    {
        /*
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
        */
    }
}
