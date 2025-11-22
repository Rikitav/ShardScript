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
#include <shard/runtime/InteractiveConsole.h>

#include <shard/framework/FrameworkLoader.h>

#include <shard/ShardScript.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>
#include <clocale>
#include <csignal>
#include <cstdlib>

using namespace shard::utilities;
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
		setlocale(LC_ALL, "C");
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
		LexicalAnalyzer lexer(diagnostics);

		for (const std::wstring& file : args.FilesToCompile)
		{
			FileReader reader = FileReader(file);
			lexer.FromSourceReader(syntaxTree, reader);
		}

		SemanticModel semanticModel = SemanticModel(syntaxTree);
		if (!args.ExcludeStd)
		{
			FrameworkLoader::Load(lexer, semanticModel, diagnostics);
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
}