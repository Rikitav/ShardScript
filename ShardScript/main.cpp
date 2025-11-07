#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/LayoutGenerator.h>
#include <shard/parsing/reading/FileReader.h>

#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/AbstractInterpreter.h>
#include <shard/runtime/InteractiveConsole.h>

#include <shard/framework/FrameworkLoader.h>

#include "src/utilities/ArgumentsParser.cpp"
//#include "src/utilities/ExecutableVersion.cpp"

//#include <Windows.h>
//#include <Shlwapi.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>
#include <clocale>

using namespace std;
using namespace shard::utilities;
using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;

int wmain(int argc, wchar_t* argv[])
{
	try
	{
		setlocale(LC_ALL, "");
		ConsoleArguments args = ParseArguments(argc, argv);

		if (args.ShowHelp)
		{
			wstring version = L"0.1"; // GetFileVersion();
			wcout << "ShardLang interpreter v" << version;
			return 0;
		}
		
		bool anyUnrealFiles = false;
		for (const wstring& file : args.FilesToCompile)
		{
			/*
			if (!PathFileExistsW(file.c_str()))
			{
				anyUnrealFiles = true;
				wcout << L"'" << file << L"doesnt exists";
			}
			*/
		}

		if (anyUnrealFiles)
			return 1;

		DiagnosticsContext diagnostics;
		SyntaxTree syntaxTree;
		LexicalAnalyzer lexer(diagnostics);

		for (const wstring& file : args.FilesToCompile)
		{
			FileReader reader = FileReader(file);
			lexer.FromSourceReader(syntaxTree, reader);
		}

		SemanticModel semanticModel = SemanticModel(syntaxTree);
		if (!args.ExcludeStd)
		{
			FrameworkLoader::Load(semanticModel);
		}

		SemanticAnalyzer semanticAnalyzer(diagnostics);
		semanticAnalyzer.Analyze(syntaxTree, semanticModel);

		LayoutGenerator layoutGenerator(diagnostics);
		layoutGenerator.Generate(semanticModel);

		if (diagnostics.AnyError)
		{
			wcout << L"=== Diagnostics output ===" << endl;
			diagnostics.WriteDiagnostics(wcout);
			return 1;
		}

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
			catch (const runtime_error& err)
			{
				cout << err.what() << endl;
				return 1;
			}
		}
	}
	catch (const runtime_error& err)
	{
		cout << "CRITICAL ERROR : " << err.what() << endl;
	}

	GarbageCollector::Terminate();
}