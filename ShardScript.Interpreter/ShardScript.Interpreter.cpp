#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>
#include <exception>
#include <clocale>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <ios>
#include <vector>

#include <ShardScript.hpp>
#include <InteractiveConsole.hpp>
#include <utilities/InterpreterUtilities.hpp>
#include <utilities/Console.hpp>
#include <utilities/Diagnostics.hpp>
#include <utilities/Exceptions.hpp>

// Platform-specific headers
#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#elif defined(__linux__)
	#include <unistd.h>
	#include <limits.h>
#elif defined(__APPLE__)
	#include <mach-o/dyld.h>
	#include <climits>
#endif

using namespace shard;
namespace fs = std::filesystem;

namespace
{
	fs::path ExecutableDirectory()
	{
#if defined(_WIN32)
		WCHAR pathBuffer[MAX_PATH];
		GetModuleFileNameW(NULL, pathBuffer, static_cast<DWORD>(MAX_PATH));
		return fs::path(pathBuffer).parent_path();

#elif defined(__linux__)
		char pathBuffer[PATH_MAX];
		ssize_t len = readlink("/proc/self/exe", pathBuffer, sizeof(pathBuffer) - 1);
		if (len == -1)
			return fs::path();
		pathBuffer[len] = '\0';
		return fs::path(pathBuffer).parent_path();

#else
		return fs::path(); // Unsupported OS
#endif
	}

	fs::path WorkingDirectory()
	{
		return fs::current_path();
	}

	static CompilationUnitSyntax* GetCompilationUnit(SyntaxNode* node)
	{
		while (node != nullptr && node->Kind != SyntaxKind::CompilationUnit)
			node = node->Parent;

		return static_cast<CompilationUnitSyntax*>(node);
	}
}

static void SigIntHandler(int signal)
{
	exit(SIGINT);
}

static bool CheckFilesExisting()
{
	bool anyUnrealFiles = false;
	for (const std::wstring& file : ConsoleArguments::FilesToCompile)
	{
		if (!fs::exists(file))
		{
			anyUnrealFiles = true;
			std::wcout << L"'" << file << L"' doesn't exists";
		}
	}

	return !anyUnrealFiles;
}

static void LoadLibrariesFromDirectoryPath(CompilationContext* compiler, fs::path path)
{
    std::vector<fs::path> libraryPaths;
    for (const fs::directory_entry& entry : fs::directory_iterator(path))
    {
        if (!entry.is_regular_file())
            continue;

        const auto filename = entry.path().filename();
#if defined(_WIN32)
        if (!filename.string().ends_with(".dll"))
#else
        if (!filename.string().ends_with(".so"))
#endif
            continue;

        libraryPaths.push_back(entry.path());
    }

    compiler->AddLibraries(libraryPaths);
}

int wmain(int argc, wchar_t* argv[])
{
	CompilationContext compiler;

	try
	{
		setlocale(LC_ALL, "");
		console::EnableColors();
		signal(SIGINT, SigIntHandler);
		ShardUtilities::ParseArguments(argc, argv);

		if (ConsoleArguments::ShowHelp)
		{
			std::wcout << std::endl << L"ShardLang interpreter v" << SHARDSCRIPT_VERSION << std::endl << std::endl;

			std::wcout << L"\t> '-h', '--help'        \t Show this help screen." << std::endl;
			std::wcout << L"\t> '-i', '--interactive' \t Run REPL console" << std::endl;
			std::wcout << L"\t> '-d', '--decompiled'  \t Instead of running program, decompile it entry point and print its bytecode" << std::endl;
			std::wcout << L"\t> '--no-std'	          \t Prevents loading standard library from STD directory" << std::endl;
			return 0;
		}

		if (!CheckFilesExisting())
			return 1;

		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		compiler.SetEntryPoint = ConsoleArguments::RunProgram || ConsoleArguments::ShowDecompile;

		if (!ConsoleArguments::ExcludeStd)
		{
			fs::path currentDirectory = ExecutableDirectory() / L"system";
			LoadLibrariesFromDirectoryPath(&compiler, currentDirectory);
		}

		for (const std::wstring& file : ConsoleArguments::LibsToLoad)
		{
			compiler.AddLib(file);
		}

		fs::path workingDirectory = WorkingDirectory();
		if (!fs::exists(workingDirectory))
		{
			std::wcout << L"Could not resolve current working directory!" << std::endl;
			return 1;
		}

		for (const std::wstring& file : ConsoleArguments::FilesToCompile)
		{
			FileReader reader(file);
			LexicalAnalyzer lexer(reader);
			compiler.EnrichTree(lexer, CompilationUnitOrigin::SourceFile);
		}

		if (ConsoleArguments::UseInteractive)
		{
			compiler.SetPopExpressionStatement(false);
		}

		auto domain = compiler.Compile();
		if (diagnostics.AnyError)
			throw diagnostics_exception("Compilation ended with errors.");

		if (ConsoleArguments::UseInteractive)
		{
			InteractiveConsole repl(&compiler, domain.get());
			repl.Run();
			return 0;
		}

		if (ConsoleArguments::ShowDecompile)
		{
			SymbolTable* table = compiler.GetSemanticModel().Table.get();
			ProgramDisassembler disassembler;

			const std::vector<MethodSymbol*>& methods = table->GetMethodSymbols();
			for (const auto& method : methods)
			{
				if (method->HandleType != MethodHandleType::Body)
					continue;

				MethodDeclarationSyntax* methodNode = static_cast<MethodDeclarationSyntax*>(table->LookupNode(method).value_or(nullptr));
				if (methodNode == nullptr)
				{
					if (method->FullName.find(L"k__AsyncStateMachine_") == std::wstring::npos)
					{
						std::cout << "Failed to resolve methods node" << std::endl;
						continue;
					}
				}
				else
				{
					CompilationUnitSyntax* unit = GetCompilationUnit(methodNode);
					if (unit == nullptr)
					{
						std::wcout << L"Failed to resolve " << methodNode->IdentifierToken.Word << L" methods unit" << std::endl;
						continue;
					}

					if (unit->Origin != CompilationUnitOrigin::SourceFile)
						continue;
				}

				disassembler.Disassemble(std::wcout, method);
			}
		}

		if (ConsoleArguments::RunProgram)
		{
			VirtualMachine& virtualMachine = domain->GetVirtualMachine();
			SymbolTable* symbolTable = compiler.GetSemanticModel().Table.get();
			virtualMachine.Run();
			ConsoleHelper::Write(L"\n");

			ObjectInstance* unhandledException = virtualMachine.GetUnhandledException();
			if (unhandledException != nullptr)
			{
				exceptions::PrintUnhandled(
					std::wcerr,
					unhandledException,
					virtualMachine.GetUnhandledExceptionMessage(),
					virtualMachine.GetUnhandledExceptionStackTrace(),
					symbolTable);

				return 1;
			}
		}
	}
	catch (const diagnostics_exception& err)
	{
		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		if (diagnostics.AnyError)
		{
			diagnostics::Print(std::wcout, diagnostics);
			return 1;
		}
	}
	catch (const std::runtime_error& err)
	{
		exceptions::PrintCritical(std::wcerr, err.what());
	}
	catch (const std::exception& err)
	{
		exceptions::PrintCritical(std::wcerr, err.what());
		return 3;
	}
	catch (...)
	{
		exceptions::PrintCritical(std::wcerr, "unknown exception");
		return 3;
	}

	return 0;
}

#if !defined(_WIN32)
int main(int argc, char* argv[])
{
	std::setlocale(LC_ALL, "");

	std::vector<std::wstring> wideArgs;
	wideArgs.reserve(argc);
	for (int i = 0; i < argc; ++i)
		wideArgs.push_back(fs::path(argv[i]).wstring());

	std::vector<wchar_t*> wideArgv;
	wideArgv.reserve(argc + 1);
	for (auto& arg : wideArgs)
		wideArgv.push_back(const_cast<wchar_t*>(arg.data()));

	wideArgv.push_back(nullptr);
	return wmain(argc, wideArgv.data());
}
#endif
