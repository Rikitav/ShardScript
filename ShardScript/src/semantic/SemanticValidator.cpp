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

#include <unordered_set>
#include <unordered_map>

namespace
{
    bool IsInterfaceImplementationMatching(shard::MethodSymbol* interfaceMethod, shard::MethodSymbol* classMethod)
    {
        if (interfaceMethod == nullptr || classMethod == nullptr)
            return false;

        if (interfaceMethod->Name != classMethod->Name)
            return false;

        if (interfaceMethod->Parameters.size() != classMethod->Parameters.size())
            return false;

        // Allow return-type covariance: the implementation may return a more derived type.
        if (!shard::SemanticModel::IsAssignableTo(interfaceMethod->ReturnType, classMethod->ReturnType))
            return false;

        for (std::size_t i = 0; i < interfaceMethod->Parameters.size(); i++)
        {
            shard::ParameterSymbol* a = interfaceMethod->Parameters[i];
            shard::ParameterSymbol* b = classMethod->Parameters[i];
            if (a == nullptr || b == nullptr)
                return false;

            if (!shard::SemanticModel::AreTypesEqual(a->Type, b->Type))
                return false;
        }

        return true;
    }

    bool IsInterfaceImplementationMatching(shard::PropertySymbol* interfaceProperty, shard::PropertySymbol* classProperty)
    {
        if (interfaceProperty == nullptr || classProperty == nullptr)
            return false;

        if (interfaceProperty->Name != classProperty->Name)
            return false;

        if (!shard::SemanticModel::AreTypesEqual(interfaceProperty->ReturnType, classProperty->ReturnType))
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
        std::unordered_set<InterfaceSymbol*>& visited)
    {
        if (interfaceSymbol == nullptr || !visited.insert(interfaceSymbol).second)
            return;

        for (TypeSymbol* baseInterfaceType : interfaceSymbol->Interfaces)
        {
            if (baseInterfaceType != nullptr && baseInterfaceType->Kind == SyntaxKind::InterfaceDeclaration)
                BindInterfaceImplementationsInternal(typeSymbol, static_cast<InterfaceSymbol*>(baseInterfaceType), visited);
        }

        for (MethodSymbol* interfaceMethod : interfaceSymbol->Methods)
        {
            auto it = typeSymbol->InterfaceMethodMap.find(interfaceMethod);
            if (it != typeSymbol->InterfaceMethodMap.end() && it->second != nullptr)
                continue;

            for (MethodSymbol* classMethod : typeSymbol->Methods)
            {
                if (IsInterfaceImplementationMatching(interfaceMethod, classMethod))
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
                    if (IsInterfaceImplementationMatching(interfaceProperty, classProperty))
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

    void SemanticValidator::BindInterfaceImplementations(TypeSymbol* typeSymbol, InterfaceSymbol* interfaceSymbol)
    {
        std::unordered_set<InterfaceSymbol*> visited;
        BindInterfaceImplementationsInternal(typeSymbol, interfaceSymbol, visited);
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
            if (interfaceType == nullptr || interfaceType->Kind != SyntaxKind::InterfaceDeclaration)
                continue;

            InterfaceSymbol* interfaceSymbol = static_cast<InterfaceSymbol*>(interfaceType);

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
                        if (IsInterfaceImplementationMatching(interfaceMethod, classMethod))
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
                else if (!IsInterfaceImplementationMatching(interfaceMethod, matchedMethod))
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
                        if (IsInterfaceImplementationMatching(interfaceProperty, classProperty))
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
                else if (!IsInterfaceImplementationMatching(interfaceProperty, matchedProperty))
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
