#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SourceReader.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/BodyDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/TypeDeclarations.h>
#include <shard/syntax/nodes/Expressions.h>
#include <shard/syntax/nodes/Statements.h>
#include <shard/syntax/nodes/ImportDirectiveSyntax.h>
#include <shard/syntax/nodes/Loops.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/IndexatorListSyntax.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxFacts.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/analysis/TextLocation.h>

#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/structures/SyntaxTree.h>

#include <memory>
#include <vector>
#include <set>

using namespace std;
using namespace shard::parsing;
using namespace shard::syntax::structures;

LexicalAnalyzer::LexicalAnalyzer(shared_ptr<SyntaxTree> tree, DiagnosticsContext& diagnostics)
	: Tree(tree), Diagnostics(diagnostics) {}

void LexicalAnalyzer::FromSourceReader(SourceReader& reader)
{
	shared_ptr<CompilationUnitSyntax> unit = ReadCompilationUnit(reader);
	Tree->CompilationUnits.push_back(unit);
}

shared_ptr<CompilationUnitSyntax> LexicalAnalyzer::ReadCompilationUnit(SourceReader& reader)
{
	shared_ptr<CompilationUnitSyntax> unit = make_shared<CompilationUnitSyntax>();

	while (reader.CanConsume())
	{
		SyntaxToken token = reader.Current();
		switch (token.Type)
		{
			case TokenType::FromKeyword:
			{
				shared_ptr<ImportDirectiveSyntax> pImport = ReadImportDirective(reader);
				pImport->Parent = unit;
				unit->Imports.push_back(pImport);
				break;
			}

			case TokenType::UsingKeyword:
			{
				shared_ptr<UsingDirectiveSyntax> pDirective = ReadUsingDirective(reader);
				pDirective->Parent = unit;
				unit->Usings.push_back(pDirective);
				break;
			}

			case TokenType::NamespaceKeyword:
			{
				shared_ptr<NamespaceDeclarationSyntax> pNamespace = ReadNamespaceDeclaration(reader);
				pNamespace->Parent = unit;
				unit->Members.push_back(pNamespace);
				break;
			}

			default:
			{
				if (IsMemberDeclaration(token.Type))
				{
					shared_ptr<MemberDeclarationSyntax> pMember = ReadMemberDeclaration(reader);
					pMember->Parent = unit;
					unit->Members.push_back(pMember);
					break;
				}

				Diagnostics.ReportError(token, "Unknown token in compilation unit declaration");
				reader.Consume();
				break;
			}
		}
	}

	return unit;
}

shared_ptr<UsingDirectiveSyntax> LexicalAnalyzer::ReadUsingDirective(SourceReader& reader)
{
	shared_ptr<UsingDirectiveSyntax> syntax = make_shared<UsingDirectiveSyntax>();
	syntax->UsingKeyword = Expect(reader, TokenType::UsingKeyword, "Exprected 'using' keyword");

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		while (!Matches(reader, { TokenType::Identifier }))
		{
			Diagnostics.ReportError(current, "Expected identifier");
			current = reader.Consume();
		}

		syntax->Tokens.push_back(current);
		SyntaxToken separatorToken = reader.Consume();

		while (!Matches(reader, { TokenType::Delimeter, TokenType::Semicolon }))
		{
			Diagnostics.ReportError(separatorToken, "Expected separator token '.' or closing token ';'");
			separatorToken = reader.Consume();
		}

		if (separatorToken.Type == TokenType::Semicolon)
		{
			syntax->Semicolon = separatorToken;
			break;
		}
	}
	
	return syntax;
}

shared_ptr<ImportDirectiveSyntax> LexicalAnalyzer::ReadImportDirective(SourceReader& reader)
{
	shared_ptr<ImportDirectiveSyntax> syntax = make_shared<ImportDirectiveSyntax>();
	syntax->FromToken = Expect(reader, TokenType::FromKeyword, "Expected 'from' keyword");

	SyntaxToken current = reader.Current();
	while (!Matches(reader, { TokenType::StringLiteral }))
	{
		Diagnostics.ReportError(current, "Expected library path");
		current = reader.Consume();
	}

	syntax->LibPathToken = current;
	current = reader.Consume();

	while (!Matches(reader, { TokenType::ImportKeyword }))
	{
		Diagnostics.ReportError(current, "Expected 'import' keyword");
		current = reader.Consume();
	}

	syntax->ImportToken = current;
	current = reader.Consume();

	while (reader.CanConsume())
	{
		while (!Matches(reader, { TokenType::Identifier }))
		{
			Diagnostics.ReportError(current, "Expected identifier");
			current = reader.Consume();
		}

		syntax->Funtctions.push_back(current);
		SyntaxToken separatorToken = reader.Consume();

		while (!Matches(reader, { TokenType::Comma, TokenType::Semicolon }))
		{
			Diagnostics.ReportError(separatorToken, "Expected separator token ',' or closing token ';'");
			separatorToken = reader.Consume();
		}

		if (separatorToken.Type == TokenType::Semicolon)
		{
			syntax->Semicolon = separatorToken;
			break;
		}
	}

	return syntax;
}

shared_ptr<NamespaceDeclarationSyntax> LexicalAnalyzer::ReadNamespaceDeclaration(SourceReader& reader)
{
	shared_ptr<NamespaceDeclarationSyntax> syntax = make_shared<NamespaceDeclarationSyntax>();
	syntax->DeclareKeyword = Expect(reader, TokenType::NamespaceKeyword, "Expected 'namespace' keyword");

	// reading identifier
	while (!Matches(reader, { TokenType::Identifier }))
	{
		Diagnostics.ReportError(reader.Current(), "Expected namespace identifier");
		reader.Consume();
	}

	syntax->Identifier = reader.Current();
	reader.Consume();

	// reading separator
	while (!Matches(reader, { TokenType::OpenBrace, TokenType::Semicolon }))
	{
		Diagnostics.ReportError(reader.Current(), "Unknown token in member declaration");
		reader.Consume();
	}

	ReadTypeBody(reader, syntax);
	return syntax;
}

shared_ptr<MemberDeclarationSyntax> LexicalAnalyzer::ReadMemberDeclaration(SourceReader& reader)
{
	// Reading identifiers
	MemberDeclarationInfo info;
	info.Modifiers = ReadMemberModifiers(reader);

	// Reading member keyword or return type
	while (reader.CanConsume())
	{
		SyntaxToken token = reader.Current();
		if (IsMemberKeyword(token.Type))
		{
			info.DeclareType = token;
			reader.Consume();
			break;
		}
		else if (IsType(token.Type))
		{
			info.ReturnType = token;
			reader.Consume();
			break;
		}
		else
		{
			Diagnostics.ReportError(token, "Expected member declaration keyword");
			reader.Consume();
			continue;
		}
	}

	// reading identifier
	while (!Matches(reader, { TokenType::Identifier }))
	{
		Diagnostics.ReportError(reader.Current(), "Expected member identifier");
		reader.Consume();
	}

	info.Identifier = reader.Current();
	reader.Consume();

	// Reading parameters list
	if (!info.ReturnType.IsMissing)
	{
		while (!Matches(reader, { TokenType::OpenCurl }))
		{
			Diagnostics.ReportError(reader.Current(), "Expected parameters list");
			reader.Consume();
		}

		info.Params = ReadParametersList(reader);
	}

	// reading anchor token
	while (!Matches(reader, { TokenType::OpenBrace, TokenType::Semicolon }))
	{
		Diagnostics.ReportError(reader.Current(), "Unknown token in member declaration");
		reader.Consume();
	}

	if (!info.ReturnType.IsMissing)
	{
		shared_ptr<MethodDeclarationSyntax> method = make_shared<MethodDeclarationSyntax>(info);
		shared_ptr<StatementsBlockSyntax> body = ReadStatementsBlock(reader);
		body->Parent = method;
		method->Body = body;

		if (method->Identifier.Word == "Main")
		{
			if (Tree->EntryPoint != nullptr)
			{
				Diagnostics.ReportError(method->Identifier, "Multiple entry points");
			}
			else
			{
				Tree->EntryPoint = method;
			}
		}

		return method;
	}
	else if (!info.DeclareType.IsMissing)
	{
		shared_ptr<TypeDeclarationSyntax> type = make_type(info);
		ReadTypeBody(reader, type);
		return type;
	}
	else
	{
		return nullptr;
	}
}

vector<SyntaxToken> LexicalAnalyzer::ReadMemberModifiers(SourceReader& reader)
{
	vector<SyntaxToken> modifiers;
	set<TokenType> seenModifiers;

	// Expected order: access -> static -> abstract -> sealed -> partial
	static const vector<TokenType> expectedOrder =
	{
		TokenType::PublicKeyword,
		TokenType::PrivateKeyword,
		TokenType::ProtectedKeyword,
		TokenType::InternalKeyword,
		TokenType::StaticKeyword,
		TokenType::AbstractKeyword,
		TokenType::SealedKeyword,
		TokenType::PartialKeyword
	};

	int currentOrderIndex = 0;
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		TokenType currentType = current.Type;

		// Check if this is a valid modifier
		bool isValidModifier = false;
		for (TokenType expectedType : expectedOrder)
		{
			if (currentType == expectedType)
			{
				isValidModifier = true;
				break;
			}
		}

		if (!isValidModifier)
		{
			// Not a modifier, stop parsing modifiers
			break;
		}

		// Check for duplicate modifiers
		if (seenModifiers.count(currentType) > 0)
		{
			Diagnostics.ReportError(current, "Duplicate modifier");
			continue;
		}

		// Check order
		int modifierIndex = -1;
		for (size_t i = 0; i < expectedOrder.size(); i++)
		{
			if (expectedOrder[i] == currentType)
			{
				modifierIndex = static_cast<int>(i);
				break;
			}
		}

		reader.Consume();
		if (modifierIndex < currentOrderIndex)
		{
			// Modifier is out of order
			Diagnostics.ReportError(current, "Modifier out of order");
			modifiers.push_back(current);
			seenModifiers.insert(currentType);
		}
		else
		{
			// Valid modifier in correct order
			modifiers.push_back(current);
			seenModifiers.insert(currentType);
			currentOrderIndex = modifierIndex + 1;
		}
	}

	return modifiers;
}

shared_ptr<ParametersListSyntax> LexicalAnalyzer::ReadParametersList(SourceReader& reader)
{
	shared_ptr<ParametersListSyntax> syntax = make_shared<ParametersListSyntax>();
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "Expected '(' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseCurl)
	{
		syntax->CloseCurlToken = checkCloser;
		reader.Consume();
		return syntax;
	}

	while (reader.CanConsume())
	{
		SyntaxToken typeToken = reader.Current();
		while (reader.CanConsume() && !IsType(typeToken.Type) && typeToken.Type != TokenType::CloseCurl)
		{
			Diagnostics.ReportError(typeToken, "Expected type");
			typeToken = reader.Consume();
		}

		if (typeToken.Type == TokenType::CloseCurl)
		{
			Diagnostics.ReportError(typeToken, "Enexpected closer token");
			break;
		}

		SyntaxToken identifierToken = reader.Current();
		while (reader.CanConsume() && identifierToken.Type != TokenType::Identifier && typeToken.Type != TokenType::CloseCurl)
		{
			Diagnostics.ReportError(typeToken, "Expected name");
			identifierToken = reader.Consume();
		}

		if (typeToken.Type == TokenType::CloseCurl)
		{
			Diagnostics.ReportError(typeToken, "Enexpected closer token");
			break;
		}

		shared_ptr<ParameterSyntax> param = make_shared<ParameterSyntax>(typeToken, identifierToken);
		syntax->Parameters.push_back(param);

		SyntaxToken separatorToken = reader.Current();
		while (reader.CanConsume() && identifierToken.Type != TokenType::CloseCurl && identifierToken.Type != TokenType::Comma)
		{
			Diagnostics.ReportError(separatorToken, "Expected separator token ',' or closing token ')'");
			separatorToken = reader.Consume();
		}

		if (separatorToken.Type == TokenType::CloseCurl)
		{
			syntax->CloseCurlToken = separatorToken;
			break;
		}
	}

	return syntax;
}

void LexicalAnalyzer::ReadTypeBody(SourceReader& reader, shared_ptr<TypeDeclarationSyntax> syntax)
{
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, "Expected '{'");

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		if (current.Type == TokenType::CloseBrace)
		{
			syntax->CloseBraceToken = current;
			reader.Consume();
			break;
		}

		if (IsMemberDeclaration(reader.Current().Type))
		{
			shared_ptr<MemberDeclarationSyntax> pMember = ReadMemberDeclaration(reader);
			pMember->Parent = pMember;
			syntax->Members.push_back(pMember);
		}
		else
		{
			Diagnostics.ReportError(reader.Current(), "Unexpected token in type declaration");
			reader.Consume();
		}
	}
}

shared_ptr<StatementsBlockSyntax> LexicalAnalyzer::ReadStatementsBlock(SourceReader& reader)
{
	shared_ptr<StatementsBlockSyntax> syntax = make_shared<StatementsBlockSyntax>();

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		// Empty block
		syntax->Semicolon;
	}
	else if (current.Type == TokenType::OpenBrace)
	{
		syntax->OpenBraceToken = current;
		current = reader.Consume();

		while (reader.CanConsume())
		{
			current = reader.Current();
			if (current.Type == TokenType::CloseBrace)
			{
				syntax->CloseBraceToken = current;
				current = reader.Consume();
				break;
			}
			else if (IsKeyword(current.Type))
			{
				shared_ptr<KeywordStatementSyntax> statement = ReadKeywordStatement(reader);
				syntax->Statements.push_back(statement);
				continue;
			}
			else
			{
				shared_ptr<StatementSyntax> statement = ReadStatement(reader);
				statement->Semicolon = Expect(reader, TokenType::Semicolon, "Missing ';' token");
				syntax->Statements.push_back(statement);
				continue;
			}
		}
	}
	else
	{
		// Single statement block
		shared_ptr<StatementSyntax> statement = ReadStatement(reader);
		statement->Semicolon = Expect(reader, TokenType::Semicolon, "Missing ';' token");
		syntax->Statements.push_back(statement);
	}

	return syntax;
}

shared_ptr<StatementSyntax> LexicalAnalyzer::ReadStatement(SourceReader& reader)
{
	SyntaxToken startToken = reader.Current();
	if (startToken.Type == TokenType::Semicolon)
	{
		return make_shared<StatementSyntax>(SyntaxKind::Statement);
	}

	if (IsType(startToken.Type))
	{
		if (!reader.CanPeek())
			return nullptr;

		SyntaxToken followingToken = reader.Peek();
		if (followingToken.Type == TokenType::Identifier)
		{
			reader.Consume(); // Id
			reader.Consume(); // Id

			SyntaxToken assignOp = Expect(reader, TokenType::AssignOperator, "Missing '=' token");
			shared_ptr<ExpressionSyntax> expr = ReadExpression(reader, 0);
			return make_shared<VariableStatementSyntax>(startToken, followingToken, assignOp, expr);
		}
	}

	shared_ptr<ExpressionSyntax> expression = ReadExpression(reader, 0);
	return make_shared<ExpressionStatementSyntax>(expression);
}

shared_ptr<KeywordStatementSyntax> LexicalAnalyzer::ReadKeywordStatement(SourceReader& reader)
{
	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::ForKeyword:
		{
			return ReadForStatement(reader);
		}

		case TokenType::ReturnKeyword:
		{
			return ReeadReturnStatement(reader);
		}

		default:
		{
			Diagnostics.ReportError(current, "unknown/unsupported keyword");
			return nullptr;
		}
	}
}

shared_ptr<ReturnStatementSyntax> LexicalAnalyzer::ReeadReturnStatement(SourceReader& reader)
{
	shared_ptr<ReturnStatementSyntax> syntax = make_shared<ReturnStatementSyntax>();
	syntax->Keyword = Expect(reader, TokenType::ReturnKeyword, "Expected return keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, 0);

	syntax->Semicolon = Expect(reader, TokenType::Semicolon, "Missing ';' token");
	return syntax;
}

shared_ptr<ForStatementSyntax> LexicalAnalyzer::ReadForStatement(SourceReader& reader)
{
	shared_ptr<ForStatementSyntax> syntax = make_shared<ForStatementSyntax>();
	syntax->Semicolon = SyntaxToken(TokenType::Semicolon, "", TextLocation(), false);
	syntax->Keyword = Expect(reader, TokenType::ForKeyword, "Expected 'for' keyword");

	// Reading open curl token
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "expected '(' token");

	// Reading init statement
	syntax->InitializerStatement = ReadStatement(reader);
	if (auto keywordStatement = dynamic_pointer_cast<KeywordStatementSyntax>(syntax->InitializerStatement))
		Diagnostics.ReportError(keywordStatement->Keyword, "Cannot use keyword statements inside for loop initializer");

	// Reading first semicolon
	syntax->FirstSemicolon = Expect(reader, TokenType::Semicolon, "expected ';' token");

	// Reading looping consition
	syntax->ConditionExpression = ReadExpression(reader, 0);

	// Reading second semicolon
	syntax->SecondSemicolon = Expect(reader, TokenType::Semicolon, "expected ';' token");

	// Reading after loop statement
	syntax->AfterRepeatStatement = ReadStatement(reader);
	if (auto keywordStatement = dynamic_pointer_cast<KeywordStatementSyntax>(syntax->AfterRepeatStatement))
		Diagnostics.ReportError(keywordStatement->Keyword, "Cannot use keyword statements inside for loop repeater");

	// Reading close curl token
    syntax->OpenCurlToken = Expect(reader, TokenType::CloseCurl, "expected ')' token");
	
	// Reading block
	syntax->Block = ReadStatementsBlock(reader);
	syntax->Block->Parent = syntax;

	return syntax;
}

shared_ptr<ExpressionSyntax> LexicalAnalyzer::ReadExpression(SourceReader& reader, int bindingPower)
{
	shared_ptr<ExpressionSyntax> leftExpr = ReadNullDenotation(reader);
	if (!reader.CanConsume())
		return leftExpr;

	SyntaxToken current = reader.Current();
	int precendence = GetOperatorPrecendence(current.Type);
	
	while (precendence != 0 && bindingPower < precendence)
	{
		leftExpr = ReadLeftDenotation(reader, leftExpr);
		current = reader.Current();
		precendence = GetOperatorPrecendence(current.Type);
	}

	return leftExpr;
}

shared_ptr<ExpressionSyntax> LexicalAnalyzer::ReadNullDenotation(SourceReader& reader)
{
	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::BooleanLiteral:
		case TokenType::StringLiteral:
		case TokenType::NumberLiteral:
		{
			reader.Consume();
			return make_shared<ConstValueExpressionSyntax>(current);
		}

		case TokenType::OpenCurl:
		{
			reader.Consume();
			shared_ptr<ExpressionSyntax> expression = ReadExpression(reader, 0);
			Expect(reader, TokenType::CloseCurl, "Expected ')' token");
			return expression;
		}

		case TokenType::Identifier:
		{
			return ReadMemberAccessExpression(reader);
		}

		case TokenType::EndOfFile:
		{
			//Diagnostics.ReportError(current, "Unexpected file end");
			return nullptr;
		}

		default:
		{
			reader.Consume();
			Diagnostics.ReportError(current, "Unknown expression token");
			break;
		}
	}
	
	return nullptr;
}

shared_ptr<ExpressionSyntax> LexicalAnalyzer::ReadLeftDenotation(SourceReader& reader, shared_ptr<ExpressionSyntax> leftExpr)
{
	if (!reader.CanConsume())
		return nullptr;

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		if (IsOperator(current.Type) || current.Type == TokenType::AssignOperator)
		{
			int precendce = GetOperatorPrecendence(current.Type);
			if (precendce == 0)
				return leftExpr;

			reader.Consume();
			shared_ptr<ExpressionSyntax> rightExpr = ReadExpression(reader, precendce);
			return make_shared<BinaryExpressionSyntax>(leftExpr, current, rightExpr);
		}

		/*
		switch (current.Type)
		{

		}
		*/

		Diagnostics.ReportError(current, "Unknown token in expression's left denotation");
		reader.Consume();
	}

	return nullptr;
}

shared_ptr<MemberAccessExpressionSyntax> LexicalAnalyzer::ReadMemberAccessExpression(SourceReader& reader)
{
	SyntaxToken identifier = Expect(reader, TokenType::Identifier, "Expected identifier");
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		switch (current.Type)
		{
			case TokenType::Delimeter:
			{
				shared_ptr<MemberAccessExpressionSyntax> syntax = make_shared<FieldAccesExpressionSyntax>();
				syntax->IdentifierToken = identifier;
				syntax->DelimeterToken = current;
				reader.Consume();

				syntax->NextAccess = ReadMemberAccessExpression(reader);
				return syntax;
			}

			case TokenType::OpenSquare:
			{
				shared_ptr<IndexatorExpressionSyntax> syntax = make_shared<IndexatorExpressionSyntax>();
				syntax->IdentifierToken = identifier;
				syntax->IndexatorList = ReadIndexatorList(reader);

				current = reader.Current();
				if (current.Type == TokenType::Delimeter)
				{
					syntax->DelimeterToken = current;
					reader.Consume();
					syntax->NextAccess = ReadMemberAccessExpression(reader);
				}

				return syntax;
			}

			case TokenType::OpenCurl:
			{
				shared_ptr<InvokationExpressionSyntax> syntax = make_shared<InvokationExpressionSyntax>();
				syntax->IdentifierToken = identifier;
				syntax->ArgumentsList = ReadArgumentsList(reader);

				current = reader.Current();
				if (current.Type == TokenType::Delimeter)
				{
					syntax->DelimeterToken = current;
					reader.Consume();
					syntax->NextAccess = ReadMemberAccessExpression(reader);
				}

				return syntax;
			}

			default:
			{
				shared_ptr<MemberAccessExpressionSyntax> syntax = make_shared<FieldAccesExpressionSyntax>();
				syntax->IdentifierToken = identifier;
				return syntax;
			}
		}
	}

	return nullptr;
}

shared_ptr<ArgumentsListSyntax> LexicalAnalyzer::ReadArgumentsList(SourceReader& reader)
{
	shared_ptr<ArgumentsListSyntax> arguments = make_shared<ArgumentsListSyntax>();
	arguments->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "Exprected '(' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseCurl)
	{
		arguments->CloseCurlToken = checkCloser;
		reader.Consume();
		return arguments;
	}

	while (reader.CanConsume())
	{
		shared_ptr<ExpressionSyntax> expr = ReadExpression(reader, 0);
		shared_ptr<ArgumentSyntax> argument = make_shared<ArgumentSyntax>(expr);
		arguments->Arguments.push_back(argument);

		SyntaxToken separatorToken = reader.Current();
		while (!Matches(reader, { TokenType::Comma, TokenType::CloseCurl }))
		{
			Diagnostics.ReportError(separatorToken, "Expected separator token ',' or closing token ')'");
			separatorToken = reader.Consume();
		}

		if (separatorToken.Type == TokenType::CloseCurl)
		{
			arguments->CloseCurlToken = separatorToken;
			reader.Consume();
			break;
		}
	}

	return arguments;
}

shared_ptr<IndexatorListSyntax> LexicalAnalyzer::ReadIndexatorList(SourceReader& reader)
{
	shared_ptr<IndexatorListSyntax> arguments = make_shared<IndexatorListSyntax>();
	arguments->OpenSquareToken = Expect(reader, TokenType::OpenSquare, "Exprected '[' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseCurl)
	{
		arguments->CloseSquareToken = checkCloser;
		reader.Consume();
		return arguments;
	}

	while (reader.CanConsume())
	{
		shared_ptr<ExpressionSyntax> expr = ReadExpression(reader, 0);
		arguments->Arguments.push_back(expr);

		SyntaxToken separatorToken = reader.Current();
		while (!Matches(reader, { TokenType::Comma, TokenType::CloseSquare }))
		{
			Diagnostics.ReportError(separatorToken, "Expected separator token ',' or closing token ']'");
			separatorToken = reader.Consume();
		}

		if (separatorToken.Type == TokenType::CloseSquare)
		{
			arguments->CloseSquareToken = separatorToken;
			reader.Consume();
			break;
		}
	}

	return arguments;
}
