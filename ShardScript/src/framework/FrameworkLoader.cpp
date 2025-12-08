#include <shard/framework/FrameworkLoader.h>
#include <shard/framework/FrameworkModule.h>

#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SemanticAnalyzer.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/reading/StringStreamReader.h>

#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>

#include <string>
#include <vector>
#include <Windows.h>
#include <stdexcept>

using namespace shard::syntax::nodes;
using namespace shard::parsing::analysis;
using namespace shard::parsing::lexical;

using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing;
using namespace shard::parsing::semantic;

std::vector<HMODULE> FrameworkLoader::LoadedLibraries;
std::vector<FrameworkModule*> FrameworkLoader::Modules;

static std::string GetLastErrorAsString()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string();

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
}

static void BindMemberDeclaration(MemberDeclarationSyntax* member, FrameworkModule* module, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	switch (member->Kind)
	{
		case SyntaxKind::NamespaceDeclaration:
		{
			NamespaceDeclarationSyntax* namespaceDecl = static_cast<NamespaceDeclarationSyntax*>(member);
			for (MemberDeclarationSyntax* member : namespaceDecl->Members)
				BindMemberDeclaration(member, module, semanticModel, diagnostics);

			break;
		}

		case SyntaxKind::ClassDeclaration:
		{
			ClassDeclarationSyntax* classDecl = static_cast<ClassDeclarationSyntax*>(member);
			for (MemberDeclarationSyntax* member : classDecl->Members)
				BindMemberDeclaration(member, module, semanticModel, diagnostics);

			break;
		}

		case SyntaxKind::StructDeclaration:
		{
			StructDeclarationSyntax* structDecl = static_cast<StructDeclarationSyntax*>(member);
			for (MemberDeclarationSyntax* member : structDecl->Members)
				BindMemberDeclaration(member, module, semanticModel, diagnostics);

			break;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodDeclarationSyntax* method = static_cast<MethodDeclarationSyntax*>(member);
			MethodSymbol* symbol = static_cast<MethodSymbol*>(semanticModel.Table->LookupSymbol(method));

			if (!symbol->IsExtern)
				break;

			if (!module->BindMethod(symbol))
				diagnostics.ReportError(method->IdentifierToken, L"Unexpected method \'" + symbol->FullName + L"\'");

			break;
		}

		case SyntaxKind::ConstructorDeclaration:
		{
			ConstructorDeclarationSyntax* ctor = static_cast<ConstructorDeclarationSyntax*>(member);
			MethodSymbol* symbol = static_cast<MethodSymbol*>(semanticModel.Table->LookupSymbol(ctor));

			if (!symbol->IsExtern)
				break;

			if (!module->BindConstructor(symbol))
				diagnostics.ReportError(ctor->IdentifierToken, L"Unexpected constructor \'" + symbol->FullName + L"\'");

			break;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertyDeclarationSyntax* prop = static_cast<PropertyDeclarationSyntax*>(member);
			PropertySymbol* symbol = static_cast<PropertySymbol*>(semanticModel.Table->LookupSymbol(prop));

			if (symbol->Getter != nullptr)
			{
				AccessorSymbol* getter = symbol->Getter;
				if (!getter->IsExtern)
					break;

				if (!module->BindAccessor(getter))
					diagnostics.ReportError(prop->IdentifierToken, L"Unexpected getter accessor \'" + symbol->FullName + L"\'");
			}

			if (symbol->Setter != nullptr)
			{
				AccessorSymbol* setter = symbol->Setter;
				if (!setter->IsExtern)
					break;

				if (!module->BindAccessor(setter))
					diagnostics.ReportError(prop->IdentifierToken, L"Unexpected setter accessor \'" + symbol->FullName + L"\'");
			}

			break;
		}
	}
}

void FrameworkLoader::AddLib(const std::wstring& path)
{
	HMODULE hModule = LoadLibraryW(path.c_str());
	if (hModule == nullptr)
	{
		std::string errorMessage = GetLastErrorAsString();
		throw std::runtime_error(errorMessage);
	}

	LoadedLibraries.push_back(hModule);
}

void FrameworkLoader::AddModule(FrameworkModule* pModule)
{
	Modules.push_back(pModule);
}

void FrameworkLoader::Destroy()
{
	for (FrameworkModule* module : Modules)
		delete module;

	for (HMODULE lib : LoadedLibraries)
		FreeLibrary(lib);
}

void FrameworkLoader::Load(SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	LexicalAnalyzer lexer = LexicalAnalyzer(diagnostics);
	SemanticAnalyzer semanter = SemanticAnalyzer(diagnostics);

	for (FrameworkModule* module : Modules)
	{
		SyntaxTree innerTree;
		StringStreamReader reader = StringStreamReader(module->GetSourceCode());
		lexer.FromSourceReader(innerTree, reader);
		semanter.Analyze(innerTree, semanticModel);

		CompilationUnitSyntax* unit = innerTree.CompilationUnits.back();
		for (MemberDeclarationSyntax* member : unit->Members)
			BindMemberDeclaration(member, module, semanticModel, diagnostics);
	}
}
