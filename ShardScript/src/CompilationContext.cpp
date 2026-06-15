#include <shard/ShardScriptAPI.hpp>
#include <shard/ShardScriptLIB.hpp>
#include <shard/ApplicationDomain.hpp>
#include <shard/CompilationContext.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/LayoutGenerator.hpp>

#include <shard/parsing/semantic/SemanticModel.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/parsing/analysis/DiagnosticsContext.hpp>
#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/lexical/SourceProvider.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>

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
#include <shard/compilation/ProgramVirtualImage.hpp>

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <filesystem>

#ifdef _WIN32
#include <windows.h> 
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
	throw std::runtime_error("Loading libraries is not supported on this platform");
#endif
}

static void FreeLibraryHandle(LibraryHandle handle)
{
#ifdef _WIN32
	FreeLibrary((HMODULE)handle);
#else
	throw std::runtime_error("Loading libraries is not supported on this platform");
#endif
}

static void* GetLibFunction(LibraryHandle handle, const char* procName)
{
#ifdef _WIN32
	return (void*)GetProcAddress((HMODULE)handle, procName);
#else
	throw std::runtime_error("Loading libraries is not supported on this platform");
#endif
}

static std::string WStringToUtf8(const std::wstring& wstr)
{
#ifdef _WIN32
	if (wstr.empty()) return {};
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
	throw std::runtime_error("Loading libraries is not supported on this platform");
#endif
}

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
		throw std::runtime_error("library does not export ShardLib_EntryPoint");

	try
	{
		entryPoint(*this);
		if (ReAnalyze)
			Semanter.Analyze(Tree, Model);

		for (const auto& unit : PendingSources)
		{
			for (const auto& member : unit->Members)
				LinkExternSymbols(member.get(), handle, Model, Diagnostics);
		}

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
