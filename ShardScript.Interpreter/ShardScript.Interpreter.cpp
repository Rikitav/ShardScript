#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/LayoutGenerator.h>
#include <shard/parsing/reading/FileReader.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/SyntaxToken.h>

#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/AbstractInterpreter.h>

#include <shard/framework/FrameworkLoader.h>

#include <Shlwapi.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>
#include <clocale>
#include <csignal>
#include <cstdlib>
#include <filesystem>

#pragma comment(lib, "shlwapi.lib")

#include "InteractiveConsole.h"
#include "utilities/InterpreterUtilities.h"

using namespace shard;
namespace fs = std::filesystem;

const fs::path stdlibFilename = "ShardScript.Framework.dll";

static void SigIntHandler(int signal)
{
	AbstractInterpreter::TerminateCallStack();
	GarbageCollector::Terminate();
	exit(SIGINT);
}

LONG WINAPI UnhandledExceptionFilterFunction(PEXCEPTION_POINTERS exception)
{
	return 0;
}

bool CheckFilesExisting(ConsoleArguments& args)
{
	bool anyUnrealFiles = false;
	for (const std::wstring& file : args.FilesToCompile)
	{
		if (!fs::exists(file))
		{
			anyUnrealFiles = true;
			std::wcout << L"'" << file << L"' doesn't exists";
		}
	}

	return !anyUnrealFiles;
}

void LoadLibrariesFromDirectoryPath(fs::path path)
{
	for (const fs::directory_entry& entry : std::filesystem::directory_iterator(path))
	{
		if (!entry.is_regular_file())
			continue;

		const auto filename = entry.path().filename();
		if (!filename.string().ends_with(".dll"))
			continue;

		FrameworkLoader::AddLib(entry.path().wstring());
	}
}

void Execute()
{

}

fs::path GetCurrentDirectoryPath()
{
	WCHAR pathBuffer[MAX_PATH];
	GetModuleFileNameW(NULL, pathBuffer, MAX_PATH);
	return fs::path(pathBuffer).parent_path();
}

fs::path GetWorkingDirectoryPath()
{
	WCHAR pathBuffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, pathBuffer);
	return fs::path(pathBuffer);
}

int wmain(int argc, wchar_t* argv[])
{
	try
	{
		setlocale(LC_ALL, "");
		signal(SIGINT, SigIntHandler);
		SetUnhandledExceptionFilter(UnhandledExceptionFilterFunction);

		ConsoleArguments args = ShardUtilities::ParseArguments(argc, argv);

		if (args.ShowHelp)
		{
			std::wstring version = ShardUtilities::GetFileVersion();
			std::wcout << "ShardLang interpreter v" << version;
			return 0;
		}

		if (args.AssociateScriptFile)
		{
			ShardUtilities::AssociateRegistry();
			std::wcout << "File association successsfuly installed" << std::endl;
			return 0;
		}

		if (!CheckFilesExisting(args))
			return 1;

		DiagnosticsContext diagnostics;
		SyntaxTree syntaxTree;
		SemanticModel semanticModel(syntaxTree);

		fs::path currentDirectory = GetCurrentDirectoryPath();
		if (!args.ExcludeStd)
		{
			fs::path stdlibFilepath = currentDirectory / stdlibFilename;
			if (!fs::exists(stdlibFilepath))
			{
				std::wcout << "'" << stdlibFilename << "' not found! use '--no-std' flag to disable standart library loading requirement" << std::endl;
				return 1;
			}

			FrameworkLoader::AddLib(stdlibFilepath.wstring());
		}

		FrameworkLoader::Load(semanticModel, diagnostics);

		fs::path workingDirectory = GetWorkingDirectoryPath();
		if (!fs::exists(workingDirectory))
		{
			std::wcout << "Could not resolve current working directory!" << std::endl;
			return 1;
		}
		else
		{

		}


		LexicalAnalyzer lexer(diagnostics);
		for (const std::wstring& file : args.FilesToCompile)
		{
			FileReader reader = FileReader(file);
			lexer.FromSourceReader(syntaxTree, reader);
		}

		SemanticAnalyzer semanticAnalyzer(diagnostics);
		semanticAnalyzer.Analyze(syntaxTree, semanticModel);

		if (!args.UseInteractive)
		{
			if (semanticModel.Table->EntryPointCandidates.empty())
			{
				diagnostics.ReportError(SyntaxToken(), L"Entry point for script not found");
			}

			if (semanticModel.Table->EntryPointCandidates.size() > 1)
			{
				for (MethodSymbol* entry : semanticModel.Table->EntryPointCandidates)
				{
					MethodDeclarationSyntax* decl = static_cast<MethodDeclarationSyntax*>(semanticModel.Table->GetSyntaxNode(entry));
					diagnostics.ReportError(decl->IdentifierToken, L"Script has multiple entry points");
				}
			}
		}

		if (diagnostics.AnyError)
		{
			std::wcout << L"=== Diagnostics output ===" << std::endl;
			diagnostics.WriteDiagnostics(std::wcout);
			return 1;
		}

		LayoutGenerator layoutGenerator(diagnostics);
		layoutGenerator.Generate(semanticModel);

		if (args.UseInteractive)
		{
			InteractiveConsole::Run(syntaxTree, semanticModel, diagnostics);
			return 0;
		}
		else
		{
			try
			{
				AbstractInterpreter::Execute(syntaxTree, semanticModel);
				return 0;
			}
			catch (const std::runtime_error& err)
			{
				std::cout << err.what() << std::endl;
				return 1;
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		std::cout << "CRITICAL ERROR : " << err.what() << std::endl;
	}

	GarbageCollector::Terminate();
	FrameworkLoader::Destroy();
}