#include <shard/parsing/SyntaxTreeParser.h>
#include <shard/parsing/StringStreamReader.h>
#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SequenceSourceReader.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>

#include <shard/runtime/AbstarctInterpreter.h>
#include <shard/runtime/CallStackFrame.h>

#include <shard/syntax/SyntaxFacts.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <shard/syntax/analysis/TextLocation.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>

#include <shard/InterpreterConsole.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <iostream>

using namespace std;
using namespace shard::syntax::nodes;
using namespace shard::parsing;
using namespace shard::runtime;

static shared_ptr<MethodDeclarationSyntax> InitImplicitEntryPoint()
{
	MemberDeclarationInfo info;
	info.ReturnType = SyntaxToken(TokenType::VoidKeyword, "void", TextLocation());
	info.Identifier = SyntaxToken(TokenType::Identifier, "Main", TextLocation());
	info.Params = make_shared<ParametersListSyntax>();

	return make_shared<MethodDeclarationSyntax>(info);
}

static shared_ptr<ClassDeclarationSyntax> InitmnplicitClassDeclaration(shared_ptr<MethodDeclarationSyntax>& entryPoint)
{
	MemberDeclarationInfo info;
	info.Identifier = SyntaxToken(TokenType::Identifier, "shardscript_implclass", TextLocation());

	shared_ptr<ClassDeclarationSyntax> implClass = make_shared<ClassDeclarationSyntax>(info);
	implClass->DeclareKeyword = SyntaxToken(TokenType::ClassKeyword, "class", TextLocation(), false);
	entryPoint = InitImplicitEntryPoint();
	implClass->Members.push_back(entryPoint);

	return implClass;
}

static shared_ptr<NamespaceDeclarationSyntax> InitImplicitNamespaceDeclaration(shared_ptr<MethodDeclarationSyntax>& entryPoint)
{
	shared_ptr<NamespaceDeclarationSyntax> implNamespace = make_shared<NamespaceDeclarationSyntax>();
	implNamespace->DeclareKeyword = SyntaxToken(TokenType::NamespaceKeyword, "namespace", TextLocation(), false);
	implNamespace->Identifier = SyntaxToken(TokenType::Identifier, "shardscript_implnamespace", TextLocation(), false);
	implNamespace->Members.push_back(InitmnplicitClassDeclaration(entryPoint));

	return implNamespace;
}

static shared_ptr<CompilationUnitSyntax> InitImplicitCompilationUnit(shared_ptr<MethodDeclarationSyntax>& entryPoint)
{
	shared_ptr<CompilationUnitSyntax> implUnit = make_shared<CompilationUnitSyntax>();
	implUnit->Members.push_back(InitImplicitNamespaceDeclaration(entryPoint));
	return implUnit;
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

static bool HasOpenedBodies(SequenceSourceReader& reader)
{
	reader.SetIndex(0);
	int openedBodiesCount = 0;

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Consume();
		switch (current.Type)
		{
			case TokenType::OpenBrace:
			{
				openedBodiesCount += 1;
				break;
			}

			case TokenType::CloseBrace:
			{
				openedBodiesCount -= 1;
				break;
			}
		}
	}

	reader.SetIndex(0);
	return openedBodiesCount > 0;
}

static shared_ptr<StatementSyntax> ReadStatement(LexicalAnalyzer& lexer)
{
	string line = ReadLine();
	StringStreamReader stringStreamReader(line);

	SequenceSourceReader sequenceReader = SequenceSourceReader();
	sequenceReader.PopulateFrom(stringStreamReader);

	SyntaxToken current = sequenceReader.Current();
	if (IsLoopKeyword(current.Type) || IsConditionalKeyword(current.Type))
	{
		string nextLine = ReadLine();
		stringStreamReader = StringStreamReader(nextLine);

		sequenceReader.PopulateFrom(stringStreamReader);
		while (HasOpenedBodies(sequenceReader))
		{
			nextLine = ReadLine();
			stringStreamReader = StringStreamReader(nextLine);
			sequenceReader.PopulateFrom(stringStreamReader);
		}

		return lexer.ReadKeywordStatement(sequenceReader);
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

void RunInterpreter(shared_ptr<SyntaxTree> tree, DiagnosticsContext& diagnostics, LexicalAnalyzer& lexer, SyntaxTreeParser& parser)
{
	shared_ptr<MethodDeclarationSyntax> entryPoint = nullptr;
	shared_ptr<CompilationUnitSyntax> implUnit = InitImplicitCompilationUnit(entryPoint);
	tree->CompilationUnits.push_back(implUnit);
	tree->EntryPoint = entryPoint;

	shared_ptr<CallStackFrame> frame = make_shared<CallStackFrame>(nullptr, entryPoint);
	AbstarctInterpreter interpreter(tree);

	while (true)
	{
		shared_ptr<StatementSyntax> statement = ReadStatement(lexer);
		parser.EnsureSyntaxTree(tree);

		if (diagnostics.AnyError)
		{
			diagnostics.WriteDiagnostics(cout);
			diagnostics.Reset();
			continue;
		}

		try
		{
			shared_ptr<VariableRegister> pRegister = interpreter.ExecuteStatement(statement, frame->Context, frame);
			if (pRegister != nullptr)
			{
				interpreter.PrintRegister(pRegister);
				cout << endl;
			}
		}
		catch (const runtime_error& err)
		{
			cout << "### " << err.what() << endl;
			continue;
		}
	}
}
