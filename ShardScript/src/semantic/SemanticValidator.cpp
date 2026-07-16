#include <shard/semantic/SemanticValidator.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/analysis/TextLocation.hpp>
#include <shard/lexical/TokenType.hpp>

namespace shard
{
    static bool IsInterfaceImplementationMatching(MethodSymbol* interfaceMethod, MethodSymbol* classMethod)
    {
        if (interfaceMethod->Name != classMethod->Name)
            return false;

        if (interfaceMethod->Parameters.size() != classMethod->Parameters.size())
            return false;

        // Allow return-type covariance: the implementation may return a more derived type.
        if (!SemanticModel::IsAssignableTo(interfaceMethod->ReturnType, classMethod->ReturnType))
            return false;

        for (std::size_t i = 0; i < interfaceMethod->Parameters.size(); i++)
        {
            if (!SemanticModel::AreTypesEqual(interfaceMethod->Parameters[i]->Type, classMethod->Parameters[i]->Type))
                return false;
        }

        return true;
    }

    static bool IsInterfaceImplementationMatching(PropertySymbol* interfaceProperty, PropertySymbol* classProperty)
    {
        if (interfaceProperty->Name != classProperty->Name)
            return false;

        if (!SemanticModel::AreTypesEqual(interfaceProperty->ReturnType, classProperty->ReturnType))
            return false;

        if (interfaceProperty->Getter != nullptr && classProperty->Getter == nullptr)
            return false;

        if (interfaceProperty->Setter != nullptr && classProperty->Setter == nullptr)
            return false;

        return true;
    }

    void SemanticValidator::ValidateInterfaceImplementation(
        TypeSymbol* typeSymbol,
        InterfaceSymbol* interfaceSymbol,
        DiagnosticsContext& diagnostics,
        SyntaxToken errorToken)
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

    void SemanticValidator::ValidateExplicitInterfaceImplementations(
        TypeSymbol* typeSymbol,
        InterfaceSymbol* interfaceSymbol,
        DiagnosticsContext& diagnostics,
        SyntaxToken errorToken)
    {
        if (typeSymbol == nullptr || interfaceSymbol == nullptr)
            return;

        for (MethodSymbol* interfaceMethod : interfaceSymbol->Methods)
        {
            auto it = typeSymbol->InterfaceMethodMap.find(interfaceMethod);
            if (it == typeSymbol->InterfaceMethodMap.end() || it->second == nullptr)
            {
                diagnostics.ReportError(
                    errorToken,
                    L"Type '" + typeSymbol->Name + L"' does not explicitly implement interface method '" + interfaceMethod->Name + L"' from '" + interfaceSymbol->Name + L"'");
            }
            else if (!IsInterfaceImplementationMatching(interfaceMethod, it->second))
            {
                diagnostics.ReportError(
                    errorToken,
                    L"Implementation of interface method '" + interfaceMethod->Name + L"' from '" + interfaceSymbol->Name + L"' on type '" + typeSymbol->Name + L"' has an incompatible signature");
            }
        }

        for (PropertySymbol* interfaceProperty : interfaceSymbol->Properties)
        {
            if (interfaceProperty->Getter != nullptr)
            {
                auto it = typeSymbol->InterfaceMethodMap.find(interfaceProperty->Getter);
                if (it == typeSymbol->InterfaceMethodMap.end() || it->second == nullptr)
                {
                    diagnostics.ReportError(
                        errorToken,
                        L"Type '" + typeSymbol->Name + L"' does not explicitly implement property getter '" + interfaceProperty->Name + L"' from '" + interfaceSymbol->Name + L"'");
                }
                else if (!IsInterfaceImplementationMatching(interfaceProperty->Getter, it->second))
                {
                    diagnostics.ReportError(
                        errorToken,
                        L"Implementation of property getter '" + interfaceProperty->Name + L"' from '" + interfaceSymbol->Name + L"' on type '" + typeSymbol->Name + L"' has an incompatible signature");
                }
            }

            if (interfaceProperty->Setter != nullptr)
            {
                auto it = typeSymbol->InterfaceMethodMap.find(interfaceProperty->Setter);
                if (it == typeSymbol->InterfaceMethodMap.end() || it->second == nullptr)
                {
                    diagnostics.ReportError(
                        errorToken,
                        L"Type '" + typeSymbol->Name + L"' does not explicitly implement property setter '" + interfaceProperty->Name + L"' from '" + interfaceSymbol->Name + L"'");
                }
                else if (!IsInterfaceImplementationMatching(interfaceProperty->Setter, it->second))
                {
                    diagnostics.ReportError(
                        errorToken,
                        L"Implementation of property setter '" + interfaceProperty->Name + L"' from '" + interfaceSymbol->Name + L"' on type '" + typeSymbol->Name + L"' has an incompatible signature");
                }
            }
        }
    }

    void SemanticValidator::ValidateAllInterfaceImplementations(
        SemanticModel& semanticModel,
        DiagnosticsContext& diagnostics)
    {
        for (TypeSymbol* typeSymbol : semanticModel.Table->GetTypeSymbols())
        {
            if (typeSymbol->Interfaces.empty())
                continue;

            // Source symbols are already validated by TypeBinder with precise
            // source locations. Native-library symbols are validated once when
            // the library is loaded (AddLib). We only re-validate symbols that
            // are neither source-based nor already marked ready.
            if (typeSymbol->IsReadyForRuntime())
                continue;

            if (semanticModel.Table->LookupNode(typeSymbol).has_value())
                continue;

            SyntaxToken errorToken(TokenType::Unknown, typeSymbol->Name, TextLocation(), false);

            for (TypeSymbol* interfaceType : typeSymbol->Interfaces)
            {
                if (interfaceType == nullptr || interfaceType->Kind != SyntaxKind::InterfaceDeclaration)
                    continue;

                InterfaceSymbol* interfaceSymbol = static_cast<InterfaceSymbol*>(interfaceType);
                ValidateExplicitInterfaceImplementations(typeSymbol, interfaceSymbol, diagnostics, errorToken);
            }
        }
    }
}
