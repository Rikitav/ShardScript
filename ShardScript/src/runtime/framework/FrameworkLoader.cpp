#include <shard/runtime/framework/FrameworkLoader.hpp>
#include <shard/runtime/framework/FrameworkModule.hpp>

#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>

#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/SourceParser.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>

#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <string>
#include <vector>
#include <Windows.h>
#include <stdexcept>

using namespace shard;

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
			ConstructorSymbol* symbol = static_cast<ConstructorSymbol*>(semanticModel.Table->LookupSymbol(ctor));

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

		case SyntaxKind::IndexatorDeclaration:
		{
			IndexatorDeclarationSyntax* prop = static_cast<IndexatorDeclarationSyntax*>(member);
			IndexatorSymbol* symbol = static_cast<IndexatorSymbol*>(semanticModel.Table->LookupSymbol(prop));

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
	SemanticAnalyzer semanter = SemanticAnalyzer(diagnostics);
	SourceParser parser = SourceParser(diagnostics);

	for (FrameworkModule* module : Modules)
	{
		SourceProvider* source = module->GetSource();
		
		parser.FromSourceProvider(semanticModel.Tree, *source);
		semanter.Analyze(semanticModel.Tree, semanticModel);
		delete source;

		CompilationUnitSyntax* unit = semanticModel.Tree.CompilationUnits.back();
		for (MemberDeclarationSyntax* member : unit->Members)
			BindMemberDeclaration(member, module, semanticModel, diagnostics);
	}
}
