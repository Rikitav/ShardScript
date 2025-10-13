#include <shard/syntax/SyntaxToken.h>

#include <shard/parsing/SourceReader.h>
#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SyntaxTreeParser.h>

#include <shard/syntax/analysis/Diagnostic.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/structures/SyntaxTree.h>
#include <shard/syntax/analysis/DiagnosticSeverity.h>
#include <shard/runtime/AbstarctInterpreter.h>

#include "src/parsing/FileReader.cpp"
#include "src/parsing/SequenceSourceReader.cpp"
#include "src/parsing/StringStreamReader.cpp"
#include <shard/parsing/ReaderExtensions.h>

#include <iostream>
#include <memory>
#include <string>
#include <Windows.h>

using namespace std;
using namespace shard::parsing;
using namespace shard::runtime;
using namespace shard::syntax::structures;

static void WriteDiagnostic(Diagnostic& diag)
{
	cout << toString(diag.Severity) << " | ";
	cout << "File: '" << diag.Location.FileName << "' | ";
	cout << "Line: " << diag.Location.Line << " | ";
	cout << "Offset: " << diag.Location.Offset << " | ";
	cout << "Word: '" << diag.Token.Word << "' | ";
	cout << diag.Description << endl;
}

int InterpretFiles(int argc, char** argv)
{
	shared_ptr<SyntaxTree> tree = make_shared<SyntaxTree>();
	DiagnosticsContext diagnostics = DiagnosticsContext();
	LexicalAnalyzer lexer = LexicalAnalyzer(tree, diagnostics);

	for (int i = 1; i < argc; i++)
	{
		string argi = argv[i];
		unique_ptr<SourceReader> reader = make_unique<FileReader>(argi);
		lexer.FromSourceReader(*reader);
	}

	if (diagnostics.AnyError)
	{
		cout << "=== Diagnostics output ===" << endl;
		for (Diagnostic& diag : diagnostics.Diagnostics)
			WriteDiagnostic(diag);

		return 1;
	}

	AbstarctInterpreter interpreter(tree);
	interpreter.Execute();
	return 0;
}

shared_ptr<MethodDeclarationSyntax> InitImplicitEntryPoint()
{
	MemberDeclarationInfo info;
	info.ReturnType = SyntaxToken(TokenType::VoidKeyword, "", TextLocation());
	info.Identifier = SyntaxToken(TokenType::Identifier, "", TextLocation());
	info.Params = make_shared<ParametersListSyntax>();

	return make_shared<MethodDeclarationSyntax>(info);
}

int ExecuteLineStatement()
{

}

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		return InterpretFiles(argc, argv);
	}

	shared_ptr<SyntaxTree> tree = make_shared<SyntaxTree>();
	DiagnosticsContext diagnostics = DiagnosticsContext();
	LexicalAnalyzer lexer = LexicalAnalyzer(tree, diagnostics);

	shared_ptr<CallStackFrame> frame = make_shared<CallStackFrame>(nullptr, InitImplicitEntryPoint());
	AbstarctInterpreter interpreter(tree);
	SequenceSourceReader sequenceReader;

	while (true)
	{
		string line;
		cout << ">>> ";
		getline(cin, line);

		if (line == "")
			continue;

		if (line == "exit")
			break;

		StringStreamReader stringStreamReader(line);
		vector<SyntaxToken> lineSequence = ReadToEnd(stringStreamReader);
		sequenceReader.SetSequence(lineSequence);
		shared_ptr<StatementSyntax> statement = lexer.ReadStatement(sequenceReader);

		if (diagnostics.AnyError)
		{
			for (Diagnostic& diag : diagnostics.Diagnostics)
				WriteDiagnostic(diag);

			//*tree = *treeSnapshot;
			diagnostics.AnyError = false;
			diagnostics.Diagnostics.clear();
		}
		else
		{
			interpreter.ExecuteStatement(statement, frame);
		}
	}

	return 0;
}