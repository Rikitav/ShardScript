#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>
#include <shard/parsing/lexical/LexicalBuffer.hpp>
#include <shard/parsing/SemanticAnalyzer.hpp>
#include <shard/parsing/LayoutGenerator.hpp>

#include <shard/parsing/analysis/TextLocation.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SyntaxTree.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>

#include <shard/parsing/semantic/SemanticModel.hpp>

#include <shard/compilation/AbstractEmiter.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>

#include <shard/runtime/AbstractInterpreter.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/ConsoleHelper.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/SyntaxFacts.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>

#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>

#include <Windows.h>
#include <processenv.h>
#include <string>
#include <iostream>
#include <vector>
#include <exception>

#include "InteractiveConsole.hpp"
#include "utilities/InterpreterUtilities.hpp"

using namespace shard;

static MethodDeclarationSyntax* InitImplicitEntryPoint(SyntaxNode* parent)
{
	MemberDeclarationInfo info;
	info.ReturnType = new PredefinedTypeSyntax(SyntaxToken(TokenType::VoidKeyword, L"void", TextLocation(), false), nullptr);
	info.Identifier = SyntaxToken(TokenType::Identifier, L"__interactive_console__", TextLocation());

	MethodDeclarationSyntax* implMethod = new MethodDeclarationSyntax(info, parent);
	implMethod->Params = new ParametersListSyntax(parent);
	implMethod->Body = new StatementsBlockSyntax(implMethod);
	return implMethod;
}

static ClassDeclarationSyntax* InitImplicitClassDeclaration(MethodDeclarationSyntax*& entryPoint, SyntaxNode* parent)
{
	MemberDeclarationInfo info;
	info.Identifier = SyntaxToken(TokenType::Identifier, L"__InteractiveClass__", TextLocation());

	ClassDeclarationSyntax* implClass = new ClassDeclarationSyntax(info, parent);
	implClass->DeclareToken = SyntaxToken(TokenType::ClassKeyword, L"class", TextLocation(), false);

	entryPoint = InitImplicitEntryPoint(implClass);
	*const_cast<SyntaxNode**>(&entryPoint->Parent) = implClass;
	implClass->Members.push_back(entryPoint);

	return implClass;
}

static NamespaceDeclarationSyntax* InitImplicitNamespaceDeclaration(MethodDeclarationSyntax*& entryPoint, SyntaxNode* parent)
{
	NamespaceDeclarationSyntax* implNamespace = new NamespaceDeclarationSyntax(parent);
	implNamespace->DeclareToken = SyntaxToken(TokenType::NamespaceKeyword, L"namespace", TextLocation(), false);
	implNamespace->IdentifierTokens.push_back(SyntaxToken(TokenType::Identifier, L"__InteractiveNamespace__", TextLocation(), false));
	implNamespace->Members.push_back(InitImplicitClassDeclaration(entryPoint, implNamespace));

	return implNamespace;
}

static CompilationUnitSyntax* InitImplicitCompilationUnit(MethodDeclarationSyntax*& entryPoint)
{
	CompilationUnitSyntax* implUnit = new CompilationUnitSyntax();
	implUnit->Members.push_back(InitImplicitNamespaceDeclaration(entryPoint, implUnit));
	return implUnit;
}

static bool IsStatementComplete(LexicalBuffer& reader)
{
	reader.SetIndex(0);
	int lastIndex = static_cast<int>(reader.Size()) - 1;
	
	/*
	if (lastIndex >= 0)
	{
		SyntaxToken lastToken = reader.At(lastIndex);
		if (lastToken.Type == TokenType::Semicolon)
			return true;
	}
	*/
	
	int braceCount = 0;
	int parenCount = 0;
	int bracketCount = 0;
	
	bool hasStatementStart = false;
	
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		
		switch (current.Type)
		{
			case TokenType::OpenBrace:
				braceCount++;
				break;
			
			case TokenType::CloseBrace:
				braceCount--;
				break;
			
			case TokenType::OpenCurl:
				parenCount++;
				break;
			
			case TokenType::CloseCurl:
				parenCount--;
				break;
			
			case TokenType::OpenSquare:
				bracketCount++;
				break;
			
			case TokenType::CloseSquare:
				bracketCount--;
				break;
				
			case TokenType::ForKeyword:
			case TokenType::WhileKeyword:
			case TokenType::UntilKeyword:
			case TokenType::IfKeyword:
			case TokenType::UnlessKeyword:
			case TokenType::ReturnKeyword:
			case TokenType::BreakKeyword:
			case TokenType::ContinueKeyword:
				hasStatementStart = true;
				break;
		}
		
		reader.Consume();
	}
	
	reader.SetIndex(0);
	
	// If all braces are balanced and we have a statement start, it might be complete
	if (braceCount == 0 && parenCount == 0 && bracketCount == 0 && hasStatementStart)
	{
		// Check if last token suggests completion
		if (lastIndex >= 0)
		{
			SyntaxToken lastToken = reader.At(lastIndex);
			if (lastToken.Type == TokenType::CloseBrace || lastToken.Type == TokenType::Semicolon)
				return true;
		}
	}
	
	return braceCount == 0 && parenCount == 0 && bracketCount == 0;
}

static bool IsExpressionComplete(LexicalBuffer& reader)
{
	reader.SetIndex(0);
	int parenCount = 0;
	int bracketCount = 0;
	
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		
		switch (current.Type)
		{
			case TokenType::OpenCurl:
				parenCount++;
				break;

			case TokenType::CloseCurl:
				parenCount--;
				break;
			
			case TokenType::OpenSquare:
				bracketCount++;
				break;
			
			case TokenType::CloseSquare:
				bracketCount--;
				break;
		}
		
		reader.Consume();
	}
	
	reader.SetIndex(0);
	return parenCount == 0 && bracketCount == 0;
}

static std::wstring ReadLine(const std::wstring& prompt = L">>> ")
{
	std::wstring line;
	std::wcout << prompt;
	std::getline(std::wcin, line);
	return line;
}

static void MoveToNewLineIfNeeded()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		if (csbi.dwCursorPosition.X > 0)
			ConsoleHelper::WriteLine();
	}
}

static void ReadMultilineInput(LexicalBuffer& sequenceReader, bool isExpression = false)
{
	// Check if already complete
	bool isComplete = isExpression ? IsExpressionComplete(sequenceReader) : IsStatementComplete(sequenceReader);
	
	if (isComplete)
		return;
	
	// Read continuation lines
	std::wstring continuationPrompt = L"... ";
	std::wstring line;
	
	int maxLines = 100; // Safety limit
	int lineCount = 0;

	while (!isComplete && lineCount < maxLines)
	{
		std::wstring line = ReadLine(continuationPrompt);
		
		if (line.empty() && lineCount == 0)
		{
			isComplete = isExpression ? IsExpressionComplete(sequenceReader) : IsStatementComplete(sequenceReader);
			if (isComplete)
				return;
		}
		
		StringStreamReader stringStreamReader(L"Interactive console", line);
		LexicalAnalyzer lexicalAnalyzer(stringStreamReader);
		sequenceReader.PopulateFrom(lexicalAnalyzer);
		lineCount++;
		isComplete = isExpression ? IsExpressionComplete(sequenceReader) : IsStatementComplete(sequenceReader);
	}
}

StatementSyntax* InteractiveConsole::ReadStatement(LexicalBuffer& sequenceReader)
{
	if (sequenceReader.Size() == 0)
		return nullptr;

	SyntaxToken firstToken = sequenceReader.Front();
	SyntaxToken lastToken = sequenceReader.Back();

	if (lastToken.Type != TokenType::Semicolon)
	{
		// Read as expression
		ReadMultilineInput(sequenceReader, true);
		ExpressionStatementSyntax* exprStatement = new ExpressionStatementSyntax(nullptr, InteractiveMethod->Body);
		exprStatement->Expression = Parser.ReadExpression(sequenceReader, exprStatement, 0);

		if (exprStatement->Expression == nullptr)
		{
			std::wcerr << L"### Failed to parse expression" << std::endl;
			delete exprStatement;
			return nullptr;
		}

		// Wrap expression in expression statement
		exprStatement->SemicolonToken = SyntaxToken(TokenType::Semicolon, L";", TextLocation(), false);
		return exprStatement;
	}

	if (IsLoopKeyword(firstToken.Type) || IsConditionalKeyword(firstToken.Type) || IsFunctionalKeyword(firstToken.Type))
	{
		ReadMultilineInput(sequenceReader, false);
		return Parser.ReadKeywordStatement(sequenceReader, InteractiveMethod->Body);
	}

	// Read as statement
	return Parser.ReadStatement(sequenceReader, InteractiveMethod->Body);
}

MemberDeclarationSyntax* InteractiveConsole::ReadMember(LexicalBuffer& sequenceReader)
{
	if (sequenceReader.Size() == 0)
		return nullptr;

	ReadMultilineInput(sequenceReader, false);
	MemberDeclarationSyntax* member = Parser.ReadMemberDeclaration(sequenceReader, InteractiveClass);

	if (member->Kind != SyntaxKind::MethodDeclaration)
		Diagnostics.ReportError(member->DeclareToken, L"Only methods compilation supported");

	return member;
}

void InteractiveConsole::EvaluateUsing(LexicalBuffer& buffer)
{
 	UsingDirectiveSyntax* directive = Parser.ReadUsingDirective(buffer, nullptr);
	NamespaceNode* node = ParentSemanticModel.Namespaces->Root;

	for (SyntaxToken token : directive->TokensList)
	{
		node = node->Lookup(token.Word);
		if (node == nullptr)
		{
			std::wcerr << L"### Namespace '" << token.Word << "' not found on namespace." << std::endl;
			return;
		}
	}

	int counter = 0;
	std::wcout << L"Loaded : ";
	for (const auto& symbol : node->Types)
	{
		Semanter.AddSymbol(symbol);
		std::wcout << symbol->Name << L", ";
		counter += 1;
	}

	std::wcout << "(" << counter << " symbols)" << std::endl;
}

InteractiveConsole::InteractiveConsole(SyntaxTree& ParentSyntaxTree, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
	: ParentSyntaxTree(ParentSyntaxTree), ParentSemanticModel(semanticModel), Diagnostics(diagnostics),
	  Parser(diagnostics), Semanter(diagnostics), Layouter(diagnostics),
	  Program(), Runtimer(Program)
{
	InteractiveUnit = InitImplicitCompilationUnit(InteractiveMethod);
	InteractiveClass = static_cast<ClassDeclarationSyntax*>(static_cast<NamespaceDeclarationSyntax*>(InteractiveUnit->Members.at(0))->Members.at(0));
	ParentSyntaxTree.CompilationUnits.push_back(InteractiveUnit);

	InteractiveEntryPoint = new MethodSymbol(InteractiveMethod->IdentifierToken.Word);
	Program.EntryPoint = InteractiveEntryPoint;
	Runtimer.PushFrame(InteractiveEntryPoint);
}

void InteractiveConsole::Run()
{
	ConsoleHelper::WriteLine(L"ShardScript Interactive Console v" + shard::ShardUtilities::GetFileVersion());
	ConsoleHelper::WriteLine(L"Type 'exit' or 'quit' to exit");
	ConsoleHelper::WriteLine();

	while (true)
	{
		try
		{
			std::wstring firstLine = ReadLine();
			if (firstLine.empty())
				continue;

			if (firstLine == L"exit" || firstLine == L"quit")
				break;

			StringStreamReader stringStreamReader(L"Interactive console", firstLine);
			LexicalAnalyzer lexer(stringStreamReader);
			LexicalBuffer sequenceReader = LexicalBuffer::From(lexer);

			if (!sequenceReader.CanConsume())
				continue;

			SyntaxToken firstToken = sequenceReader.Current();
			if (firstToken.Type == TokenType::Semicolon)
				continue;

			if (firstToken.Type == TokenType::UsingKeyword)
			{
				EvaluateUsing(sequenceReader);
				continue;
			}

			if (sequenceReader.Size() > 1)
			{
				SyntaxToken secondToken = sequenceReader.At(1);
				if (IsMemberDeclaration(firstToken.Type, secondToken.Type))
				{
					MemberDeclarationSyntax* member = ReadMember(sequenceReader);
					if (member == nullptr)
						continue;

					MethodDeclarationSyntax* method = static_cast<MethodDeclarationSyntax*>(member);
					InteractiveClass->Members.push_back(method);

					// Re-analyze syntax tree
					Semanter.Analyze(ParentSyntaxTree, ParentSemanticModel);
					Layouter.Generate(ParentSemanticModel);

					// Check for errors
					if (Diagnostics.AnyError)
					{
						Diagnostics.WriteDiagnostics(std::wcerr);
						Diagnostics.Reset();

						InteractiveClass->Members.pop_back();
						delete method;
						continue;
					}

					AbstractEmiter abstractEmiter(Program, ParentSemanticModel, Diagnostics);
					abstractEmiter.VisitMethodDeclaration(method);
					continue;
				}
			}

			StatementSyntax* statement = ReadStatement(sequenceReader);
			if (statement == nullptr)
				continue;

			InteractiveMethod->Body->Statements.push_back(statement);

			// Re-analyze syntax tree
			Semanter.Analyze(ParentSyntaxTree, ParentSemanticModel);
			Layouter.Generate(ParentSemanticModel);

			// Check for errors
			if (Diagnostics.AnyError)
			{
				Diagnostics.WriteDiagnostics(std::wcerr);
				Diagnostics.Reset();

				InteractiveMethod->Body->Statements.pop_back();
				delete statement;
				continue;
			}

			AbstractEmiter abstractEmiter(Program, ParentSemanticModel, Diagnostics);
			abstractEmiter.SetGeneratingTarget(InteractiveEntryPoint);

			abstractEmiter.VisitStatement(statement);
			ObjectInstance* result = Runtimer.RunInteractive(Breakpoint);

			if (result != nullptr)
			{
				ConsoleHelper::Write(result);
				GarbageCollector::CollectInstance(result);
			}

			MoveToNewLineIfNeeded();
		}
		catch (const std::exception& err)
		{
			std::wcerr << L"### Runtime error: " << err.what() << std::endl;
		}
		catch (...)
		{
			std::wcerr << L"### Unknown error occurred" << std::endl;
		}
	}
}
