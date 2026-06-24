#include <shard/parsing/semantic/visiting/TypeBinder.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SymbolFactory.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/VariableSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/InterfaceSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>

#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/TypeArgumentsListSyntax.hpp>

#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.hpp>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <shard/syntax/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/DeferStatementSyntax.hpp>

#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>

#include <vector>
#include <string>
#include <stdexcept>

using namespace shard;

static void BindParametersList(ParametersListSyntax* node, std::vector<ParameterSymbol*>& symbols)
{
	if (node == nullptr)
		return;

	if (node->Parameters.size() == symbols.size())
	{
		for (std::size_t i = 0; i < node->Parameters.size(); i++)
		{
			ParameterSyntax* paramSyntax = node->Parameters[i].get();
			ParameterSymbol* paramSymbol = symbols[i];

			if (paramSymbol == nullptr)
				continue;

			if (paramSyntax->Type != nullptr && paramSyntax->Type->Symbol != nullptr)
				paramSymbol->Type = paramSyntax->Type->Symbol;
		}
	}
	else
	{
		// Symbols and syntax parameters may differ when the compiler injects
		// implicit parameters (e.g. the receiver for the access operator).
		for (ParameterSymbol* paramSymbol : symbols)
		{
			if (paramSymbol == nullptr || paramSymbol->Type != nullptr)
				continue;

			for (const auto& paramSyntax : node->Parameters)
			{
				if (paramSyntax->Identifier.Word == paramSymbol->Name)
				{
					if (paramSyntax->Type != nullptr && paramSyntax->Type->Symbol != nullptr)
						paramSymbol->Type = paramSyntax->Type->Symbol;
					break;
				}
			}
		}
	}
}

void TypeBinder::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	PushScope(nullptr);
	for (const auto& directive : node->Usings)
		VisitUsingDirective(directive.get());

	if (node->Namespace != nullptr)
	{
		NamespaceSymbol* symbol = LookupSymbol<NamespaceSymbol>(node->Namespace.get()).value_or(nullptr);
		if (symbol == nullptr)
			throw std::runtime_error("symbol not found");

		Declare(symbol);
		PushScope(symbol);

		if (symbol->Node != nullptr)
			CurrentScope()->Namespace = symbol->Node;
	}

	for (const auto& member : node->Members)
	{
		SyntaxSymbol* memberSymbol = Table->LookupSymbol(member.get()).value_or(nullptr);
		if (memberSymbol != nullptr)
			Declare(memberSymbol);
	}

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());

	if (node->Namespace != nullptr)
		PopScope();

	PopScope();
}

void TypeBinder::VisitUsingDirective(UsingDirectiveSyntax* node)
{
	NamespaceNode* nsNode = Namespaces->Root;
	for (SyntaxToken token : node->TokensList)
	{
		NamespaceNode* nextNsNode = nsNode->Lookup(token.Word);
		if (nextNsNode == nullptr)
		{
			std::wstring nsName = nsNode->Owners.empty() ? L"<global>" : nsNode->Owners.at(0)->FullName;
			Diagnostics.ReportError(token, L"Identifier \'" + token.Word + L"\' doesnt exists in namespace \'" + nsName + L"\'");
			return;
		}

		nsNode = nextNsNode;
		continue;
	}

	node->Namespace = nsNode;
	SemanticScope* current = CurrentScope();
	for (const auto& symbol : nsNode->Members)
		current->DeclareSymbol(symbol);
}

static bool IsInterfaceImplementationMatching(MethodSymbol* interfaceMethod, MethodSymbol* classMethod)
{
    if (interfaceMethod->Name != classMethod->Name)
        return false;

    if (interfaceMethod->Parameters.size() != classMethod->Parameters.size())
        return false;

    if (!TypeSymbol::Equals(interfaceMethod->ReturnType, classMethod->ReturnType))
        return false;

    for (std::size_t i = 0; i < interfaceMethod->Parameters.size(); i++)
    {
        if (!TypeSymbol::Equals(interfaceMethod->Parameters[i]->Type, classMethod->Parameters[i]->Type))
            return false;
    }

    return true;
}

static bool IsInterfaceImplementationMatching(PropertySymbol* interfaceProperty, PropertySymbol* classProperty)
{
    if (interfaceProperty->Name != classProperty->Name)
        return false;

    if (!TypeSymbol::Equals(interfaceProperty->ReturnType, classProperty->ReturnType))
        return false;

    if (interfaceProperty->Getter != nullptr && classProperty->Getter == nullptr)
        return false;

    if (interfaceProperty->Setter != nullptr && classProperty->Setter == nullptr)
        return false;

    return true;
}

static void ValidateInterfaceImplementation(TypeSymbol* typeSymbol, InterfaceSymbol* interfaceSymbol, DiagnosticsContext& diagnostics, SyntaxToken errorToken)
{
    for (MethodSymbol* interfaceMethod : interfaceSymbol->Methods)
    {
        MethodSymbol* matchedMethod = nullptr;
        for (MethodSymbol* classMethod : typeSymbol->Methods)
        {
            if (IsInterfaceImplementationMatching(interfaceMethod, classMethod))
            {
                matchedMethod = classMethod;
                break;
            }
        }

        if (matchedMethod == nullptr)
        {
            diagnostics.ReportError(
                errorToken,
                L"Type '" + typeSymbol->Name + L"' does not implement interface method '" + interfaceMethod->Name + L"' from '" + interfaceSymbol->Name + L"'");
        }
        else
        {
            typeSymbol->InterfaceMethodMap[interfaceMethod] = matchedMethod;
        }
    }

    for (PropertySymbol* interfaceProperty : interfaceSymbol->Properties)
    {
        PropertySymbol* matchedProperty = nullptr;
        for (PropertySymbol* classProperty : typeSymbol->Properties)
        {
            if (IsInterfaceImplementationMatching(interfaceProperty, classProperty))
            {
                matchedProperty = classProperty;
                break;
            }
        }

        if (matchedProperty == nullptr)
        {
            diagnostics.ReportError(
                errorToken,
                L"Type '" + typeSymbol->Name + L"' does not implement interface property '" + interfaceProperty->Name + L"' from '" + interfaceSymbol->Name + L"'");
        }
        else
        {
            if (interfaceProperty->Getter != nullptr && matchedProperty->Getter != nullptr)
                typeSymbol->InterfaceMethodMap[interfaceProperty->Getter] = matchedProperty->Getter;
            
			if (interfaceProperty->Setter != nullptr && matchedProperty->Setter != nullptr)
                typeSymbol->InterfaceMethodMap[interfaceProperty->Setter] = matchedProperty->Setter;
        }
    }
}

void TypeBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	// Namespace declarations are now handled inline in VisitCompilationUnit
}

void TypeBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (CheckNameDeclared(symbol))
		Diagnostics.ReportError(node->IdentifierToken, L"Symbol with the same name is already declared in current, or including context");

	PushScope(symbol);
	for (const auto& typeParam : symbol->TypeParameters)
		Declare(typeParam);

	std::vector<InterfaceSymbol*> baseInterfaces;
	for (const auto& baseInterface : node->BaseInterfaces)
	{
		VisitType(baseInterface.get());
		TypeSymbol* baseSymbol = baseInterface->Symbol;
		if (baseSymbol == nullptr)
			continue;

		if (baseSymbol->Kind != SyntaxKind::InterfaceDeclaration)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Base type must be an interface");
			continue;
		}

		InterfaceSymbol* interfaceSymbol = static_cast<InterfaceSymbol*>(baseSymbol);
		symbol->Interfaces.push_back(interfaceSymbol);
		baseInterfaces.push_back(interfaceSymbol);
	}

	for (const auto& member : node->Members)
	{
		SyntaxSymbol* memberSymbol = Table->LookupSymbol(member.get()).value_or(nullptr);
		if (memberSymbol != nullptr)
			Declare(memberSymbol);
	}

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());

	for (InterfaceSymbol* interfaceSymbol : baseInterfaces)
		ValidateInterfaceImplementation(symbol, interfaceSymbol, Diagnostics, node->IdentifierToken);

	PopScope();
}

void TypeBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = LookupSymbol<StructSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	PushScope(symbol);
	for (const auto& typeParam : symbol->TypeParameters)
		Declare(typeParam);

	std::vector<InterfaceSymbol*> baseInterfaces;
	for (const auto& baseInterface : node->BaseInterfaces)
	{
		VisitType(baseInterface.get());
		TypeSymbol* baseSymbol = baseInterface->Symbol;
		if (baseSymbol == nullptr)
			continue;

		if (baseSymbol->Kind != SyntaxKind::InterfaceDeclaration)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Base type must be an interface");
			continue;
		}

		InterfaceSymbol* interfaceSymbol = static_cast<InterfaceSymbol*>(baseSymbol);
		symbol->Interfaces.push_back(interfaceSymbol);
		baseInterfaces.push_back(interfaceSymbol);
	}

	for (const auto& member : node->Members)
	{
		SyntaxSymbol* memberSymbol = Table->LookupSymbol(member.get()).value_or(nullptr);
		if (memberSymbol != nullptr)
			Declare(memberSymbol);
	}

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());

	for (InterfaceSymbol* interfaceSymbol : baseInterfaces)
		ValidateInterfaceImplementation(symbol, interfaceSymbol, Diagnostics, node->IdentifierToken);

	PopScope();
}

void TypeBinder::VisitInterfaceDeclaration(InterfaceDeclarationSyntax* node)
{
    InterfaceSymbol* symbol = LookupSymbol<InterfaceSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
        throw std::runtime_error("symbol not found");

    PushScope(symbol);
    for (const auto& typeParam : symbol->TypeParameters)
        Declare(typeParam);

    for (const auto& member : node->Members)
    {
        SyntaxSymbol* memberSymbol = Table->LookupSymbol(member.get()).value_or(nullptr);
        if (memberSymbol != nullptr)
            Declare(memberSymbol);
    }

    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    PopScope();
}

void TypeBinder::VisitDelegateDeclaration(DelegateDeclarationSyntax* node)
{
	DelegateTypeSymbol* symbol = LookupSymbol<DelegateTypeSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	PushScope(symbol);
	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());
		symbol->ReturnType = node->ReturnType->Symbol;
		if (symbol->AnonymousSymbol != nullptr)
			symbol->AnonymousSymbol->ReturnType = symbol->ReturnType;
	}

	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		BindParametersList(node->ParametersList.get(), symbol->Parameters);
	}

	PopScope();
}

void TypeBinder::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	PushScope(symbol);
	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		BindParametersList(node->ParametersList.get(), symbol->Parameters);
	}

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());

	PopScope();
}

void TypeBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	PushScope(symbol);
	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());
		symbol->ReturnType = node->ReturnType->Symbol;
	}

	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		BindParametersList(node->ParametersList.get(), symbol->Parameters);
	}

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());

	PopScope();
}

void TypeBinder::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("operator symbol not found");

	PushScope(symbol);
	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());
		symbol->ReturnType = node->ReturnType->Symbol;
	}

	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		BindParametersList(node->ParametersList.get(), symbol->Parameters);
	}

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());

	PopScope();
}

void TypeBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());
		symbol->ReturnType = node->ReturnType->Symbol;
	}

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression.get());
}

void TypeBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (node->ReturnType == nullptr)
		return;

	PushScope(symbol);
	VisitType(node->ReturnType.get());

	// Resolve property return type
	TypeSymbol* propertyType = node->ReturnType->Symbol;
	symbol->ReturnType = propertyType;

	// Resolve backing field type if it exists
	if (symbol->BackingField != nullptr)
		symbol->BackingField->ReturnType = propertyType;

	// Resolve getter return type
	if (symbol->Getter != nullptr)
		symbol->Getter->ReturnType = propertyType;

	// Resolve setter parameter type
	if (symbol->Setter != nullptr && !symbol->Setter->Parameters.empty())
		symbol->Setter->Parameters[0]->Type = propertyType;

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression.get());

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Setter.get());

	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	PopScope();
}

void TypeBinder::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
	IndexatorSymbol* symbol = LookupSymbol<IndexatorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	PushScope(symbol);
	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());

		// Resolve property return type
		TypeSymbol* propertyType = node->ReturnType->Symbol;
		symbol->ReturnType = propertyType;

		// Resolve getter return type
		if (symbol->Getter != nullptr && node->Getter->Body != nullptr)
			symbol->Getter->ReturnType = propertyType;

		// Resolve setter parameter type
		if (symbol->Setter != nullptr && node->Setter->Body != nullptr && !symbol->Setter->Parameters.empty())
			symbol->Setter->Parameters[0]->Type = propertyType;
	}
	
	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		BindParametersList(node->ParametersList.get(), symbol->Parameters);
	}

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Setter.get());

	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	PopScope();
}

void TypeBinder::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	AccessorSymbol* symbol = LookupSymbol<AccessorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->Parent == nullptr)
		throw std::runtime_error("accessor parent not found");

	PropertySymbol* propSymbol = static_cast<PropertySymbol*>(symbol->Parent);
	if (propSymbol->Kind == SyntaxKind::IndexatorDeclaration)
	{
		IndexatorSymbol* indexSymbol = static_cast<IndexatorSymbol*>(propSymbol);
		// Append indexer parameters to the accessor's own parameter list.
		// For a setter the implicit 'value' parameter is created by SymbolFactory and already
		// occupies the last slot; indexer arguments must precede it.
		for (ParameterSymbol* indexParam : indexSymbol->Parameters)
			symbol->Parameters.push_back(indexParam);
	}

	if (node->KeywordToken.Type == TokenType::GetKeyword)
	{
		symbol->ReturnType = propSymbol->ReturnType;
	}
}

void TypeBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (node->Type != nullptr)
	{
		VisitType(node->Type.get());
		symbol->Type = node->Type->Symbol;
	}

	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());
}

void TypeBinder::VisitDeferStatement(DeferStatementSyntax* node)
{
	if (node->Statement != nullptr)
		VisitStatement(node->Statement.get());
}

void TypeBinder::VisitTryStatement(TryStatementSyntax* node)
{
	if (node->TryBlock != nullptr)
		VisitStatementsBlock(node->TryBlock.get());

	for (const auto& clause : node->CatchClauses)
	{
		TypeSymbol* exceptionType = SymbolTable::Primitives::Any;
		if (clause->ExceptionType != nullptr)
		{
			VisitType(clause->ExceptionType.get());
			if (clause->ExceptionType->Symbol != nullptr)
				exceptionType = clause->ExceptionType->Symbol;
		}

		if (exceptionType != SymbolTable::Primitives::Any)
		{
			TypeSymbol* throwable = TRAIT_THROWABLE;
			if (throwable != nullptr && !TypeSymbol::IsAssignableFrom(throwable, exceptionType))
			{
				Diagnostics.ReportError(clause->CatchKeywordToken,
					L"Catch type must implement IThrowable");
			}
		}

		VariableSymbol* catchVariable = clause->Symbol;
		if (catchVariable != nullptr)
			catchVariable->Type = exceptionType;

		if (clause->Body != nullptr)
			VisitStatementsBlock(clause->Body.get());
	}
}

void TypeBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	if (node->Type == nullptr)
		return;

	VisitType(node->Type.get());
	VisitArgumentsList(node->ArgumentsList.get());
	node->Symbol = node->Type->Symbol;
}

void TypeBinder::VisitParameter(ParameterSyntax* node)
{
	ParameterSymbol* paramSymbol = Factory.Parameter(node);
	if (node->Type != nullptr)
	{
		TypeSyntax* type = const_cast<TypeSyntax*>(node->Type.get());
		VisitType(type);

		if (type->Symbol != nullptr)
		{
			node->Symbol = type->Symbol;
			paramSymbol->Type = type->Symbol;
		}
	}
}

void TypeBinder::VisitPredefinedType(PredefinedTypeSyntax* node)
{
	switch (node->TypeToken.Type)
	{
		case TokenType::BooleanKeyword:
		{
			if (SymbolTable::Primitives::Boolean == nullptr)
				Diagnostics.ReportError(node->TypeToken, L"Primitive 'bool' wasn't resolved");

			node->Symbol = SymbolTable::Primitives::Boolean;
			break;
		}

		case TokenType::IntegerKeyword:
		{
			if (SymbolTable::Primitives::Integer == nullptr)
				Diagnostics.ReportError(node->TypeToken, L"Primitive 'int' wasn't resolved");

			node->Symbol = SymbolTable::Primitives::Integer;
			break;
		}

		case TokenType::DoubleKeyword:
		{
			if (SymbolTable::Primitives::Double == nullptr)
				Diagnostics.ReportError(node->TypeToken, L"Primitive 'double' wasn't resolved");

			node->Symbol = SymbolTable::Primitives::Double;
			break;
		}

		case TokenType::CharKeyword:
		{
			if (SymbolTable::Primitives::Char == nullptr)
				Diagnostics.ReportError(node->TypeToken, L"Primitive 'char' wasn't resolved");

			node->Symbol = SymbolTable::Primitives::Char;
			break;
		}

		case TokenType::StringKeyword:
		{
			if (SymbolTable::Primitives::String == nullptr)
				Diagnostics.ReportError(node->TypeToken, L"Primitive 'string' wasn't resolved");

			node->Symbol = SymbolTable::Primitives::String;
			break;
		}

		case TokenType::VoidKeyword:
		{
			if (SymbolTable::Primitives::Void == nullptr)
				Diagnostics.ReportError(node->TypeToken, L"Primitive 'void' wasn't resolved");

			node->Symbol = SymbolTable::Primitives::Void;
			break;
		}

		case TokenType::VarKeyword:
		{
			if (SymbolTable::Primitives::Any == nullptr)
				Diagnostics.ReportError(node->TypeToken, L"Primitive 'any' wasn't resolved");

			node->Symbol = SymbolTable::Primitives::Any;
			break;
		}

		default:
		{
			Diagnostics.ReportError(node->TypeToken, L"Unknown primitive : " + node->TypeToken.Word);
			break;
		}
	}
}

void TypeBinder::VisitIdentifierNameType(IdentifierNameTypeSyntax* node)
{
	std::wstring name = node->Identifier.Word;
	SemanticScope* currentScope = CurrentScope();
	SyntaxSymbol* symbol = currentScope->Lookup(name).value_or(nullptr);

	if (name == L"auto")
	{
		node->Symbol = SymbolTable::Primitives::Any;
		return;
	}

	if (symbol == nullptr)
	{
		Diagnostics.ReportError(node->Identifier, L"Symbol wasnt found in current scope");
		return;
	}

	if (symbol->Kind == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* typeParamSymbol = static_cast<TypeParameterSymbol*>(symbol);
		node->Symbol = typeParamSymbol;
		
		if (!IsSymbolAccessible(typeParamSymbol, Table->LookupNode(typeParamSymbol).value_or(nullptr), node))
			Diagnostics.ReportError(node->Identifier, L"Symbol inaccessible");

		return;
	}

	if (!symbol->IsType())
	{
		Diagnostics.ReportError(node->Identifier, L"Symbol is not a type");
		return;
	}

	TypeSymbol* typeSymbol = static_cast<TypeSymbol*>(symbol);
	node->Symbol = typeSymbol;

	if (!IsSymbolAccessible(symbol, Table->LookupNode(symbol).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->Identifier, L"Symbol inaccessible");
		return;
	}
}

void TypeBinder::VisitArrayType(ArrayTypeSyntax* node)
{
	if (node->UnderlayingType == nullptr)
		return;

	VisitType(node->UnderlayingType.get());
	TypeSymbol* underlayingType = node->UnderlayingType->Symbol;

	if (underlayingType == nullptr)
		return;

	ArrayTypeSymbol* symbol = Factory.Array(node);
	symbol->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	node->Symbol = symbol;
}

void TypeBinder::VisitNullableType(NullableTypeSyntax* node)
{
	if (node->UnderlayingType == nullptr)
		return;

	VisitType(node->UnderlayingType.get());
	TypeSymbol* underlayingType = node->UnderlayingType->Symbol;
	
	if (underlayingType == nullptr)
		return;

	if (underlayingType->IsNullable)
	{
		//Diagnostics.ReportError();
	}

	//underlayingType->IsNullable = true;
	//Table->BindSymbol(node, underlayingType);
}

void TypeBinder::VisitGenericType(GenericTypeSyntax* node)
{
	if (node->UnderlayingType == nullptr)
		return;
	
	VisitType(node->UnderlayingType.get());
	TypeSymbol* underlayingType = node->UnderlayingType->Symbol;

	if (underlayingType == nullptr)
		return;

	if (node->Arguments == nullptr || node->Arguments->Types.empty())
	{
		if (node->Arguments != nullptr)
			Diagnostics.ReportError(node->Arguments->OpenToken, L"Generic type requires type arguments");

		return;
	}

	GenericTypeSymbol* symbol = Factory.GenericType(node);
	node->Symbol = symbol;

	std::size_t argsCount = node->Arguments->Types.size();
	std::size_t paramsCount = underlayingType->TypeParameters.size();

	if (argsCount != paramsCount)
	{
		Diagnostics.ReportError(node->Arguments->OpenToken, L"\'" + underlayingType->FullName + L"\' requires " + std::to_wstring(paramsCount) + L" type arguments, but got " + std::to_wstring(argsCount));
		return;
	}

	for (std::size_t i = 0; i < argsCount; i++)
	{
		TypeSyntax* typeArg = node->Arguments->Types[i].get();
		VisitType(typeArg);

		TypeSymbol* typeArgSymbol = typeArg->Symbol;
		TypeParameterSymbol* typeParam = symbol->UnderlayingType->TypeParameters.at(i);

		//symbol->TypeParameters.push_back(typeParam);
		symbol->AddTypeParameter(typeParam, typeArgSymbol);

		if (typeArgSymbol == nullptr)
		{
			Diagnostics.ReportError(node->Arguments->OpenToken, L"Type argument " + std::to_wstring(i + 1) + L" could not be resolved");
			continue;
		}
	}
}

void TypeBinder::VisitDelegateType(DelegateTypeSyntax* node)
{
	if (node->ReturnType == nullptr || node->Params == nullptr)
		return;

	VisitType(node->ReturnType.get());
	VisitParametersList(node->Params.get());

	DelegateTypeSymbol* symbol = Factory.Delegate(node);
	node->Symbol = symbol;

	for (const auto& param : node->Params->Parameters)
	{
		ParameterSymbol* delegateParamSymbol = LookupSymbol<ParameterSymbol>(param.get()).value_or(nullptr);
		if (delegateParamSymbol == nullptr)
			continue;

		if (param->Type != nullptr)
			delegateParamSymbol->Type = param->Type->Symbol;

		symbol->Parameters.push_back(delegateParamSymbol);
		symbol->AnonymousSymbol->Parameters.push_back(delegateParamSymbol);
	}
}
