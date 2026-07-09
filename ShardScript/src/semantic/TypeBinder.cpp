#include <shard/semantic/TypeBinder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/NamespaceTree.hpp>
#include <shard/semantic/SemanticValidator.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/lexical/TokenType.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/TypeArgumentsListSyntax.hpp>

#include <shard/parsing/nodes/Types/DelegateTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/NullableTypeSyntax.hpp>

#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/GenericTypeSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>

#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>

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

void TypeBinder::VisitNamespaceDeclaration(NamespaceDeclarationSyntax* node)
{
	// Namespace declarations are now handled inline in VisitCompilationUnit
}

void TypeBinder::VisitClassDeclaration(ClassDeclarationSyntax* node)
{
	ClassSymbol* symbol = LookupSymbol<ClassSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

	if (CheckNameDeclared(symbol))
		Diagnostics.ReportError(node->IdentifierToken, L"Symbol with the same name is already declared in current, or including context");

	PushScope(symbol);
	for (const auto& typeParam : symbol->TypeParameters)
		CurrentScope()->DeclareSymbol(typeParam);

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
		SemanticValidator::ValidateInterfaceImplementation(symbol, interfaceSymbol, Diagnostics, node->IdentifierToken);

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

void TypeBinder::VisitStructDeclaration(StructDeclarationSyntax* node)
{
	StructSymbol* symbol = LookupSymbol<StructSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

	PushScope(symbol);
	for (const auto& typeParam : symbol->TypeParameters)
		CurrentScope()->DeclareSymbol(typeParam);

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
		SemanticValidator::ValidateInterfaceImplementation(symbol, interfaceSymbol, Diagnostics, node->IdentifierToken);

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

void TypeBinder::VisitInterfaceDeclaration(InterfaceDeclarationSyntax* node)
{
    InterfaceSymbol* symbol = LookupSymbol<InterfaceSymbol>(node).value_or(nullptr);
    if (symbol == nullptr)
        throw std::runtime_error("symbol not found");

    if (symbol->IsReadyForRuntime())
        return;

    PushScope(symbol);
    for (const auto& typeParam : symbol->TypeParameters)
        CurrentScope()->DeclareSymbol(typeParam);

    for (const auto& member : node->Members)
    {
        SyntaxSymbol* memberSymbol = Table->LookupSymbol(member.get()).value_or(nullptr);
        if (memberSymbol != nullptr)
            Declare(memberSymbol);
    }

    for (const auto& member : node->Members)
        VisitMemberDeclaration(member.get());

    symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
    PopScope();
}

void TypeBinder::VisitDelegateDeclaration(DelegateDeclarationSyntax* node)
{
	DelegateTypeSymbol* symbol = LookupSymbol<DelegateTypeSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

	PushScope(symbol);

	for (TypeParameterSymbol* typeParam : symbol->TypeParameters)
		CurrentScope()->DeclareSymbol(typeParam);

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

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

static ObjectInstance* AllocateEnumValue(GarbageCollector& collector, TypeSymbol* enumType, std::int64_t value)
{
	ObjectInstance* instance = collector.AllocateInstance(enumType);
	instance->WriteInteger(value);
	return instance;
}

static ObjectInstance* enum_operator_or(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return AllocateEnumValue(context.Collector, static_cast<TypeSymbol*>(context.Method->Parent), left | right);
}

static ObjectInstance* enum_operator_and(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return AllocateEnumValue(context.Collector, static_cast<TypeSymbol*>(context.Method->Parent), left & right);
}

static ObjectInstance* enum_operator_equals(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left == right);
}

static ObjectInstance* enum_operator_not_equals(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left != right);
}

static ObjectInstance* enum_has_flag(const CallState& context)
{
	std::int64_t self = context.Args[0]->AsInteger();
	std::int64_t flag = context.Args[1]->AsInteger();
	bool result = (self & flag) == flag;
	return context.Collector.FromValue(result);
}

static ObjectInstance* enum_to_string(const CallState& context)
{
	TypeSymbol* enumType = static_cast<TypeSymbol*>(context.Method->Parent);
	if (enumType == nullptr)
		return context.Collector.FromValue(std::wstring());

	std::int64_t value = context.Args[0]->AsInteger();

	EnumSymbol* enumSymbol = nullptr;
	if (enumType->Kind == SyntaxKind::EnumDeclaration)
		enumSymbol = static_cast<EnumSymbol*>(enumType);

	if (enumSymbol != nullptr && enumSymbol->IsFlags)
	{
		if (value == 0)
			return context.Collector.FromValue(std::to_wstring(value));

		std::wostringstream result;
		bool first = true;

		for (FieldSymbol* field : enumSymbol->Fields)
		{
			if (field == nullptr || !field->IsEnumValue)
				continue;

			std::int64_t fieldValue = field->EnumValue;
			if (fieldValue == 0)
				continue;

			if ((value & fieldValue) == fieldValue)
			{
				if (!first)
					result << L" | ";
				result << field->Name;
				first = false;
			}
		}

		if (first)
			return context.Collector.FromValue(std::to_wstring(value));

		return context.Collector.FromValue(result.str());
	}
	else
	{
		for (FieldSymbol* field : enumType->Fields)
		{
			if (field != nullptr && field->IsEnumValue && field->EnumValue == value)
				return context.Collector.FromValue(field->Name);
		}

		return context.Collector.FromValue(std::to_wstring(value));
	}
}

static void RegisterEnumHelpers(EnumSymbol* enumSymbol, SymbolFactory& factory)
{
	// ToString for IPrintable
	MethodSymbol* toString = factory.Method(ACS_PUBLIC, LINK_INSTANCE, SymbolTable::Primitives::String, L"ToString", enum_to_string);
	toString->Parent = enumSymbol;
	enumSymbol->Methods.push_back(toString);
	enumSymbol->Interfaces.push_back(TRAIT_PRINTABLE);
	enumSymbol->InterfaceMethodMap[TRAIT_PRINTABLE_ToString] = toString;

	// HasFlag
	MethodSymbol* hasFlag = factory.Method(ACS_PUBLIC, LINK_INSTANCE, SymbolTable::Primitives::Boolean, L"HasFlag", enum_has_flag);
	hasFlag->Parent = enumSymbol;
	ParameterSymbol* hasFlagParam = factory.Parameter(L"flag", enumSymbol);
	hasFlagParam->Parent = hasFlag;
	hasFlag->Parameters.push_back(hasFlagParam);
	enumSymbol->Methods.push_back(hasFlag);

	// Bitwise operators (useful for flags, but registered for every enum)
	OperatorSymbol* orOp = factory.Operator(L"op_OrOperator", TokenType::OrOperator, enumSymbol, enum_operator_or, { enumSymbol, enumSymbol });
	orOp->Parent = enumSymbol;
	enumSymbol->Operators.push_back(orOp);

	OperatorSymbol* andOp = factory.Operator(L"op_AndOperator", TokenType::AndOperator, enumSymbol, enum_operator_and, { enumSymbol, enumSymbol });
	andOp->Parent = enumSymbol;
	enumSymbol->Operators.push_back(andOp);

	OperatorSymbol* eqOp = factory.Operator(L"op_EqualsOperator", TokenType::EqualsOperator, SymbolTable::Primitives::Boolean, enum_operator_equals, { enumSymbol, enumSymbol });
	eqOp->Parent = enumSymbol;
	enumSymbol->Operators.push_back(eqOp);

	OperatorSymbol* neqOp = factory.Operator(L"op_NotEqualsOperator", TokenType::NotEqualsOperator, SymbolTable::Primitives::Boolean, enum_operator_not_equals, { enumSymbol, enumSymbol });
	neqOp->Parent = enumSymbol;
	enumSymbol->Operators.push_back(neqOp);
}

void TypeBinder::VisitEnumDeclaration(EnumDeclarationSyntax* node)
{
	EnumSymbol* symbol = LookupSymbol<EnumSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

	if (CheckNameDeclared(symbol))
		Diagnostics.ReportError(node->IdentifierToken, L"Symbol with the same name is already declared in current, or including context");

	PushScope(symbol);

	for (const auto& field : node->Fields)
	{
		if (field->InitializerExpression != nullptr)
			VisitExpression(field->InitializerExpression.get());
	}

	RegisterEnumHelpers(symbol, Factory);

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

void TypeBinder::VisitConstructorDeclaration(ConstructorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

	PushScope(symbol);
	if (node->ParametersList != nullptr)
	{
		VisitParametersList(node->ParametersList.get());
		BindParametersList(node->ParametersList.get(), symbol->Parameters);
	}

	if (node->Body != nullptr)
		VisitStatementsBlock(node->Body.get());

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

void TypeBinder::VisitMethodDeclaration(MethodDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

	PushScope(symbol);

	for (TypeParameterSymbol* typeParam : symbol->TypeParameters)
		CurrentScope()->DeclareSymbol(typeParam);

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

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

void TypeBinder::VisitOperatorDeclaration(OperatorDeclarationSyntax* node)
{
	MethodSymbol* symbol = LookupSymbol<MethodSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("operator symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

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

	OperatorSymbol* opSymbol = static_cast<OperatorSymbol*>(symbol);
	if (opSymbol->OperatorToken == TokenType::AsOperator)
	{
		if (opSymbol->Linking != SymbolLinking::Static)
		{
			Diagnostics.ReportError(
				node->OperatorToken,
				L"Conversion operator 'as' must be static.");
		}

		if (opSymbol->Parameters.size() != 1)
		{
			Diagnostics.ReportError(
				node->OperatorToken,
				L"Conversion operator 'as' must take exactly one parameter.");
		}
	}

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

void TypeBinder::VisitFieldDeclaration(FieldDeclarationSyntax* node)
{
	FieldSymbol* symbol = LookupSymbol<FieldSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

	if (node->ReturnType != nullptr)
	{
		VisitType(node->ReturnType.get());
		symbol->ReturnType = node->ReturnType->Symbol;
	}

	if (node->InitializerExpression != nullptr)
		VisitExpression(node->InitializerExpression.get());

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
}

void TypeBinder::VisitPropertyDeclaration(PropertyDeclarationSyntax* node)
{
	PropertySymbol* symbol = LookupSymbol<PropertySymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

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

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
	PopScope();
}

void TypeBinder::VisitIndexatorDeclaration(IndexatorDeclarationSyntax* node)
{
	IndexatorSymbol* symbol = LookupSymbol<IndexatorSymbol>(node).value_or(nullptr);
	if (symbol == nullptr)
		throw std::runtime_error("symbol not found");

	if (symbol->IsReadyForRuntime())
		return;

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

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
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

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
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

	symbol->AdvanceAnalysisState(SymbolAnalysisState::TypeResolved);
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
			if (throwable != nullptr && !SemanticModel::IsAssignableTo(throwable, exceptionType))
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
