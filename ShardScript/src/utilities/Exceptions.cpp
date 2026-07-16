#include <utilities/Exceptions.hpp>

#include <sstream>
#include <string>
#include <unordered_map>

#include <utilities/Console.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/analysis/TextLocation.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>

namespace shard::exceptions
{
    void PrintUnhandled(std::wostream& out, ObjectInstance* exception,
                        const std::wstring& message, const std::wstring& stackTrace,
                        SymbolTable* table)
    {
        std::wstring exceptionType = (exception != nullptr && exception->getInfo() != nullptr)
            ? exception->getInfo()->Name
            : L"UnknownException";

        out << std::endl;
        out << console::FG_RED << console::ST_BOLD << L"Unhandled exception." << console::ST_RESET;
        out << L" " << console::FG_CYAN << exceptionType << console::ST_RESET;
        out << console::FG_RED << L":" << console::ST_RESET;
        out << L" " << console::FG_RED << message << console::ST_RESET;
        out << std::endl;

        out << std::endl;
        out << console::FG_WHITE << L"Stack trace:" << console::ST_RESET << std::endl;

        std::unordered_map<std::wstring, MethodSymbol*> methodMap;
        if (table != nullptr)
        {
            for (MethodSymbol* method : table->GetMethodSymbols())
            {
                if (method != nullptr)
                    methodMap[method->FullName] = method;
            }
        }

        std::wistringstream traceReader(stackTrace);
        std::wstring frameName;
        bool anyFrame = false;

        while (std::getline(traceReader, frameName))
        {
            if (frameName.empty())
                continue;

            anyFrame = true;

            out << std::endl;
            out << console::FG_LIGHT_GRAY << L"   at " << console::ST_RESET
                << console::FG_CYAN << frameName << console::ST_RESET;

            MethodSymbol* method = nullptr;
            auto it = methodMap.find(frameName);
            if (it != methodMap.end())
                method = it->second;

            if (method != nullptr && table != nullptr)
            {
                MethodDeclarationSyntax* methodNode = static_cast<MethodDeclarationSyntax*>(
                    table->LookupNode(method).value_or(nullptr));

                if (methodNode != nullptr)
                {
                    const TextLocation& loc = methodNode->IdentifierToken.Location;
                    if (!loc.FileName.empty())
                    {
                        out << console::FG_LIGHT_GRAY << L" in " << console::ST_RESET
                            << console::FG_BLUE << loc.FileName << console::ST_RESET
                            << console::FG_LIGHT_GRAY << L":line " << console::ST_RESET
                            << console::FG_WHITE << loc.Line << console::ST_RESET;
                    }
                }
            }
        }

        if (!anyFrame)
        {
            out << console::FG_LIGHT_GRAY << L"   <no stack trace available>" << console::ST_RESET << std::endl;
        }

        out << std::endl;
    }

    void PrintCritical(std::wostream& out, const char* message)
    {
        out << std::endl;
        out << console::FG_RED << console::ST_BOLD << L"Critical error:" << console::ST_RESET
            << L" " << console::FG_WHITE << message << console::ST_RESET << std::endl;
        out << std::endl;
    }
}
