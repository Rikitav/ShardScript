#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <shard/syntax/structures/SyntaxTree.h>

#include <shard/parsing/FileReader.h>
#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SyntaxTreeParser.h>

#include "ArgumentsParser.cpp"
#include <shard/InterpreterConsole.h>
#include <shard/runtime/AbstarctInterpreter.h>

#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

using namespace std;
using namespace shard::utilities;
using namespace shard::parsing;
using namespace shard::runtime;
using namespace shard::syntax::structures;

int main(int argc, char** argv)
{
	shared_ptr<SyntaxTree> tree = make_shared<SyntaxTree>();
	DiagnosticsContext diagnostics = DiagnosticsContext();
	LexicalAnalyzer lexer = LexicalAnalyzer(tree, diagnostics);
	SyntaxTreeParser parser(diagnostics);

	ConsoleArguments args = ParseArguments(argc, argv);
	for (const string& file : args.FilesToCompile)
	{
		unique_ptr<FileReader> reader = make_unique<FileReader>(file);
		lexer.FromSourceReader(*reader);
	}

	if (args.UseInterpreter)
	{
		RunInterpreter(tree, diagnostics, lexer, parser);
		return 0;
	}

	parser.EnsureSyntaxTree(tree);
	if (diagnostics.AnyError)
	{
		cout << "=== Diagnostics output ===" << endl;
		diagnostics.WriteDiagnostics(cout);
		return 1;
	}

	try
	{
		AbstarctInterpreter interpreter(tree);
		interpreter.Execute();
		return 0;
	}
	catch (const runtime_error& err)
	{
		cout << err.what() << endl;
		return 1;
	}
}