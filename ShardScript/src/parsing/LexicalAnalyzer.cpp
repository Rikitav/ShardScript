#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/lexical/MemberDeclarationInfo.h>
#include <shard/parsing/lexical/SyntaxTree.h>

#include <shard/syntax/nodes/CompilationUnitSyntax.h>
#include <shard/syntax/nodes/TypeDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ParametersListSyntax.h>
#include <shard/syntax/nodes/BodyDeclarationSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxFacts.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/parsing/analysis/TextLocation.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.h>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.h>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.h>
#include <shard/syntax/nodes/Directives/ImportDirectiveSyntax.h>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.h>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.h>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.h>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.h>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.h>

#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.h>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.h>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
#include <shard/syntax/nodes/Types/NullableTypeSyntax.h>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>

#include <vector>
#include <set>
#include <initializer_list>
#include <stdexcept>
#include <new>

using namespace std;
using namespace shard::syntax;
using namespace shard::syntax::nodes;
using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::lexical;

void LexicalAnalyzer::FromSourceReader(SyntaxTree& syntaxTree, SourceReader& reader)
{
	CompilationUnitSyntax* unit = ReadCompilationUnit(reader);
	syntaxTree.CompilationUnits.push_back(unit);
}

CompilationUnitSyntax* LexicalAnalyzer::ReadCompilationUnit(SourceReader& reader)
{
	CompilationUnitSyntax* unit = new CompilationUnitSyntax();

	while (reader.CanConsume())
	{
		SyntaxToken token = reader.Current();
		switch (token.Type)
		{
			case TokenType::FromKeyword:
			{
				ImportDirectiveSyntax* pImport = ReadImportDirective(reader, unit);
				pImport->Parent = unit;
				unit->Imports.push_back(pImport);
				break;
			}

			case TokenType::UsingKeyword:
			{
				UsingDirectiveSyntax* pDirective = ReadUsingDirective(reader, unit);
				pDirective->Parent = unit;
				unit->Usings.push_back(pDirective);
				break;
			}

			case TokenType::NamespaceKeyword:
			{
				NamespaceDeclarationSyntax* pNamespace = ReadNamespaceDeclaration(reader, unit);
				pNamespace->Parent = unit;
				unit->Members.push_back(pNamespace);
				break;
			}

			default:
			{
				SyntaxToken peek = reader.Peek();
				if (IsMemberDeclaration(token.Type, peek.Type))
				{
					MemberDeclarationSyntax* pMember = ReadMemberDeclaration(reader, unit);
					pMember->Parent = unit;
					//Diagnostics.ReportError(token, "Compilation unit cannot have members other than namespaces");
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

UsingDirectiveSyntax* LexicalAnalyzer::ReadUsingDirective(SourceReader& reader, SyntaxNode* parent)
{
	UsingDirectiveSyntax* syntax = new UsingDirectiveSyntax(parent);
	syntax->UsingKeywordToken = Expect(reader, TokenType::UsingKeyword, "Exprected 'using' keyword");

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		while (!Matches(reader, { TokenType::Identifier }))
		{
			Diagnostics.ReportError(current, "Expected identifier");
			current = reader.Consume();
		}

		syntax->TokensList.push_back(current);
		SyntaxToken separatorToken = reader.Consume();

		while (!Matches(reader, { TokenType::Delimeter, TokenType::Semicolon }))
		{
			Diagnostics.ReportError(separatorToken, "Expected separator token '.' or closing token ';'");
			separatorToken = reader.Consume();
		}

		if (separatorToken.Type == TokenType::Semicolon)
		{
			syntax->SemicolonToken = separatorToken;
			break;
		}
	}
	
	return syntax;
}

ImportDirectiveSyntax* LexicalAnalyzer::ReadImportDirective(SourceReader& reader, SyntaxNode* parent)
{
	ImportDirectiveSyntax* syntax = new ImportDirectiveSyntax(parent);
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

		syntax->FunctionsList.push_back(current);
		SyntaxToken separatorToken = reader.Consume();

		while (!Matches(reader, { TokenType::Comma, TokenType::Semicolon }))
		{
			Diagnostics.ReportError(separatorToken, "Expected separator token ',' or closing token ';'");
			separatorToken = reader.Consume();
		}

		if (separatorToken.Type == TokenType::Semicolon)
		{
			syntax->SemicolonToken = separatorToken;
			break;
		}
	}

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, "Expected ';' token");
	return syntax;
}

NamespaceDeclarationSyntax* LexicalAnalyzer::ReadNamespaceDeclaration(SourceReader& reader, SyntaxNode* parent)
{
	NamespaceDeclarationSyntax* syntax = new NamespaceDeclarationSyntax(parent);
	syntax->DeclareToken = Expect(reader, TokenType::NamespaceKeyword, "Expected 'namespace' keyword");

	// reading identifier
	while (!Matches(reader, { TokenType::Identifier }))
	{
		Diagnostics.ReportError(reader.Current(), "Expected namespace identifier");
		reader.Consume();
	}

	syntax->IdentifierToken = reader.Current();
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

MemberDeclarationSyntax* LexicalAnalyzer::ReadMemberDeclaration(SourceReader& reader, SyntaxNode* parent)
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
		
		SyntaxToken peek = reader.Peek();
		if (IsType(token.Type, peek.Type))
		{
			info.ReturnType = ReadType(reader, parent);
			//reader.Consume();
			break;
		}

		Diagnostics.ReportError(token, "Expected member declaration keyword");
		reader.Consume();
		continue;
	}

	// reading identifier
	while (!Matches(reader, { TokenType::Identifier, TokenType::OpenCurl, TokenType::Semicolon, TokenType::AssignOperator }))
	{
		Diagnostics.ReportError(reader.Current(), "Expected member identifier");
		reader.Consume();
	}

	info.Identifier = reader.Current();
	reader.Consume();

	// Reading parameters list
	if (info.ReturnType != nullptr)
	{
		// Checking if member is field
		while (!Matches(reader, { TokenType::OpenCurl, TokenType::Semicolon, TokenType::AssignOperator }))
		{
			Diagnostics.ReportError(reader.Current(), "Expected parameters list or semicolon");
			reader.Consume();
		}

		while (reader.CanConsume())
		{
			SyntaxToken current = reader.Current();
			switch (current.Type)
			{
				case TokenType::Semicolon:
				{
					FieldDeclarationSyntax* syntax = new FieldDeclarationSyntax(info, parent);
					syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, "Expected ';' token");
					return syntax;
				}

				case TokenType::AssignOperator:
				{
					FieldDeclarationSyntax* syntax = new FieldDeclarationSyntax(info, parent);

					syntax->SemicolonToken = current;
					reader.Consume();
					return syntax;
				}

				case TokenType::OpenCurl:
				{
					info.Params = ReadParametersList(reader, parent);
					goto breakloop;
				}

				default:
				{
					Diagnostics.ReportError(current, "Expected parameters list or semicolon");
					reader.Consume();
				}
			}
		}

		breakloop:
		if (!reader.CanConsume())
			return nullptr;
	}

	// reading anchor token
	while (reader.CanConsume() && !Matches(reader, { TokenType::OpenBrace, TokenType::Semicolon }))
	{
		Diagnostics.ReportError(reader.Current(), "Unknown token in member declaration");
		reader.Consume();
	}

	if (!reader.CanConsume())
		return nullptr;

	if (info.ReturnType != nullptr)
	{
		MethodDeclarationSyntax* method = new MethodDeclarationSyntax(info, parent);
		method->Body = ReadStatementsBlock(reader, method);
		return method;
	}
	else if (!info.DeclareType.IsMissing)
	{
		TypeDeclarationSyntax* type = make_type(info, parent);
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

ParametersListSyntax* LexicalAnalyzer::ReadParametersList(SourceReader& reader, SyntaxNode* parent)
{
	ParametersListSyntax* syntax = new ParametersListSyntax(parent);
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
		TypeSyntax* type = ReadType(reader, syntax);

		SyntaxToken identifierToken = reader.Current();
		while (reader.CanConsume() && identifierToken.Type != TokenType::Identifier)
		{
			Diagnostics.ReportError(identifierToken, "Expected name");
			identifierToken = reader.Consume();
		}

		if (identifierToken.Type == TokenType::CloseCurl)
		{
			Diagnostics.ReportError(identifierToken, "Enexpected closer token");
			break;
		}

		ParameterSyntax* param = new ParameterSyntax(type, identifierToken, syntax);
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

void LexicalAnalyzer::ReadTypeBody(SourceReader& reader, TypeDeclarationSyntax* syntax)
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

		SyntaxToken peek = reader.Peek();
		if (IsMemberDeclaration(current.Type, peek.Type))
		{
			MemberDeclarationSyntax* pMember = ReadMemberDeclaration(reader, syntax);
			pMember->Parent = syntax;
			syntax->Members.push_back(pMember);
		}
		else
		{
			Diagnostics.ReportError(reader.Current(), "Unexpected token in type declaration");
			reader.Consume();
		}
	}
}

StatementsBlockSyntax* LexicalAnalyzer::ReadStatementsBlock(SourceReader& reader, SyntaxNode* parent)
{
	StatementsBlockSyntax* syntax = new StatementsBlockSyntax(parent);

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		// Empty block
		syntax->SemicolonToken;
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
				KeywordStatementSyntax* statement = ReadKeywordStatement(reader, syntax);
				syntax->Statements.push_back(statement);
				continue;
			}
			else
			{
				StatementSyntax* statement = ReadStatement(reader, syntax);
				statement->SemicolonToken = Expect(reader, TokenType::Semicolon, "Missing ';' token");
				syntax->Statements.push_back(statement);
				continue;
			}
		}
	}
	else
	{
		// Single statement block
		if (IsKeyword(current.Type))
		{
			KeywordStatementSyntax* statement = ReadKeywordStatement(reader, syntax);
			syntax->Statements.push_back(statement);
		}
		else
		{
			StatementSyntax* statement = ReadStatement(reader, syntax);
			statement->SemicolonToken = Expect(reader, TokenType::Semicolon, "Missing ';' token");
			syntax->Statements.push_back(statement);
		}
	}

	return syntax;
}

StatementSyntax* LexicalAnalyzer::ReadStatement(SourceReader& reader, SyntaxNode* parent)
{
	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		return new StatementSyntax(SyntaxKind::ExpressionStatement, parent);
	}
	
	if (reader.CanPeek())
	{
		SyntaxToken peek = reader.Peek();
		if (IsType(current.Type, peek.Type))
		{
			TypeSyntax* type = ReadType(reader, parent);
			if (reader.CanPeek())
			{
				current = reader.Current();
				peek = reader.Peek();

				if (current.Type == TokenType::Identifier && peek.Type == TokenType::AssignOperator)
				{
					reader.Consume(); // Id
					reader.Consume(); // =

					//SyntaxToken assignOp = Expect(reader, TokenType::AssignOperator, "Missing '=' token");
					ExpressionSyntax* expr = ReadExpression(reader, parent, 0);
					return new VariableStatementSyntax(type, current, peek, expr, parent);
				}
			}
		}
	}

	ExpressionSyntax* expression = ReadExpression(reader, parent, 0);
	return new ExpressionStatementSyntax(expression, parent);
}

KeywordStatementSyntax* LexicalAnalyzer::ReadKeywordStatement(SourceReader& reader, SyntaxNode* parent)
{
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		switch (current.Type)
		{
			case TokenType::ForKeyword:
				return ReadForStatement(reader, parent);

			case TokenType::WhileKeyword:
				return ReadWhileStatement(reader, parent);

			case TokenType::UntilKeyword:
				return ReadUntilStatement(reader, parent);

			case TokenType::ReturnKeyword:
				return ReadReturnStatement(reader, parent);

			case TokenType::BreakKeyword:
				return ReadBreakStatement(reader, parent);

			case TokenType::ContinueKeyword:
				return ReadContinueStatement(reader, parent);

			case TokenType::IfKeyword:
			case TokenType::UnlessKeyword:
			case TokenType::ElseKeyword:
				return ReadConditionalClause(reader, parent);

			default:
			{
				Diagnostics.ReportError(current, "unknown/unsupported keyword");
				continue;
			}
		}
	}
}

ReturnStatementSyntax* LexicalAnalyzer::ReadReturnStatement(SourceReader& reader, SyntaxNode* parent)
{
	ReturnStatementSyntax* syntax = new ReturnStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ReturnKeyword, "Expected return keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, parent, 0);

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, "Missing ';' token");
	return syntax;
}

ThrowStatementSyntax* LexicalAnalyzer::ReadThrowStatement(SourceReader& reader, SyntaxNode* parent)
{
	ThrowStatementSyntax* syntax = new ThrowStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ReturnKeyword, "Expected return keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, parent, 0);

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, "Missing ';' token");
	return syntax;
}

BreakStatementSyntax* LexicalAnalyzer::ReadBreakStatement(SourceReader& reader, SyntaxNode* parent)
{
	BreakStatementSyntax* syntax = new BreakStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::BreakKeyword, "Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, "Missing ';' token");
	return syntax;
}

ContinueStatementSyntax* LexicalAnalyzer::ReadContinueStatement(SourceReader& reader, SyntaxNode* parent)
{
	ContinueStatementSyntax* syntax = new ContinueStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ContinueKeyword, "Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, "Missing ';' token");
	return syntax;
}

ConditionalClauseBaseSyntax* LexicalAnalyzer::ReadConditionalClause(SourceReader& reader, SyntaxNode* parent)
{
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		switch (current.Type)
		{
			case TokenType::IfKeyword:
			{
				IfStatementSyntax* syntax = new IfStatementSyntax(parent);
				syntax->KeywordToken = current;
				reader.Consume();

				syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "Expected '(' token");
				syntax->ConditionExpression = ReadStatement(reader, syntax);
				syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, "Expected ')' token");
				syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

				TokenType nextType = reader.Current().Type;
				if (nextType == TokenType::ElseKeyword)
					syntax->NextStatement = ReadConditionalClause(reader, syntax);

				return syntax;
			}

			case TokenType::UnlessKeyword:
			{
				UnlessStatementSyntax* syntax = new UnlessStatementSyntax(parent);
				syntax->KeywordToken = current;
				reader.Consume();

				syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "Expected '(' token");
				syntax->ConditionExpression = ReadStatement(reader, syntax);
				syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, "Expected ')' token");
				syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

				TokenType nextType = reader.Current().Type;
				if (nextType == TokenType::ElseKeyword)
					syntax->NextStatement = ReadConditionalClause(reader, syntax);

				return syntax;
			}

			case TokenType::ElseKeyword:
			{
				SyntaxToken elseKeyword = current;
				current = reader.Consume();
				switch (current.Type)
				{
					case TokenType::IfKeyword:
					case TokenType::UnlessKeyword:
						return ReadConditionalClause(reader, parent);

					case TokenType::OpenBrace:
					{
						ElseStatementSyntax* syntax = new ElseStatementSyntax(parent);
						syntax->KeywordToken = elseKeyword;
						syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);
						return syntax;
					}
				}
			}

			default:
			{
				Diagnostics.ReportError(current, "Unknown token in deside statement");
				continue;
			}
		}
	}

	return nullptr;
}

WhileStatementSyntax* LexicalAnalyzer::ReadWhileStatement(SourceReader& reader, SyntaxNode* parent)
{
	WhileStatementSyntax* syntax = new WhileStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::WhileKeyword, "Expected 'while' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "expected '(' token");
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, "expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

	return syntax;
}

UntilStatementSyntax* LexicalAnalyzer::ReadUntilStatement(SourceReader& reader, SyntaxNode* parent)
{
	UntilStatementSyntax* syntax = new UntilStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::UntilKeyword, "Expected 'until' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "expected '(' token");
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, "expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

	return syntax;
}

ForStatementSyntax* LexicalAnalyzer::ReadForStatement(SourceReader& reader, SyntaxNode* parent)
{
	ForStatementSyntax* syntax = new ForStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::ForKeyword, "Expected 'for' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "expected '(' token");

	// Reading init statement
	syntax->InitializerStatement = ReadStatement(reader, syntax);
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax*>(syntax->InitializerStatement))
		Diagnostics.ReportError(keywordStatement->KeywordToken, "Cannot use keyword statements inside for loop initializer");

	// Reading first semicolon
	syntax->FirstSemicolon = Expect(reader, TokenType::Semicolon, "expected ';' token");

	// Reading looping consition
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);

	// Reading second semicolon
	syntax->SecondSemicolon = Expect(reader, TokenType::Semicolon, "expected ';' token");

	// Reading after loop statement
	syntax->AfterRepeatStatement = ReadStatement(reader, syntax);
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax*>(syntax->AfterRepeatStatement))
		Diagnostics.ReportError(keywordStatement->KeywordToken, "Cannot use keyword statements inside for loop repeater");

	// Reading close curl token
    syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, "expected ')' token");
	
	// Reading block
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);
	return syntax;
}

ExpressionSyntax* LexicalAnalyzer::ReadExpression(SourceReader& reader, SyntaxNode* parent, int bindingPower)
{
	ExpressionSyntax* leftExpr = ReadNullDenotation(reader, parent);
	if (!reader.CanConsume())
		return leftExpr;

	SyntaxToken current = reader.Current();
	int precendence = GetOperatorPrecendence(current.Type);
	
	while (precendence != 0 && bindingPower < precendence)
	{
		leftExpr = ReadLeftDenotation(reader, parent, leftExpr);
		current = reader.Current();
		precendence = GetOperatorPrecendence(current.Type);
	}

	return leftExpr;
}

ExpressionSyntax* LexicalAnalyzer::ReadNullDenotation(SourceReader& reader, SyntaxNode* parent)
{
	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::SubOperator:
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		{
			reader.Consume();
			UnaryExpressionSyntax* syntax = new UnaryExpressionSyntax(current, false, parent);
			syntax->Expression = ReadNullDenotation(reader, syntax);
			return syntax;
		}

		case TokenType::BooleanLiteral:
		case TokenType::StringLiteral:
		case TokenType::NumberLiteral:
		{
			reader.Consume();
			return new LiteralExpressionSyntax(current, parent);
		}

		case TokenType::OpenCurl:
		{
			reader.Consume();
			ExpressionSyntax* expression = ReadExpression(reader, parent, 0);
			Expect(reader, TokenType::CloseCurl, "Expected ')' token");
			return expression;
		}

		case TokenType::Identifier:
		{
			return ReadLinkedExpression(reader, parent);
		}

		case TokenType::NewKeyword:
		{
			return ReadObjectExpression(reader, parent);
		}

		case TokenType::EndOfFile:
		{
			Diagnostics.ReportError(current, "Unexpected file end");
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

ExpressionSyntax* LexicalAnalyzer::ReadLeftDenotation(SourceReader& reader, SyntaxNode* parent, ExpressionSyntax* leftExpr)
{
	if (!reader.CanConsume())
		return nullptr;

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		if (IsUnaryOperator(current.Type))
		{
			int precendce = GetOperatorPrecendence(current.Type);
			if (precendce == 0)
				return leftExpr;

			reader.Consume();
			UnaryExpressionSyntax* syntax = new UnaryExpressionSyntax(current, true, parent);
			syntax->Expression = leftExpr;
			return syntax;
		}

		if (IsBinaryOperator(current.Type) || current.Type == TokenType::AssignOperator)
		{
			int precendce = GetOperatorPrecendence(current.Type);
			if (precendce == 0)
				return leftExpr;

			reader.Consume();
			BinaryExpressionSyntax* syntax = new BinaryExpressionSyntax(current, parent);
			leftExpr->Parent = syntax;

			syntax->Left = leftExpr;
			syntax->Right = ReadExpression(reader, syntax, precendce);
			return syntax;
		}

		Diagnostics.ReportError(current, "Unknown token in expression's left denotation");
		reader.Consume();
	}

	return nullptr;
}

LinkedExpressionSyntax* LexicalAnalyzer::ReadLinkedExpression(SourceReader& reader, SyntaxNode* parent)
{
	LinkedExpressionSyntax* syntax = new LinkedExpressionSyntax(parent);
	while (reader.CanConsume())
	{
		LinkedExpressionNode* node = ReadLinkedExpressionNode(reader, syntax, syntax->Last);
		if (syntax->Last != nullptr)
			syntax->Last->NextNode = node;

		syntax->Nodes.push_back(node);
		syntax->Last = node;

		if (node->NextDelimeterToken.IsMissing)
		{
			syntax->First = syntax->Nodes.at(0);
			return syntax;
		}
	}

	return nullptr;
}

LinkedExpressionNode* LexicalAnalyzer::ReadLinkedExpressionNode(SourceReader& reader, LinkedExpressionSyntax* parent, LinkedExpressionNode* prevNode)
{
	SyntaxToken identifier = Expect(reader, TokenType::Identifier, "Expected identifier");
	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::Delimeter:
		{
			MemberAccessExpressionSyntax* node = new MemberAccessExpressionSyntax(identifier, prevNode, parent);
			node->NextDelimeterToken = current;
			reader.Consume();
			return node;
		}

		case TokenType::OpenCurl:
		{
			InvokationExpressionSyntax* node = new InvokationExpressionSyntax(identifier, prevNode, parent);
			node->ArgumentsList = ReadArgumentsList(reader, node);

			current = reader.Current();
			if (current.Type == TokenType::Delimeter)
			{
				node->NextDelimeterToken = current;
				reader.Consume();
			}

			return node;
		}

		case TokenType::OpenSquare:
		{
			IndexatorExpressionSyntax* node = new IndexatorExpressionSyntax(identifier, prevNode, parent);
			node->IndexatorList = ReadIndexatorList(reader, node);

			current = reader.Current();
			if (current.Type == TokenType::Delimeter)
			{
				node->NextDelimeterToken = current;
				reader.Consume();
			}

			return node;
		}

		default:
		{
			SyntaxToken peek = reader.Peek();
			if (peek.Type == TokenType::Identifier)
			{
				Diagnostics.ReportError(current, "Tokens must be separated with delimeter");
				return ReadLinkedExpressionNode(reader, parent, prevNode);
			}

			return new MemberAccessExpressionSyntax(identifier, prevNode, parent);
		}
	}
}

ObjectExpressionSyntax* LexicalAnalyzer::ReadObjectExpression(SourceReader& reader, SyntaxNode* parent)
{
	ObjectExpressionSyntax* syntax = new ObjectExpressionSyntax(parent);
	syntax->NewToken = Expect(reader, TokenType::NewKeyword, "Expected 'new' keyword");
	//syntax->IdentifierToken = Expect(reader, TokenType::Identifier, "Expected identifier");
	syntax->Type = ReadType(reader, syntax);
	syntax->Arguments = ReadArgumentsList(reader, syntax);
	return syntax;
}

ArgumentsListSyntax* LexicalAnalyzer::ReadArgumentsList(SourceReader& reader, SyntaxNode* parent)
{
	ArgumentsListSyntax* arguments = new ArgumentsListSyntax(parent);
	arguments->OpenCurlToken = Expect(reader, TokenType::OpenCurl, "Expected '(' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseCurl)
	{
		arguments->CloseCurlToken = checkCloser;
		reader.Consume();
		return arguments;
	}

	while (reader.CanConsume())
	{
		ExpressionSyntax* expr = ReadExpression(reader, arguments, 0);
		ArgumentSyntax* argument = new ArgumentSyntax(expr, arguments);
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

IndexatorListSyntax* LexicalAnalyzer::ReadIndexatorList(SourceReader& reader, SyntaxNode* parent)
{
	IndexatorListSyntax* arguments = new IndexatorListSyntax(parent);
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
		ExpressionSyntax* expr = ReadExpression(reader, arguments, 0);
		ArgumentSyntax* argument = new ArgumentSyntax(expr, arguments);
		arguments->Arguments.push_back(argument);

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

TypeSyntax* LexicalAnalyzer::ReadType(SourceReader& reader, SyntaxNode* parent)
{
	TypeSyntax* syntax = nullptr;
	SyntaxToken current = reader.Current();
	
	if (IsPredefinedType(current.Type))
	{
		syntax = new PredefinedTypeSyntax(current, parent);
		current = reader.Consume();
	}
	else if (current.Type == TokenType::Identifier)
	{
		IdentifierNameTypeSyntax* identifier = new IdentifierNameTypeSyntax(parent);
		identifier->Identifiers.push_back(current);

		current = reader.Consume();
		while (current.Type == TokenType::Delimeter)
		{
			current = reader.Consume();
			identifier->Identifiers.push_back(Expect(reader, TokenType::Identifier, "Expected identifier"));
			continue;
		}

		syntax = identifier;
	}
	else
	{
		Diagnostics.ReportError(current, "Unexpected token in type syntax");
		return nullptr;
	}

	current = reader.Current();
	switch (current.Type)
	{
		case TokenType::Question:
		{
			return new NullableTypeSyntax(syntax, parent);
		}

		case TokenType::OpenBrace:
		{
			ArrayTypeSyntax* array = new ArrayTypeSyntax(parent);
			array->Rank = 1;
			array->OpenBraceToken = current;
			array->CloseBraceToken = Expect(reader, TokenType::CloseBrace, "Expected ']'");
			array->UnderlayingType = syntax;
		}

		default:
			return syntax;
	}

}

SyntaxToken LexicalAnalyzer::Expect(SourceReader& reader, TokenType kind, const char* message)
{
	SyntaxToken current = reader.Current();
	if (current.Type == kind)
	{
		reader.Consume();
		return current;
	}

	if (message != nullptr)
		Diagnostics.ReportError(current, message);

	return SyntaxToken(kind, L"", current.Location, true);
}

bool LexicalAnalyzer::Matches(SourceReader& reader, initializer_list<TokenType> types)
{
	if (!reader.CanConsume())
		return false;

	SyntaxToken current = reader.Current();
	for (const TokenType& type : types)
	{
		if (current.Type == type)
			return true;
	}

	return false;
}

TypeDeclarationSyntax* LexicalAnalyzer::make_type(MemberDeclarationInfo& info, SyntaxNode* parent)
{
	if (info.DeclareType.IsMissing)
		throw runtime_error("declare type is missing");

	switch (info.DeclareType.Type)
	{
		case TokenType::ClassKeyword:
			return new ClassDeclarationSyntax(info, parent);

		default:
			throw runtime_error("unknown type delcaration");
	}
}
