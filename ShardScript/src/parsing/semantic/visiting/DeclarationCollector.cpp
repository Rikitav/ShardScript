#include <shard/parsing/semantic/visiting/DeclarationCollector.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/SyntaxHelpers.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SymbolFactory.hpp>

#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>
#include <shard/syntax/nodes/AttributeSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/EnumFieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/InterfaceDeclarationSyntax.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/EnumSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/OperatorSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/InterfaceSymbol.hpp>

#include <string>

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
            Declare(symbol);

            if (!node->Namespace->IdentifierTokens.empty())
            {
                NamespaceNode* nsNode = Namespaces->Root;
                for (SyntaxToken token : node->Namespace->IdentifierTokens)
                    nsNode = nsNode->LookupOrCreate(token.Word, symbol);

                symbol->Node = nsNode;
                CurrentScope()->Namespace = nsNode;
            }
        }

        PushScope(symbol);
    }

    // Visiting members of unit
    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    if (node->Namespace != nullptr)
        PopScope();

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
        else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Classes can only be declared inside Namespace");
        }

        if (node->TypeParameters != nullptr)
        {
            for (const auto& param : node->TypeParameters->Types)
                Declare(Factory.TypeParameter(param.Word, symbol));
        }
    }

    PushScope(symbol);

    for (const auto& typeParam : symbol->TypeParameters)
        Declare(typeParam);

    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    if (symbol->Linking == LINK_INSTANCE && symbol->Constructors.empty())
        Declare(Factory.Constructor(symbol, ACS_PUBLIC));

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
        else if(symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Interfaces can only be declared inside Namespace");
        }

        if (node->TypeParameters != nullptr)
        {
            for (const auto& param : node->TypeParameters->Types)
                Declare(Factory.TypeParameter(param.Word, symbol));
        }
    }

    PushScope(symbol);

    for (const auto& typeParam : symbol->TypeParameters)
        Declare(typeParam);

    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    for (const auto& member : node->Members)
    {
        SyntaxSymbol* memberSymbol = Table->LookupSymbol(member.get()).value_or(nullptr);
        if (memberSymbol != nullptr)
            memberSymbol->Accesibility = SymbolAccesibility::Public;
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
        else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Structs can only be declared inside Namespace");
        }

        if (node->TypeParameters != nullptr)
        {
            for (const auto& param : node->TypeParameters->Types)
                Declare(Factory.TypeParameter(param.Word, symbol));
        }
    }

    // Pushing new semantic scope, with symbol, as owner
    PushScope(symbol);

    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    if (!symbolOpt.has_value())
    {
        if (symbol->Linking == LINK_INSTANCE && symbol->Constructors.empty())
            Declare(Factory.Constructor(symbol, ACS_PUBLIC));
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
        else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
        {
            Diagnostics.ReportError(node->IdentifierToken, L"Delegates can only be declared inside Namespace");
        }

        if (node->TypeParameters != nullptr)
        {
            for (const auto& param : node->TypeParameters->Types)
                Declare(Factory.TypeParameter(param.Word, symbol));
        }
    }

    PushScope(symbol);

    VisitType(node->ReturnType.get());
    VisitParametersList(node->ParametersList.get());

    PopScope();
}

static std::int64_t ReadEnumFieldValue(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return 0;

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
				return 0;
			}
		}
	}

	return 0;
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
		else if (symbol->Parent->Kind != SyntaxKind::NamespaceDeclaration)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Enums can only be declared inside Namespace");
		}
	}

	PushScope(symbol);

	std::int64_t previousValue = 0;
	for (std::size_t i = 0; i < node->Fields.size(); i++)
	{
		EnumFieldDeclarationSyntax* fieldNode = node->Fields[i].get();
		std::int64_t value = 0;

		if (fieldNode->InitializerExpression != nullptr)
		{
			value = ReadEnumFieldValue(fieldNode->InitializerExpression.get());
			previousValue = value;
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

    VisitType(node->ReturnType.get());
    VisitExpression(node->InitializerExpression.get());
    
    PopScope();
}

void DeclarationCollector::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
    // Creating symbol
    MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
    bool isNewSymbol = symbol == nullptr;
    if (isNewSymbol)
    {
        // Creating symbol
        symbol = Factory.Method(node);
        Declare(symbol);
        ApplyMethodAttributes(symbol, node->Attributes);
    }

    PushScope(symbol);

    if (isNewSymbol)
    {
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

            // Assert: Method should have body (interfaces may declare abstract methods)
            if (node->Body == nullptr && symbol->Parent->Kind != SyntaxKind::InterfaceDeclaration)
                Diagnostics.ReportError(node->IdentifierToken, L"Method should have a Body, as it's not marked as 'abstract'");
        }
    }

    VisitType(node->ReturnType.get());
    VisitParametersList(node->ParametersList.get());
    VisitStatementsBlock(node->Body.get());

    PopScope();
}
void DeclarationCollector::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
    OperatorSymbol* symbol = LookupSymbol<OperatorSymbol>(node).value_or(nullptr);
    bool isNewSymbol = symbol == nullptr;
    if (isNewSymbol)
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

    TypeSymbol* ownerType = nullptr;
    if (symbol->Parent != nullptr && symbol->Parent->IsType())
        ownerType = static_cast<TypeSymbol*>(symbol->Parent);

    if (isNewSymbol)
    {
        std::uint16_t baseIndex = 1;
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
    }

    VisitType(node->ReturnType.get());
    VisitParametersList(node->ParametersList.get());

    if (isNewSymbol && ownerType != nullptr)
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

    PopScope();
}
void DeclarationCollector::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
    // Creating symbol
    ConstructorSymbol* symbol = LookupSymbol<ConstructorSymbol>(node).value_or(nullptr);
    bool isNewSymbol = symbol == nullptr;
    if (isNewSymbol)
    {
        symbol = Factory.Constructor(node);
        Declare(symbol);
        ApplyMethodAttributes(symbol, node->Attributes);
    }

    PushScope(symbol);

    if (isNewSymbol)
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
    }

    VisitParametersList(node->ParametersList.get());
    VisitStatementsBlock(node->Body.get());

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
    if (node->Getter != nullptr)
        VisitAccessorDeclaration(node->Getter.get());

    if (node->Setter != nullptr)
        VisitAccessorDeclaration(node->Setter.get());

    VisitExpression(node->InitializerExpression.get());
    PopScope();
}

void DeclarationCollector::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
    // Creating symbol
    IndexatorSymbol* symbol = LookupSymbol<IndexatorSymbol>(node).value_or(nullptr);
    bool isNewSymbol = symbol == nullptr;
    if (isNewSymbol)
    {
        symbol = Factory.Indexator(node);
        Declare(symbol);
    }

    PushScope(symbol);

    if (isNewSymbol)
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
    }

    if (node->ParametersList != nullptr)
        VisitParametersList(node->ParametersList.get());

    if (node->Getter != nullptr)
        VisitAccessorDeclaration(node->Getter.get());

    if (node->Setter != nullptr)
        VisitAccessorDeclaration(node->Setter.get());

    if (node->Getter == nullptr && node->Setter == nullptr)
        Diagnostics.ReportError(node->IdentifierToken, L"Indexator should have at least one accessor");

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
        Declare(symbol);

        // Resolving owner symbol
        if (symbol->Parent == nullptr)
        {
            // Failed
            Diagnostics.ReportError(node->IdentifierToken, L"Cannot resolve Accessors' owner Type");
            return;
        }

        ApplyMethodAttributes(symbol, node->Attributes);

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

void DeclarationCollector::VisitVariableStatement(VariableStatementSyntax* node)
{
    VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
        std::wstring varName = node->IdentifierToken.Word;
        symbol = Factory.Variable(node);

        MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();


        symbol->Parent = OwnerSymbol().value_or(nullptr);
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

        MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();

        symbol->Parent = OwnerSymbol().value_or(nullptr);
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

void DeclarationCollector::VisitForInStatement(ForInStatementSyntax* node)
{
    VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
    {
		symbol = Factory.Variable(node);

        MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
        symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();

        symbol->Parent = OwnerSymbol().value_or(nullptr);
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

void DeclarationCollector::VisitTryStatement(TryStatementSyntax* node)
{
    if (node->TryBlock != nullptr)
        VisitStatementsBlock(node->TryBlock.get());

    for (const auto& clause : node->CatchClauses)
    {
        if (clause->IdentifierToken.Type != TokenType::Unknown)
        {
            VariableSymbol* symbol = LookupSymbol<VariableSymbol>(clause.get()).value_or(nullptr);
            if (symbol == nullptr)
            {
                symbol = Factory.Variable(clause->IdentifierToken.Word, SymbolTable::Primitives::Any);
                Declare(symbol);

                MethodSymbol* hostMethod = FindHostMethodSymbol().value_or(nullptr);
                symbol->SlotIndex = hostMethod->GetEvalStackArgumentsCount() + hostMethod->AddVariableCount();
            }

            clause->Symbol = symbol;
            Declare(symbol);
            PushScope(symbol);
        }

        if (clause->Body != nullptr)
            VisitStatementsBlock(clause->Body.get());

        if (clause->IdentifierToken.Type != TokenType::Unknown)
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
