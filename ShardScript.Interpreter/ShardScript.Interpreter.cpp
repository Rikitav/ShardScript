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

using namespace shard::interpreter::utilities;
using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;
using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;

static void SigIntHandler(int signal)
{
	AbstractInterpreter::TerminateCallStack();
	GarbageCollector::Terminate();
	exit(SIGINT);
}

int wmain(int argc, wchar_t* argv[])
{
	try
	{
		setlocale(LC_ALL, "");
		signal(SIGINT, SigIntHandler);
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

		bool anyUnrealFiles = false;
		for (const std::wstring& file : args.FilesToCompile)
		{
			if (!PathFileExistsW(file.c_str()))
			{
				anyUnrealFiles = true;
				std::wcout << L"'" << file << L"' doesn't exists";
			}
		}

		if (anyUnrealFiles)
			return 1;

		DiagnosticsContext diagnostics;
		SyntaxTree syntaxTree;
		SemanticModel semanticModel(syntaxTree);

		if (!args.ExcludeStd)
		{
			WCHAR buffer[MAX_PATH];
			GetModuleFileNameW(NULL, buffer, sizeof(buffer) / sizeof(buffer[0]));
			std::filesystem::path current = std::filesystem::path(buffer);
			current = current.parent_path();

			for (const auto entry : std::filesystem::directory_iterator(current))
			{
				if (!entry.is_regular_file())
					continue;

				const auto filename = entry.path().filename();
				if (!filename.string().ends_with(".dll"))
					continue;

				FrameworkLoader::AddLib(entry.path().wstring());
			}

			//const std::wstring stdLibPath = L"ShardScript.Framework.dll";
			//FrameworkLoader::AddLib(stdLibPath);
			FrameworkLoader::Load(semanticModel, diagnostics);
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