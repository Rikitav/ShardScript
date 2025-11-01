#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/reading/StringStreamReader.h>
#include <shard/parsing/reading/SequenceSourceReader.h>

#include <shard/runtime/interactive/InteractiveConsole.h>

#include <shard/syntax/SyntaxFacts.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>
#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>

#include <cstdlib>
#include <string>
#include <iostream>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::semantic;
using namespace shard::parsing::lexical;

static MethodDeclarationSyntax* InitImplicitEntryPoint(SyntaxNode* parent)
{
	MemberDeclarationInfo info;
	//info.ReturnType = SyntaxToken(TokenType::VoidKeyword, L"void", TextLocation());
	info.Identifier = SyntaxToken(TokenType::Identifier, L"Main", TextLocation());
	info.Params = new ParametersListSyntax(parent);

	return new MethodDeclarationSyntax(info, parent);
}

static ClassDeclarationSyntax* InitImplicitClassDeclaration(MethodDeclarationSyntax*& entryPoint, SyntaxNode* parent)
{
	MemberDeclarationInfo info;
	info.Identifier = SyntaxToken(TokenType::Identifier, L"__interacive_class__", TextLocation());

	ClassDeclarationSyntax* implClass = new ClassDeclarationSyntax(info, parent);
	implClass->DeclareToken = SyntaxToken(TokenType::ClassKeyword, L"class", TextLocation(), false);
	
	entryPoint = InitImplicitEntryPoint(implClass);
	implClass->Members.push_back(entryPoint);

	return implClass;
}

static NamespaceDeclarationSyntax* InitImplicitNamespaceDeclaration(MethodDeclarationSyntax*& entryPoint, SyntaxNode* parent)
{
	NamespaceDeclarationSyntax* implNamespace = new NamespaceDeclarationSyntax(parent);
	implNamespace->DeclareToken = SyntaxToken(TokenType::NamespaceKeyword, L"namespace", TextLocation(), false);
	implNamespace->IdentifierToken = SyntaxToken(TokenType::Identifier, L"__interactive_namespace_", TextLocation(), false);
	implNamespace->Members.push_back(InitImplicitClassDeclaration(entryPoint, implNamespace));

	return implNamespace;
}

static CompilationUnitSyntax* InitImplicitCompilationUnit(MethodDeclarationSyntax*& entryPoint)
{
	CompilationUnitSyntax* implUnit = new CompilationUnitSyntax();
	implUnit->Members.push_back(InitImplicitNamespaceDeclaration(entryPoint, implUnit));
	return implUnit;
}

static wstring ReadLine()
{
	wstring line;
	wcout << ">>> ";
	getline(wcin, line);

	if (line == L"exit")
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

static StatementSyntax* ReadStatement(LexicalAnalyzer& lexer, SyntaxNode* parent)
{
	wstring line = ReadLine();
	StringStreamReader stringStreamReader(line);

	SequenceSourceReader sequenceReader = SequenceSourceReader();
	sequenceReader.PopulateFrom(stringStreamReader);

	SyntaxToken current = sequenceReader.Current();
	if (IsLoopKeyword(current.Type) || IsConditionalKeyword(current.Type))
	{
		wstring nextLine = ReadLine();
		stringStreamReader = StringStreamReader(nextLine);

		sequenceReader.PopulateFrom(stringStreamReader);
		while (HasOpenedBodies(sequenceReader))
		{
			nextLine = ReadLine();
			stringStreamReader = StringStreamReader(nextLine);
			sequenceReader.PopulateFrom(stringStreamReader);
		}

		return lexer.ReadKeywordStatement(sequenceReader, parent);
	}
	else if (IsFunctionalKeyword(current.Type))
	{
		return lexer.ReadKeywordStatement(sequenceReader, parent);
	}
	else
	{
		return lexer.ReadStatement(sequenceReader, parent);
	}
}

void InteractiveConsole::Run(SyntaxTree& syntaxTree, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	/*
	shared_ptr<MethodDeclarationSyntax> entryPoint = nullptr;
	shared_ptr<CompilationUnitSyntax> implUnit = InitImplicitCompilationUnit(entryPoint);
	tree.CompilationUnits.push_back(implUnit);
	tree.EntryPoint = entryPoint;

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
			ObjectInstance* pRegister = interpreter.ExecuteStatement(statement, frame->Context, frame);
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
	*/
}
