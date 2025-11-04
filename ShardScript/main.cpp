#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/reading/FileReader.h>

#include <shard/runtime/interpreter/AbstractInterpreter.h>
#include <shard/runtime/interactive/InteractiveConsole.h>
#include "ArgumentsParser.cpp"

#include <iostream>
#include <string>
#include <stdexcept>
#include <exception>
#include <clocale>
#include <shard/parsing/LayoutGenerator.h>

using namespace std;
using namespace shard::utilities;
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

		DiagnosticsContext diagnostics;
		SyntaxTree syntaxTree;
		LexicalAnalyzer lexer(diagnostics);
		
		for (const wstring& file : args.FilesToCompile)
		{
			FileReader reader = FileReader(file);
			lexer.FromSourceReader(syntaxTree, reader);
		}

		SemanticModel semanticModel = SemanticModel(syntaxTree);
		semanticModel.Table->ResolvePrmitives();
		semanticModel.Table->ResolveGlobalMethods();

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
			return 0;
		}

		try
		{
			AbstractInterpreter interpreter(syntaxTree, semanticModel);
			interpreter.Execute();
			return 0;
		}
		catch (const runtime_error& err)
		{
			cout << err.what() << endl;
			return 1;
		}
	}
	catch (const runtime_error& err)
	{
		cout << "CRITICAL ERROR : " << err.what() << endl;
		return 1;
	}
}