#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxFacts.h>

#include <shard/parsing/SourceReader.h>
#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SyntaxTreeParser.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>

#include <shard/syntax/analysis/Diagnostic.h>
#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/analysis/DiagnosticSeverity.h>
#include <shard/syntax/analysis/TextLocation.h>

#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/structures/SyntaxTree.h>

#include <shard/runtime/AbstarctInterpreter.h>
#include <shard/runtime/CallStackFrame.h>

#include <shard/parsing/FileReader.h>
#include <shard/parsing/SequenceSourceReader.h>
#include <shard/parsing/StringStreamReader.h>

#include "ArgumentsParser.cpp"
//#include "InterpreterConsole.cpp"

#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

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
	cout << "Index: '" << diag.Token.Index << "' | ";
	cout << diag.Description;
	cout << endl;
}

static int InterpretFiles(int argc, char** argv)
{
	shared_ptr<SyntaxTree> tree = make_shared<SyntaxTree>();
	DiagnosticsContext diagnostics = DiagnosticsContext();
	LexicalAnalyzer lexer = LexicalAnalyzer(tree, diagnostics);

	for (int i = 1; i < argc; i++)
	{
		string argi = shard::utilities::normalizePath(argv[i]);
		unique_ptr<SourceReader> reader = make_unique<FileReader>(argi);
		lexer.FromSourceReader(*reader);
	}

	SyntaxTreeParser parser(diagnostics);
	parser.EnsureSyntaxTree(tree);

	if (diagnostics.AnyError)
	{
		cout << "=== Diagnostics output ===" << endl;
		for (Diagnostic& diag : diagnostics.Diagnostics)
			WriteDiagnostic(diag);

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

static shared_ptr<MethodDeclarationSyntax> InitImplicitEntryPoint()
{
	MemberDeclarationInfo info;
	info.ReturnType = SyntaxToken(TokenType::VoidKeyword, "void", TextLocation());
	info.Identifier = SyntaxToken(TokenType::Identifier, "Main", TextLocation());
	info.Params = make_shared<ParametersListSyntax>();

	return make_shared<MethodDeclarationSyntax>(info);
}

static string ReadLine()
{
	string line;
	cout << ">>> ";
	getline(cin, line);

	if (line == "exit")
		exit(0);

	return line;
}

static bool LocateOpenBraceToken(TokenType keywordType, SequenceSourceReader& reader)
{
	enum LocatinoProgress
	{
		None,
		Keyword,
		OpenCurl,
		CloseCurl
	};

	LocatinoProgress progress = None;
	reader.SetIndex(0);

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		switch (progress)
		{
			case None:
			{
				if (progress >= Keyword)
					return false;

				if (current.Type == keywordType)
					progress = Keyword;

				reader.Consume();
				break;
			}

			case Keyword:
			{
				if (progress >= OpenCurl)
					return false;

				if (current.Type == TokenType::OpenCurl)
					progress = OpenCurl;

				reader.Consume();
				break;
			}

			case OpenCurl:
			{
				if (progress >= CloseCurl)
					return false;

				if (current.Type == TokenType::CloseCurl)
					progress = CloseCurl;

				reader.Consume();
				break;
			}

			case CloseCurl:
			{
				if (current.Type == TokenType::OpenBrace)
				{
					reader.SetIndex(0);
					return true;
				}

				reader.Consume();
				break;
			}

			default:
			{
				// unknown progress, fault
				reader.SetIndex(0);
				return false;
			}
		}
	}

	reader.SetIndex(0);
	return false;
}

static shared_ptr<StatementSyntax> ReadStatement(LexicalAnalyzer& lexer)
{
	string line = ReadLine();
	StringStreamReader stringStreamReader(line);

	SequenceSourceReader sequenceReader = SequenceSourceReader();
	sequenceReader.PopulateFrom(stringStreamReader);

	SyntaxToken current = sequenceReader.Current();
	if (IsLoopKeyword(current.Type))
	{
		string nextLine = ReadLine();
		stringStreamReader = StringStreamReader(nextLine);

		sequenceReader.PopulateFrom(stringStreamReader);
		if (LocateOpenBraceToken(current.Type, sequenceReader))
		{
			while (sequenceReader.Back().Type != TokenType::CloseBrace)
			{
				nextLine = ReadLine();
				stringStreamReader = StringStreamReader(nextLine);
				sequenceReader.PopulateFrom(stringStreamReader);
			}

			return lexer.ReadKeywordStatement(sequenceReader);
		}
		else
		{
			sequenceReader.PopulateFrom(stringStreamReader);
			return lexer.ReadKeywordStatement(sequenceReader);
		}
	}
	else if (IsFunctionalKeyword(current.Type))
	{
		return lexer.ReadKeywordStatement(sequenceReader);
	}
	else
	{
		return lexer.ReadStatement(sequenceReader);
	}
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

	while (true)
	{
		shared_ptr<StatementSyntax> statement = ReadStatement(lexer);
		if (diagnostics.AnyError)
		{
			for (Diagnostic& diag : diagnostics.Diagnostics)
				WriteDiagnostic(diag);

			diagnostics.Reset();
		}

		try
		{
			shared_ptr<Register> pRegister = interpreter.ExecuteStatement(statement, frame);
			if (pRegister != nullptr)
			{
				interpreter.PrintRegister(pRegister);
				cout << endl;
			}
		}
		catch (const runtime_error& err)
		{
			cout << err.what() << endl;
			continue;
		}
	}

	return 0;
}