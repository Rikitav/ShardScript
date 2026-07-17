#include <shard/semantic/ExpressionBinder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticScope.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/lexical/TokenType.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/semantic/symbols/LeftDenotationSymbol.hpp>
#include <shard/semantic/symbols/LiteralSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>

#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>

#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/SwitchExpressionSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>

#include <iostream>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>

#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <limits>
#include <exception>
#include <stdexcept>
#include <charconv>
#include <system_error>

using namespace shard;

static bool IsTaskType(TypeSymbol* type)
{
	return type != nullptr && type->FullName == L"async.Task";
}

static bool IsValueTaskType(TypeSymbol* type, TypeSymbol*& outValue)
{
	if (type == nullptr || type->Kind != SyntaxKind::GenericType)
		return false;

	GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(type);
	TypeSymbol* underlying = genericType->UnderlayingType;
	if (underlying == nullptr || underlying->FullName != L"async.ValueTask")
		return false;

	if (!underlying->TypeParameters.empty())
		outValue = genericType->SubstituteTypeParameters(underlying->TypeParameters[0]);

	return true;
}

static bool IsValidAsyncReturnType(TypeSymbol* type)
{
	if (type == nullptr)
		return false;

	if (IsTaskType(type))
		return true;

	TypeSymbol* valueType = nullptr;
	return IsValueTaskType(type, valueType);
}

static TypeSymbol* GetAsyncMethodElementType(MethodSymbol* method)
{
	if (method == nullptr || method->ReturnType == nullptr)
		return nullptr;

	if (IsTaskType(method->ReturnType))
		return SymbolTable::Primitives::Void;

	TypeSymbol* valueType = nullptr;
	if (IsValueTaskType(method->ReturnType, valueType))
		return valueType;

	return nullptr;
}

static GenericTypeSymbol* AsGenericInstance(TypeSymbol* type)
{
	if (type != nullptr && type->Kind == SyntaxKind::GenericType)
		return static_cast<GenericTypeSymbol*>(type);

	return nullptr;
}

static TypeSymbol* SubstituteAwaiterTypeArgument(TypeSymbol* type, GenericTypeSymbol* genericInstance)
{
	if (type == nullptr || genericInstance == nullptr)
		return type;

	if (type->Kind == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(type);
		TypeSymbol* substituted = genericInstance->SubstituteTypeParameters(typeParam);
		if (substituted != nullptr)
			return substituted;
	}

	return type;
}

static const std::vector<MethodSymbol*>& GetMethodTable(TypeSymbol* type)
{
	if (type != nullptr && type->Kind == SyntaxKind::GenericType)
		return static_cast<GenericTypeSymbol*>(type)->UnderlayingType->Methods;

	if (type != nullptr)
		return type->Methods;

	static const std::vector<MethodSymbol*> empty;
	return empty;
}

static const std::vector<TypeSymbol*>& GetInterfaceTable(TypeSymbol* type)
{
	if (type != nullptr && type->Kind == SyntaxKind::GenericType)
		return static_cast<GenericTypeSymbol*>(type)->UnderlayingType->Interfaces;

	if (type != nullptr)
		return type->Interfaces;

	static const std::vector<TypeSymbol*> empty;
	return empty;
}

static bool TypeImplementsInterface(TypeSymbol* type, InterfaceSymbol* interfaceSymbol)
{
	if (type == nullptr || interfaceSymbol == nullptr)
		return false;

	const std::vector<TypeSymbol*>& interfaces = GetInterfaceTable(type);
	for (TypeSymbol* iface : interfaces)
	{
		if (iface == interfaceSymbol)
			return true;

		if (iface->Kind == SyntaxKind::GenericType &&
			static_cast<GenericTypeSymbol*>(iface)->UnderlayingType == interfaceSymbol)
		{
			return true;
		}
	}

	return false;
}

static TypeSymbol* ResolveAwaitablePattern(AwaitExpressionSyntax* node, TypeSymbol* awaitedType, DiagnosticsContext& diagnostics)
{
	GenericTypeSymbol* awaitedGeneric = AsGenericInstance(awaitedType);

	// 1. Resolve the awaiter type. A type is awaitable if it implements IAwaitable
	// or, as a self-awaiter, directly implements IAwaiter.
	TypeSymbol* awaiterType = nullptr;
	InterfaceSymbol* iawaitable = SymbolTable::StandardTypes::IAwaitable;
	MethodSymbol* iawaitableGetAwaiter = SymbolTable::StandardTypes::IAwaitable_GetAwaiter;
	InterfaceSymbol* iawaiter = SymbolTable::StandardTypes::IAwaiter;

	if (iawaitable != nullptr && iawaitableGetAwaiter != nullptr &&
		TypeImplementsInterface(awaitedType, iawaitable))
	{
		MethodSymbol* getAwaiter = awaitedType->FindInterfaceImplementation(iawaitableGetAwaiter);
		if (getAwaiter != nullptr && getAwaiter->ReturnType != nullptr && getAwaiter->ReturnType != TYPE_VOID)
		{
			node->GetAwaiterMethod = getAwaiter;
			awaiterType = SubstituteAwaiterTypeArgument(getAwaiter->ReturnType, awaitedGeneric);
		}
	}

	if (awaiterType == nullptr && iawaiter != nullptr && TypeImplementsInterface(awaitedType, iawaiter))
	{
		awaiterType = awaitedType;
	}

	if (awaiterType == nullptr)
	{
		diagnostics.ReportError(node->AwaitKeywordToken,
			L"Type '" + (awaitedType != nullptr ? awaitedType->Name : L"?") +
			L"' is not awaitable; it must implement 'IAwaitable' or 'IAwaiter'");
		return nullptr;
	}

	// Built-ins implement IAwaitable but are their own typed awaiters; keep the concrete type for codegen.
	if (IsTaskType(awaitedType) || IsValueTaskType(awaitedType, awaiterType))
		awaiterType = awaitedType;

	node->AwaiterType = awaiterType;
	GenericTypeSymbol* awaiterGeneric = AsGenericInstance(awaiterType);

	// 2. Resolve the result type. Built-ins are special-cased; otherwise use GetResult's return type.
	TypeSymbol* resultType = nullptr;

	if (IsTaskType(awaitedType))
	{
		resultType = TYPE_VOID;
	}
	else
	{
		TypeSymbol* valueType = nullptr;
		if (IsValueTaskType(awaitedType, valueType))
			resultType = valueType;
	}

	// 3. Validate the awaiter members through the IAwaiter interface contract.
	bool hasIsCompleted = false;
	bool hasOnCompleted = false;
	bool hasGetResult = false;

	MethodSymbol* iawaiterIsCompleted = SymbolTable::StandardTypes::IAwaiter_IsCompleted;
	MethodSymbol* iawaiterOnCompleted = SymbolTable::StandardTypes::IAwaiter_OnCompleted;
	MethodSymbol* iawaiterGetResult = SymbolTable::StandardTypes::IAwaiter_GetResult;

	if (iawaiter != nullptr && TypeImplementsInterface(awaiterType, iawaiter))
	{
		if (iawaiterIsCompleted != nullptr)
		{
			MethodSymbol* impl = awaiterType->FindInterfaceImplementation(iawaiterIsCompleted);
			if (impl != nullptr)
			{
				hasIsCompleted = true;
				node->IsCompletedMethod = impl;
			}
		}

		if (iawaiterOnCompleted != nullptr)
		{
			MethodSymbol* impl = awaiterType->FindInterfaceImplementation(iawaiterOnCompleted);
			if (impl != nullptr)
			{
				hasOnCompleted = true;
				node->OnCompletedMethod = impl;
			}
		}

		if (iawaiterGetResult != nullptr)
		{
			MethodSymbol* impl = awaiterType->FindInterfaceImplementation(iawaiterGetResult);
			if (impl != nullptr)
			{
				hasGetResult = true;
				node->GetResultMethod = impl;
				if (resultType == nullptr)
					resultType = SubstituteAwaiterTypeArgument(impl->ReturnType, awaiterGeneric);
			}
		}
	}

	if (!hasIsCompleted || !hasOnCompleted || !hasGetResult)
	{
		diagnostics.ReportError(node->AwaitKeywordToken,
			L"Type '" + (awaitedType != nullptr ? awaitedType->Name : L"?") +
			L"' is not awaitable; awaiter must implement 'IAwaiter'");
		return nullptr;
	}

	return resultType;
}

static void GeneratePropertyBackingField(SymbolFactory& factory, PropertySymbol* symbol)
{
	FieldSymbol* backingField = factory.Field(L"<" + symbol->Name + L">k__BackingField", symbol->ReturnType, symbol->Linking);
	backingField->Accesibility = SymbolAccesibility::Private;
	backingField->DefaultValueExpression = symbol->DefaultValueExpression;

	symbol->BackingField = backingField;
}

static OperatorSymbol* ResolveOperatorMethod(TypeSymbol* ownerType, shard::TokenType opToken, const std::vector<TypeSymbol*>& paramTypes)
{
	if (ownerType == nullptr || !IsOverloadableOperator(opToken))
		return nullptr;

	return ownerType->FindOperator(opToken, paramTypes);
}

static bool IsAssignmentOperator(shard::TokenType type)
{
	switch (type)
	{
		case TokenType::AssignOperator:
		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
			return true;
		default:
			return false;
	}
}

static bool IsAssignmentContext(const MemberAccessExpressionSyntax* expression)
{
	if (expression->Parent == nullptr)
		return false;

	if (expression->Parent->Kind != SyntaxKind::BinaryExpression)
		return false;

	BinaryExpressionSyntax* binaryExpr = static_cast<BinaryExpressionSyntax*>(expression->Parent);
	if (binaryExpr->OperatorToken.Type != TokenType::AssignOperator)
		return false;

	return true;
}

static bool IsAssignmentContext(const IndexatorExpressionSyntax* expression)
{
	if (expression->Parent == nullptr)
		return false;

	if (expression->Parent->Kind != SyntaxKind::BinaryExpression)
		return false;

	BinaryExpressionSyntax* binaryExpr = static_cast<BinaryExpressionSyntax*>(expression->Parent);
	if (!IsAssignmentOperator(binaryExpr->OperatorToken.Type))
		return false;

	if (binaryExpr->Left.get() != expression)
		return false;

	return true;
}

bool ExpressionBinder::GetIsStaticContext(const ExpressionSyntax* expression)
{
	if (expression == nullptr)
	{
		return true;
	}

	if (expression->Kind == SyntaxKind::TypeExpression)
	{
		return true;
	}

	if (IsLinkedExpressionNode(expression->Kind))
	{
		const LinkedExpressionNode* asNode = static_cast<const LinkedExpressionNode*>(expression);
		return asNode->IsStaticContext;
	}

	return false;
}

void ExpressionBinder::SetExpressionType(ExpressionSyntax* expression, TypeSymbol* type)
{
	if (expression != nullptr)
		expressionTypes[expression] = type;
}

TypeSymbol* ExpressionBinder::GetExpressionType(ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return nullptr;
	
	auto it = expressionTypes.find(expression);
	return it != expressionTypes.end() ? it->second : nullptr;
}

void ExpressionBinder::VisitCompilationUnit(CompilationUnitSyntax* node)
{
	PushScope(nullptr);

	for (const auto& directive : node->Usings)
		VisitUsingDirective(directive.get());

	if (node->Namespace != nullptr)
	{
		NamespaceSymbol* symbol = LookupSymbol<NamespaceSymbol>(node->Namespace.get()).value_or(nullptr);
		if (symbol != nullptr)
		{
			PushScope(symbol);
			if (symbol->Node != nullptr)
				CurrentScope()->Namespace = symbol->Node;
		}
	}

	for (const auto& member : node->Members)
	{
		SyntaxSymbol* symbol = Table->LookupSymbol(member.get()).value_or(nullptr);
		if (symbol != nullptr)
			Declare(symbol);
	}

	for (const auto& member : node->Members)
		VisitMemberDeclaration(member.get());

	if (node->Namespace != nullptr)
		PopScope();

	PopScope();
}

void ExpressionBinder::VisitUsingDirective(UsingDirectiveSyntax* node)
{
	if (node->Namespace == nullptr)
		return;

	ImportNamespace(node, node->Namespace);
}

void ExpressionBinder::ImportNamespace(UsingDirectiveSyntax* node, NamespaceNode* nsNode)
{
	SemanticScope* current = CurrentScope();
	if (current->ImportedNamespaces.count(nsNode))
	{
		SyntaxToken blameToken = node->TokensList.empty() ? node->UsingKeywordToken : node->TokensList.back();
		Diagnostics.ReportError(blameToken, L"Namespace is already imported");
		return;
	}

	current->ImportedNamespaces.insert(nsNode);

	std::unordered_set<SyntaxSymbol*> nsMemberSet;
	for (SyntaxSymbol* member : nsNode->Members)
	{
		if (member != nullptr)
			nsMemberSet.insert(member);
	}

	for (SyntaxSymbol* symbol : nsMemberSet)
	{
		auto existing = current->_symbols.find(symbol->Name);
		if (existing != current->_symbols.end())
		{
			if (existing->second != symbol && !nsMemberSet.count(existing->second))
			{
				SyntaxSymbol* existingSymbol = existing->second;
				if (existingSymbol->Kind != SyntaxKind::VariableStatement && existingSymbol->Kind != SyntaxKind::Parameter)
					current->AmbiguousNames.insert(symbol->Name);
			}

			continue;
		}

		current->DeclareSymbol(symbol);
	}
}

void ExpressionBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	// Namespace declarations are now handled inline in VisitCompilationUnit
}

void ExpressionBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node).value_or(nullptr);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		for (const auto& member : node->Members)
		{
			SyntaxSymbol* symbol = Table->LookupSymbol(member.get()).value_or(nullptr);
			if (symbol != nullptr)
				Declare(symbol);
		}

		for (const auto& member : node->Members)
			VisitMemberDeclaration(member.get());

		PopScope();
	}
}

void ExpressionBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = LookupSymbol<StructSymbol>(node).value_or(nullptr);
	if (symbol != nullptr)
	{
		PushScope(symbol);

		for (const auto& member : node->Members)
		{
			SyntaxSymbol* symbol = Table->LookupSymbol(member.get()).value_or(nullptr);
			if (symbol != nullptr)
				Declare(symbol);
		}

		for (const auto& member : node->Members)
			VisitMemberDeclaration(member.get());

		PopScope();
	}
}

void ExpressionBinder::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr || symbol->Parent == nullptr || !symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol != nullptr && node->Body != nullptr)
	{
		PushScope(symbol);

		for (TypeParameterSymbol* typeParam : symbol->TypeParameters)
			CurrentScope()->DeclareSymbol(typeParam);

		auto thisVar = Factory.Parameter(L"this", ownerType);
		if (symbol->Linking == LINK_INSTANCE) // how tf not?
			CurrentScope()->DeclareSymbol(thisVar);

		int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
		for (const auto& parameter : symbol->Parameters)
		{
			parameter->SlotIndex = counter++;
			CurrentScope()->DeclareSymbol(parameter);
		}

		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body.get());
		
		PopScope();
	}

	if (symbol != nullptr)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

void ExpressionBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->Parent == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot find parent node of method.");
		return;
	}

	/*
	if (!symbol->Parent->IsType())
		return;
	*/

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	PushScope(symbol);

	for (TypeParameterSymbol* typeParam : symbol->TypeParameters)
		CurrentScope()->DeclareSymbol(typeParam);

	auto thisVar = Factory.Parameter(L"this", ownerType);
	if (symbol->Linking == LINK_INSTANCE)
		CurrentScope()->DeclareSymbol(thisVar);

	int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
	for (const auto& parameter : symbol->Parameters)
	{
		parameter->SlotIndex = counter++;
		CurrentScope()->DeclareSymbol(parameter);
	}

	if (node->Body != nullptr)
	{
		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body.get());

		if (!symbol->IsAsync && symbol->ReturnType != nullptr)
		{
			if (symbol->ReturnType->Name != L"Void")
			{
				if (!CurrentScope()->ReturnFound)
					Diagnostics.ReportError(node->IdentifierToken, L"Method must return a value of type '" + symbol->ReturnType->Name + L"'");
			}
		}
	}

	PopScope();

	symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

void ExpressionBinder::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("operator symbol not found");

	if (symbol->Parent == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Cannot find parent node of operator.");
		return;
	}

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	PushScope(symbol);

	auto thisVar = Factory.Parameter(L"this", ownerType);
	if (symbol->Linking == LINK_INSTANCE)
		CurrentScope()->DeclareSymbol(thisVar);

	int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
	for (const auto& parameter : symbol->Parameters)
	{
		parameter->SlotIndex = counter++;
		CurrentScope()->DeclareSymbol(parameter);
	}

	if (node->Body != nullptr)
	{
		CurrentScope()->ReturnFound = false;
		VisitStatementsBlock(node->Body.get());

		if (symbol->ReturnType != nullptr)
		{
			if (symbol->ReturnType->Name != L"Void")
			{
				if (!CurrentScope()->ReturnFound)
					Diagnostics.ReportError(node->OperatorToken, L"Operator must return a value of type '" + symbol->ReturnType->Name + L"'");
			}
		}
	}

	if (node->OperatorToken.Type == TokenType::Delimeter)
	{
		if (symbol->Parameters.size() != 1 || symbol->Parameters[0]->Type != SymbolTable::Primitives::String)
			Diagnostics.ReportError(node->OperatorToken, L"Access operator ('.') parameter must be of type 'string'");
	}

	PopScope();

	symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

void ExpressionBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (!symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol->ReturnType == SymbolTable::Primitives::Void)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Property '" + node->IdentifierToken.Word + L"' must have return value");
	}

	PushScope(symbol);
	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	PopScope();

	symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

void ExpressionBinder::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
	IndexatorSymbol* symbol = LookupSymbol<IndexatorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (!symbol->Parent->IsType())
		return;

	TypeSymbol* ownerType = static_cast<TypeSymbol*>(symbol->Parent);
	if (symbol->ReturnType == SymbolTable::Primitives::Void)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Indexator '" + node->IdentifierToken.Word + L"' must have return value");
	}

	PushScope(symbol);
	if (node->Getter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	if (node->Setter != nullptr)
		VisitAccessorDeclaration(node->Getter.get());

	PopScope();

	symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

void ExpressionBinder::VisitAccessorDeclaration(AccessorDeclarationSyntax* node)
{
	AccessorSymbol* symbol = LookupSymbol<AccessorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	PushScope(symbol);
	
	if (node->KeywordToken.Type == TokenType::GetKeyword)
	{
		// Getter
		if (symbol->Parent == nullptr)
		{
			PopScope();
			return;
		}

		PropertySymbol* propSymbol = static_cast<PropertySymbol*>(symbol->Parent);
		TypeSymbol* ownerType = propSymbol->Parent != nullptr ? static_cast<TypeSymbol*>(propSymbol->Parent) : nullptr;

		auto thisVar = Factory.Parameter(L"this", ownerType);
		if (symbol->Linking == LINK_INSTANCE)
			CurrentScope()->DeclareSymbol(thisVar);

		int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
		for (const auto& parameter : symbol->Parameters)
		{
			parameter->SlotIndex = counter++;
			CurrentScope()->DeclareSymbol(parameter);
		}

		if (node->Body != nullptr)
		{
			SemanticScope* scope = CurrentScope();
			scope->ReturnFound = false;

			VisitStatementsBlock(node->Body.get());

			if (!scope->ReturnFound)
			{
				if (symbol->ReturnType != nullptr)
					Diagnostics.ReportError(node->KeywordToken, L"Accessor must return a value of type '" + symbol->ReturnType->Name + L"'");
				else
					Diagnostics.ReportError(node->KeywordToken, L"Accessor must return a value");
			}
		}

		PopScope();
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
	}
	else if (node->KeywordToken.Type == TokenType::SetKeyword)
	{
		// Setter
		if (symbol->Parent == nullptr)
		{
			PopScope();
			return;
		}

		PropertySymbol* propSymbol = static_cast<PropertySymbol*>(symbol->Parent);
		TypeSymbol* ownerType = propSymbol->Parent != nullptr ? static_cast<TypeSymbol*>(propSymbol->Parent) : nullptr;

		auto thisVar = Factory.Parameter(L"this", ownerType);
		if (symbol->Linking == LINK_INSTANCE)
			CurrentScope()->DeclareSymbol(thisVar);

		int counter = symbol->Linking == LINK_STATIC ? 0 : 1;
		for (const auto& parameter : symbol->Parameters)
		{
			parameter->SlotIndex = counter++;
			CurrentScope()->DeclareSymbol(parameter);
		}

		if (node->Body != nullptr)
		{
			SemanticScope* scope = CurrentScope();
			scope->ReturnsAnything = false;
			VisitStatementsBlock(node->Body.get());

			if (scope->ReturnsAnything)
				Diagnostics.ReportError(node->KeywordToken, L"Setter method of '" + node->IdentifierToken.Word + L"' should not return any values");
		}

		PopScope();
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
	}
	else
	{
		PopScope();
		throw std::runtime_error("Unknown accessor type");
	}

	symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

void ExpressionBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	if (node->InitializerExpression != nullptr)
	{
		VisitExpression(node->InitializerExpression.get());
	}

	FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (node->InitializerExpression != nullptr)
	{
		TypeSymbol* initExprType = GetExpressionType(node->InitializerExpression.get());
		if (initExprType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer expression type could not be determined");
			return;
		}

		if (symbol->ReturnType == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field return type not resolved");
			return;
		}

		if (!SemanticModel::IsAssignableTo(symbol->ReturnType, initExprType))
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Field initializer type mismatch: expected '" + symbol->ReturnType->Name + L"' but got '" + initExprType->Name + L"'");
			return;
		}
	}

	symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

void ExpressionBinder::VisitVariableStatement(VariableStatementSyntax* node)
{
	VariableSymbol* symbol = LookupSymbol<VariableSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	Declare(symbol);
	if (symbol->Type == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Variable type not resolved");
		return;
	}

	if (node->Expression != nullptr)
	{
		PushScope(new LeftDenotationSymbol(symbol->Type));
		VisitExpression(node->Expression.get());
		PopScope();
	}

	TypeSymbol* expressionType = GetExpressionType(node->Expression.get());
	if (expressionType == nullptr)
	{
		return;
	}

	if (symbol->Type == SymbolTable::Primitives::Any)
	{
		symbol->Type = expressionType;
		return;
	}

	if (!SemanticModel::IsAssignableTo(symbol->Type, expressionType))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Type mismatch: expected '" + symbol->Type->Name + L"' but got '" + expressionType->Name + L"'");
		return;
	}

	symbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
}

TypeSymbol* ExpressionBinder::AnalyzeLiteralExpression(LiteralExpressionSyntax* node)
{
	LiteralSymbol* symbol = static_cast<LiteralSymbol*>(Table->BindSymbol(node, std::make_unique<LiteralSymbol>(node->LiteralToken.Type)));
	switch (node->LiteralToken.Type)
	{
		case TokenType::NullLiteral:
		{
			return SymbolTable::Primitives::Null;
		}

		case TokenType::BooleanLiteral:
		{
			symbol->AsBooleanValue = node->LiteralToken.Word == L"true";
			return SymbolTable::Primitives::Boolean;
		}

		case TokenType::StringLiteral:
		{
			// TODO: add interpolation
			return SymbolTable::Primitives::String;
		}

		case TokenType::DoubleLiteral:
		{
			return AnalyzeDoubleLiteral(node);
		}

		case TokenType::NumberLiteral:
		{
			return AnalyzeNumberLiteral(node);
		}

		case TokenType::CharLiteral:
		{
			// TODO: add \u support
			if (node->LiteralToken.Word.empty())
				Diagnostics.ReportError(node->LiteralToken, L"empty Char literal");
			else if (node->LiteralToken.Word.size() > 1)
				Diagnostics.ReportError(node->LiteralToken, L"invalid Char literal length");
			else
				symbol->AsCharValue = node->LiteralToken.Word[0];

			return SymbolTable::Primitives::Char;
		}

		default:
			return nullptr;
	}
}

void ExpressionBinder::VisitLiteralExpression(LiteralExpressionSyntax* node)
{
	TypeSymbol* type = AnalyzeLiteralExpression(node);
	SetExpressionType(node, type);
	
	if (type == nullptr && node->LiteralToken.Type != TokenType::NullLiteral)
	{
		std::wstring tokenType = L"unknown";
		Diagnostics.ReportError(node->LiteralToken, L"Unsupported literal type: " + tokenType);
	}
}

TypeSymbol* ExpressionBinder::AnalyzeBinaryExpression(BinaryExpressionSyntax* node)
{
	VisitExpression(node->Left.get());
	TypeSymbol* leftType = GetExpressionType(node->Left.get());
	
	if (leftType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Left operand type could not be determined");
		return nullptr;
	}

	if (node->OperatorToken.Type == TokenType::AssignOperator)
	{
		LeftDenotationSymbol leftDenotation(leftType);
		PushScope(&leftDenotation);
		VisitExpression(node->Right.get());
		PopScope();

		TypeSymbol* rightType = GetExpressionType(node->Right.get());
		if (rightType == nullptr)
		{
			Diagnostics.ReportError(node->OperatorToken, L"Right operand type could not be determined");
			return leftType;
		}

		if (leftType != SymbolTable::Primitives::Any && !SemanticModel::IsAssignableTo(leftType, rightType))
		{
			Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in assignment: expected '" + leftType->Name + L"' but got '" + rightType->Name + L"'");
			return leftType;
		}

		return leftType;
	}

	bool enumBitwiseContext =
		leftType != nullptr &&
		leftType->Kind == SyntaxKind::EnumDeclaration &&
		(node->OperatorToken.Type == TokenType::OrOperator || node->OperatorToken.Type == TokenType::AndOperator);

	if (enumBitwiseContext)
	{
		PushScope(nullptr);
		for (FieldSymbol* field : static_cast<EnumSymbol*>(leftType)->Fields)
		{
			if (field != nullptr && field->IsEnumValue)
				CurrentScope()->DeclareSymbol(field);
		}
	}

	VisitExpression(node->Right.get());

	if (enumBitwiseContext)
		PopScope();

	TypeSymbol* rightType = GetExpressionType(node->Right.get());

	if (rightType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Right operand type could not be determined");
		return leftType;
	}

	auto isNumericPrimitive = [](TypeSymbol* type)
	{
		return type == SymbolTable::Primitives::Integer
			|| type == SymbolTable::Primitives::Double
			|| type == SymbolTable::Primitives::Char
			|| type == SymbolTable::Primitives::Byte
			|| type == SymbolTable::Primitives::NativeInteger;
	};

	auto isIntegralPrimitive = [](TypeSymbol* type)
	{
		return type == SymbolTable::Primitives::Integer
			|| type == SymbolTable::Primitives::Char
			|| type == SymbolTable::Primitives::Byte
			|| type == SymbolTable::Primitives::NativeInteger;
	};

	bool bothPrimitive = SemanticModel::IsPrimitiveType(leftType) && SemanticModel::IsPrimitiveType(rightType);
	bool bothTypeParameters = leftType->Kind == SyntaxKind::TypeParameter && leftType == rightType;
	bool isPrimitiveBinaryOp = (bothPrimitive || bothTypeParameters) &&
		(IsBinaryArithmeticOperator(node->OperatorToken.Type) ||
		 IsBinaryBooleanOperator(node->OperatorToken.Type) ||
		 IsBinaryBitOperator(node->OperatorToken.Type));

	if (!isPrimitiveBinaryOp)
	{
		std::vector<TypeSymbol*> paramTypes = { leftType, rightType };
		OperatorSymbol* opMethod = ResolveOperatorMethod(leftType, node->OperatorToken.Type, paramTypes);
		if (opMethod != nullptr)
		{
			node->ToOperator = opMethod;
			return opMethod->ReturnType;
		}

		if (!SemanticModel::IsPrimitiveType(leftType) &&
			node->OperatorToken.Type != TokenType::EqualsOperator &&
			node->OperatorToken.Type != TokenType::NotEqualsOperator)
		{
			Diagnostics.ReportError(node->OperatorToken, L"Operator '" + GetOperatorMethodName(node->OperatorToken.Type) + L"' is not defined for type '" + leftType->Name + L"'");
			return nullptr;
		}
	}

	switch (node->OperatorToken.Type)
	{
		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		{
			bool nullComparison =
				(leftType == SymbolTable::Primitives::Null && rightType->Inlining == TypeInlining::ByReference) ||
				(rightType == SymbolTable::Primitives::Null && leftType->Inlining == TypeInlining::ByReference);

			bool comparable = nullComparison
				|| (leftType == SymbolTable::Primitives::String && rightType == SymbolTable::Primitives::String)
				|| (isNumericPrimitive(leftType) && isNumericPrimitive(rightType))
				|| (leftType == SymbolTable::Primitives::Boolean && rightType == SymbolTable::Primitives::Boolean)
				|| bothTypeParameters;

			if (!comparable)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in comparison: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
			}
			
			return SymbolTable::Primitives::Boolean;
		}

		case TokenType::OrOperator:
		case TokenType::AndOperator:
		{
			if (bothTypeParameters)
				return leftType;

			bool boolOp = leftType == SymbolTable::Primitives::Boolean && rightType == SymbolTable::Primitives::Boolean;
			bool bitwiseOp = isIntegralPrimitive(leftType) && isIntegralPrimitive(rightType);

			if (!boolOp && !bitwiseOp)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Operator '" + GetOperatorMethodName(node->OperatorToken.Type) + L"' is not defined for type '" + leftType->Name + L"'");
				return nullptr;
			}

			return boolOp ? SymbolTable::Primitives::Boolean : SymbolTable::Primitives::Integer;
		}

		case TokenType::LeftShiftOperator:
		case TokenType::RightShiftOperator:
		{
			if (bothTypeParameters)
				return leftType;

			if (!isIntegralPrimitive(leftType) || !isIntegralPrimitive(rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Operator '" + GetOperatorMethodName(node->OperatorToken.Type) + L"' is not defined for type '" + leftType->Name + L"'");
				return nullptr;
			}

			return SymbolTable::Primitives::Integer;
		}
			
		case TokenType::AddOperator:
		{
			if (bothTypeParameters)
				return leftType;

			if (leftType == SymbolTable::Primitives::String || rightType == SymbolTable::Primitives::String)
				return SymbolTable::Primitives::String;

			if (!isNumericPrimitive(leftType) || !isNumericPrimitive(rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in addition: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			if (leftType == SymbolTable::Primitives::Double || rightType == SymbolTable::Primitives::Double)
				return SymbolTable::Primitives::Double;

			return SymbolTable::Primitives::Integer;
		}

		case TokenType::SubOperator:
		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
		case TokenType::PowOperator:
		{
			if (bothTypeParameters)
				return leftType;

			if (!isNumericPrimitive(leftType) || !isNumericPrimitive(rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in arithmetic operation: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			if (leftType == SymbolTable::Primitives::Double || rightType == SymbolTable::Primitives::Double)
				return SymbolTable::Primitives::Double;

			return SymbolTable::Primitives::Integer;
		}
			
		case TokenType::AddAssignOperator:
		{
			if (leftType == SymbolTable::Primitives::String)
				return leftType;

			if (!isNumericPrimitive(leftType) || !isNumericPrimitive(rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in addition assignment: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			if (leftType == SymbolTable::Primitives::Double || rightType == SymbolTable::Primitives::Double)
				return SymbolTable::Primitives::Double;

			return SymbolTable::Primitives::Integer;
		}

		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
		{
			if (!isNumericPrimitive(leftType) || !isNumericPrimitive(rightType))
			{
				Diagnostics.ReportError(node->OperatorToken, L"Type mismatch in arithmetic assignment: '" + leftType->Name + L"' and '" + rightType->Name + L"'");
				return nullptr;
			}

			if (leftType == SymbolTable::Primitives::Double || rightType == SymbolTable::Primitives::Double)
				return SymbolTable::Primitives::Double;

			return SymbolTable::Primitives::Integer;
		}
			
		default:
			return leftType;
	}
}

void ExpressionBinder::VisitBinaryExpression(BinaryExpressionSyntax* node)
{
	TypeSymbol* type = AnalyzeBinaryExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeUnaryExpression(UnaryExpressionSyntax* node)
{
	TypeSymbol* exprType = GetExpressionType(node->Expression.get());
	
	if (exprType == nullptr)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Operand type could not be determined");
		return nullptr;
	}

	{
		bool tryOperatorMethod = !SemanticModel::IsPrimitiveType(exprType) &&
			(node->OperatorToken.Type != TokenType::IncrementOperator && node->OperatorToken.Type != TokenType::DecrementOperator);

		if (tryOperatorMethod)
		{
			std::vector<TypeSymbol*> paramTypes = { exprType };
			OperatorSymbol* opMethod = ResolveOperatorMethod(exprType, node->OperatorToken.Type, paramTypes);
			if (opMethod != nullptr)
			{
				node->ToOperator = opMethod;
				return opMethod->ReturnType;
			}
		}

		if (!SemanticModel::IsPrimitiveType(exprType))
		{
			Diagnostics.ReportError(node->OperatorToken, L"Operator '" + GetOperatorMethodName(node->OperatorToken.Type) + L"' is not defined for type '" + exprType->Name + L"'");
			return nullptr;
		}
	}

	switch (node->OperatorToken.Type)
	{
		case TokenType::NotOperator:
		{
			if (exprType != SymbolTable::Primitives::Boolean)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Logical NOT operator requires boolean type, got '" + exprType->Name + L"'");
				return nullptr;
			}

			return SymbolTable::Primitives::Boolean;
		}

		case TokenType::SubOperator:
		case TokenType::AddOperator:
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		{
			if (exprType != SymbolTable::Primitives::Integer && exprType != SymbolTable::Primitives::Double)
			{
				Diagnostics.ReportError(node->OperatorToken, L"Arithmetic unary operator requires numeric type, got '" + exprType->Name + L"'");
				return nullptr;
			}

			return exprType;
		}
			
		default:
		{
			Diagnostics.ReportError(node->OperatorToken, L"Unknown unary operator");
			return nullptr;
		}
	}
}

void ExpressionBinder::VisitUnaryExpression(UnaryExpressionSyntax* node)
{
	VisitExpression(node->Expression.get());

	TypeSymbol* type = AnalyzeUnaryExpression(node);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitAwaitExpression(AwaitExpressionSyntax* node)
{
	VisitExpression(node->Expression.get());

	auto hostMethod = FindHostMethodSymbol();
	if (!hostMethod.has_value() || !hostMethod.value()->IsAsync)
	{
		Diagnostics.ReportError(node->AwaitKeywordToken, L"await can only be used inside async methods");
		SetExpressionType(node, SymbolTable::Primitives::Any);
		return;
	}

	TypeSymbol* awaitedType = GetExpressionType(node->Expression.get());
	if (awaitedType == nullptr)
	{
		SetExpressionType(node, SymbolTable::Primitives::Any);
		return;
	}

	TypeSymbol* resultType = ResolveAwaitablePattern(node, awaitedType, Diagnostics);
	if (resultType == nullptr)
	{
		SetExpressionType(node, SymbolTable::Primitives::Any);
		return;
	}

	SetExpressionType(node, resultType);
}

TypeSymbol* ExpressionBinder::AnalyzeObjectExpression(ObjectExpressionSyntax* node)
{
	if (node->Symbol == nullptr)
		return nullptr;

	std::wstring methodName = node->IdentifierToken.Word;
	ConstructorSymbol* method = ResolveConstructor(node);
	if (method == nullptr)
		return node->Symbol;

	if (!IsSymbolAccessible(method, Table->LookupNode(method).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return node->Symbol;
	}

	GenericTypeSymbol* genericType = nullptr;
	if (node->Symbol->Kind == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(node->Symbol);

	if (node->ArgumentsList == nullptr)
		return node->Symbol;

	if (!MatchMethodArguments(node->IdentifierToken, method->Parameters, node->ArgumentsList->Arguments, genericType))
		return node->Symbol;

	node->CtorSymbol = method;
	return node->Symbol;
}

void ExpressionBinder::VisitObjectCreationExpression(ObjectExpressionSyntax* node)
{
	VisitType(node->Type.get());
	VisitArgumentsList(node->ArgumentsList.get());

	TypeSymbol* type = AnalyzeObjectExpression(node);
	SetExpressionType(node, type);
}

TypeSymbol* ExpressionBinder::AnalyzeCollectionExpression(CollectionExpressionSyntax* node)
{
	TypeSymbol* collectionType = nullptr;
	for (std::size_t i = 0; i < node->ValuesExpressions.size() && collectionType == nullptr; i++)
		collectionType = GetExpressionType(node->ValuesExpressions.at(i).get());

	for (const auto& expression : node->ValuesExpressions)
	{
		TypeSymbol* exprType = GetExpressionType(expression.get());
		if (exprType != collectionType)
		{
			Diagnostics.ReportError(node->OpenSquareToken, L"Element have type different from collection's target type");
		}
	}

	return collectionType;
}

void ExpressionBinder::VisitCollectionExpression(CollectionExpressionSyntax* node)
{
	for (const auto& expression : node->ValuesExpressions)
		VisitExpression(expression.get());

	TypeSymbol* type = AnalyzeCollectionExpression(node);
	auto arrayType = std::make_unique<ArrayTypeSymbol>(type);

	arrayType->Length = node->ValuesExpressions.size();
	ArrayTypeSymbol* rawArrayType = arrayType.get();
	node->Symbol = rawArrayType;

	Table->BindSymbol(node, std::move(arrayType));
	SetExpressionType(node, rawArrayType);
}

void ExpressionBinder::VisitRangeExpression(RangeExpressionSyntax* node)
{
	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());

	TypeSymbol* leftType = GetExpressionType(node->Left.get());
	TypeSymbol* rightType = GetExpressionType(node->Right.get());

	if (leftType != SymbolTable::Primitives::Integer || rightType != SymbolTable::Primitives::Integer)
	{
		Diagnostics.ReportError(node->OperatorToken, L"Range bounds must be integers");
	}

	ArrayTypeSymbol* arrayType = Factory.Array(SymbolTable::Primitives::Integer);
	SetExpressionType(node, arrayType);
}

void ExpressionBinder::VisitLambdaExpression(LambdaExpressionSyntax* node)
{
	MethodSymbol* anonymousMethod = Factory.CreateAnonymousMethod(L"Lambda", SymbolTable::Primitives::Any);

	DelegateTypeSymbol* delegate = Factory.Delegate(anonymousMethod);
	node->Symbol = delegate;

	bool isAsync = node->AsyncModifierToken.Type != TokenType::Unknown;
	if (isAsync)
		anonymousMethod->IsAsync = true;

	bool hasExplicitReturnType = false;
	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());
		if (node->ReturnType->Symbol != nullptr)
		{
			anonymousMethod->ReturnType = node->ReturnType->Symbol;
			delegate->ReturnType = node->ReturnType->Symbol;
			hasExplicitReturnType = true;

			if (isAsync && !IsValidAsyncReturnType(anonymousMethod->ReturnType))
			{
				Diagnostics.ReportError(node->AsyncModifierToken,
					L"Async lambda must return 'Task' or 'ValueTask<T>'");
			}
		}
	}
	else if (isAsync)
	{
		Diagnostics.ReportError(node->AsyncModifierToken,
			L"Async lambda must declare an explicit return type");
	}

	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		for (const auto& parameter : node->ParametersList->Parameters)
		{
			TypeSymbol* paramType = parameter->Type != nullptr ? parameter->Type->Symbol : SymbolTable::Primitives::Any;
			ParameterSymbol* paramSymbol = Factory.Parameter(parameter->Identifier.Word, paramType);
			delegate->Parameters.push_back(paramSymbol);
		}
	}

	PushScope(anonymousMethod);
	for (const auto& parameter : delegate->Parameters)
		Declare(parameter);

	CurrentScope()->ReturnFound = false;
	VisitStatementsBlock(node->Body.get());
	PopScope();

	if (!hasExplicitReturnType)
		delegate->ReturnType = anonymousMethod->ReturnType;

	SetExpressionType(node, delegate);
}

void ExpressionBinder::VisitTypeExpression(TypeExpressionSyntax* node)
{
	if (node->Type == nullptr)
	{
		SetExpressionType(node, nullptr);
		return;
	}

	TypeSymbol* type = ResolveTypeExpression(node->Type.get());
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitTernaryExpression(TernaryExpressionSyntax* node)
{
	VisitExpression(node->Condition.get());
	VisitExpression(node->Left.get());
	VisitExpression(node->Right.get());

	TypeSymbol* conditionType = GetExpressionType(node->Condition.get());
	if (conditionType == nullptr)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition return type could not be determined");
	}
	else if (conditionType != SymbolTable::Primitives::Boolean)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition expected to return 'bool', but got '" + conditionType->Name + L"'");
	}

	TypeSymbol* leftType = GetExpressionType(node->Left.get());
	TypeSymbol* rightType = GetExpressionType(node->Right.get());
	SetExpressionType(node, leftType == nullptr ? rightType : leftType);

	if (leftType == nullptr)
	{
		Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's, left expression's return type could not be determined");
	}
	else
	{
		if (rightType == nullptr)
		{
			Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's, right expression's return type could not be determined");
		}
		else if (rightType != leftType)
		{
			Diagnostics.ReportError(node->QuestionToken, L"Ternary expression's condition expected to return '" + leftType->Name + L"', but got '" + rightType->Name + L"'");
		}
	}
}

void ExpressionBinder::VisitIfExpression(IfExpressionSyntax* node)
{
	VisitExpression(node->Condition.get());
	VisitExpression(node->ThenExpression.get());
	VisitExpression(node->ElseExpression.get());

	TypeSymbol* conditionType = GetExpressionType(node->Condition.get());
	if (conditionType == nullptr)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's condition return type could not be determined");
	}
	else if (conditionType != SymbolTable::Primitives::Boolean)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's condition expected to return 'bool', but got '" + conditionType->Name + L"'");
	}

	TypeSymbol* thenType = GetExpressionType(node->ThenExpression.get());
	TypeSymbol* elseType = GetExpressionType(node->ElseExpression.get());
	SetExpressionType(node, thenType == nullptr ? elseType : thenType);

	if (thenType == nullptr)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's then-branch return type could not be determined");
	}
	else if (elseType == nullptr)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's else-branch return type could not be determined");
	}
	else if (thenType != elseType)
	{
		Diagnostics.ReportError(node->IfKeywordToken, L"If expression's branches have different types: '" + thenType->Name + L"' and '" + elseType->Name + L"'");
	}
}

void ExpressionBinder::VisitSwitchExpression(SwitchExpressionSyntax* node)
{
	VisitExpression(node->Expression.get());
	TypeSymbol* exprType = GetExpressionType(node->Expression.get());

	TypeSymbol* armType = nullptr;
	for (const auto& arm : node->Arms)
	{
		VisitExpression(arm->Pattern.get());
		VisitExpression(arm->Expression.get());

		TypeSymbol* currentArmType = GetExpressionType(arm->Expression.get());
		if (armType == nullptr)
			armType = currentArmType;
	}

	SetExpressionType(node, armType);
}

void ExpressionBinder::VisitTryStatement(TryStatementSyntax* node)
{
	if (node->TryBlock != nullptr)
		VisitStatementsBlock(node->TryBlock.get());

	for (const auto& clause : node->CatchClauses)
	{
		VariableSymbol* catchVariable = clause->Symbol;
		if (catchVariable != nullptr)
		{
			PushScope(catchVariable);
			Declare(catchVariable);
		}

		if (clause->Body != nullptr)
			VisitStatementsBlock(clause->Body.get());

		if (catchVariable != nullptr)
			PopScope();
	}
}

bool ExpressionBinder::MatchMethodArguments(SyntaxToken blameToken, std::vector<ParameterSymbol*>& parameters, std::vector<std::unique_ptr<ArgumentSyntax>>& arguments, GenericTypeSymbol* genericType, std::size_t parameterOffset)
{
	if (parameterOffset > parameters.size())
		parameterOffset = parameters.size();

	std::size_t expectedArguments = parameters.size() - parameterOffset;
	if (expectedArguments != arguments.size())
	{
		Diagnostics.ReportError(blameToken, L"Method expects " + std::to_wstring(expectedArguments) + L" arguments but got " + std::to_wstring(arguments.size()));
		return false;
	}
	
	for (std::size_t i = parameterOffset; i < parameters.size(); i++)
	{
		ParameterSymbol* param = parameters[i];
		ArgumentSyntax* arg = arguments[i - parameterOffset].get();

		if (param == nullptr || arg == nullptr || arg->Expression == nullptr)
		{
			Diagnostics.ReportError(blameToken, L"Invalid argument at position " + std::to_wstring(i - parameterOffset));
			return false;
		}
		
		/*
		PushScope(new LeftDenotationSymbol(param->Type));
		VisitExpression(expr);
		PopScope();
		*/
		
		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
		{
			//Diagnostics.ReportError(blameToken, L"Argument type could not be determined for parameter '" + param->Name + L"'");
			return false;
		}

		TypeSymbol* paramType = param->Type;
		if (paramType == nullptr)
		{
			return false;
		}

		paramType = SubstituteTypeParameters(paramType, genericType, nullptr, {});

		if (paramType == SymbolTable::Primitives::Any)
		{
			return true;
		}

		if (!SemanticModel::IsAssignableTo(paramType, argType))
		{
			Diagnostics.ReportError(blameToken, L"Argument type mismatch for parameter '" + param->Name + L"': expected '" + paramType->Name + L"' but got '" + argType->Name + L"'");
			return false;
		}
	}
	
	return true;
}

TypeSymbol* ExpressionBinder::AnalyzeMemberAccessExpression(MemberAccessExpressionSyntax* node, TypeSymbol* currentType)
{
	std::wstring memberName = node->IdentifierToken.Word;
	SyntaxSymbol* symbol = nullptr;

	if (node->PreviousExpression == nullptr)
	{
		switch (node->IdentifierToken.Type)
		{
			// Check if this is the 'field' keyword - resolve to backing field of current property
			case TokenType::FieldKeyword:
				return AnalyzeFieldKeywordExpression(node, nullptr);

			case TokenType::StringKeyword:
				return TYPE_STRING;
			
			case TokenType::CharKeyword:
				return TYPE_CHAR;
			
			case TokenType::NativeIntegerKeyword:
				return TYPE_NINT;
			
			case TokenType::IntegerKeyword:
				return TYPE_INT;
			
			case TokenType::BooleanKeyword:
				return TYPE_BOOL;
			
			case TokenType::ByteKeyword:
				return TYPE_BYTE;
			
			case TokenType::DoubleKeyword:
				return TYPE_DOUBLE;
		}

		symbol = CurrentScope()->Lookup(memberName).value_or(nullptr);
		if (symbol == nullptr)
		{
			if (IsAmbiguousName(memberName))
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Ambiguous reference: '" + memberName + L"' is defined in multiple imported namespaces");
				return nullptr;
			}

			TypeSymbol* expectedType = ResolveLeftDenotation();
			if (expectedType != nullptr && expectedType->Kind == SyntaxKind::EnumDeclaration)
			{
				EnumSymbol* enumType = static_cast<EnumSymbol*>(expectedType);
				FieldSymbol* field = enumType->FindField(memberName);

				if (field != nullptr && field->IsEnumValue)
				{
					symbol = field;
					currentType = enumType;
				}
			}

			if (symbol == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + memberName + L"' not found in current scope");
				return nullptr;
			}
		}

		if (!IsSymbolAccessible(symbol, Table->LookupNode(symbol).value_or(nullptr), node))
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + memberName + L"' is not accessible");
			return nullptr;
		}

		if (symbol->IsType())
		{
			node->IsStaticContext = true;
			return static_cast<TypeSymbol*>(symbol);
		}
	}
	else
	{
		if (currentType == nullptr)
			return nullptr;

		ExpressionSyntax* previousExpression = node->PreviousExpression.get();
		VisitExpression(previousExpression);
		currentType = GetExpressionType(previousExpression);

		if (currentType == nullptr)
			return nullptr;

		if (!currentType->IsType())
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Cannot access member on non-type '" + currentType->Name + L"'");
			return nullptr;
		}

		// First check for property (properties take precedence over fields)
		symbol = currentType->FindProperty(memberName);
		if (symbol == nullptr)
		{
			// Check for field
			symbol = currentType->FindField(memberName);
			if (symbol == nullptr)
			{
				auto methodIt = std::find_if(
					currentType->Methods.begin(), currentType->Methods.end(),
					[memberName](const MethodSymbol* method) { return method->Name == memberName; });

				if (methodIt == currentType->Methods.end())
				{
					OperatorSymbol* accessOp = currentType->FindOperator(TokenType::Delimeter, { SymbolTable::Primitives::String });
					if (accessOp != nullptr)
					{
						node->ToOperator = accessOp;
						return accessOp->ReturnType;
					}

					Diagnostics.ReportError(node->IdentifierToken, L"Member '" + memberName + L"' not found in type '" + currentType->Name + L"'");
					return nullptr;
				}

				symbol = *methodIt;
			}
		}
	}

	if (!IsSymbolAccessible(node->ToField, Table->LookupNode(node->ToField).value_or(nullptr), node))
	{
		std::wstring declName;
		Diagnostics.ReportError(node->IdentifierToken, declName + L" '" + memberName + L"' is not accessible");
		return nullptr;
	}

	switch (symbol->Kind)
	{
		case SyntaxKind::VariableStatement:
		{
			VariableSymbol* varSymbol = static_cast<VariableSymbol*>(symbol);
			node->IsStaticContext = false;
			node->ToVariable = varSymbol;
			return const_cast<TypeSymbol*>(varSymbol->Type);
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* propertySymbol = static_cast<PropertySymbol*>(symbol);
			node->IsStaticContext = false;
			node->ToProperty = propertySymbol;
			return AnalyzePropertyAccessExpression(node, propertySymbol, currentType);
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* fieldSymbol = static_cast<FieldSymbol*>(symbol);
			TypeSymbol* fieldType = fieldSymbol->ReturnType;
			
			if (fieldType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Field '" + memberName + L"' type not resolved");
				return nullptr;
			}

			if (currentType->Kind == SyntaxKind::GenericType)
			{
				GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(currentType);
				fieldType = SubstituteTypeParameters(fieldType, genericType, nullptr, {});
			}

			node->IsStaticContext = false;
			node->ToField = fieldSymbol;

			bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());
			if (isStaticContext && fieldSymbol->Linking == LINK_INSTANCE)
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance field '" + fieldSymbol->FullName + L"' from type context");

			if (!isStaticContext && fieldSymbol->Linking == LINK_STATIC)
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot access static field '" + fieldSymbol->FullName + L"' from instance reference");

			return fieldType;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodSymbol* methodSymbol = static_cast<MethodSymbol*>(symbol);
			DelegateTypeSymbol* delegate = Factory.Delegate(methodSymbol);

			node->ToDelegate = delegate;
			node->IsStaticContext = false;
			
			return delegate;
		}

		case SyntaxKind::Parameter:
		{
			ParameterSymbol* paramSymbol = static_cast<ParameterSymbol*>(symbol);
			node->ToParameter = paramSymbol;
			node->IsStaticContext = false;
			return paramSymbol->Type;
		}

		default:
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Symbol '" + memberName + L"' is not a variable, parameter or field (found " + std::to_wstring(static_cast<int>(symbol->Kind)) + L")");
			return nullptr;
		}
	}
}

TypeSymbol* ExpressionBinder::AnalyzePropertyAccessExpression(MemberAccessExpressionSyntax* node, PropertySymbol* property, TypeSymbol* currentType)
{
	if (property == nullptr)
		return nullptr;

	if (currentType == nullptr)
		return nullptr;

	std::wstring memberName = node->IdentifierToken.Word;
	if (property->ReturnType == nullptr)
	{
		if (currentType->Kind == SyntaxKind::ArrayType)
		{
			ArrayTypeSymbol* array = static_cast<ArrayTypeSymbol*>(currentType);
			if (array->UnderlayingType == nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' type not resolved");
				return nullptr;
			}
		}
		else
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' type not resolved");
			return nullptr;
		}
	}

	bool requiresSetter = IsAssignmentContext(node);
	AccessorSymbol* accessor = requiresSetter ? property->Setter : property->Getter;

	if (accessor == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Property '" + memberName + L"' does not have a " + (requiresSetter ? L"set" : L"get") + L" accessor");
		return nullptr;
	}

	if (!IsSymbolAccessible(accessor, Table->LookupNode(accessor).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->IdentifierToken, (requiresSetter ? L"Setter" : L"Getter") + (L" of property '" + memberName + L"' is not accessible"));
		return nullptr;
	}

	bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());
	if (isStaticContext && accessor->Linking == LINK_INSTANCE)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot access instance property '" + memberName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && accessor->Linking == LINK_STATIC)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot access static property '" + memberName + L"' from instance reference");
		return nullptr;
	}

	node->IsStaticContext = false;
	if (currentType->Kind == SyntaxKind::ArrayType && node->Kind == SyntaxKind::IndexatorExpression)
	{
		ArrayTypeSymbol* array = static_cast<ArrayTypeSymbol*>(currentType);
		return array->UnderlayingType;
	}

	TypeSymbol* propertyType = property->ReturnType;
	
	// Если currentType является GenericTypeSymbol, заменяем type parameters на type arguments
	if (currentType->Kind == SyntaxKind::GenericType && propertyType != nullptr)
	{
		GenericTypeSymbol* genericType = static_cast<GenericTypeSymbol*>(currentType);
		propertyType = SubstituteTypeParameters(propertyType, genericType, nullptr, {});
	}
	
	return propertyType;
}

TypeSymbol* ExpressionBinder::AnalyzeFieldKeywordExpression(MemberAccessExpressionSyntax* node, TypeSymbol* currentType)
{
	// Find PropertySymbol in current scope chain
	PropertySymbol* propertySymbol = nullptr;
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		SyntaxSymbol* symbol = const_cast<SyntaxSymbol*>(scope->Owner);
		if (symbol == nullptr)
			continue;

		if (symbol->IsType())
			break;

		if (symbol->Kind != SyntaxKind::PropertyDeclaration)
			continue;

		propertySymbol = static_cast<PropertySymbol*>(symbol);
		break;
	}

	if (propertySymbol == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Keyword 'field' can only be used in property accessors");
		return nullptr;
	}

	// Create backing field if it doesn't exist yet (for explicit property bodies using 'field')
	if (propertySymbol->BackingField == nullptr)
	{
		GeneratePropertyBackingField(Factory, propertySymbol);
		if (propertySymbol->BackingField == nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Backing field type not resolved for property '" + propertySymbol->Name + L"'");
			return nullptr;
		}
	}

	// Resolve 'field' as the backing field
	node->ToField = propertySymbol->BackingField;
	node->IsStaticContext = propertySymbol->Linking == LINK_STATIC;

	if (propertySymbol->BackingField->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Backing field type not resolved for property '" + propertySymbol->Name + L"'");
		return nullptr;
	}

	return propertySymbol->BackingField->ReturnType;
}

TypeSymbol* ExpressionBinder::AnalyzeInvokationExpression(InvokationExpressionSyntax* node, TypeSymbol* currentType)
{
	std::wstring methodName = node->IdentifierToken.Word;
	MethodSymbol* method = ResolveMethod(node, currentType);

	if (method == nullptr)
		return nullptr;

	if (!IsSymbolAccessible(method, Table->LookupNode(method).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return nullptr;
	}

	// Delegate invocations where the target is a bare variable/parameter/field have their receiver
	// rewritten by ResolveMethod. Re-evaluate the receiver type so generic delegate substitutions
	// are applied correctly.
	if (node->IsDelegateInvocation && node->PreviousExpression != nullptr)
	{
		VisitExpression(node->PreviousExpression.get());
		currentType = GetExpressionType(node->PreviousExpression.get());
		node->ReceiverType = currentType;
	}

	node->Symbol = method;
	node->IsStaticContext = GetIsStaticContext(node->PreviousExpression.get());

	GenericTypeSymbol* genericType = nullptr;
	if (node->ReceiverType != nullptr && node->ReceiverType->Kind == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(node->ReceiverType);

	if (method->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' return type not resolved");
		return nullptr;
	}

	TypeSymbol* returnType = SubstituteTypeParameters(method->ReturnType, genericType, method, node->BoundTypeArguments);
	return returnType;
}

static std::wstring BuildQualifiedName(const shard::ExpressionSyntax* qualifier, const std::wstring& memberName)
{
	std::vector<std::wstring> parts;
	const shard::ExpressionSyntax* current = qualifier;
	while (current != nullptr)
	{
		const shard::MemberAccessExpressionSyntax* member = dynamic_cast<const shard::MemberAccessExpressionSyntax*>(current);
		if (member == nullptr)
			break;

		parts.push_back(member->IdentifierToken.Word);
		current = member->PreviousExpression.get();
	}

	std::reverse(parts.begin(), parts.end());

	std::wstring result;
	for (const std::wstring& part : parts)
	{
		if (!result.empty())
			result += L".";
		result += part;
	}

	if (!memberName.empty())
	{
		if (!result.empty())
			result += L".";
		result += memberName;
	}

	return result;
}

NamespaceNode* ExpressionBinder::ResolveNamespaceQualifier(ExpressionSyntax* expression)
{
	if (expression == nullptr)
		return nullptr;

	MemberAccessExpressionSyntax* member = dynamic_cast<MemberAccessExpressionSyntax*>(expression);
	if (member == nullptr)
		return nullptr;

	if (member->PreviousExpression == nullptr)
	{
		SyntaxSymbol* symbol = CurrentScope()->Lookup(member->IdentifierToken.Word).value_or(nullptr);
		if (symbol != nullptr)
		{
			if (symbol->Kind != SyntaxKind::NamespaceDeclaration)
				return nullptr;

			NamespaceSymbol* nsSymbol = static_cast<NamespaceSymbol*>(symbol);
			return nsSymbol->Node;
		}

		// Not in scope directly: allow qualification with any namespace declared in the project.
		NamespaceNode* childNs = Namespaces->Root->Lookup(member->IdentifierToken.Word);
		if (childNs == nullptr)
			return nullptr;

		if (childNs->Owners.empty() || childNs->Owners[0]->Kind != SyntaxKind::NamespaceDeclaration)
			return nullptr;

		return childNs;
	}

	NamespaceNode* parentNs = ResolveNamespaceQualifier(member->PreviousExpression.get());
	if (parentNs == nullptr)
		return nullptr;

	NamespaceNode* childNs = parentNs->Lookup(member->IdentifierToken.Word);
	if (childNs == nullptr)
		return nullptr;

	if (childNs->Owners.empty() || childNs->Owners[0]->Kind != SyntaxKind::NamespaceDeclaration)
		return nullptr;

	return childNs;
}

bool ExpressionBinder::HasAnyMethodNamedInNamespace(const std::wstring& name, NamespaceNode* nsNode)
{
	if (nsNode == nullptr)
		return false;

	std::unordered_set<SyntaxSymbol*> memberSet;
	for (SyntaxSymbol* member : nsNode->Members)
	{
		if (member != nullptr)
			memberSet.insert(member);
	}

	for (SyntaxSymbol* symbol : memberSet)
	{
		if (symbol == nullptr || symbol->Kind != SyntaxKind::MethodDeclaration)
			continue;

		MethodSymbol* method = static_cast<MethodSymbol*>(symbol);
		if (method->Name == name)
			return true;
	}

	return false;
}

TypeSymbol* ExpressionBinder::FindTypeInNamespace(const std::wstring& name, NamespaceNode* nsNode)
{
	if (nsNode == nullptr)
		return nullptr;

	std::unordered_set<SyntaxSymbol*> memberSet;
	for (SyntaxSymbol* member : nsNode->Members)
	{
		if (member != nullptr)
			memberSet.insert(member);
	}

	for (SyntaxSymbol* symbol : memberSet)
	{
		if (symbol == nullptr || symbol->Name != name)
			continue;

		if (symbol->IsType())
			return static_cast<TypeSymbol*>(symbol);
	}

	return nullptr;
}

NamespaceNode* ExpressionBinder::FindNamespaceInNamespace(const std::wstring& name, NamespaceNode* nsNode)
{
	if (nsNode == nullptr)
		return nullptr;

	NamespaceNode* childNs = nsNode->Lookup(name);
	if (childNs == nullptr)
		return nullptr;

	if (childNs->Owners.empty() || childNs->Owners[0]->Kind != SyntaxKind::NamespaceDeclaration)
		return nullptr;

	return childNs;
}

bool ExpressionBinder::IsAmbiguousName(const std::wstring& name)
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->AmbiguousNames.count(name))
			return true;
	}

	return false;
}

NamespaceNode* ExpressionBinder::FindNamespaceByName(const std::wstring& name)
{
	SyntaxSymbol* symbol = CurrentScope()->Lookup(name).value_or(nullptr);
	if (symbol != nullptr && symbol->Kind == SyntaxKind::NamespaceDeclaration)
		return static_cast<NamespaceSymbol*>(symbol)->Node;

	NamespaceNode* childNs = Namespaces->Root->Lookup(name);
	if (childNs != nullptr && !childNs->Owners.empty() && childNs->Owners[0]->Kind == SyntaxKind::NamespaceDeclaration)
		return childNs;

	return nullptr;
}

MethodSymbol* ExpressionBinder::ResolveQualifiedMethod(
	InvokationExpressionSyntax* node,
	NamespaceNode* nsNode,
	const std::vector<TypeSymbol*>& argTypes,
	const std::vector<TypeSymbol*>& explicitTypeArgs,
	std::vector<TypeSymbol*>& outMethodTypeArgs)
{
	std::wstring methodName = node->IdentifierToken.Word;

	std::unordered_set<SyntaxSymbol*> memberSet;
	for (SyntaxSymbol* member : nsNode->Members)
	{
		if (member != nullptr)
			memberSet.insert(member);
	}

	for (SyntaxSymbol* symbol : memberSet)
	{
		if (symbol == nullptr || symbol->Kind != SyntaxKind::MethodDeclaration)
			continue;

		MethodSymbol* method = static_cast<MethodSymbol*>(symbol);
		if (method->Name != methodName)
			continue;

		if (method->Linking != LINK_STATIC)
			continue;

		if (TryMatchMethod(method, methodName, argTypes, nullptr, explicitTypeArgs, outMethodTypeArgs))
			return method;
	}

	return nullptr;
}

TypeSymbol* ExpressionBinder::AnalyzeQualifiedInvokationExpression(InvokationExpressionSyntax* node, NamespaceNode* nsNode)
{
	std::wstring methodName = node->IdentifierToken.Word;

	if (node->ArgumentsList == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' invocation has no arguments list");
		return nullptr;
	}

	std::vector<TypeSymbol*> argTypes;
	if (!CollectArgumentTypes(node, argTypes))
		return nullptr;

	std::vector<TypeSymbol*> explicitMethodTypeArgs;
	if (!CollectExplicitTypeArguments(node, explicitMethodTypeArgs))
		return nullptr;

	std::vector<TypeSymbol*> selectedMethodTypeArgs;
	MethodSymbol* method = ResolveQualifiedMethod(node, nsNode, argTypes, explicitMethodTypeArgs, selectedMethodTypeArgs);

	if (method == nullptr)
	{
		std::wstring qualifierName = BuildQualifiedName(node->PreviousExpression.get(), L"");
		std::wstring qualifiedName = qualifierName.empty() ? methodName : qualifierName + L"." + methodName;

		if (FindTypeInNamespace(methodName, nsNode) != nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Use 'new " + qualifiedName + L"()' instead of '" + qualifiedName + L"()' to construct a type");
			return nullptr;
		}

		if (FindNamespaceInNamespace(methodName, nsNode) != nullptr)
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Cannot call namespace '" + qualifiedName + L"'; use '" + qualifiedName + L".Member()' to access its members");
			return nullptr;
		}

		if (HasAnyMethodNamedInNamespace(methodName, nsNode))
			ReportNoMatchingOverload(methodName, argTypes, node->IdentifierToken);
		else
			Diagnostics.ReportError(node->IdentifierToken, L"No method named '" + methodName + L"' exists in namespace '" + qualifierName + L"'");

		return nullptr;
	}

	if (!IsSymbolAccessible(method, Table->LookupNode(method).value_or(nullptr), node))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' is not accessible");
		return nullptr;
	}

	node->Symbol = method;
	node->ReceiverType = nullptr;
	node->IsStaticContext = true;
	node->BoundTypeArguments = std::move(selectedMethodTypeArgs);

	if (method->ReturnType == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' return type not resolved");
		return nullptr;
	}

	return SubstituteTypeParameters(method->ReturnType, nullptr, method, node->BoundTypeArguments);
}

ConstructorSymbol* ExpressionBinder::ResolveConstructor(ObjectExpressionSyntax* node)
{
	TypeSymbol* symbol = node->Symbol;
	if (symbol == nullptr)
		return nullptr;

	if (symbol->Kind == SyntaxKind::GenericType)
		symbol = static_cast<GenericTypeSymbol*>(symbol)->UnderlayingType;

	std::vector<TypeSymbol*> argTypes;
	if (node->ArgumentsList == nullptr)
		return nullptr;

	for (const auto& arg : node->ArgumentsList->Arguments)
	{
		if (arg == nullptr || arg->Expression == nullptr)
			return nullptr;

		VisitExpression(arg->Expression.get());

		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
			return nullptr;

		argTypes.push_back(argType);
	}

	ConstructorSymbol* method = symbol->FindConstructor(argTypes);
	if (method == nullptr)
	{
		std::wstringstream diag;
		diag << "No constructor of \"" << symbol->FullName << "\" found that accepts ";

		switch (argTypes.size())
		{
			case 0:
			{
				diag << "no arguments";
				break;
			}

			case 1:
			{
				TypeSymbol* type = argTypes.at(0);
				std::wstring typeName = type == nullptr ? L"<error>" : type->Name;
				diag << "argument (" << typeName << ")";
				break;
			}

			default:
			{
				TypeSymbol* type = argTypes.at(0);
				diag << L"arguments (" << type->Name;

				for (int i = 1; i < argTypes.size(); i++)
				{
					type = argTypes.at(i);
					diag << ", " << type->Name;
				}

				diag << ")";
				break;
			}
		}

		Diagnostics.ReportError(node->IdentifierToken, diag.str());
		return nullptr;
	}

	return method;
}

static bool IsExtensionMethodCandidate(MethodSymbol* method, TypeSymbol* receiverType, const std::vector<TypeSymbol*>& argTypes)
{
	if (method == nullptr || method->Linking != LINK_STATIC)
		return false;

	if (method->Parameters.empty())
		return false;

	if (receiverType == nullptr)
		return false;

	if (!SemanticModel::IsAssignableTo(method->Parameters[0]->Type, receiverType))
		return false;

	if (method->Parameters.size() != argTypes.size() + 1)
		return false;

	for (std::size_t i = 1; i < method->Parameters.size(); ++i)
	{
		TypeSymbol* paramType = method->Parameters[i]->Type;
		TypeSymbol* argType = argTypes[i - 1];
		if (paramType == nullptr || argType == nullptr)
			return false;

		if (!SemanticModel::IsAssignableTo(paramType, argType))
			return false;
	}

	return true;
}

static DelegateTypeSymbol* GetDelegateType(const TypeSymbol* type)
{
	if (type == nullptr)
		return nullptr;

	if (type->Kind == SyntaxKind::DelegateType)
		return const_cast<DelegateTypeSymbol*>(static_cast<const DelegateTypeSymbol*>(type));

	if (type->Kind == SyntaxKind::GenericType)
	{
		const GenericTypeSymbol* generic = static_cast<const GenericTypeSymbol*>(type);
		TypeSymbol* underlying = generic->UnderlayingType;
		if (underlying != nullptr && underlying->Kind == SyntaxKind::DelegateType)
			return const_cast<DelegateTypeSymbol*>(static_cast<const DelegateTypeSymbol*>(underlying));
	}

	return nullptr;
}

static std::wstring FormatArgumentList(const std::vector<TypeSymbol*>& argTypes)
{
	std::wstringstream diag;
	switch (argTypes.size())
	{
		case 0:
		{
			diag << L"no arguments";
			break;
		}

		case 1:
		{
			TypeSymbol* type = argTypes.at(0);
			std::wstring typeName = type == nullptr ? L"<error>" : type->Name;
			diag << L"argument (" << typeName << L")";
			break;
		}

		default:
		{
			TypeSymbol* type = argTypes.at(0);
			diag << L"arguments (" << type->Name;

			for (std::size_t i = 1; i < argTypes.size(); i++)
			{
				type = argTypes.at(i);
				diag << L", " << type->Name;
			}

			diag << L")";
			break;
		}
	}

	return diag.str();
}

bool ExpressionBinder::CollectArgumentTypes(InvokationExpressionSyntax* node, std::vector<TypeSymbol*>& outArgTypes)
{
	if (node->ArgumentsList == nullptr)
		return false;

	for (const auto& arg : node->ArgumentsList->Arguments)
	{
		if (arg == nullptr || arg->Expression == nullptr)
			return false;

		VisitExpression(arg->Expression.get());

		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
			return false;

		outArgTypes.push_back(argType);
	}

	return true;
}

bool ExpressionBinder::CollectExplicitTypeArguments(InvokationExpressionSyntax* node, std::vector<TypeSymbol*>& outTypeArgs)
{
	if (node->TypeArguments == nullptr)
		return true;

	for (const auto& typeArg : node->TypeArguments->Types)
	{
		VisitType(typeArg.get());

		TypeSymbol* concreteType = typeArg->Symbol;
		if (concreteType == nullptr)
			return false;

		outTypeArgs.push_back(concreteType);
	}

	return true;
}

MethodSymbol* ExpressionBinder::TryResolveDelegateInvocation(InvokationExpressionSyntax* node, SyntaxSymbol* lookup)
{
	if (lookup == nullptr)
		return nullptr;

	if (lookup->Kind == SyntaxKind::VariableStatement)
	{
		VariableSymbol* delegateVar = static_cast<VariableSymbol*>(lookup);
		if (DelegateTypeSymbol* delegate = GetDelegateType(delegateVar->Type))
		{
			if (node->PreviousExpression == nullptr)
			{
				auto target = std::make_unique<MemberAccessExpressionSyntax>(node->IdentifierToken, nullptr, node);
				target->ToVariable = delegateVar;
				node->PreviousExpression = std::move(target);
			}

			node->Symbol = delegate->AnonymousSymbol;
			node->IsDelegateInvocation = true;
			return node->Symbol;
		}
	}
	else if (lookup->Kind == SyntaxKind::Parameter)
	{
		ParameterSymbol* delegateParam = static_cast<ParameterSymbol*>(lookup);
		if (DelegateTypeSymbol* delegate = GetDelegateType(delegateParam->Type))
		{
			if (node->PreviousExpression == nullptr)
			{
				auto target = std::make_unique<MemberAccessExpressionSyntax>(node->IdentifierToken, nullptr, node);
				target->ToParameter = delegateParam;
				node->PreviousExpression = std::move(target);
			}

			node->Symbol = delegate->AnonymousSymbol;
			node->IsDelegateInvocation = true;
			return node->Symbol;
		}
	}
	else if (lookup->Kind == SyntaxKind::FieldDeclaration)
	{
		FieldSymbol* delegateField = static_cast<FieldSymbol*>(lookup);
		if (DelegateTypeSymbol* delegate = GetDelegateType(delegateField->ReturnType))
		{
			if (node->PreviousExpression == nullptr && delegateField->Linking == LINK_STATIC)
			{
				auto target = std::make_unique<MemberAccessExpressionSyntax>(node->IdentifierToken, nullptr, node);
				target->ToField = delegateField;
				node->PreviousExpression = std::move(target);
			}

			node->Symbol = delegate->AnonymousSymbol;
			node->IsDelegateInvocation = true;
			return node->Symbol;
		}
	}

	return nullptr;
}

bool ExpressionBinder::HasAnyMethodNamed(const std::wstring& name, TypeSymbol* currentType)
{
	if (currentType != nullptr)
	{
		TypeSymbol* searchType = currentType;
		if (searchType->Kind == SyntaxKind::GenericType)
			searchType = static_cast<GenericTypeSymbol*>(searchType)->UnderlayingType;

		for (MethodSymbol* method : searchType->Methods)
		{
			if (method != nullptr && method->Name == name)
				return true;
		}
	}

	for (MethodSymbol* method : Table->GetMethodSymbols())
	{
		if (method != nullptr && method->Linking == LINK_STATIC && method->Name == name)
			return true;
	}

	SyntaxSymbol* lookup = CurrentScope()->Lookup(name).value_or(nullptr);
	if (lookup != nullptr && lookup->Kind == SyntaxKind::MethodDeclaration)
		return true;

	return false;
}

bool ExpressionBinder::TryMatchMethod(
	MethodSymbol* method,
	const std::wstring& expectedName,
	const std::vector<TypeSymbol*>& argTypes,
	GenericTypeSymbol* genericType,
	const std::vector<TypeSymbol*>& explicitTypeArgs,
	std::vector<TypeSymbol*>& outMethodTypeArgs)
{
	if (method == nullptr || method->Name != expectedName)
		return false;

	if (method->TypeParameters.empty())
	{
		if (argTypes.size() != method->Parameters.size())
			return false;

		for (std::size_t i = 0; i < method->Parameters.size(); ++i)
		{
			TypeSymbol* paramType = SubstituteTypeParameters(method->Parameters[i]->Type, genericType, nullptr, {});
			if (paramType == SymbolTable::Primitives::Any)
				continue;

			if (!SemanticModel::IsAssignableTo(paramType, argTypes[i]))
				return false;
		}

		outMethodTypeArgs.clear();
		return true;
	}

	std::vector<TypeSymbol*> methodTypeArgs = explicitTypeArgs;
	if (methodTypeArgs.empty())
	{
		if (!InferMethodTypeArguments(method, argTypes, genericType, methodTypeArgs))
			return false;
	}

	if (methodTypeArgs.size() != method->TypeParameters.size())
		return false;

	if (!MatchGenericMethodArguments(method, argTypes, genericType, methodTypeArgs))
		return false;

	outMethodTypeArgs = std::move(methodTypeArgs);
	return true;
}

MethodSymbol* ExpressionBinder::FindMethodOverload(
	const std::vector<MethodSymbol*>& candidates,
	const std::wstring& name,
	const std::vector<TypeSymbol*>& argTypes,
	GenericTypeSymbol* genericType,
	const std::vector<TypeSymbol*>& explicitTypeArgs,
	std::vector<TypeSymbol*>& outMethodTypeArgs)
{
	for (MethodSymbol* method : candidates)
	{
		if (TryMatchMethod(method, name, argTypes, genericType, explicitTypeArgs, outMethodTypeArgs))
			return method;
	}

	return nullptr;
}

void ExpressionBinder::ReportNoMatchingMethod(const std::wstring& methodName, SyntaxToken blameToken)
{
	Diagnostics.ReportError(blameToken, L"No method named '" + methodName + L"' exists in the current context");
}

void ExpressionBinder::ReportNoMatchingOverload(const std::wstring& methodName, const std::vector<TypeSymbol*>& argTypes, SyntaxToken blameToken)
{
	std::wstringstream diag;
	diag << L"Method '" << methodName << L"' does not have an overload that accepts the supplied arguments (";
	diag << FormatArgumentList(argTypes);
	diag << L")";
	Diagnostics.ReportError(blameToken, diag.str());
}

MethodSymbol* ExpressionBinder::ResolveMethod(InvokationExpressionSyntax* node, TypeSymbol* currentType)
{
	std::wstring methodName = node->IdentifierToken.Word;

	if (node->ArgumentsList == nullptr)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Method '" + methodName + L"' invocation has no arguments list");
		return nullptr;
	}

	std::vector<TypeSymbol*> argTypes;
	if (!CollectArgumentTypes(node, argTypes))
		return nullptr;

	std::vector<TypeSymbol*> explicitMethodTypeArgs;
	if (!CollectExplicitTypeArguments(node, explicitMethodTypeArgs))
		return nullptr;

	if (currentType == nullptr && IsAmbiguousName(methodName))
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Ambiguous reference: '" + methodName + L"' is defined in multiple imported namespaces");
		return nullptr;
	}

	GenericTypeSymbol* genericType = nullptr;
	if (currentType != nullptr && currentType->Kind == SyntaxKind::GenericType)
		genericType = static_cast<GenericTypeSymbol*>(currentType);

	bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());

	std::vector<TypeSymbol*> extensionArgs;
	if (currentType != nullptr && !isStaticContext)
		extensionArgs.push_back(currentType);

	extensionArgs.insert(extensionArgs.end(), argTypes.begin(), argTypes.end());
	std::vector<TypeSymbol*> selectedMethodTypeArgs;
	MethodSymbol* symbol = nullptr;

	// 1. Instance and static methods declared on the receiver type.
	if (currentType != nullptr)
	{
		TypeSymbol* searchType = currentType;
		if (searchType->Kind == SyntaxKind::GenericType)
		{
			genericType = static_cast<GenericTypeSymbol*>(searchType);
			searchType = genericType->UnderlayingType;
		}

		if (searchType != nullptr && !searchType->TypeParameters.empty() && genericType == nullptr)
		{
			for (MethodSymbol* method : searchType->Methods)
			{
				std::vector<TypeSymbol*> classTypeArgs;
				bool inferred = InferClassTypeArguments(searchType, method, argTypes, classTypeArgs);

				std::vector<TypeSymbol*> methodTypeArgs;
				if (inferred)
				{
					std::unordered_map<std::wstring, TypeSymbol*> typeArgMap;
					for (std::size_t i = 0; i < searchType->TypeParameters.size(); ++i)
						typeArgMap[searchType->TypeParameters[i]->Name] = classTypeArgs[i];

					GenericTypeSymbol* constructed = Factory.GenericType(searchType, typeArgMap);
					if (TryMatchMethod(method, methodName, argTypes, constructed, explicitMethodTypeArgs, methodTypeArgs))
					{
						symbol = method;
						genericType = constructed;
						selectedMethodTypeArgs = std::move(methodTypeArgs);
						break;
					}
				}
				else if (method->Linking == LINK_STATIC)
				{
					// Static methods on a generic class that do not use the class's
					// type parameters (e.g. ValueTask.Wait) can be invoked from the
					// open generic type name without explicit type arguments.
					if (TryMatchMethod(method, methodName, argTypes, nullptr, explicitMethodTypeArgs, methodTypeArgs))
					{
						symbol = method;
						genericType = nullptr;
						selectedMethodTypeArgs = std::move(methodTypeArgs);
						break;
					}
				}
			}
		}
		else
		{
			symbol = FindMethodOverload(searchType->Methods, methodName, argTypes, genericType, explicitMethodTypeArgs, selectedMethodTypeArgs);
		}
	}

	// 2. Top-level static methods and extension methods.
	if (symbol == nullptr)
	{
		const std::vector<TypeSymbol*>& candidateArgs = isStaticContext ? argTypes : extensionArgs;
		bool matchingAsExtension = !isStaticContext;

		for (MethodSymbol* method : Table->GetMethodSymbols())
		{
			if (method->Linking != LINK_STATIC)
				continue;

			if (TryMatchMethod(method, methodName, candidateArgs, genericType, explicitMethodTypeArgs, selectedMethodTypeArgs))
			{
				symbol = method;
				if (matchingAsExtension)
					node->IsExtensionMethodInvocation = true;
				break;
			}
		}
	}

	// 3. Local methods in scope, or delegate variables/fields/parameters.
	if (symbol == nullptr)
	{
		SyntaxSymbol* lookupMethod = CurrentScope()->Lookup(methodName).value_or(nullptr);
		if (lookupMethod != nullptr)
		{
			if (lookupMethod->Kind == SyntaxKind::MethodDeclaration)
			{
				const std::vector<TypeSymbol*>& candidateArgs = isStaticContext ? argTypes : extensionArgs;
				bool matchingAsExtension = !isStaticContext;

				MethodSymbol* localMethod = static_cast<MethodSymbol*>(lookupMethod);
				if (TryMatchMethod(localMethod, methodName, candidateArgs, genericType, explicitMethodTypeArgs, selectedMethodTypeArgs))
				{
					symbol = localMethod;
					if (matchingAsExtension)
						node->IsExtensionMethodInvocation = true;
				}
			}
			else if (explicitMethodTypeArgs.empty())
			{
				MethodSymbol* delegateMethod = TryResolveDelegateInvocation(node, lookupMethod);
				if (delegateMethod != nullptr)
					return delegateMethod;
			}
		}
	}

	if (symbol == nullptr)
	{
		if (currentType == nullptr)
		{
			SyntaxSymbol* lookup = CurrentScope()->Lookup(methodName).value_or(nullptr);
			if (lookup != nullptr && lookup->IsType())
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Use 'new " + methodName + L"()' instead of '" + methodName + L"()' to construct a type");
				return nullptr;
			}

			NamespaceNode* nsNode = FindNamespaceByName(methodName);
			if (nsNode != nullptr)
			{
				Diagnostics.ReportError(node->IdentifierToken, L"Cannot call namespace '" + methodName + L"'; use '" + methodName + L".Member()' to access its members");
				return nullptr;
			}
		}

		if (HasAnyMethodNamed(methodName, currentType))
			ReportNoMatchingOverload(methodName, argTypes, node->IdentifierToken);
		else
			ReportNoMatchingMethod(methodName, node->IdentifierToken);
		
		return nullptr;
	}

	node->BoundTypeArguments = std::move(selectedMethodTypeArgs);
	node->ReceiverType = genericType != nullptr ? genericType : currentType;

	if (isStaticContext && symbol->Linking == LINK_INSTANCE)
	{
		Diagnostics.ReportError(node->IdentifierToken, L"Cannot call instance method '" + methodName + L"' from type context");
		return nullptr;
	}

	if (!node->IsExtensionMethodInvocation && !isStaticContext && symbol->Linking == LINK_STATIC)
	{
		if (IsExtensionMethodCandidate(symbol, currentType, argTypes))
		{
			node->IsExtensionMethodInvocation = true;
		}
		else
		{
			Diagnostics.ReportError(node->IdentifierToken, L"Cannot call static method '" + methodName + L"' on instance reference");
			return nullptr;
		}
	}

	return symbol;
}

TypeSymbol* ExpressionBinder::AnalyzeIndexatorExpression(IndexatorExpressionSyntax* node, TypeSymbol* currentType)
{
	if (currentType == nullptr)
		return nullptr;

	if (node->PreviousExpression == nullptr)
		return nullptr;

	VisitExpression(node->PreviousExpression.get());
	currentType = GetExpressionType(node->PreviousExpression.get());

	IndexatorSymbol* indexator = ResolveIndexator(node, currentType);
	TypeSymbol* resultType = AnalyzePropertyAccessExpression(node, indexator, currentType);

	if (resultType == nullptr)
		return nullptr;

	if (indexator == nullptr)
		return nullptr;

	bool requiresSetter = IsAssignmentContext(node);
	AccessorSymbol* accessor = requiresSetter ? indexator->Setter : indexator->Getter;

	if (accessor != nullptr)
	{
		GenericTypeSymbol* genericType = nullptr;
		if (currentType->Kind == SyntaxKind::GenericType)
			genericType = static_cast<GenericTypeSymbol*>(currentType);

		if (!MatchMethodArguments(node->IdentifierToken, indexator->Parameters, node->IndexatorList->Arguments, genericType))
		{
			//Diagnostics.ReportError(node->MemberAccess->IdentifierToken, L"Indexator arguments types do not match");
			return nullptr;
		}
	}

	node->ToProperty = indexator;
	node->IsStaticContext = node->IsStaticContext;
	return resultType;
}

TypeSymbol* ExpressionBinder::SubstituteTypeParameters(TypeSymbol* type, GenericTypeSymbol* genericType)
{
	if (type == nullptr || genericType == nullptr || type->Kind != SyntaxKind::TypeParameter)
		return type;

	TypeSymbol* substituted = genericType->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(type));
	return substituted != nullptr ? substituted : type;
}

TypeSymbol* ExpressionBinder::SubstituteTypeParameters(TypeSymbol* type, GenericTypeSymbol* genericType, MethodSymbol* method, const std::vector<TypeSymbol*>& methodTypeArgs)
{
	if (type == nullptr)
		return nullptr;

	if (type->Kind == SyntaxKind::TypeParameter)
	{
		TypeSymbol* substituted = SubstituteTypeParameters(type, genericType);
		if (substituted != nullptr && substituted->Kind == SyntaxKind::TypeParameter)
			substituted = SubstituteMethodTypeParameter(substituted, method, methodTypeArgs);

		return substituted != nullptr ? substituted : type;
	}

	if (type->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
		TypeSymbol* underlying = generic->UnderlayingType;

		bool changed = false;
		std::unordered_map<std::wstring, TypeSymbol*> newArgs;
		for (TypeParameterSymbol* param : underlying->TypeParameters)
		{
			TypeSymbol* arg = generic->SubstituteTypeParameters(param);
			TypeSymbol* newArg = SubstituteTypeParameters(arg, genericType, method, methodTypeArgs);
			if (newArg != arg)
				changed = true;

			newArgs[param->Name] = newArg != nullptr ? newArg : arg;
		}

		if (!changed)
			return type;

		return Factory.GenericType(underlying, newArgs);
	}

	if (type->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* array = static_cast<ArrayTypeSymbol*>(type);
		TypeSymbol* elementType = SubstituteTypeParameters(array->UnderlayingType, genericType, method, methodTypeArgs);
		if (elementType == array->UnderlayingType)
			return type;

		return Factory.Array(elementType);
	}

	return type;
}

TypeSymbol* ExpressionBinder::ResolveTypeExpression(TypeSyntax* type)
{
	if (type == nullptr)
		return nullptr;

	if (type->Symbol != nullptr)
		return type->Symbol;

	switch (type->Kind)
	{
		case SyntaxKind::PredefinedType:
		{
			PredefinedTypeSyntax* pre = static_cast<PredefinedTypeSyntax*>(type);
			switch (pre->TypeToken.Type)
			{
				case TokenType::BooleanKeyword: type->Symbol = SymbolTable::Primitives::Boolean; break;
				case TokenType::IntegerKeyword: type->Symbol = SymbolTable::Primitives::Integer; break;
				case TokenType::DoubleKeyword: type->Symbol = SymbolTable::Primitives::Double; break;
				case TokenType::CharKeyword: type->Symbol = SymbolTable::Primitives::Char; break;
				case TokenType::ByteKeyword: type->Symbol = SymbolTable::Primitives::Byte; break;
				case TokenType::NativeIntegerKeyword: type->Symbol = SymbolTable::Primitives::NativeInteger; break;
				case TokenType::StringKeyword: type->Symbol = SymbolTable::Primitives::String; break;
				case TokenType::VoidKeyword: type->Symbol = SymbolTable::Primitives::Void; break;
				case TokenType::VarKeyword: type->Symbol = SymbolTable::Primitives::Any; break;
				default: break;
			}

			return type->Symbol;
		}

		case SyntaxKind::IdentifierNameType:
		{
			IdentifierNameTypeSyntax* id = static_cast<IdentifierNameTypeSyntax*>(type);
			SyntaxSymbol* symbol = CurrentScope()->Lookup(id->Identifier.Word).value_or(nullptr);
			if (symbol == nullptr)
			{
				Diagnostics.ReportError(id->Identifier, L"Symbol wasnt found in current scope");
				return nullptr;
			}

			if (symbol->Kind == SyntaxKind::TypeParameter)
			{
				type->Symbol = static_cast<TypeParameterSymbol*>(symbol);
				return type->Symbol;
			}

			if (!symbol->IsType())
			{
				Diagnostics.ReportError(id->Identifier, L"Symbol is not a type");
				return nullptr;
			}

			type->Symbol = static_cast<TypeSymbol*>(symbol);
			return type->Symbol;
		}

		case SyntaxKind::GenericType:
		{
			GenericTypeSyntax* generic = static_cast<GenericTypeSyntax*>(type);
			TypeSymbol* underlying = ResolveTypeExpression(generic->UnderlayingType.get());
			if (underlying == nullptr)
				return nullptr;

			std::size_t argsCount = generic->Arguments != nullptr ? generic->Arguments->Types.size() : 0;
			std::size_t paramsCount = underlying->TypeParameters.size();
			if (argsCount != paramsCount)
			{
				Diagnostics.ReportError(generic->Arguments->OpenToken,
					L"'" + underlying->FullName + L"' requires " + std::to_wstring(paramsCount) +
					L" type arguments, but got " + std::to_wstring(argsCount));
				return nullptr;
			}

			std::unordered_map<std::wstring, TypeSymbol*> typeArgs;
			for (std::size_t i = 0; i < argsCount; i++)
			{
				TypeSymbol* arg = ResolveTypeExpression(generic->Arguments->Types[i].get());
				if (arg == nullptr)
					return nullptr;

				typeArgs[underlying->TypeParameters[i]->Name] = arg;
			}

			GenericTypeSymbol* symbol = Factory.GenericType(underlying, typeArgs);
			type->Symbol = symbol;
			return symbol;
		}

		default:
			break;
	}

	return nullptr;
}

TypeSymbol* ExpressionBinder::SubstituteMethodTypeParameter(TypeSymbol* type, MethodSymbol* method, const std::vector<TypeSymbol*>& methodTypeArgs)
{
	if (type == nullptr || type->Kind != SyntaxKind::TypeParameter)
		return type;

	TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(type);
	if (typeParam->Parent != method)
		return type;

	for (std::size_t i = 0; i < method->TypeParameters.size(); ++i)
	{
		if (method->TypeParameters[i] == typeParam)
		{
			if (i < methodTypeArgs.size())
				return methodTypeArgs[i];
			break;
		}
	}

	return type;
}

bool ExpressionBinder::TryInferTypeArgument(TypeSymbol* pattern, TypeSymbol* concrete, MethodSymbol* method, GenericTypeSymbol* genericType, std::vector<TypeSymbol*>& outMethodTypeArgs)
{
	if (pattern == nullptr || concrete == nullptr)
		return true;

	if (pattern->Kind == SyntaxKind::TypeParameter && genericType != nullptr)
	{
		TypeSymbol* substituted = genericType->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(pattern));
		if (substituted != nullptr)
			pattern = substituted;
	}

	if (pattern->Kind == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(pattern);
		if (typeParam->Parent != method)
			return true;

		auto it = std::find(method->TypeParameters.begin(), method->TypeParameters.end(), typeParam);
		if (it == method->TypeParameters.end())
			return true;

		std::size_t index = static_cast<std::size_t>(std::distance(method->TypeParameters.begin(), it));
		if (outMethodTypeArgs[index] == nullptr)
			outMethodTypeArgs[index] = concrete;
		else if (!SemanticModel::AreTypesEqual(outMethodTypeArgs[index], concrete))
			return false;

		return true;
	}

	if (pattern->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* patternGeneric = static_cast<GenericTypeSymbol*>(pattern);
		TypeSymbol* patternUnderlying = patternGeneric->UnderlayingType;

		if (patternUnderlying == TRAIT_ENUMERABLE && concrete->Kind == SyntaxKind::ArrayType)
		{
			TypeSymbol* elementType = static_cast<ArrayTypeSymbol*>(concrete)->UnderlayingType;
			TypeParameterSymbol* enumerableT = TRAIT_ENUMERABLE->TypeParameters[0];
			TypeSymbol* elementPattern = patternGeneric->SubstituteTypeParameters(enumerableT);
			return TryInferTypeArgument(elementPattern, elementType, method, genericType, outMethodTypeArgs);
		}

		if (concrete->Kind == SyntaxKind::GenericType)
		{
			GenericTypeSymbol* concreteGeneric = static_cast<GenericTypeSymbol*>(concrete);
			if (concreteGeneric->UnderlayingType == patternUnderlying)
			{
				for (TypeParameterSymbol* param : patternUnderlying->TypeParameters)
				{
					TypeSymbol* patternArg = patternGeneric->SubstituteTypeParameters(param);
					TypeSymbol* concreteArg = concreteGeneric->SubstituteTypeParameters(param);
					if (!TryInferTypeArgument(patternArg, concreteArg, method, genericType, outMethodTypeArgs))
						return false;
				}
			}
			return true;
		}

		TypeSymbol* concreteUnderlying = concrete;
		GenericTypeSymbol* concreteGeneric = nullptr;
		if (concrete->Kind == SyntaxKind::GenericType)
		{
			concreteGeneric = static_cast<GenericTypeSymbol*>(concrete);
			concreteUnderlying = concreteGeneric->UnderlayingType;
		}

		if (concreteUnderlying->Kind == SyntaxKind::ClassDeclaration ||
		    concreteUnderlying->Kind == SyntaxKind::StructDeclaration ||
		    concreteUnderlying->Kind == SyntaxKind::InterfaceDeclaration)
		{
			for (TypeSymbol* iface : concreteUnderlying->Interfaces)
			{
				if (iface == nullptr)
					continue;

				if (iface->Kind != SyntaxKind::GenericType)
					continue;

				GenericTypeSymbol* ifaceGeneric = static_cast<GenericTypeSymbol*>(iface);
				if (ifaceGeneric->UnderlayingType != patternUnderlying)
					continue;

				for (TypeParameterSymbol* param : patternUnderlying->TypeParameters)
				{
					TypeSymbol* patternArg = patternGeneric->SubstituteTypeParameters(param);
					TypeSymbol* concreteArg = ifaceGeneric->SubstituteTypeParameters(param);
					if (concreteArg != nullptr && concreteArg->Kind == SyntaxKind::TypeParameter && concreteGeneric != nullptr)
					{
						TypeSymbol* resolved = concreteGeneric->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(concreteArg));
						if (resolved != nullptr)
							concreteArg = resolved;
					}

					if (!TryInferTypeArgument(patternArg, concreteArg, method, genericType, outMethodTypeArgs))
						return false;
				}

				return true;
			}
		}

		if (patternUnderlying->Kind == SyntaxKind::DelegateType && concrete->Kind == SyntaxKind::DelegateType)
		{
			const DelegateTypeSymbol* patternDelegate = static_cast<const DelegateTypeSymbol*>(patternUnderlying);
			const DelegateTypeSymbol* concreteDelegate = static_cast<const DelegateTypeSymbol*>(concrete);

			if (patternDelegate->ReturnType != nullptr && concreteDelegate->ReturnType != nullptr)
			{
				TypeSymbol* returnPattern = SubstituteTypeParameters(patternDelegate->ReturnType, patternGeneric);
				if (!TryInferTypeArgument(returnPattern, concreteDelegate->ReturnType, method, genericType, outMethodTypeArgs))
					return false;
			}

			if (patternDelegate->Parameters.size() == concreteDelegate->Parameters.size())
			{
				for (std::size_t i = 0; i < patternDelegate->Parameters.size(); ++i)
				{
					TypeSymbol* paramPattern = SubstituteTypeParameters(patternDelegate->Parameters[i]->Type, patternGeneric);
					if (!TryInferTypeArgument(paramPattern, concreteDelegate->Parameters[i]->Type, method, genericType, outMethodTypeArgs))
						return false;
				}
			}

			return true;
		}
	}

	if (pattern->Kind == SyntaxKind::ArrayType && concrete->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* patternArray = static_cast<ArrayTypeSymbol*>(pattern);
		ArrayTypeSymbol* concreteArray = static_cast<ArrayTypeSymbol*>(concrete);
		return TryInferTypeArgument(patternArray->UnderlayingType, concreteArray->UnderlayingType, method, genericType, outMethodTypeArgs);
	}

	return true;
}

bool ExpressionBinder::InferMethodTypeArguments(MethodSymbol* method, const std::vector<TypeSymbol*>& argTypes, GenericTypeSymbol* genericType, std::vector<TypeSymbol*>& outMethodTypeArgs)
{
	outMethodTypeArgs.assign(method->TypeParameters.size(), nullptr);

	std::size_t start = method->Linking == LINK_INSTANCE ? 1 : 0;
	if (argTypes.size() + start != method->Parameters.size())
		return false;

	for (std::size_t i = start; i < method->Parameters.size(); ++i)
	{
		TypeSymbol* paramType = method->Parameters[i]->Type;
		if (!TryInferTypeArgument(paramType, argTypes[i - start], method, genericType, outMethodTypeArgs))
			return false;
	}

	for (TypeSymbol* type : outMethodTypeArgs)
		if (type == nullptr)
			return false;

	return true;
}

static bool TryInferClassTypeArgument(TypeSymbol* pattern, TypeSymbol* concrete, TypeSymbol* classDef, std::vector<TypeSymbol*>& outArgs)
{
	if (pattern == nullptr || concrete == nullptr)
		return true;

	if (pattern->Kind == SyntaxKind::TypeParameter)
	{
		TypeParameterSymbol* tp = static_cast<TypeParameterSymbol*>(pattern);
		if (tp->TypeArgumentIndex < outArgs.size() && classDef->TypeParameters[tp->TypeArgumentIndex] == tp)
		{
			TypeSymbol*& slot = outArgs[tp->TypeArgumentIndex];
			if (slot == nullptr)
				slot = concrete;
			else if (!SemanticModel::AreTypesEqual(slot, concrete))
				return false;
		}
		return true;
	}

	if (pattern->Kind == SyntaxKind::ArrayType)
	{
		if (concrete->Kind != SyntaxKind::ArrayType)
			return false;

		return TryInferClassTypeArgument(static_cast<ArrayTypeSymbol*>(pattern)->UnderlayingType,
			static_cast<ArrayTypeSymbol*>(concrete)->UnderlayingType, classDef, outArgs);
	}

	if (pattern->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* patternGeneric = static_cast<GenericTypeSymbol*>(pattern);
		if (concrete->Kind != SyntaxKind::GenericType)
			return false;

		GenericTypeSymbol* concreteGeneric = static_cast<GenericTypeSymbol*>(concrete);
		if (patternGeneric->UnderlayingType != concreteGeneric->UnderlayingType)
			return false;

		for (TypeParameterSymbol* param : patternGeneric->UnderlayingType->TypeParameters)
		{
			TypeSymbol* patternArg = patternGeneric->SubstituteTypeParameters(param);
			TypeSymbol* concreteArg = concreteGeneric->SubstituteTypeParameters(param);
			if (!TryInferClassTypeArgument(patternArg, concreteArg, classDef, outArgs))
				return false;
		}
		return true;
	}

	return true;
}

bool ExpressionBinder::InferClassTypeArguments(TypeSymbol* classDef, MethodSymbol* method, const std::vector<TypeSymbol*>& argTypes, std::vector<TypeSymbol*>& outClassTypeArgs)
{
	outClassTypeArgs.assign(classDef->TypeParameters.size(), nullptr);

	std::size_t start = method->Linking == LINK_INSTANCE ? 1 : 0;
	if (argTypes.size() + start != method->Parameters.size())
		return false;

	for (std::size_t i = start; i < method->Parameters.size(); ++i)
	{
		if (!TryInferClassTypeArgument(method->Parameters[i]->Type, argTypes[i - start], classDef, outClassTypeArgs))
			return false;
	}

	for (TypeSymbol* type : outClassTypeArgs)
		if (type == nullptr)
			return false;

	return true;
}

bool ExpressionBinder::MatchGenericMethodArguments(MethodSymbol* method, const std::vector<TypeSymbol*>& argTypes, GenericTypeSymbol* genericType, const std::vector<TypeSymbol*>& methodTypeArgs)
{
	if (argTypes.size() != method->Parameters.size())
		return false;

	for (std::size_t i = 0; i < method->Parameters.size(); ++i)
	{
		TypeSymbol* paramType = SubstituteTypeParameters(method->Parameters[i]->Type, genericType, method, methodTypeArgs);
		if (paramType == nullptr)
			return false;

		if (paramType == SymbolTable::Primitives::Any)
			continue;

		TypeSymbol* argType = argTypes[i];
		if (!SemanticModel::IsAssignableTo(paramType, argType))
			return false;
	}

	return true;
}

namespace
{
	struct NumericParseResult
	{
		bool Success = false;
		std::wstring Error;
	};

	bool IsValidIntegerDigit(wchar_t symbol, int base)
	{
		if (symbol >= L'0' && symbol <= L'1')
			return base >= 2;

		if (symbol >= L'2' && symbol <= L'9')
			return base >= 10;

		if ((symbol >= L'a' && symbol <= L'f') || (symbol >= L'A' && symbol <= L'F'))
			return base >= 16;

		return false;
	}

	bool TryParseBasePrefix(const std::wstring& word, int& base, std::size_t& prefixLength)
	{
		base = 10;
		prefixLength = 0;

		if (word.size() < 2 || word[0] != L'0')
			return false;

		switch (word[1])
		{
			case L'x': case L'X': base = 16; prefixLength = 2; return true;
			case L'd': case L'D': base = 10; prefixLength = 2; return true;
			case L'b': case L'B': base =  2; prefixLength = 2; return true;
			default: return false;
		}
	}

	bool TryParseVolumeSuffix(const std::wstring& word, std::size_t& multiplier, std::size_t& suffixLength)
	{
		multiplier = 1;
		suffixLength = 0;

		if (word.size() < 2)
			return false;

		wchar_t first = word[word.size() - 2];
		wchar_t second = word[word.size() - 1];

		if (second != L'B' && second != L'b')
			return false;

		switch (first)
		{
			case L'k': case L'K': multiplier = 1ULL << 10; break;
			case L'm': case L'M': multiplier = 1ULL << 20; break;
			case L'g': case L'G': multiplier = 1ULL << 30; break;
			case L't': case L'T': multiplier = 1ULL << 40; break;
			case L'p': case L'P': multiplier = 1ULL << 50; break;
			default: return false;
		}

		suffixLength = 2;
		return true;
	}

	double wstod_independent(const std::wstring& str, std::size_t* pos = nullptr)
	{
		if (str.empty())
			throw std::invalid_argument("wstod_independent: empty string");

		wchar_t* endptr = nullptr;
		const wchar_t* start = str.c_str();
		errno = 0;
		double value = 0.0;

#ifdef _WIN32
		static _locale_t c_locale = _create_locale(LC_NUMERIC, "C");
		value = _wcstod_l(start, &endptr, c_locale);
#else
		static locale_t c_locale = newlocale(LC_NUMERIC_MASK, "C", nullptr);
		value = wcstod_l(start, &endptr, c_locale);
#endif

		if (endptr == start)
		{
			throw std::invalid_argument("wstod_independent: no conversion could be performed");
		}

		if (errno == ERANGE)
		{
			throw std::out_of_range("wstod_independent: argument out of range");
		}

		if (pos)
		{
			*pos = static_cast<std::size_t>(endptr - start);
		}

		return value;
	}

	NumericParseResult ParseIntegerLiteral(const std::wstring& word, std::int64_t& outValue)
	{
		int base;
		std::size_t prefixLength;
		TryParseBasePrefix(word, base, prefixLength);

		std::size_t numLength = word.size() - prefixLength;
		std::size_t multiplier = 1;
		std::size_t suffixLength = 0;

		if (TryParseVolumeSuffix(word.substr(prefixLength), multiplier, suffixLength))
			numLength -= suffixLength;

		std::wstring numPart = word.substr(prefixLength, numLength);

		for (wchar_t symbol : numPart)
		{
			if (!IsValidIntegerDigit(symbol, base))
				return { false, L"Invalid characters in number" };
		}

		try
		{
			std::size_t pos = 0;
			long long raw = std::stoll(numPart, &pos, base);

			if (pos != numPart.size())
				return { false, L"Invalid number format" };

			if (multiplier > 1)
			{
				if (raw > (std::numeric_limits<std::int64_t>::max)() / static_cast<long long>(multiplier))
					return { false, L"Multiplication overflow" };

				raw *= static_cast<long long>(multiplier);
			}

			outValue = static_cast<std::int64_t>(raw);
			return { true, L"" };
		}
		catch (const std::out_of_range&)
		{
			return { false, L"Number out of range" };
		}
		catch (const std::exception&)
		{
			return { false, L"Invalid number format" };
		}
	}

	NumericParseResult ParseDoubleLiteral(const std::wstring& word, double& outValue)
	{
		int base;
		std::size_t prefixLength;
		if (TryParseBasePrefix(word, base, prefixLength))
			return { false, L"Floating point number cannot have base prefix" };

		std::size_t multiplier;
		std::size_t suffixLength;
		if (TryParseVolumeSuffix(word, multiplier, suffixLength))
			return { false, L"Floating point number cannot have suffix" };

		try
		{
			std::size_t pos = 0;
			double value = wstod_independent(word, &pos);

			if (pos != word.size())
				return { false, L"Invalid characters in floating point number" };

			outValue = value;
			return { true, L"" };
		}
		catch (const std::out_of_range&)
		{
			return { false, L"Number out of range" };
		}
		catch (const std::exception&)
		{
			return { false, L"Invalid floating point number format" };
		}
	}
}

TypeSymbol* ExpressionBinder::AnalyzeNumberLiteral(LiteralExpressionSyntax* node)
{
	LiteralSymbol* symbol = LookupSymbol<LiteralSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return SymbolTable::Primitives::Integer;

	NumericParseResult result = ParseIntegerLiteral(node->LiteralToken.Word, symbol->AsIntegerValue);
	if (!result.Success)
		Diagnostics.ReportError(node->LiteralToken, result.Error);

	TypeSymbol* expectedType = ResolveLeftDenotation();
	if (expectedType != nullptr && (
		expectedType == SymbolTable::Primitives::Byte ||
		expectedType == SymbolTable::Primitives::NativeInteger))
	{
		symbol->BoundType = expectedType;
		return expectedType;
	}

	return SymbolTable::Primitives::Integer;
}

TypeSymbol* ExpressionBinder::AnalyzeDoubleLiteral(LiteralExpressionSyntax* node)
{
	LiteralSymbol* symbol = LookupSymbol<LiteralSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		return SymbolTable::Primitives::Double;

	NumericParseResult result = ParseDoubleLiteral(node->LiteralToken.Word, symbol->AsDoubleValue);
	if (!result.Success)
		Diagnostics.ReportError(node->LiteralToken, result.Error);

	return SymbolTable::Primitives::Double;
}

IndexatorSymbol* ExpressionBinder::ResolveIndexator(IndexatorExpressionSyntax* node, TypeSymbol* currentType)
{
	MemberAccessExpressionSyntax* access = static_cast<MemberAccessExpressionSyntax*>(const_cast<ExpressionSyntax*>(node->PreviousExpression.get()));
	std::wstring methodName = access->IdentifierToken.Word;
	std::vector<TypeSymbol*> argTypes;

	for (const auto& arg : node->IndexatorList->Arguments)
	{
		VisitExpression(arg->Expression.get());

		TypeSymbol* argType = GetExpressionType(arg->Expression.get());
		if (argType == nullptr)
			return nullptr;

		argTypes.push_back(argType);
	}

	if (currentType->Kind == SyntaxKind::ArrayType)
	{
		if (SymbolTable::Primitives::Array == nullptr || SymbolTable::Primitives::Array->Indexators.empty())
			return nullptr;

		return SymbolTable::Primitives::Array->Indexators[0];
	}

	IndexatorSymbol* symbol = currentType->FindIndexator(argTypes);
	/*
	if (symbol == nullptr)
		symbol = SymbolTable::Global::Namespace->FindIndexator(argTypes);
	*/

	if (symbol == nullptr)
	{
		std::wstringstream diag;
		diag << "No indeaxtors for type \"" << currentType->Name << "\" found that accepts ";

		switch (argTypes.size())
		{
			case 0:
			{
				diag << "no arguments";
				break;
			}

			case 1:
			{
				TypeSymbol* type = argTypes.at(0);
				std::wstring typeName = type == nullptr ? L"<error>" : type->Name;
				diag << "argument (" << typeName << ")";
				break;
			}

			default:
			{
				TypeSymbol* type = argTypes.at(0);
				diag << L"arguments (" << type->Name;

				for (int i = 1; i < argTypes.size(); i++)
				{
					type = argTypes.at(i);
					diag << ", " << type->Name;
				}

				diag << ")";
				break;
			}
		}

		Diagnostics.ReportError(access->IdentifierToken, diag.str());
		return nullptr;
	}

	bool isStaticContext = GetIsStaticContext(node->PreviousExpression.get());
	if (isStaticContext && symbol->Linking == LINK_INSTANCE)
	{
		Diagnostics.ReportError(access->IdentifierToken, L"Cannot call instance indexator '" + methodName + L"' from type context");
		return nullptr;
	}

	if (!isStaticContext && symbol->Linking == LINK_STATIC)
	{
		Diagnostics.ReportError(access->IdentifierToken, L"Cannot call static indexator '" + methodName + L"' on instance reference");
		return nullptr;
	}

	return symbol;
}

void ExpressionBinder::VisitMemberAccessExpression(MemberAccessExpressionSyntax* node)
{
	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression.get());
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType().value_or(nullptr) : GetExpressionType(previous);
	type = AnalyzeMemberAccessExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitInvocationExpression(InvokationExpressionSyntax* node)
{
	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression.get());

	// Namespace-qualified invocation: e.g. a.Foo() where 'a' is a namespace.
	// Resolve the qualifier chain without visiting the intermediate member-access
	// nodes, since namespaces are not expression values.
	NamespaceNode* nsQualifier = ResolveNamespaceQualifier(previous);
	if (nsQualifier != nullptr)
	{
		TypeSymbol* type = AnalyzeQualifiedInvokationExpression(node, nsQualifier);
		SetExpressionType(node, type);
		return;
	}

	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType().value_or(nullptr) : GetExpressionType(previous);
	type = AnalyzeInvokationExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitIndexatorExpression(IndexatorExpressionSyntax* node)
{
	if (node->IndexatorList != nullptr)
		VisitIndexatorList(node->IndexatorList.get());

	ExpressionSyntax* previous = const_cast<ExpressionSyntax*>(node->PreviousExpression.get());
	if (previous != nullptr)
		VisitExpression(previous);

	TypeSymbol* type = previous == nullptr ? OwnerType().value_or(nullptr) : GetExpressionType(previous);
	type = AnalyzeIndexatorExpression(node, type);
	SetExpressionType(node, type);
}

void ExpressionBinder::VisitWhileStatement(WhileStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression.get());
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression.get());
		
		if (conditionType == nullptr)
		{
			Diagnostics.ReportError(node->KeywordToken, L"While loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			Diagnostics.ReportError(node->KeywordToken, L"While loop condition must be boolean, got '" + conditionType->Name + L"'");
		}
	}
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void ExpressionBinder::VisitUntilStatement(UntilStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression.get());
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression.get());
		
		if (conditionType == nullptr)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Until loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Until loop condition must be boolean, got '" + conditionType->Name + L"'");
		}
	}
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

void ExpressionBinder::VisitForStatement(ForStatementSyntax* node)
{
	if (node->InitializerStatement != nullptr)
		VisitStatement(node->InitializerStatement.get());
	
	if (node->ConditionExpression != nullptr)
	{
		VisitExpression(node->ConditionExpression.get());
		TypeSymbol* conditionType = GetExpressionType(node->ConditionExpression.get());
		
		if (conditionType == nullptr)
		{
			Diagnostics.ReportError(node->FirstSemicolon, L"For loop condition type could not be determined");
		}
		else if (conditionType != SymbolTable::Primitives::Boolean)
		{
			Diagnostics.ReportError(node->FirstSemicolon, L"For loop condition must be boolean, got '" + conditionType->Name + L"'");
		}
	}
	
	if (node->AfterRepeatStatement != nullptr)
		VisitStatement(node->AfterRepeatStatement.get());
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
}

TypeSymbol* ExpressionBinder::FindEnumerableElementType(TypeSymbol* rangeType, bool& isArrayRange, bool allowArray)
{
	isArrayRange = false;
	if (rangeType == nullptr)
		return nullptr;

	if (rangeType->Kind == SyntaxKind::ArrayType)
	{
		isArrayRange = allowArray;
		return static_cast<ArrayTypeSymbol*>(rangeType)->UnderlayingType;
	}

	TypeSymbol* underlying = rangeType;
	GenericTypeSymbol* genericRange = nullptr;
	if (rangeType->Kind == SyntaxKind::GenericType)
	{
		genericRange = static_cast<GenericTypeSymbol*>(rangeType);
		underlying = genericRange->UnderlayingType;
	}

	auto tryExtractElement = [&](GenericTypeSymbol* genericIface) -> TypeSymbol*
	{
		if (genericIface->UnderlayingType != TRAIT_ENUMERABLE)
			return nullptr;

		TypeParameterSymbol* enumerableT = TRAIT_ENUMERABLE->TypeParameters[0];
		TypeSymbol* elementType = genericIface->SubstituteTypeParameters(enumerableT);

		// Resolve type parameters belonging to the underlying generic type
		// (e.g. List<T> implements IEnumerable<T> or Dictionary<K,V> implements IEnumerable<KeyValuePair<K,V>>).
		if (genericRange != nullptr && elementType != nullptr)
			elementType = SubstituteTypeParameters(elementType, genericRange, nullptr, {});

		return elementType;
	};

	if (underlying == TRAIT_ENUMERABLE && genericRange != nullptr)
		return tryExtractElement(genericRange);

	for (TypeSymbol* iface : underlying->Interfaces)
	{
		if (iface->Kind != SyntaxKind::GenericType)
			continue;

		GenericTypeSymbol* genericIface = static_cast<GenericTypeSymbol*>(iface);
		if (TypeSymbol* elementType = tryExtractElement(genericIface))
			return elementType;
	}

	return nullptr;
}

void ExpressionBinder::VisitForEachStatement(ForEachStatementSyntax* node)
{
	VariableSymbol* variable = static_cast<VariableSymbol*>(Table->LookupSymbol(node).value_or(nullptr));

	if (node->RangeExpression != nullptr)
	{
		VisitExpression(node->RangeExpression.get());
		TypeSymbol* rangeType = GetExpressionType(node->RangeExpression.get());
		node->RangeType = rangeType;

		bool isArrayRange = false;
		if (TypeSymbol* elementType = FindEnumerableElementType(rangeType, isArrayRange, false))
		{
			node->IsArrayRange = false;
			if (variable != nullptr)
				variable->Type = elementType;
		}
		else
		{
			Diagnostics.ReportError(node->InKeywordToken, L"'foreach' expression must implement IEnumerable<T>");
		}
	}

	if (variable != nullptr)
	{
		Declare(variable);
		PushScope(variable);
	}

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());

	if (variable != nullptr)
		PopScope();
}

void ExpressionBinder::VisitForInStatement(ForInStatementSyntax* node)
{
	VariableSymbol* variable = static_cast<VariableSymbol*>(Table->LookupSymbol(node).value_or(nullptr));

	if (node->RangeExpression != nullptr)
	{
		VisitExpression(node->RangeExpression.get());
		TypeSymbol* rangeType = GetExpressionType(node->RangeExpression.get());
		node->RangeType = rangeType;

		bool isArrayRange = false;
		if (TypeSymbol* elementType = FindEnumerableElementType(rangeType, isArrayRange))
		{
			node->IsArrayRange = isArrayRange;
			if (!isArrayRange)
			{
				Diagnostics.ReportError(node->InKeywordToken, L"'for-in' expression must be an array");
			}
			else if (variable != nullptr)
			{
				variable->Type = elementType;
			}
		}
		else
		{
			Diagnostics.ReportError(node->InKeywordToken, L"'for-in' expression must be an array");
		}
	}

	if (variable != nullptr)
	{
		Declare(variable);
		PushScope(variable);
	}

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());

	if (variable != nullptr)
		PopScope();
}

void ExpressionBinder::VisitIfStatement(IfStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		if (node->ConditionExpression->Kind != SyntaxKind::ExpressionStatement)
		{
			Diagnostics.ReportError(node->KeywordToken, L"");
		}
		else
		{
			ExpressionStatementSyntax* statement = static_cast<ExpressionStatementSyntax*>(node->ConditionExpression.get());
			ExpressionSyntax* conditionExpr = statement->Expression.get();

			if (conditionExpr != nullptr)
			{
				VisitExpression(conditionExpr);
				TypeSymbol* conditionType = GetExpressionType(conditionExpr);
				
				if (conditionType == nullptr)
				{
					Diagnostics.ReportError(node->KeywordToken, L"If condition type could not be determined");
				}
				else if (conditionType != SymbolTable::Primitives::Boolean)
				{
					Diagnostics.ReportError(node->KeywordToken, L"If condition must be boolean, got '" + conditionType->Name + L"'");
				}
			}
			else
			{
				Diagnostics.ReportError(node->KeywordToken, L"If condition must be an expression");
			}
		}
	}
	
	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement.get());
}

void ExpressionBinder::VisitUnlessStatement(UnlessStatementSyntax* node)
{
	if (node->ConditionExpression != nullptr)
	{
		if (node->ConditionExpression->Kind != SyntaxKind::ExpressionStatement)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Unless condition must be an expression statement");
		}
		else
		{
			ExpressionStatementSyntax* statement = static_cast<ExpressionStatementSyntax*>(node->ConditionExpression.get());
			ExpressionSyntax* conditionExpr = statement->Expression.get();

		if (conditionExpr != nullptr)
		{
			VisitExpression(conditionExpr);
			TypeSymbol* conditionType = GetExpressionType(conditionExpr);
			
			if (conditionType == nullptr)
			{
				Diagnostics.ReportError(node->KeywordToken, L"Unless condition type could not be determined");
			}
			else if (conditionType != SymbolTable::Primitives::Boolean)
			{
				Diagnostics.ReportError(node->KeywordToken, L"Unless condition must be boolean, got '" + conditionType->Name + L"'");
			}
		}
			else
			{
				Diagnostics.ReportError(node->KeywordToken, L"Unless condition must be an expression");
			}
		}
	}

	if (node->StatementsBlock != nullptr)
		VisitStatementsBlock(node->StatementsBlock.get());
	
	if (node->NextStatement != nullptr)
		VisitConditionalClause(node->NextStatement.get());
}

static bool SymbolHasReturnType(const SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		return false;

	switch (symbol->Kind)
	{
		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::OperatorDeclaration:
		case SyntaxKind::FieldDeclaration:
		case SyntaxKind::PropertyDeclaration:
		case SyntaxKind::IndexatorDeclaration:
		case SyntaxKind::AccessorDeclaration:
			return true;

		default:
			return false;
	}
}

TypeSymbol* ExpressionBinder::FindTargetReturnType(SemanticScope*& scope)
{
	scope = CurrentScope();
	SyntaxSymbol* symbol = const_cast<SyntaxSymbol*>(scope->Owner);
	
	while (!SymbolHasReturnType(symbol))
	{
		if (scope->Parent == nullptr)
			return nullptr;

		scope = const_cast<SemanticScope*>(scope->Parent);
		symbol = const_cast<SyntaxSymbol*>(scope->Owner);
	}

	switch (symbol->Kind)
	{
		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::OperatorDeclaration:
		{
			MethodSymbol* methodSymbol = static_cast<MethodSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* methodSymbol = static_cast<FieldSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		case SyntaxKind::AccessorDeclaration:
		{
			AccessorSymbol* methodSymbol = static_cast<AccessorSymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		case SyntaxKind::IndexatorDeclaration:
		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* methodSymbol = static_cast<PropertySymbol*>(symbol);
			return methodSymbol->ReturnType;
		}

		default:
			return nullptr;
	}
}

TypeSymbol* ExpressionBinder::ResolveLeftDenotation()
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		const SyntaxSymbol* owner = scope->Owner;
		if (owner == nullptr)
			break;

		if (owner->Kind == SyntaxKind::Argument)
		{
			const LeftDenotationSymbol* param = static_cast<const LeftDenotationSymbol*>(owner);
			return const_cast<TypeSymbol*>(param->ExpectedType);
		}
	}

	return nullptr;
}

void ExpressionBinder::VisitReturnStatement(ReturnStatementSyntax* node)
{
	SemanticScope* searchingScope = nullptr;
	TypeSymbol* returnType = FindTargetReturnType(searchingScope);

	if (searchingScope == nullptr)
	{
		Diagnostics.ReportError(node->KeywordToken, L"Couldnt find scope to return within");
		return;
	}

	searchingScope->ReturnFound = true;

	if (searchingScope->Owner != nullptr && searchingScope->Owner->Kind == SyntaxKind::MethodDeclaration)
	{
		MethodSymbol* method = const_cast<MethodSymbol*>(static_cast<const MethodSymbol*>(searchingScope->Owner));
		if (method->IsAsync)
		{
			TypeSymbol* elementType = GetAsyncMethodElementType(method);
			if (elementType == SymbolTable::Primitives::Void)
			{
				if (node->Expression != nullptr)
				{
					VisitExpression(node->Expression.get());
					Diagnostics.ReportError(node->KeywordToken, L"Async Task method cannot return a value");
				}
			}
			else if (elementType != nullptr)
			{
				if (node->Expression == nullptr)
				{
					Diagnostics.ReportError(node->KeywordToken, L"Async ValueTask<T> method must return a value of type '" + elementType->Name + L"'");
					return;
				}

				VisitExpression(node->Expression.get());
				TypeSymbol* returnExprType = GetExpressionType(node->Expression.get());
				if (returnExprType == nullptr)
				{
					Diagnostics.ReportError(node->KeywordToken, L"Return expression type could not be determined");
				}
				else if (!SemanticModel::IsAssignableTo(elementType, returnExprType))
				{
					Diagnostics.ReportError(node->KeywordToken, L"Return type mismatch: expected '" + elementType->Name + L"' but got '" + returnExprType->Name + L"'");
				}
			}
			return;
		}
	}
	if (returnType == SymbolTable::Primitives::Any)
	{
		if (searchingScope->Owner == nullptr || searchingScope->Owner->Kind != SyntaxKind::MethodDeclaration)
		{
			Diagnostics.ReportError(node->KeywordToken, L"Return within invalid scope");
			return;
		}

		MethodSymbol* delegate = const_cast<MethodSymbol*>(static_cast<const MethodSymbol*>(searchingScope->Owner));
		if (node->Expression == nullptr)
		{
			delegate->ReturnType = SymbolTable::Primitives::Void;
			return;
		}
		else
		{
			VisitExpression(node->Expression.get());
			delegate->ReturnType = GetExpressionType(node->Expression.get());
			return;
		}
	}

	if (returnType == SymbolTable::Primitives::Void)
	{
		if (node->Expression != nullptr)
		{
			searchingScope->ReturnsAnything = true;
			VisitExpression(node->Expression.get());

			Diagnostics.ReportError(node->KeywordToken, L"Void method cannot return a value");
		}

		return;
	}

	if (returnType == nullptr)
	{
		Diagnostics.ReportError(node->KeywordToken, L"Return target type could not be determined");
		return;
	}

	if (node->Expression == nullptr)
	{
		Diagnostics.ReportError(SyntaxToken(), L"Return statement must return a value of type '" + returnType->Name + L"'");
		return;
	}

	searchingScope->ReturnsAnything = true;
	VisitExpression(node->Expression.get());
	TypeSymbol* returnExprType = GetExpressionType(node->Expression.get());

	if (returnExprType == nullptr)
	{
		Diagnostics.ReportError(node->KeywordToken, L"Return expression type could not be determined");
	}
	else if (returnType->Kind != SyntaxKind::TypeParameter && !SemanticModel::IsAssignableTo(returnType, returnExprType))
	{
		Diagnostics.ReportError(node->KeywordToken, L"Return type mismatch: expected '" + returnType->Name + L"' but got '" + returnExprType->Name + L"'");
	}
}

void ExpressionBinder::VisitDeferStatement(DeferStatementSyntax* node)
{
	if (node->Statement == nullptr)
		return;

	VisitStatement(node->Statement.get());

	if (node->Statement->Kind == SyntaxKind::VariableStatement)
	{
		VariableStatementSyntax* variableStatement = static_cast<VariableStatementSyntax*>(node->Statement.get());
		VariableSymbol* variable = LookupSymbol<VariableSymbol>(variableStatement).value_or(nullptr);
		if (variable == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"Variable declared in defer statement could not be resolved");
			return;
		}

		TypeSymbol* variableType = const_cast<TypeSymbol*>(variable->Type);
		if (variableType == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"Variable type could not be determined");
			return;
		}

		InterfaceSymbol* disposable = TRAIT_DISPOSABLE;
		if (disposable == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"IDisposable interface is not defined");
			return;
		}

		if (!SemanticModel::IsAssignableTo(disposable, variableType))
		{
			Diagnostics.ReportError(node->DeferToken, L"Type '" + variableType->Name + L"' declared in defer statement must implement IDisposable");
			return;
		}

		std::wstring disposeName = L"Dispose";
		MethodSymbol* interfaceDispose = disposable->FindMethod(disposeName, std::vector<TypeSymbol*>());
		if (interfaceDispose == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"IDisposable.Dispose() method not found");
			return;
		}

		MethodSymbol* implementation = variableType->FindInterfaceImplementation(interfaceDispose);
		if (implementation == nullptr)
		{
			Diagnostics.ReportError(node->DeferToken, L"Type '" + variableType->Name + L"' does not provide an implementation for IDisposable.Dispose()");
			return;
		}

		node->Variable = variable;
		node->DisposeMethod = implementation;
		node->IsResourceDefer = true;
	}
}

static bool IsScalarPrimitive(TypeSymbol* type)
{
	if (type == nullptr)
		return false;

	return type == SymbolTable::Primitives::Boolean
		|| type == SymbolTable::Primitives::Integer
		|| type == SymbolTable::Primitives::Double
		|| type == SymbolTable::Primitives::Char
		|| type == SymbolTable::Primitives::Byte
		|| type == SymbolTable::Primitives::NativeInteger;
}

static bool IsReferenceCastAllowed(TypeSymbol* source, TypeSymbol* target)
{
	if (source == nullptr || target == nullptr)
		return false;

	if (SemanticModel::AreTypesEqual(source, target))
		return true;

	if (source == SymbolTable::Primitives::Null)
		return target->Inlining == TypeInlining::ByReference || target == SymbolTable::Primitives::Any;

	if (target == SymbolTable::Primitives::Any || source == SymbolTable::Primitives::Any)
		return true;

	if (target->Inlining != TypeInlining::ByReference || source->Inlining != TypeInlining::ByReference)
		return false;

	if (SemanticModel::IsAssignableTo(target, source) || SemanticModel::IsAssignableTo(source, target))
		return true;

	if (target->Kind == SyntaxKind::InterfaceDeclaration || source->Kind == SyntaxKind::InterfaceDeclaration)
		return true;

	return false;
}

static OperatorSymbol* FindConversionOperator(TypeSymbol* ownerType, TypeSymbol* sourceType, TypeSymbol* targetType)
{
	if (ownerType == nullptr || sourceType == nullptr || targetType == nullptr)
		return nullptr;

	OperatorSymbol* op = ownerType->FindOperator(TokenType::AsOperator, { sourceType });
	if (op != nullptr && op->ReturnType == targetType)
		return op;

	op = ownerType->FindOperator(TokenType::AsOperator, {});
	if (op != nullptr && op->ReturnType == targetType)
		return op;

	return nullptr;
}

void ExpressionBinder::VisitCastExpression(CastExpressionSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	if (node->TargetType != nullptr)
		VisitType(node->TargetType.get());

	if (node->TargetType == nullptr || node->TargetType->Symbol == nullptr)
	{
		SetExpressionType(node, nullptr);
		return;
	}

	TypeSymbol* sourceType = GetExpressionType(node->Expression.get());
	TypeSymbol* targetType = node->TargetType->Symbol;

	if (sourceType == nullptr)
	{
		SetExpressionType(node, targetType);
		return;
	}

	if (SemanticModel::AreTypesEqual(sourceType, targetType))
	{
		SetExpressionType(node, targetType);
		return;
	}

	// 1. User-defined conversion operators.
	OperatorSymbol* sourceOperator = FindConversionOperator(sourceType, sourceType, targetType);
	OperatorSymbol* targetOperator = FindConversionOperator(targetType, sourceType, targetType);

	int conversionCount = 0;
	if (sourceOperator != nullptr) conversionCount++;
	if (targetOperator != nullptr && targetOperator != sourceOperator) conversionCount++;

	if (conversionCount > 1)
	{
		Diagnostics.ReportError(
			node->OperatorToken,
			L"Ambiguous cast from '" + sourceType->Name + L"' to '" + targetType->Name +
			L"'. Multiple user-defined 'operator as' conversions are available.");
		SetExpressionType(node, targetType);
		return;
	}

	if (sourceOperator != nullptr)
	{
		node->ToOperator = sourceOperator;
		SetExpressionType(node, targetType);
		return;
	}

	if (targetOperator != nullptr)
	{
		node->ToOperator = targetOperator;
		SetExpressionType(node, targetType);
		return;
	}

	// 2. Built-in reference / interface casts.
	if (IsReferenceCastAllowed(sourceType, targetType))
	{
		SetExpressionType(node, targetType);
		return;
	}

	// 3. Built-in primitive conversions.
	if (IsScalarPrimitive(sourceType) && IsScalarPrimitive(targetType))
	{
		node->IsPrimitiveCast = true;
		SetExpressionType(node, targetType);
		return;
	}

	Diagnostics.ReportError(
		node->OperatorToken,
		L"Cannot cast from '" + sourceType->Name + L"' to '" + targetType->Name + L"'.");

	SetExpressionType(node, targetType);
}

void ExpressionBinder::VisitIsExpression(IsExpressionSyntax* node)
{
	if (node->Expression != nullptr)
		VisitExpression(node->Expression.get());

	if (node->TargetType != nullptr)
		VisitType(node->TargetType.get());

	if (node->TargetType == nullptr || node->TargetType->Symbol == nullptr)
	{
		SetExpressionType(node, SymbolTable::Primitives::Boolean);
		return;
	}

	SetExpressionType(node, SymbolTable::Primitives::Boolean);
}
