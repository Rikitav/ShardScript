#include <shard/parsing/lexical/LexicalAnalyzer.h>
#include <shard/parsing/lexical/reading/StringStreamReader.h>
#include <shard/parsing/lexical/LexicalBuffer.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/LayoutGenerator.h>

#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

#include <shard/parsing/SourceParser.h>
#include <shard/parsing/SyntaxTree.h>
#include <shard/parsing/MemberDeclarationInfo.h>

#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/compilation/AbstractEmiter.h>
#include <shard/compilation/ProgramVirtualImage.h>
#include <shard/compilation/ByteCodeDecoder.h>

#include <shard/runtime/AbstractInterpreter.h>
#include <shard/runtime/VirtualMachine.h>
#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/SyntaxFacts.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>

#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>

#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

#include <Windows.h>
#include <processenv.h>
#include <string>
#include <iostream>
#include <vector>
#include <exception>

#include "InteractiveConsole.h"
#include "utilities/InterpreterUtilities.h"

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
	
	if (lastIndex >= 0)
	{
		SyntaxToken lastToken = reader.At(lastIndex);
		if (lastToken.Type == TokenType::Semicolon)
			return true;
	}
	
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

static StatementSyntax* ReadStatement(StatementsBlockSyntax* interactiveBody, LexicalBuffer& sequenceReader, SyntaxNode* parent, DiagnosticsContext& diagnostics)
{
	if (sequenceReader.Size() == 0)
		return nullptr;

	SyntaxToken firstToken = sequenceReader.Front();
	SourceParser parser(diagnostics);
	
	SyntaxToken lastToken = sequenceReader.Back();
	if (lastToken.Type != TokenType::Semicolon)
	{
		// Read as expression
		ReadMultilineInput(sequenceReader, true);
		ExpressionStatementSyntax* exprStatement = new ExpressionStatementSyntax(nullptr, interactiveBody);
		exprStatement->Expression = parser.ReadExpression(sequenceReader, exprStatement, 0);

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
		return parser.ReadKeywordStatement(sequenceReader, parent);
	}

	// Read as statement
	return parser.ReadStatement(sequenceReader, parent);
}

static void EvaluateUsing(LexicalBuffer& buffer, SemanticModel& semanticModel, SemanticAnalyzer& semanticAnalyzer, DiagnosticsContext& diagnostics)
{
	SourceParser sourceParser(diagnostics);
 	UsingDirectiveSyntax* directive = sourceParser.ReadUsingDirective(buffer, nullptr);
	NamespaceNode* node = semanticModel.Namespaces->Root;

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
		semanticAnalyzer.AddSymbol(symbol);
		std::wcout << symbol->Name << L", ";
		counter += 1;
	}

	std::wcout << "(" << counter << " symbols)" << std::endl;
}

static void CompileMember()
{

}

static void InterpretStatement(VirtualMachine& virtualMachine, AbstractEmiter& abstractEmiter, MethodSymbol* entryPointSymbol, StatementSyntax* statement, size_t& pointer)
{
	abstractEmiter.VisitStatement(statement);
	ObjectInstance* result = virtualMachine.RunInteractive(pointer);

	if (result != nullptr)
	{
		ConsoleHelper::Write(result);
		GarbageCollector::CollectInstance(result);
	}

	MoveToNewLineIfNeeded();
}

void InteractiveConsole::Run(SyntaxTree& syntaxTree, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	// TODO: REWRITE LOGIC TO BYTECODE
	ProgramVirtualImage program;
	size_t pointer = 0;

	// Initializing parsing
	SemanticAnalyzer semanticAnalyzer(diagnostics);
	LayoutGenerator layoutGenerator(diagnostics);
	VirtualMachine virtualMachine(program);
	
	// Creating interactive entry point
	MethodDeclarationSyntax* implMethod = nullptr;
	CompilationUnitSyntax* implUnit = InitImplicitCompilationUnit(implMethod);
	syntaxTree.CompilationUnits.push_back(implUnit);

	StatementsBlockSyntax* interactiveBody = implMethod->Body;
	MethodSymbol* entryPointSymbol = new MethodSymbol(implMethod->IdentifierToken.Word);
	virtualMachine.PushFrame(entryPointSymbol);

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
				EvaluateUsing(sequenceReader, semanticModel, semanticAnalyzer, diagnostics);
				continue;
			}

			StatementSyntax* statement = ReadStatement(interactiveBody, sequenceReader, interactiveBody, diagnostics);
			if (statement == nullptr)
				continue;

			interactiveBody->Statements.push_back(statement);

			// Re-analyze syntax tree
			SemanticModel newSemanticModel(syntaxTree);
			semanticAnalyzer.Analyze(syntaxTree, newSemanticModel);
			layoutGenerator.Generate(newSemanticModel);

			// Check for errors
			if (diagnostics.AnyError)
			{
				diagnostics.WriteDiagnostics(std::wcerr);
				diagnostics.Reset();
				continue;
			}

			AbstractEmiter abstractEmiter(program, newSemanticModel, diagnostics);
			abstractEmiter.SetGeneratingTarget(entryPointSymbol);
			program.EntryPoint = entryPointSymbol;

			InterpretStatement(virtualMachine, abstractEmiter, entryPointSymbol, statement, pointer);
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
