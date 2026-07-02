#include <string>
#include <iostream>
#include <vector>
#include <exception>

#include <ShardScript.hpp>
#include <InteractiveConsole.hpp>
#include <utilities/InterpreterUtilities.hpp>

using namespace shard;

static std::unique_ptr<MethodDeclarationSyntax> InitImplicitEntryPoint(SyntaxNode* parent)
{
	MemberDeclarationInfo info;
	info.ReturnType = std::make_unique<PredefinedTypeSyntax>(SyntaxToken(TokenType::VoidKeyword, L"void", TextLocation(), false), nullptr);
	info.Identifier = SyntaxToken(TokenType::Identifier, L"__interactive_console__", TextLocation());
	info.Modifiers = { SyntaxToken(TokenType::StaticKeyword, L"static", TextLocation(), false) };

	auto implMethod = std::make_unique<MethodDeclarationSyntax>(info, parent);
	implMethod->ParametersList = std::make_unique<ParametersListSyntax>(parent);
	implMethod->Body = std::make_unique<StatementsBlockSyntax>(implMethod.get());
	return implMethod;
}

static std::unique_ptr<ClassDeclarationSyntax> InitImplicitClassDeclaration(SyntaxNode* parent)
{
	MemberDeclarationInfo info;
	info.Identifier = SyntaxToken(TokenType::Identifier, L"__InteractiveClass__", TextLocation());

	auto implClass = std::make_unique<ClassDeclarationSyntax>(info, parent);
	implClass->DeclareToken = SyntaxToken(TokenType::ClassKeyword, L"class", TextLocation(), false);

	auto entryPoint = InitImplicitEntryPoint(implClass.get());
	implClass->Members.push_back(std::move(entryPoint));

	return implClass;
}

static std::unique_ptr<NamespaceDeclarationSyntax> InitImplicitNamespaceDeclaration(SyntaxNode* parent)
{
	auto implNamespace = std::make_unique<NamespaceDeclarationSyntax>(parent);
	implNamespace->DeclareToken = SyntaxToken(TokenType::NamespaceKeyword, L"namespace", TextLocation(), false);
	implNamespace->IdentifierTokens.push_back(SyntaxToken(TokenType::Identifier, L"__InteractiveNamespace__", TextLocation(), false));

	return implNamespace;
}

static CompilationUnitSyntax* InitImplicitCompilationUnit()
{
	CompilationUnitSyntax* implUnit = new CompilationUnitSyntax();
	implUnit->Namespace = InitImplicitNamespaceDeclaration(implUnit);
	implUnit->Members.push_back(std::unique_ptr<MemberDeclarationSyntax>(InitImplicitClassDeclaration(implUnit)));
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
	if (!std::getline(std::wcin, line))
	{
		std::wcin.clear();
		std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), L'\n');
		return L"exit";
	}

	return line;
}

static void MoveToNewLineIfNeeded()
{
	/*
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		if (csbi.dwCursorPosition.X > 0)
			ConsoleHelper::WriteLine();
	}
	*/
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
		ExpressionStatementSyntax* exprStatement = new ExpressionStatementSyntax(InteractiveMethod->Body.get());
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
		return Parser.ReadKeywordStatement(sequenceReader, InteractiveMethod->Body.get()).release();
	}

	// Read as statement
	return Parser.ReadStatement(sequenceReader, InteractiveMethod->Body.get()).release();
}

MemberDeclarationSyntax* InteractiveConsole::ReadMember(LexicalBuffer& sequenceReader)
{
	if (sequenceReader.Size() == 0)
		return nullptr;

	ReadMultilineInput(sequenceReader, false);
	MemberDeclarationSyntax* member = Parser.ReadMemberDeclaration(sequenceReader, InteractiveClass).release();

	if (member->Kind != SyntaxKind::MethodDeclaration)
		Diagnostics.ReportError(member->DeclareToken, L"Only methods compilation supported");

	return member;
}

void InteractiveConsole::EvaluateUsing(LexicalBuffer& buffer)
{
	auto directive = Parser.ReadUsingDirective(buffer, nullptr);
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
	for (const auto& symbol : node->Members)
	{
		compilationContext->GetSemanticAnalyzer().AddSymbol(symbol);
		std::wcout << symbol->Name << L", ";
		counter += 1;
	}

	std::wcout << "(" << counter << " symbols)" << std::endl;
}

InteractiveConsole::InteractiveConsole(shard::CompilationContext* context, shard::ApplicationDomain* domain) :
	compilationContext(context),
	applicationDomain(domain),
	ParentSyntaxTree(context->GetSyntaxTree()),
	ParentSemanticModel(context->GetSemanticModel()),
	Diagnostics(context->GetDiagnosticsContext()),
	Parser(context->GetParser()),
	Semanter(context->GetSemanticAnalyzer()),
	Layouter(context->GetLayoutGenerator()),
	Runtimer(domain->GetVirtualMachine()),
	Program(domain->GetProgram())
{
	InteractiveUnit = InitImplicitCompilationUnit();
	InteractiveClass = static_cast<ClassDeclarationSyntax*>(InteractiveUnit->Members.at(0).get());
	InteractiveMethod = static_cast<MethodDeclarationSyntax*>(InteractiveClass->Members.at(0).get());
	ParentSyntaxTree.CompilationUnits.push_back(std::unique_ptr<CompilationUnitSyntax>(InteractiveUnit));
	
	//InteractiveEntryPoint = new MethodSymbol(InteractiveMethod->IdentifierToken.Word);
	//Program.EntryPoint = InteractiveEntryPoint;

	//Runtimer.PushFrame(InteractiveEntryPoint);
}

void InteractiveConsole::Run()
{
	ConsoleHelper::WriteLine(L"ShardScript Interactive Console v0.2.0");
	ConsoleHelper::WriteLine(L"Type 'exit' or 'quit' to exit");
	ConsoleHelper::WriteLine();
	bool pushedFrame = false;

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
				bool isMemberDecl = IsMemberDeclaration(firstToken.Type, secondToken.Type);
				if (firstToken.Type == TokenType::Identifier && secondToken.Type == TokenType::Colon)
					isMemberDecl = false;

				if (isMemberDecl)
				{
					MemberDeclarationSyntax* member = ReadMember(sequenceReader);
					if (member == nullptr)
						continue;

					MethodDeclarationSyntax* method = static_cast<MethodDeclarationSyntax*>(member);
					InteractiveClass->Members.push_back(std::unique_ptr<MemberDeclarationSyntax>(member));

					// Re-analyze syntax tree
					Semanter.Analyze(ParentSyntaxTree, ParentSemanticModel);
					Layouter.Generate(ParentSemanticModel);

					// Check for errors
					if (Diagnostics.AnyError)
					{
						Diagnostics.WriteDiagnostics(std::wcerr);
						Diagnostics.Reset();

						InteractiveClass->Members.pop_back();
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

			InteractiveMethod->Body->Statements.push_back(std::unique_ptr<StatementSyntax>(statement));

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

			if (!pushedFrame)
			{
				InteractiveEntryPoint = static_cast<MethodSymbol*>(ParentSemanticModel.Table->LookupSymbol(InteractiveMethod).value_or(nullptr));
				Program.EntryPoint = InteractiveEntryPoint;
				Runtimer.PushFrame(InteractiveEntryPoint);
				pushedFrame = true;
			}

			AbstractEmiter abstractEmiter(Program, ParentSemanticModel, Diagnostics);
			abstractEmiter.SetGeneratingTarget(InteractiveEntryPoint);
			abstractEmiter.VisitStatement(statement);

			ObjectInstance* result = Runtimer.RunInteractive(Breakpoint);
			if (result != nullptr)
			{
				ConsoleHelper::Write(result);
				applicationDomain->GetGarbageCollector().CollectInstance(result);
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
