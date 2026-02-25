#include <shard/CompilationContext.hpp>
#include <shard/FrameworkModule.hpp>
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/LayoutGenerator.hpp>

#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>

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

#include <shard/compilation/AbstractEmiter.hpp>

#include <string>
#include <vector>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h> // TODO: remove
#endif

using namespace shard;

static LibraryHandle LoadLibraryHandle(const std::filesystem::path& path)
{
#ifdef _WIN32
	HMODULE hModule = LoadLibraryW(path.c_str());
	return hModule;
#else
	throw std::runtime_error("Loading libraries is not supported on this platform");
#endif
}

static void FreeLibraryHandle(LibraryHandle handle)
{
#ifdef _WIN32
	FreeLibrary(handle);
#else
	throw std::runtime_error("Loading libraries is not supported on this platform");
#endif
}

static std::wstring GetLastErrorAsString()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::wstring();

	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

	std::wstring message(messageBuffer, size);
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

CompilationContext::CompilationContext()
	: Tree(), Model(Tree), Diagnostics(), Parser(Diagnostics), Semanter(Diagnostics), Layouter(Diagnostics)
{

}

CompilationContext::~CompilationContext()
{
	for (FrameworkModule* module : LibModules)
		delete module;

	for (LibraryHandle hLib : LibHandles)
		FreeLibraryHandle(hLib);
}

SyntaxTree& CompilationContext::GetSyntaxTree()
{
	return Tree;
}

SemanticModel& CompilationContext::GetSemanticModel()
{
	return Model;
}

DiagnosticsContext& CompilationContext::GetDiagnosticsContext()
{
	return Diagnostics;
}

SourceParser& CompilationContext::GetParser()
{
	return Parser;
}

SemanticAnalyzer& CompilationContext::GetSemanticAnalyzer()
{
	return Semanter;
}

LayoutGenerator& CompilationContext::GetLayoutGenerator()
{
	return Layouter;
}

void CompilationContext::AddLib(const std::filesystem::path& path)
{
	LibraryHandle hLib = LoadLibraryHandle(path);
	AddLib(hLib);
}

void CompilationContext::AddLib(const LibraryHandle& handle)
{
	if (handle == nullptr)
	{
		/*
		std::wstring errorMessage = GetLastErrorAsString();
		throw std::runtime_error(errorMessage);
		*/
		return;
	}

	LibHandles.push_back(handle);

	// TODO: add modules reading and handling
	
	/*
	SourceProvider* source = module->GetSource();
	EnrichTree(source);
	delete source;

	CompilationUnitSyntax* unit = semanticModel.Tree.CompilationUnits.back();
	for (MemberDeclarationSyntax* member : unit->Members)
		BindMemberDeclaration(member, module, semanticModel, diagnostics);
	*/

	ReAnalyze = true;
}

void CompilationContext::EnrichTree(SourceProvider& sourceProvider)
{
	Parser.FromSourceProvider(Tree, sourceProvider);
	ReAnalyze = true;
}

void CompilationContext::AnalyzeTree()
{
	if (!ReAnalyze)
		return;

	Semanter.Analyze(Tree, Model);
	ReAnalyze = false;
}

ApplicationDomain* CompilationContext::Compile()
{
	if (ReAnalyze)
		AnalyzeTree();

	Layouter.Generate(Model);
	if (Diagnostics.AnyError)
		throw diagnostics_exception("Model layout generation ended with errors.");

	ProgramVirtualImage* program = new ProgramVirtualImage();
	AbstractEmiter emiter(*program, Model, Diagnostics);
	emiter.VisitSyntaxTree(Tree);

	if (SetEntryPoint)
		emiter.SetEntryPoint();

	if (Diagnostics.AnyError)
		throw diagnostics_exception("Code Compilation ended with errors.");

	ApplicationDomain* domain = new ApplicationDomain(program);
	return domain;
}
