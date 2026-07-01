#include <shard/ShardScriptAPI.hpp>
#include <shard/ShardScriptLIB.hpp>
#include <shard/ApplicationDomain.hpp>
#include <shard/CompilationContext.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/semantic/SemanticAnalyzer.hpp>
#include <shard/compilation/LayoutGenerator.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticValidator.hpp>

#include <shard/analysis/DiagnosticsContext.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/SourceProvider.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>

#include <shard/compilation/AbstractEmiter.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <filesystem>

// Platform-specific headers
#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#elif defined(__linux__)
	#include <unistd.h>
	#include <limits.h>
	#include <dlfcn.h>
#elif defined(__APPLE__)
	#include <mach-o/dyld.h>
	#include <climits>
#endif

using namespace shard;

typedef void (*GetMetadataFunction)(ShardLibMetadata& lib);
typedef void (*EntryPointFunction)(CompilationContext& context);

static LibraryHandle LoadLibraryHandle(const std::filesystem::path& path)
{
#ifdef _WIN32
	HMODULE hModule = LoadLibraryW(path.c_str());
	return hModule;
#else
	void* handle = dlopen(path.c_str(), RTLD_NOW);
	if (!handle)
		throw std::runtime_error(std::string("Failed to load library: ") + dlerror());
	return handle;
#endif
}

static void FreeLibraryHandle(LibraryHandle handle)
{
#ifdef _WIN32
	FreeLibrary((HMODULE)handle);
#else
	dlclose(handle);
#endif
}

static void* GetLibFunction(LibraryHandle handle, const char* procName)
{
#ifdef _WIN32
	return (void*)GetProcAddress((HMODULE)handle, procName);
#else
	void* func = dlsym(handle, procName);
	if (!func)
		throw std::runtime_error(std::string("Failed to resolve symbol: ") + dlerror());
	return func;
#endif
}

static std::string WStringToUtf8(const std::wstring& wstr)
{
#ifdef _WIN32
	if (wstr.empty())
		return {};
	
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
	return str;
#else
	// naive fallback
	return std::string(wstr.begin(), wstr.end());
#endif
}

static std::wstring GetLastErrorAsString()
{
#ifdef _WIN32
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0)
		return std::wstring();

	LPWSTR messageBuffer = nullptr;
	std::size_t size = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

	std::wstring message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
#else
	const char* err = dlerror();
	if (!err)
		return std::wstring();
	std::string errStr(err);
	return std::wstring(errStr.begin(), errStr.end());
#endif
}

/*
static void LinkExternSymbols(MemberDeclarationSyntax* member, LibraryHandle handle, SemanticModel& model, DiagnosticsContext& diagnostics)
{
	switch (member->Kind)
	{
		case SyntaxKind::ClassDeclaration:
		{
			ClassDeclarationSyntax* classDecl = static_cast<ClassDeclarationSyntax*>(member);
			for (const auto& child : classDecl->Members)
				LinkExternSymbols(child.get(), handle, model, diagnostics);
			
			break;
		}

		case SyntaxKind::StructDeclaration:
		{
			StructDeclarationSyntax* structDecl = static_cast<StructDeclarationSyntax*>(member);
			for (const auto& child : structDecl->Members)
				LinkExternSymbols(child.get(), handle, model, diagnostics);

			break;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodDeclarationSyntax* method = static_cast<MethodDeclarationSyntax*>(member);
			MethodSymbol* symbol = static_cast<MethodSymbol*>(model.Table->LookupSymbol(method).value_or(nullptr));

			if (symbol == nullptr || !symbol->IsExtern || symbol->LinkSymbol.empty())
				break;

			std::string procName = WStringToUtf8(symbol->LinkSymbol);
			void* proc = GetLibFunction(handle, procName.c_str());

			if (proc != nullptr)
			{
				symbol->FunctionPointer = reinterpret_cast<MethodSymbolDelegate>(proc);
			}
			else
			{
				diagnostics.ReportError(method->IdentifierToken, L"Could not link extern method '" + symbol->FullName + L"' to symbol '" + symbol->LinkSymbol + L"'");
			}

			break;
		}

		case SyntaxKind::ConstructorDeclaration:
		{
			ConstructorDeclarationSyntax* ctor = static_cast<ConstructorDeclarationSyntax*>(member);
			ConstructorSymbol* symbol = static_cast<ConstructorSymbol*>(model.Table->LookupSymbol(ctor).value_or(nullptr));
			
			if (symbol == nullptr || !symbol->IsExtern || symbol->LinkSymbol.empty())
				break;

			std::string procName = WStringToUtf8(symbol->LinkSymbol);
			void* proc = GetLibFunction(handle, procName.c_str());
			
			if (proc != nullptr)
			{
				symbol->FunctionPointer = reinterpret_cast<MethodSymbolDelegate>(proc);
			}
			else
			{
				diagnostics.ReportError(ctor->IdentifierToken, L"Could not link extern constructor '" + symbol->FullName + L"' to symbol '" + symbol->LinkSymbol + L"'");
			}
			
			break;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertyDeclarationSyntax* prop = static_cast<PropertyDeclarationSyntax*>(member);
			PropertySymbol* propSymbol = static_cast<PropertySymbol*>(model.Table->LookupSymbol(prop).value_or(nullptr));
			
			if (propSymbol == nullptr)
				break;

			if (propSymbol->Getter != nullptr && propSymbol->Getter->IsExtern && !propSymbol->Getter->LinkSymbol.empty())
			{
				std::string procName = WStringToUtf8(propSymbol->Getter->LinkSymbol);
				void* proc = GetLibFunction(handle, procName.c_str());
				
				if (proc != nullptr)
				{
					propSymbol->Getter->FunctionPointer = reinterpret_cast<MethodSymbolDelegate>(proc);
				}
				else
				{
					diagnostics.ReportError(prop->IdentifierToken, L"Could not link extern getter '" + propSymbol->Getter->FullName + L"' to symbol '" + propSymbol->Getter->LinkSymbol + L"'");
				}
			}

			if (propSymbol->Setter != nullptr && propSymbol->Setter->IsExtern && !propSymbol->Setter->LinkSymbol.empty())
			{
				std::string procName = WStringToUtf8(propSymbol->Setter->LinkSymbol);
				void* proc = GetLibFunction(handle, procName.c_str());

				if (proc != nullptr)
				{
					propSymbol->Setter->FunctionPointer = reinterpret_cast<MethodSymbolDelegate>(proc);
				}
				else
				{
					diagnostics.ReportError(prop->IdentifierToken, L"Could not link extern setter '" + propSymbol->Setter->FullName + L"' to symbol '" + propSymbol->Setter->LinkSymbol + L"'");
				}
			}

			break;
		}

		case SyntaxKind::IndexatorDeclaration:
		{
			IndexatorDeclarationSyntax* indexer = static_cast<IndexatorDeclarationSyntax*>(member);
			IndexatorSymbol* idxSymbol = static_cast<IndexatorSymbol*>(model.Table->LookupSymbol(indexer).value_or(nullptr));

			if (idxSymbol == nullptr)
				break;

			if (idxSymbol->Getter != nullptr && idxSymbol->Getter->IsExtern && !idxSymbol->Getter->LinkSymbol.empty())
			{
				std::string procName = WStringToUtf8(idxSymbol->Getter->LinkSymbol);
				void* proc = GetLibFunction(handle, procName.c_str());

				if (proc != nullptr)
				{
					idxSymbol->Getter->FunctionPointer = reinterpret_cast<MethodSymbolDelegate>(proc);
				}
				else
				{
					diagnostics.ReportError(indexer->IdentifierToken, L"Could not link extern indexer getter '" + idxSymbol->Getter->FullName + L"' to symbol '" + idxSymbol->Getter->LinkSymbol + L"'");
				}
			}

			if (idxSymbol->Setter != nullptr && idxSymbol->Setter->IsExtern && !idxSymbol->Setter->LinkSymbol.empty())
			{
				std::string procName = WStringToUtf8(idxSymbol->Setter->LinkSymbol);
				void* proc = GetLibFunction(handle, procName.c_str());

				if (proc != nullptr)
				{
					idxSymbol->Setter->FunctionPointer = reinterpret_cast<MethodSymbolDelegate>(proc);
				}
				else
				{
					diagnostics.ReportError(indexer->IdentifierToken, L"Could not link extern indexer setter '" + idxSymbol->Setter->FullName + L"' to symbol '" + idxSymbol->Setter->LinkSymbol + L"'");
				}
			}

			break;
		}
	}
}
*/

CompilationContext::CompilationContext()
	: Tree(), Model(Tree), Diagnostics(), Parser(Diagnostics), Semanter(Diagnostics), Layouter(Diagnostics)
{

}

CompilationContext::~CompilationContext()
{
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
	if (hLib == nullptr)
		throw std::runtime_error("could not load library: " + WStringToUtf8(path.wstring()));

	AddLib(hLib);
}

void CompilationContext::AddLib(const LibraryHandle& handle)
{
	if (handle == nullptr)
		throw std::runtime_error("library handle is null");

	LibHandles.push_back(handle);
	EntryPointFunction entryPoint = reinterpret_cast<EntryPointFunction>(GetLibFunction(handle, "ShardLib_EntryPoint"));

	if (entryPoint == nullptr)
	{
#ifdef _WIN32
		char dllPath[MAX_PATH];
		if (GetModuleFileNameA(static_cast<HMODULE>(handle), dllPath, MAX_PATH) > 0)
		{
			throw std::runtime_error("library \"" + std::string(dllPath) + "\" does not export ShardLib_EntryPoint");
		}
#endif

		throw std::runtime_error("library \"<UNKNOWN>\" does not export ShardLib_EntryPoint");
	}

	try
	{
		entryPoint(*this);

		// Run cross-symbol checks (interface implementations, etc.) for
		// symbols produced by the library entry point before marking them ready.
		SemanticValidator::ValidateAllInterfaceImplementations(Model, Diagnostics);

		// Native-library symbols are fully constructed by the entry point and do
		// not go through the source-based analyzer passes.
		Model.Table->MarkJustCreatedSymbolsReady();

		if (ReAnalyze)
			Semanter.Analyze(Tree, Model);

		/*
		for (const auto& unit : PendingSources)
		{
			for (const auto& member : unit->Members)
				LinkExternSymbols(member.get(), handle, Model, Diagnostics);
		}
		*/

		PendingSources.clear();
		ReAnalyze = false;
	}
	catch (const std::exception&)
	{
		throw;
	}
}

void CompilationContext::ProvideSource(SourceTextProvider* source)
{
	try
	{
		std::size_t beforeEnrich = Tree.CompilationUnits.size();
		LexicalAnalyzer lexer(source, false);
		EnrichTree(lexer, CompilationUnitOrigin::DynamicLib);

		std::size_t afterEnrich = Tree.CompilationUnits.size();
		for (std::size_t i = beforeEnrich; i < afterEnrich; i++)
		{
			CompilationUnitSyntax* unit = Tree.CompilationUnits[i].get();
			PendingSources.push_back(unit);
		}

		ReAnalyze = true;
	}
	catch (...)
	{

	}
}

void CompilationContext::EnrichTree(SourceProvider& sourceProvider, CompilationUnitOrigin origin)
{
	Parser.FromSourceProvider(Tree, sourceProvider);
	Tree.CompilationUnits.back()->Origin = origin;
	ReAnalyze = true;
}

void CompilationContext::AnalyzeTree()
{
	if (!ReAnalyze)
		return;

	Semanter.Analyze(Tree, Model);
	ReAnalyze = false;
}

std::unique_ptr<ApplicationDomain> CompilationContext::Compile()
{
	if (ReAnalyze)
		AnalyzeTree();

	Layouter.Generate(Model);
	if (Diagnostics.AnyError)
		throw diagnostics_exception("Model layout generation ended with errors.");

	Model.Table->MarkAllSymbolsReady();

	auto program = std::make_unique<ProgramVirtualImage>();
	AbstractEmiter emiter(*program, Model, Diagnostics);
	if (!PopExpressionStatement)
		emiter.SetPopExpressionStatement(false);

	emiter.VisitSyntaxTree(Tree);
	if (SetEntryPoint)
		emiter.SetEntryPoint();

	if (Diagnostics.AnyError)
		throw diagnostics_exception("Code Compilation ended with errors.");

	return std::make_unique<ApplicationDomain>(std::move(program));
}
