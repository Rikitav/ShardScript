#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SourceReader.h>
#include <shard/parsing/structures/MemberDeclarationInfo.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/BlockDeclarationSyntax.h>
#include <shard/syntax/nodes/MethodBodySyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/TypeDeclarations.h>
#include <shard/syntax/nodes/Expressions.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxFacts.h>

#include <shard/syntax/analysis/DiagnosticsContext.h>
#include <shard/syntax/structures/SyntaxTree.h>

#include <memory>
#include <vector>
#include <set>
#include <shard/syntax/nodes/Statements.h>
#include <shard/syntax/SyntaxKind.h>

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
			case TokenType::UsingKeyword:
			{
				shared_ptr<UsingDirectiveSyntax> directive = ReadUsingDirective(reader);
				unit->Usings.push_back(directive);
				break;
			}

			case TokenType::NamespaceKeyword:
			{
				shared_ptr<NamespaceDeclarationSyntax> syntax = ReadNamespaceDeclaration(reader);
				unit->Members.push_back(syntax);
				break;
			}

			default:
			{
				if (IsMemberDeclaration(token.Type))
				{
					shared_ptr<MemberDeclarationSyntax> syntax = ReadMemberDeclaration(reader);
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
	reader.Consume();
	return shared_ptr<UsingDirectiveSyntax>();
}

shared_ptr<NamespaceDeclarationSyntax> LexicalAnalyzer::ReadNamespaceDeclaration(SourceReader& reader)
{
	shared_ptr<NamespaceDeclarationSyntax> syntax = make_shared<NamespaceDeclarationSyntax>();
	syntax->DeclareKeyword = Expect(reader, TokenType::NamespaceKeyword, "Expected namespace keyword");

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
		method->Body = ReadMethodBody(reader);

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
			break;
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
			shared_ptr<MemberDeclarationSyntax> member = ReadMemberDeclaration(reader);
			syntax->Members.push_back(member);
		}
		else
		{
			Diagnostics.ReportError(reader.Current(), "Unexpected token in type declaration");
			reader.Consume();
		}
	}
}

shared_ptr<MethodBodySyntax> LexicalAnalyzer::ReadMethodBody(SourceReader& reader)
{
	shared_ptr<MethodBodySyntax> syntax = make_shared<MethodBodySyntax>();
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, "Expected '{' token");

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		if (current.Type == TokenType::CloseBrace)
		{
			syntax->CloseBraceToken = current;
			reader.Consume();
			break;
		}
		else
		{
			shared_ptr<StatementSyntax> statement = ReadStatement(reader);
			syntax->Statements.push_back(statement);
			continue;
		}
	}

	return syntax;
}

shared_ptr<StatementSyntax> LexicalAnalyzer::ReadStatement(SourceReader& reader)
{
	SyntaxToken startToken = reader.Current();
	if (startToken.Type == TokenType::Semicolon)
	{
		shared_ptr<StatementSyntax> statement = make_shared<StatementSyntax>(SyntaxKind::Statement);
		statement->Semicolon = Expect(reader, TokenType::Semicolon, "Missing ';' token");
		return statement;
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
			shared_ptr<VariableStatementSyntax> statement = make_shared<VariableStatementSyntax>(startToken, followingToken, assignOp, expr);
			statement->Semicolon = Expect(reader, TokenType::Semicolon, "Missing ';' token");
			return statement;
		}
	}

	if (IsKeyword(startToken.Type))
	{
		shared_ptr<KeywordStatementSyntax> statement = ReadKeywordStatement(reader);
		statement->Semicolon = Expect(reader, TokenType::Semicolon, "Missing ';' token");
		return statement;
	}

	shared_ptr<ExpressionSyntax> expression = ReadExpression(reader, 0);
	shared_ptr<ExpressionStatementSyntax> statement = make_shared<ExpressionStatementSyntax>(expression);
	statement->Semicolon = Expect(reader, TokenType::Semicolon, "Missing ';' token");
	return statement;
}

shared_ptr<KeywordStatementSyntax> LexicalAnalyzer::ReadKeywordStatement(SourceReader& reader)
{
	return shared_ptr<KeywordStatementSyntax>();
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
	while (reader.CanConsume())
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

			default:
			{
				reader.Consume();
				Diagnostics.ReportError(current, "Unknown expression token");
				break;
			}
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
		if (IsBinaryArithmeticOperator(current.Type) || IsBinaryBooleanOperator(current.Type) || current.Type == TokenType::AssignOperator)
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

shared_ptr<ExpressionSyntax> LexicalAnalyzer::ReadMemberAccessExpression(SourceReader& reader)
{
	shared_ptr<MemberAccessExpressionSyntax> memberAccess = make_shared<MemberAccessExpressionSyntax>();
	while (reader.CanConsume())
	{
		SyntaxToken identifierToken = Expect(reader, TokenType::Identifier, "Expected identifier");
		memberAccess->Path.push_back(identifierToken);

		if (!reader.CanConsume())
			break;

		SyntaxToken separatorToken = reader.Current();
		if (separatorToken.Type != TokenType::Delimeter)
			break;

		reader.Consume();
	}

	if (reader.CanConsume() && reader.Current().Type == TokenType::OpenCurl)
	{
		shared_ptr<ArgumentsListSyntax> arguments = ReadArgumentsList(reader);
		return make_shared<InvokationExpressionSyntax>(memberAccess, arguments);
	}

	return memberAccess;
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
			break;
	}

	arguments->CloseCurlToken = Expect(reader, TokenType::CloseCurl, "Exprected ')' token");
	return arguments;
}
