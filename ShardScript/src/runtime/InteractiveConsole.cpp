#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/reading/StringStreamReader.h>
#include <shard/parsing/reading/SequenceSourceReader.h>
#include <shard/parsing/SemanticAnalyzer.h>
#include <shard/parsing/LayoutGenerator.h>

#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>

#include <shard/parsing/semantic/SemanticModel.h>

#include <shard/runtime/InteractiveConsole.h>
#include <shard/runtime/AbstractInterpreter.h>
#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/InboundVariablesContext.h>

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

#include <shard/ShardScript.h>
#include <Windows.h>
#include <consoleapi2.h>
#include <processenv.h>

#include <string>
#include <iostream>
#include <vector>
#include <exception>

using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::syntax::symbols;
using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::semantic;
using namespace shard::parsing::lexical;

static bool IsStatementComplete(SequenceSourceReader& reader)
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

static bool IsExpressionComplete(SequenceSourceReader& reader)
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

static std::wstring ReadMultilineInput(LexicalAnalyzer& lexer, const std::wstring& firstLine, bool isExpression = false)
{
	std::wstring fullInput = firstLine;
	
	StringStreamReader stringStreamReader(firstLine);
	SequenceSourceReader sequenceReader;
	sequenceReader.PopulateFrom(stringStreamReader);
	
	// Check if already complete
	bool isComplete = isExpression ? IsExpressionComplete(sequenceReader) : IsStatementComplete(sequenceReader);
	
	if (isComplete)
		return fullInput;
	
	// Read continuation lines
	std::wstring continuationPrompt = L"... ";
	std::wstring line;
	
	int maxLines = 100; // Safety limit
	int lineCount = 0;
	
	while (!isComplete && lineCount < maxLines)
	{
		line = ReadLine(continuationPrompt);
		
		if (line.empty() && lineCount == 0)
		{
			stringStreamReader = StringStreamReader(fullInput);
			sequenceReader.PopulateFrom(stringStreamReader);
			isComplete = isExpression ? IsExpressionComplete(sequenceReader) : IsStatementComplete(sequenceReader);
			
			if (isComplete)
				break;
		}
		
		fullInput += L"\n" + line;
		stringStreamReader = StringStreamReader(fullInput);
		sequenceReader.PopulateFrom(stringStreamReader);
		isComplete = isExpression ? IsExpressionComplete(sequenceReader) : IsStatementComplete(sequenceReader);
		lineCount++;
	}
	
	return fullInput;
}

static StatementSyntax* ReadStatement(LexicalAnalyzer& lexer, SyntaxNode* parent, DiagnosticsContext& diagnostics, const std::wstring& firstLine)
{
	if (firstLine.empty())
		return nullptr;
	
	std::wstring fullInput = ReadMultilineInput(lexer, firstLine, false);
	
	StringStreamReader stringStreamReader(fullInput);
	SequenceSourceReader sequenceReader;
	sequenceReader.PopulateFrom(stringStreamReader);
	SyntaxToken current = sequenceReader.Current();
	
	if (IsLoopKeyword(current.Type) || IsConditionalKeyword(current.Type) || IsFunctionalKeyword(current.Type))
	{
		return lexer.ReadKeywordStatement(sequenceReader, parent);
	}
	else
	{
		return lexer.ReadStatement(sequenceReader, parent);
	}
}

static MethodDeclarationSyntax* InitImplicitEntryPoint(SyntaxNode* parent)
{
	MemberDeclarationInfo info;
	info.ReturnType = new PredefinedTypeSyntax(SyntaxToken(TokenType::VoidKeyword, L"void", TextLocation(), false), nullptr);
	info.Identifier = SyntaxToken(TokenType::Identifier, L"__interactive_console__", TextLocation());
	info.Params = new ParametersListSyntax(parent);

	MethodDeclarationSyntax* implMethod = new MethodDeclarationSyntax(info, parent);
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
	entryPoint->Parent = implClass;
	implClass->Members.push_back(entryPoint);

	return implClass;
}

static NamespaceDeclarationSyntax* InitImplicitNamespaceDeclaration(MethodDeclarationSyntax*& entryPoint, SyntaxNode* parent)
{
	NamespaceDeclarationSyntax* implNamespace = new NamespaceDeclarationSyntax(parent);
	implNamespace->DeclareToken = SyntaxToken(TokenType::NamespaceKeyword, L"namespace", TextLocation(), false);
	implNamespace->IdentifierToken = SyntaxToken(TokenType::Identifier, L"__InteractiveNamespace__", TextLocation(), false);
	implNamespace->Members.push_back(InitImplicitClassDeclaration(entryPoint, implNamespace));

	return implNamespace;
}

static CompilationUnitSyntax* InitImplicitCompilationUnit(MethodDeclarationSyntax*& entryPoint)
{
	CompilationUnitSyntax* implUnit = new CompilationUnitSyntax();
	implUnit->Members.push_back(InitImplicitNamespaceDeclaration(entryPoint, implUnit));
	return implUnit;
}

void InteractiveConsole::Run(SyntaxTree& syntaxTree, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	// Initializing parsing
	LexicalAnalyzer lexer(diagnostics);
	SemanticAnalyzer semanticAnalyzer(diagnostics);
	LayoutGenerator layoutGenerator(diagnostics);
	
	// Creating interactive entry point
	MethodDeclarationSyntax* implMethod = nullptr;
	CompilationUnitSyntax* implUnit = InitImplicitCompilationUnit(implMethod);
	syntaxTree.CompilationUnits.push_back(implUnit);

	StatementsBlockSyntax* interactiveBody = implMethod->Body;
	MethodSymbol* entryPointSymbol = new MethodSymbol(implMethod->IdentifierToken.Word, interactiveBody);

	AbstractInterpreter::PushFrame(entryPointSymbol);
	AbstractInterpreter::PushContext(new InboundVariablesContext(nullptr));
	
	ConsoleHelper::WriteLine(L"ShardScript Interactive Console v" + shard::utilities::ShardUtilities::GetFileVersion());
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

			StringStreamReader stringStreamReader(firstLine);
			SequenceSourceReader sequenceReader;
			sequenceReader.PopulateFrom(stringStreamReader);

			if (!sequenceReader.CanConsume())
				continue;

			SyntaxToken firstToken = sequenceReader.Current();
			if (firstToken.Type == TokenType::Semicolon)
				continue;

			// Check if it looks like an expression (no keywords, ends with semicolon or not)
			bool isExpression = false;
			if (!IsLoopKeyword(firstToken.Type) && !IsConditionalKeyword(firstToken.Type) && !IsFunctionalKeyword(firstToken.Type))
			{
				// Check if it ends with semicolon - if not, its likely an expression
				size_t lastIndex = sequenceReader.Size() - 1;
				if (lastIndex >= 0)
				{
					SyntaxToken lastToken = sequenceReader.At(lastIndex);
					if (lastToken.Type != TokenType::Semicolon)
					{
						isExpression = true;
					}
				}
				else
				{
					isExpression = true;
				}
			}

			if (isExpression)
			{
				// Read as expression
				std::wstring fullInput = ReadMultilineInput(lexer, firstLine, true);

				StringStreamReader exprReader(fullInput);
				SequenceSourceReader exprSequence;
				exprSequence.PopulateFrom(exprReader);

				ExpressionSyntax* expression = lexer.ReadExpression(exprSequence, interactiveBody, 0);

				if (expression == nullptr)
				{
					std::wcerr << L"### Failed to parse expression" << std::endl;
					continue;
				}

				// Wrap expression in expression statement
				ExpressionStatementSyntax* exprStatement = new ExpressionStatementSyntax(expression, interactiveBody);
				exprStatement->SemicolonToken = SyntaxToken(TokenType::Semicolon, L";", TextLocation(), false);
				expression->Parent = exprStatement;

				// Add statement to interactive body
				exprStatement->Parent = interactiveBody;
				interactiveBody->Statements.push_back(exprStatement);

				// Re-analyze syntax tree
				semanticModel.Table->ClearSymbols();
				semanticAnalyzer.Analyze(syntaxTree, semanticModel);
				layoutGenerator.Generate(semanticModel);

				// Check for errors
				if (diagnostics.AnyError)
				{
					diagnostics.WriteDiagnostics(std::wcerr);
					diagnostics.Reset();

					if (!interactiveBody->Statements.empty())
					{
						interactiveBody->Statements.pop_back();
					}

					continue;
				}

				// Execute expression
				ObjectInstance* result = AbstractInterpreter::EvaluateExpression(expression);
				if (result != nullptr)
				{
					ConsoleHelper::Write(result);
					GarbageCollector::DestroyInstance(result);
				}

				MoveToNewLineIfNeeded();
			}
			else
			{
				// Read as statement
				StatementSyntax* statement = ReadStatement(lexer, interactiveBody, diagnostics, firstLine);

				if (statement == nullptr)
				{
					continue;
				}

				// Add statement to interactive body
				statement->Parent = interactiveBody;
				interactiveBody->Statements.push_back(statement);

				// Re-analyze syntax tree with new statement
				semanticModel.Table->ClearSymbols();
				semanticAnalyzer.Analyze(syntaxTree, semanticModel);
				layoutGenerator.Generate(semanticModel);

				// Check for errors
				if (diagnostics.AnyError)
				{
					// Write diagnostics
					diagnostics.WriteDiagnostics(std::wcerr);
					diagnostics.Reset();

					// Remove the statement that caused error
					if (!interactiveBody->Statements.empty())
					{
						interactiveBody->Statements.pop_back();
					}

					continue;
				}

				// Execute statement
				ObjectInstance* result = AbstractInterpreter::ExecuteStatement(statement);
				if (result != nullptr)
				{
					ConsoleHelper::Write(result);
					GarbageCollector::DestroyInstance(result);
				}

				MoveToNewLineIfNeeded();
			}
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
