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

#include <ShardScript.hpp>
#include <InteractiveConsole.hpp>
#include <utilities/InterpreterUtilities.hpp>

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

const fs::path stdlibFilename = "ShardScript.Framework.dll";

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
	for (const fs::directory_entry& entry : fs::directory_iterator(path))
	{
		if (!entry.is_regular_file())
			continue;

		const auto filename = entry.path().filename();
		if (!filename.string().ends_with(".dll"))
			continue;

		try
		{
			compiler->AddLib(entry.path().wstring());
		}
		catch (std::runtime_error& err)
		{
			std::cout << err.what();
		}
	}
}

static fs::path GetCurrentDirectoryPath()
{
#if defined(_WIN32)
	// Windows implementation
	WCHAR pathBuffer[MAX_PATH];
	DWORD size = GetModuleFileNameW(NULL, pathBuffer, static_cast<DWORD>(MAX_PATH));
	return fs::path(pathBuffer).parent_path();

#elif defined(__linux__)
	// Linux Implementation
	WCHAR pathBuffer[MAX_PATH];
	while (true)
	{
		ssize_t len = readlink("/proc/self/exe", buffer, MAX_PATH);
		return std::filesystem::path(buffer.data()).parent_path();
	}

#else
	return ""; // Unsupported OS
#endif
}

static fs::path GetWorkingDirectoryPath()
{
	return fs::current_path();
}

static CompilationUnitSyntax* GetCompilationUnit(SyntaxNode* node)
{
	while (node != nullptr && node->Kind != SyntaxKind::CompilationUnit)
		node = node->Parent;

	return static_cast<CompilationUnitSyntax*>(node);
}

int wmain(int argc, wchar_t* argv[])
{
	CompilationContext compiler;

	try
	{
		setlocale(LC_ALL, "");
		signal(SIGINT, SigIntHandler);
		ShardUtilities::ParseArguments(argc, argv);

		if (ConsoleArguments::ShowHelp)
		{
			std::wcout << std::endl << L"ShardLang interpreter v0.2.0" << std::endl << std::endl;

			std::wcout << L"\t> '-h', '--help'        \t Show this help screen." << std::endl;
			std::wcout << L"\t> '-i', '--interactive' \t Run REPL console" << std::endl;
			std::wcout << L"\t> '-d', '--decompiled'  \t Instead of running program, decompile it entry point and print its bytecode" << std::endl;
			std::wcout << L"\t> '--no-std'	          \t Prevents loading standard library from " << stdlibFilename << std::endl;
			return 0;
		}

		if (!CheckFilesExisting())
			return 1;

		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		compiler.SetEntryPoint = ConsoleArguments::RunProgram || ConsoleArguments::ShowDecompile;

		if (!ConsoleArguments::ExcludeStd)
		{
			fs::path currentDirectory = GetCurrentDirectoryPath() / L"framework";
			LoadLibrariesFromDirectoryPath(&compiler, currentDirectory);

			/*
			fs::path stdlibFilepath = currentDirectory / stdlibFilename;
			if (!fs::exists(stdlibFilepath))
			{
				std::wcout << L"'" << stdlibFilename << L"' not found! use '--no-std' flag to disable standard library loading requirement" << std::endl;
				return 1;
			}

			compiler.AddLib(stdlibFilepath.wstring());
			*/
		}

		fs::path workingDirectory = GetWorkingDirectoryPath();
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
					std::cout << "Failed to resolve methods node" << std::endl;
					continue;
				}

				CompilationUnitSyntax* unit = GetCompilationUnit(methodNode);
				if (unit == nullptr)
				{
					std::wcout << L"Failed to resolve " << methodNode->IdentifierToken.Word << L" methods unit" << std::endl;
					continue;
				}

				if (unit->Origin != CompilationUnitOrigin::SourceFile)
					continue;

				disassembler.Disassemble(std::wcout, method);
			}
		}

		if (ConsoleArguments::RunProgram)
		{
			VirtualMachine& virtualMachine = domain->GetVirtualMachine();
			virtualMachine.Run();

			ObjectInstance* unhandledException = virtualMachine.GetUnhandledException();
			if (unhandledException != nullptr)
			{
				std::wcerr << L"Unhandled exception";
				const std::wstring& message = virtualMachine.GetUnhandledExceptionMessage();
				if (!message.empty())
					std::wcerr << L": " << message;

				std::wcerr << L"\nStack trace:\n" << virtualMachine.GetUnhandledExceptionStackTrace() << std::endl;
				return 1;
			}
		}
	}
	catch (const diagnostics_exception& err)
	{
		DiagnosticsContext& diagnostics = compiler.GetDiagnosticsContext();
		if (diagnostics.AnyError)
		{
			std::wcout << L"=== Diagnostics output ===" << std::endl;
			diagnostics.WriteDiagnostics(std::wcout);
			return 1;
		}
	}
	catch (const std::runtime_error& err)
	{
		std::cout << "CRITICAL ERROR : " << err.what() << std::endl;
	}

	return 0;
}