#include <shard/semantic/SemanticValidator.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/analysis/TextLocation.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <unordered_set>
#include <unordered_map>

namespace
{
    shard::TypeSymbol* SubstituteInterfaceType(shard::TypeSymbol* type, shard::GenericTypeSymbol* genericInterface)
    {
        if (type == nullptr || genericInterface == nullptr)
            return type;

        if (type->Kind == shard::SyntaxKind::TypeParameter)
        {
            shard::TypeSymbol* substituted = genericInterface->SubstituteTypeParameters(static_cast<shard::TypeParameterSymbol*>(type));
            return substituted != nullptr ? substituted : type;
        }

        if (type->Kind == shard::SyntaxKind::GenericType)
        {
            shard::GenericTypeSymbol* generic = static_cast<shard::GenericTypeSymbol*>(type);
            shard::TypeSymbol* underlying = generic->UnderlayingType;

            if (underlying == nullptr)
                return type;

            bool changed = false;
            std::vector<shard::TypeSymbol*> newArgs;
            newArgs.reserve(underlying->TypeParameters.size());

            for (shard::TypeParameterSymbol* param : underlying->TypeParameters)
            {
                shard::TypeSymbol* arg = generic->SubstituteTypeParameters(param);
                shard::TypeSymbol* newArg = SubstituteInterfaceType(arg, genericInterface);
                if (newArg != arg)
                    changed = true;
                newArgs.push_back(newArg != nullptr ? newArg : arg);
            }

            if (!changed)
                return type;

            shard::GenericTypeSymbol* result = new shard::GenericTypeSymbol(underlying);
            for (std::size_t i = 0; i < newArgs.size(); ++i)
                result->AddTypeParameter(underlying->TypeParameters[i], newArgs[i]);

            return result;
        }

        if (type->Kind == shard::SyntaxKind::ArrayType)
        {
            shard::ArrayTypeSymbol* array = static_cast<shard::ArrayTypeSymbol*>(type);
            shard::TypeSymbol* newElement = SubstituteInterfaceType(array->UnderlayingType, genericInterface);
            if (newElement == array->UnderlayingType)
                return type;

            return new shard::ArrayTypeSymbol(newElement);
        }

        return type;
    }

    bool IsInterfaceImplementationMatching(shard::MethodSymbol* interfaceMethod, shard::MethodSymbol* classMethod, shard::GenericTypeSymbol* genericInterface = nullptr)
    {
        if (interfaceMethod == nullptr || classMethod == nullptr)
            return false;

        if (interfaceMethod->Name != classMethod->Name)
            return false;

        if (interfaceMethod->Parameters.size() != classMethod->Parameters.size())
            return false;

        // Allow return-type covariance: the implementation may return a more derived type.
        shard::TypeSymbol* expectedReturn = SubstituteInterfaceType(interfaceMethod->ReturnType, genericInterface);
        if (!shard::SemanticModel::IsAssignableTo(expectedReturn, classMethod->ReturnType))
            return false;

        for (std::size_t i = 0; i < interfaceMethod->Parameters.size(); i++)
        {
            shard::ParameterSymbol* a = interfaceMethod->Parameters[i];
            shard::ParameterSymbol* b = classMethod->Parameters[i];
            if (a == nullptr || b == nullptr)
                return false;

            shard::TypeSymbol* expectedType = SubstituteInterfaceType(a->Type, genericInterface);
            if (!shard::SemanticModel::AreTypesEqual(expectedType, b->Type))
                return false;
        }

        return true;
    }

    bool IsInterfaceImplementationMatching(shard::PropertySymbol* interfaceProperty, shard::PropertySymbol* classProperty, shard::GenericTypeSymbol* genericInterface = nullptr)
    {
        if (interfaceProperty == nullptr || classProperty == nullptr)
            return false;

        if (interfaceProperty->Name != classProperty->Name)
            return false;

        shard::TypeSymbol* expectedReturn = SubstituteInterfaceType(interfaceProperty->ReturnType, genericInterface);
        if (!shard::SemanticModel::AreTypesEqual(expectedReturn, classProperty->ReturnType))
            return false;

        if (interfaceProperty->Getter != nullptr && classProperty->Getter == nullptr)
            return false;

        if (interfaceProperty->Setter != nullptr && classProperty->Setter == nullptr)
            return false;

        return true;
    }
}

namespace shard
{
    // =====================================================================
    //  Construction / entry points
    // =====================================================================

    SemanticValidator::SemanticValidator(SemanticModel& model, DiagnosticsContext& diagnostics)
        : SyntaxVisitor(model, diagnostics), ScopeVisitor(model.Table.get())
    {
    }

    void SemanticValidator::Validate(SyntaxTree& syntaxTree)
    {
        // Walk the source tree for any source-specific final checks and to
        // keep the phase symmetrical with the other analyzer passes.
        VisitSyntaxTree(syntaxTree);

        // The symbol-table pass catches symbols produced by native libraries
        // and any source symbol that was not covered by the tree walk.
        ValidateAllSymbols();
    }

    void SemanticValidator::ValidateAllSymbols()
    {
        for (TypeSymbol* typeSymbol : Table->GetTypeSymbols())
        {
            if (typeSymbol == nullptr)
                continue;

            if (typeSymbol->IsReadyForRuntime())
                continue;

            if (typeSymbol->AnalysisState >= SymbolAnalysisState::Verified)
                continue;

            ValidateType(typeSymbol);
            typeSymbol->AdvanceAnalysisState(SymbolAnalysisState::Verified);
        }
    }

    // =====================================================================
    //  Per-type validation
    // =====================================================================

    void SemanticValidator::ValidateType(TypeSymbol* typeSymbol)
    {
        if (typeSymbol == nullptr)
            return;

        // Interfaces declare contracts; only concrete types must satisfy them.
        if (typeSymbol->Kind != SyntaxKind::InterfaceDeclaration)
            ValidateInterfaceImplementations(typeSymbol);

        if (typeSymbol->Kind == SyntaxKind::EnumDeclaration)
            ValidateEnumValues(typeSymbol);
    }

    SyntaxToken SemanticValidator::GetErrorToken(SyntaxSymbol* symbol)
    {
        if (symbol == nullptr)
            return SyntaxToken();

        auto nodeOpt = Table->LookupNode(symbol);
        if (nodeOpt.has_value())
        {
            SyntaxNode* node = nodeOpt.value();
            MemberDeclarationSyntax* member = dynamic_cast<MemberDeclarationSyntax*>(node);
            if (member != nullptr)
                return member->IdentifierToken;
        }

        return SyntaxToken(TokenType::Unknown, symbol->Name, TextLocation(), false);
    }

    // =====================================================================
    //  Interface implementation binding (used by TypeBinder)
    // =====================================================================

    static void BindInterfaceImplementationsInternal(
        TypeSymbol* typeSymbol,
        InterfaceSymbol* interfaceSymbol,
        GenericTypeSymbol* genericInterface,
        std::unordered_set<TypeSymbol*>& visited)
    {
        if (interfaceSymbol == nullptr)
            return;

        TypeSymbol* visitedKey = genericInterface != nullptr ? static_cast<TypeSymbol*>(genericInterface) : static_cast<TypeSymbol*>(interfaceSymbol);
        if (!visited.insert(visitedKey).second)
            return;

        for (TypeSymbol* baseInterfaceType : interfaceSymbol->Interfaces)
        {
            if (baseInterfaceType == nullptr)
                continue;

            TypeSymbol* effectiveBase = baseInterfaceType;
            if (baseInterfaceType->Kind == SyntaxKind::GenericType && genericInterface != nullptr)
                effectiveBase = SubstituteInterfaceType(baseInterfaceType, genericInterface);

            if (effectiveBase->Kind == SyntaxKind::InterfaceDeclaration)
            {
                BindInterfaceImplementationsInternal(typeSymbol, static_cast<InterfaceSymbol*>(effectiveBase), nullptr, visited);
            }
            else if (effectiveBase->Kind == SyntaxKind::GenericType)
            {
                GenericTypeSymbol* baseGeneric = static_cast<GenericTypeSymbol*>(effectiveBase);
                if (baseGeneric->UnderlayingType != nullptr && baseGeneric->UnderlayingType->Kind == SyntaxKind::InterfaceDeclaration)
                    BindInterfaceImplementationsInternal(typeSymbol, static_cast<InterfaceSymbol*>(baseGeneric->UnderlayingType), baseGeneric, visited);
            }
        }

        for (MethodSymbol* interfaceMethod : interfaceSymbol->Methods)
        {
            auto it = typeSymbol->InterfaceMethodMap.find(interfaceMethod);
            if (it != typeSymbol->InterfaceMethodMap.end() && it->second != nullptr)
                continue;

            for (MethodSymbol* classMethod : typeSymbol->Methods)
            {
                if (IsInterfaceImplementationMatching(interfaceMethod, classMethod, genericInterface))
                {
                    typeSymbol->InterfaceMethodMap[interfaceMethod] = classMethod;
                    break;
                }
            }
        }

        for (PropertySymbol* interfaceProperty : interfaceSymbol->Properties)
        {
            PropertySymbol* matchedProperty = nullptr;

            if (interfaceProperty->Getter != nullptr)
            {
                auto it = typeSymbol->InterfaceMethodMap.find(interfaceProperty->Getter);
                if (it != typeSymbol->InterfaceMethodMap.end() && it->second != nullptr)
                {
                    SyntaxSymbol* owner = it->second->Parent;
                    if (owner != nullptr && owner->Kind == SyntaxKind::PropertyDeclaration)
                        matchedProperty = static_cast<PropertySymbol*>(owner);
                }
            }

            if (matchedProperty == nullptr && interfaceProperty->Setter != nullptr)
            {
                auto it = typeSymbol->InterfaceMethodMap.find(interfaceProperty->Setter);
                if (it != typeSymbol->InterfaceMethodMap.end() && it->second != nullptr)
                {
                    SyntaxSymbol* owner = it->second->Parent;
                    if (owner != nullptr && owner->Kind == SyntaxKind::PropertyDeclaration)
                        matchedProperty = static_cast<PropertySymbol*>(owner);
                }
            }

            if (matchedProperty == nullptr)
            {
                for (PropertySymbol* classProperty : typeSymbol->Properties)
                {
                    if (IsInterfaceImplementationMatching(interfaceProperty, classProperty, genericInterface))
                    {
                        matchedProperty = classProperty;
                        break;
                    }
                }
            }

            if (matchedProperty == nullptr)
                continue;

            if (interfaceProperty->Getter != nullptr && matchedProperty->Getter != nullptr)
                typeSymbol->InterfaceMethodMap[interfaceProperty->Getter] = matchedProperty->Getter;

            if (interfaceProperty->Setter != nullptr && matchedProperty->Setter != nullptr)
                typeSymbol->InterfaceMethodMap[interfaceProperty->Setter] = matchedProperty->Setter;
        }
    }

    void SemanticValidator::BindInterfaceImplementations(TypeSymbol* typeSymbol, TypeSymbol* interfaceType)
    {
        std::unordered_set<TypeSymbol*> visited;

        if (interfaceType->Kind == SyntaxKind::InterfaceDeclaration)
        {
            BindInterfaceImplementationsInternal(typeSymbol, static_cast<InterfaceSymbol*>(interfaceType), nullptr, visited);
        }
        else if (interfaceType->Kind == SyntaxKind::GenericType)
        {
            GenericTypeSymbol* genericInterface = static_cast<GenericTypeSymbol*>(interfaceType);
            if (genericInterface->UnderlayingType != nullptr && genericInterface->UnderlayingType->Kind == SyntaxKind::InterfaceDeclaration)
                BindInterfaceImplementationsInternal(typeSymbol, static_cast<InterfaceSymbol*>(genericInterface->UnderlayingType), genericInterface, visited);
        }
    }

    // =====================================================================
    //  Interface implementation validation
    // =====================================================================

    void SemanticValidator::ValidateInterfaceImplementations(TypeSymbol* typeSymbol)
    {
        if (typeSymbol == nullptr || typeSymbol->Interfaces.empty())
            return;

        SyntaxToken errorToken = GetErrorToken(typeSymbol);

        for (TypeSymbol* interfaceType : GetAllInterfaces(typeSymbol))
        {
            if (interfaceType == nullptr)
                continue;

            InterfaceSymbol* interfaceSymbol = nullptr;
            GenericTypeSymbol* genericInterface = nullptr;
            if (interfaceType->Kind == SyntaxKind::InterfaceDeclaration)
            {
                interfaceSymbol = static_cast<InterfaceSymbol*>(interfaceType);
            }
            else if (interfaceType->Kind == SyntaxKind::GenericType)
            {
                genericInterface = static_cast<GenericTypeSymbol*>(interfaceType);
                if (genericInterface->UnderlayingType != nullptr && genericInterface->UnderlayingType->Kind == SyntaxKind::InterfaceDeclaration)
                    interfaceSymbol = static_cast<InterfaceSymbol*>(genericInterface->UnderlayingType);
            }

            if (interfaceSymbol == nullptr)
                continue;

            for (MethodSymbol* interfaceMethod : interfaceSymbol->Methods)
            {
                MethodSymbol* matchedMethod = nullptr;

                auto it = typeSymbol->InterfaceMethodMap.find(interfaceMethod);
                if (it != typeSymbol->InterfaceMethodMap.end())
                    matchedMethod = it->second;

                if (matchedMethod == nullptr)
                {
                    for (MethodSymbol* classMethod : typeSymbol->Methods)
                    {
                        if (IsInterfaceImplementationMatching(interfaceMethod, classMethod, genericInterface))
                        {
                            matchedMethod = classMethod;
                            break;
                        }
                    }
                }

                if (matchedMethod == nullptr)
                {
                    Diagnostics.ReportError(errorToken,
                        L"Type '" + typeSymbol->Name + L"' does not implement interface method '" + interfaceMethod->Name + L"' from '" + interfaceSymbol->Name + L"'");
                }
                else if (!IsInterfaceImplementationMatching(interfaceMethod, matchedMethod, genericInterface))
                {
                    Diagnostics.ReportError(errorToken,
                        L"Implementation of interface method '" + interfaceMethod->Name + L"' from '" + interfaceSymbol->Name + L"' on type '" + typeSymbol->Name + L"' has an incompatible signature");
                }
                else
                {
                    typeSymbol->InterfaceMethodMap[interfaceMethod] = matchedMethod;
                }
            }

            for (PropertySymbol* interfaceProperty : interfaceSymbol->Properties)
            {
                PropertySymbol* matchedProperty = nullptr;

                if (interfaceProperty->Getter != nullptr)
                {
                    auto it = typeSymbol->InterfaceMethodMap.find(interfaceProperty->Getter);
                    if (it != typeSymbol->InterfaceMethodMap.end() && it->second != nullptr)
                    {
                        SyntaxSymbol* owner = it->second->Parent;
                        if (owner != nullptr && owner->Kind == SyntaxKind::PropertyDeclaration)
                            matchedProperty = static_cast<PropertySymbol*>(owner);
                    }
                }

                if (matchedProperty == nullptr && interfaceProperty->Setter != nullptr)
                {
                    auto it = typeSymbol->InterfaceMethodMap.find(interfaceProperty->Setter);
                    if (it != typeSymbol->InterfaceMethodMap.end() && it->second != nullptr)
                    {
                        SyntaxSymbol* owner = it->second->Parent;
                        if (owner != nullptr && owner->Kind == SyntaxKind::PropertyDeclaration)
                            matchedProperty = static_cast<PropertySymbol*>(owner);
                    }
                }

                if (matchedProperty == nullptr)
                {
                    for (PropertySymbol* classProperty : typeSymbol->Properties)
                    {
                        if (IsInterfaceImplementationMatching(interfaceProperty, classProperty, genericInterface))
                        {
                            matchedProperty = classProperty;
                            break;
                        }
                    }
                }

                if (matchedProperty == nullptr)
                {
                    Diagnostics.ReportError(errorToken,
                        L"Type '" + typeSymbol->Name + L"' does not implement interface property '" + interfaceProperty->Name + L"' from '" + interfaceSymbol->Name + L"'");
                }
                else if (!IsInterfaceImplementationMatching(interfaceProperty, matchedProperty, genericInterface))
                {
                    Diagnostics.ReportError(errorToken,
                        L"Implementation of interface property '" + interfaceProperty->Name + L"' from '" + interfaceSymbol->Name + L"' on type '" + typeSymbol->Name + L"' has an incompatible signature");
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
    }

    // =====================================================================
    //  Enum validation
    // =====================================================================

    void SemanticValidator::ValidateEnumValues(TypeSymbol* typeSymbol)
    {
        EnumSymbol* enumSymbol = static_cast<EnumSymbol*>(typeSymbol);
        if (enumSymbol->IsFlags)
            return;

        SyntaxToken errorToken = GetErrorToken(typeSymbol);
        std::unordered_map<std::int64_t, FieldSymbol*> seen;

        for (FieldSymbol* field : enumSymbol->Fields)
        {
            if (field == nullptr || !field->IsEnumValue)
                continue;

            auto it = seen.find(field->EnumValue);
            if (it != seen.end())
            {
                Diagnostics.ReportError(errorToken,
                    L"Enum '" + enumSymbol->Name + L"' has duplicate value " + std::to_wstring(field->EnumValue) +
                    L" on fields '" + it->second->Name + L"' and '" + field->Name + L"'");
            }
            else
            {
                seen[field->EnumValue] = field;
            }
        }
    }

    // =====================================================================
    //  Interface graph traversal
    // =====================================================================

    void SemanticValidator::CollectAllInterfaces(const TypeSymbol* type, std::vector<TypeSymbol*>& out, std::unordered_set<TypeSymbol*>& visited)
    {
        if (type == nullptr)
            return;

        for (TypeSymbol* iface : type->Interfaces)
        {
            if (iface == nullptr)
                continue;

            if (visited.insert(iface).second)
            {
                out.push_back(iface);
                CollectAllInterfaces(iface, out, visited);
            }
        }
    }

    std::vector<TypeSymbol*> SemanticValidator::GetAllInterfaces(const TypeSymbol* type)
    {
        std::vector<TypeSymbol*> result;
        std::unordered_set<TypeSymbol*> visited;
        CollectAllInterfaces(type, result, visited);
        return result;
    }
}
