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
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.h>

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
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.h>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.h>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.h>
//#include <shard/syntax/nodes/Types/NullableTypeSyntax.h>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.h>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.h>

#include <vector>
#include <set>
#include <initializer_list>
#include <stdexcept>
#include <new>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.h>

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
					unit->Members.push_back(pMember);
					break;
				}

				Diagnostics.ReportError(token, L"Unknown token in compilation unit declaration");
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
	syntax->UsingKeywordToken = Expect(reader, TokenType::UsingKeyword, L"Exprected 'using' keyword");

	while (reader.CanConsume())
	{
		if (!TryMatch(reader, { TokenType::Identifier }, L"Expected identifier", 3))
		{
			if (TryMatch(reader, { TokenType::Semicolon }, nullptr, 10))
			{
				syntax->SemicolonToken = reader.Current();
				reader.Consume();
			}

			break;
		}

		SyntaxToken identifier = reader.Current();
		syntax->TokensList.push_back(identifier);
		reader.Consume();

		if (!TryMatch(reader, { TokenType::Delimeter, TokenType::Semicolon }, L"Expected separator token '.' or closing token ';'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

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
	syntax->FromToken = Expect(reader, TokenType::FromKeyword, L"Expected 'from' keyword");

	if (!TryMatch(reader, { TokenType::StringLiteral }, L"Expected library path", 5))
	{
		// Create missing token if we couldn't recover
		syntax->LibPathToken = SyntaxToken(TokenType::StringLiteral, L"", TextLocation(), true);
	}
	else
	{
		syntax->LibPathToken = reader.Current();
		reader.Consume();
	}

	if (!TryMatch(reader, { TokenType::ImportKeyword }, L"Expected 'import' keyword", 5))
	{
		syntax->ImportToken = SyntaxToken(TokenType::ImportKeyword, L"", TextLocation(), true);
	}
	else
	{
		syntax->ImportToken = reader.Current();
		reader.Consume();
	}

	syntax->ReturnType = ReadType(reader, syntax);
	if (!reader.CanConsume())
	{
		Diagnostics.ReportError(syntax->ImportToken, L"Unexpected end of file");
		return syntax;
	}

	if (!TryMatch(reader, { TokenType::Identifier }, L"Expected identifier", 3))
	{
		if (TryMatch(reader, { TokenType::Semicolon }, nullptr, 10))
		{
			syntax->SemicolonToken = reader.Current();
			reader.Consume();
			return syntax;
		}
	}

	syntax->IdentifierToken = reader.Current();
	reader.Consume();

	syntax->Params = ReadParametersList(reader, syntax);
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
	return syntax;
}

NamespaceDeclarationSyntax* LexicalAnalyzer::ReadNamespaceDeclaration(SourceReader& reader, SyntaxNode* parent)
{
	NamespaceDeclarationSyntax* syntax = new NamespaceDeclarationSyntax(parent);
	syntax->DeclareToken = Expect(reader, TokenType::NamespaceKeyword, L"Expected 'namespace' keyword");

	if (!TryMatch(reader, { TokenType::Identifier }, L"Expected namespace identifier", 5))
	{
		// Create missing identifier if we couldn't recover
		syntax->IdentifierToken = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
		syntax->IdentifierTokens.push_back(syntax->IdentifierToken);
	}
	else
	{
		syntax->IdentifierToken = reader.Current();
		syntax->IdentifierTokens.push_back(syntax->IdentifierToken);
		reader.Consume();
	}

	SyntaxToken current = reader.Current();
	while (current.Type == TokenType::Delimeter)
	{
		SyntaxToken identifier = reader.Consume();
		syntax->IdentifierTokens.push_back(identifier);
		current = reader.Consume();
	}

	if (!TryMatch(reader, { TokenType::OpenBrace }, L"Expected '{' or ';'", 5))
	{
		if (reader.CanConsume() && reader.Current().Type == TokenType::OpenBrace)
		{
			ReadTypeBody(reader, syntax);
		}

		return syntax;
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
			break;
		}

		if (token.Type == TokenType::Identifier && peek.Type == TokenType::OpenCurl)
		{
			// Constructor
			info.IsCtor = true;
			break;
		}

		Diagnostics.ReportError(token, L"Expected member declaration keyword");
		reader.Consume();
		continue;
	}

	if (info.DeclareType.Type == TokenType::DelegateKeyword)
	{
		info.ReturnType = ReadType(reader, parent);
	}

	// reading identifier - try to match with error recovery
	if (!TryMatch(reader, { TokenType::Identifier, TokenType::OpenCurl, TokenType::OpenBrace, TokenType::Semicolon, TokenType::AssignOperator }, L"Expected member identifier, '{', '(', ';' or '='", 5))
	{
		// Create missing identifier if we couldn't recover
		info.Identifier = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
	}
	else
	{
		info.Identifier = reader.Current();
		reader.Consume();
	}

	// Reading parameters list
	if (info.IsCtor)
	{
		return ReadConstructorDeclaration(reader, info, parent);
	}
	else if (info.DeclareType.Type == TokenType::DelegateKeyword)
	{
		return ReadDelegateDeclaration(reader, info, parent);
	}
	else if (info.ReturnType != nullptr)
	{
		// Checking if member is field - try to match with error recovery
		if (!TryMatch(reader, { TokenType::OpenCurl, TokenType::OpenBrace, TokenType::Semicolon, TokenType::AssignOperator }, L"Expected parameters list '(', accessors body '{', semicolon ';' or assignment '='", 5))
		{
			// If we couldn't recover, try to continue anyway
		}

		SyntaxToken current = reader.Current();
		switch (current.Type)
		{
			case TokenType::OpenBrace:
				return ReadPropertyDeclaration(reader, info, parent);

			case TokenType::Semicolon:
			case TokenType::AssignOperator:
				return ReadFieldDeclaration(reader, info, parent);

			case TokenType::OpenCurl:
				return ReadMethodDeclaration(reader, info, parent);

			default:
			{
				Diagnostics.ReportError(current, L"Expected parameters list or semicolon");
				reader.Consume();
			}
		}
	}
	else if (!info.DeclareType.IsMissing)
	{
		if (!TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected type body '{' or semicolon ';'", 5))
		{
			// If we couldn't recover, try to continue anyway
		}

		TypeDeclarationSyntax* type = make_type(info, parent);
		ReadTypeBody(reader, type);
		return type;
	}

	return nullptr;
}

ConstructorDeclarationSyntax* LexicalAnalyzer::ReadConstructorDeclaration(SourceReader& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	ConstructorDeclarationSyntax* syntax = new ConstructorDeclarationSyntax(info, parent);
	syntax->Params = ReadParametersList(reader, syntax);

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		syntax->Semicolon = current;
		reader.Consume();
		return syntax;
	}

	syntax->Body = ReadStatementsBlock(reader, syntax);
	return syntax;
}

MethodDeclarationSyntax* LexicalAnalyzer::ReadMethodDeclaration(SourceReader& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	MethodDeclarationSyntax* syntax = new MethodDeclarationSyntax(info, parent);
	syntax->Params = ReadParametersList(reader, syntax);
	
	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		syntax->Semicolon = current;
		reader.Consume();
		return syntax;
	}

	syntax->Body = ReadStatementsBlock(reader, syntax);
	return syntax;
}

FieldDeclarationSyntax* LexicalAnalyzer::ReadFieldDeclaration(SourceReader& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	FieldDeclarationSyntax* syntax = new FieldDeclarationSyntax(info, parent);

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::AssignOperator:
		{
			syntax->InitializerAssignToken = current;
			reader.Consume();

			syntax->InitializerExpression = ReadExpression(reader, syntax, 0);
			syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
			return syntax;
		}

		default:
		{
			syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
			return syntax;
		}
	}

	return nullptr;
}

DelegateDeclarationSyntax* LexicalAnalyzer::ReadDelegateDeclaration(SourceReader& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	DelegateDeclarationSyntax* syntax = new DelegateDeclarationSyntax(info, parent);
	syntax->Params = ReadParametersList(reader, syntax);
	syntax->Semicolon = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
	return syntax;
}

PropertyDeclarationSyntax* LexicalAnalyzer::ReadPropertyDeclaration(SourceReader& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	PropertyDeclarationSyntax* property = new PropertyDeclarationSyntax(info, parent);
	property->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{' for property accessors");

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();

		if (current.Type == TokenType::CloseBrace)
		{
			property->CloseBraceToken = current;
			reader.Consume();
			break;
		}

		if (current.Type == TokenType::EndOfFile)
		{
			Diagnostics.ReportError(current, L"Unexpected end of file in property - expected '}'");
			property->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
			break;
		}

		if (IsModifier(current.Type) || current.Type == TokenType::GetKeyword || current.Type == TokenType::SetKeyword)
		{
			AccessorDeclarationSyntax* accessor = ReadAccessorDeclaration(reader, property);

			if (accessor->KeywordToken.Type == TokenType::GetKeyword)
			{
				if (property->Getter != nullptr)
					Diagnostics.ReportError(current, L"Duplicate get accessor");
				else
					property->Getter = accessor;
			}
			else if (accessor->KeywordToken.Type == TokenType::SetKeyword)
			{
				if (property->Setter != nullptr)
					Diagnostics.ReportError(current, L"Duplicate set accessor");
				else
					property->Setter = accessor;
			}

			continue;
		}

		Diagnostics.ReportError(current, L"Expected accessor declaration or '}'");
		if (!TryMatch(reader, { TokenType::GetKeyword, TokenType::SetKeyword, TokenType::CloseBrace }, nullptr, 5))
			reader.Consume();
	}

	if (property->Getter == nullptr && property->Setter == nullptr)
		Diagnostics.ReportError(property->IdentifierToken, L"Property must have at least one accessor (get or set)");

	return property;
}

AccessorDeclarationSyntax* LexicalAnalyzer::ReadAccessorDeclaration(SourceReader& reader, SyntaxNode* parent)
{
	AccessorDeclarationSyntax* accessor = new AccessorDeclarationSyntax(parent);

	if (!reader.CanConsume())
	{
		Diagnostics.ReportError(accessor->KeywordToken, L"Unexpected end of file after accessor keyword");
		return accessor;
	}

	SyntaxToken current = reader.Current();
	if (IsModifier(current.Type))
		accessor->Modifiers = ReadMemberModifiers(reader);

	if (!TryMatch(reader, { TokenType::SetKeyword, TokenType::GetKeyword }, L"Expected 'get' or 'set' keywprd"))
		return accessor;

	accessor->KeywordToken = reader.Current();
	current = reader.Consume();

	if (!TryMatch(reader, { TokenType::Semicolon, TokenType::OpenBrace }, L"Expected ';' or '{' after accessor keyword"))
		return accessor;

	switch (current.Type)
	{
		case TokenType::Semicolon:
		{
			accessor->SemicolonToken = current;
			reader.Consume();
			break;
		}

		case TokenType::OpenBrace:
		{
			accessor->Body = ReadStatementsBlock(reader, accessor);
			break;
		}

		default:
		{
			Diagnostics.ReportError(current, L"Expected ';' or '{' after accessor keyword");
			reader.Consume();
		}
	}

	return accessor;
}

std::vector<SyntaxToken> LexicalAnalyzer::ReadMemberModifiers(SourceReader& reader)
{
	std::vector<SyntaxToken> modifiers;
	std::set<TokenType> seenModifiers;

	// Expected order: access -> static -> abstract -> sealed -> partial
	static const std::vector<TokenType> expectedOrder =
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
			Diagnostics.ReportError(current, L"Duplicate modifier");
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
			Diagnostics.ReportError(current, L"Modifier out of order");
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
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"Expected '(' token");

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
		SyntaxToken identifierToken;

		if (!TryMatch(reader, { TokenType::Identifier }, L"Expected parameter name", 3))
		{
			if (reader.CanConsume() && reader.Current().Type == TokenType::CloseCurl)
			{
				Diagnostics.ReportError(reader.Current(), L"Unexpected ')' - expected parameter name");
				syntax->CloseCurlToken = reader.Current();
				reader.Consume();
				break;
			}
			
			identifierToken = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
		}
		else
		{
			identifierToken = reader.Current();
			reader.Consume();
		}

		if (identifierToken.Type == TokenType::CloseCurl)
		{
			Diagnostics.ReportError(identifierToken, L"Unexpected ')' - expected parameter name");
			syntax->CloseCurlToken = identifierToken;
			break;
		}

		ParameterSyntax* param = new ParameterSyntax(type, identifierToken, syntax);
		syntax->Parameters.push_back(param);

		// Try to match separator
		if (!TryMatch(reader, { TokenType::Comma, TokenType::CloseCurl }, L"Expected ',' or ')'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

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
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{'");

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		if (current.Type == TokenType::CloseBrace)
		{
			syntax->CloseBraceToken = current;
			reader.Consume();
			break;
		}

		if (current.Type == TokenType::EndOfFile)
		{
			Diagnostics.ReportError(current, L"Unexpected end of file in type body - expected '}'");
			syntax->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
			break;
		}

		SyntaxToken peek = reader.Peek();
		if (IsMemberDeclaration(current.Type, peek.Type))
		{
			MemberDeclarationSyntax* pMember = ReadMemberDeclaration(reader, syntax);
			if (pMember != nullptr)
			{
				pMember->Parent = syntax;
				syntax->Members.push_back(pMember);
				continue; // Continue reading more members
			}
			else
			{
				// If ReadMemberDeclaration returned nullptr, try to synchronize
				if (!TryMatch(reader, { TokenType::CloseBrace }, nullptr, 10))
				{
					// Couldn't find closing brace - break to avoid infinite loop
					break;
				}
			}
		}
		else
		{
			Diagnostics.ReportError(reader.Current(), L"Unexpected token in type declaration");
			if (!TryMatch(reader, { TokenType::CloseBrace }, nullptr, 5))
			{
				reader.Consume();
			}
		}
	}
}

StatementsBlockSyntax* LexicalAnalyzer::ReadStatementsBlock(SourceReader& reader, SyntaxNode* parent)
{
	StatementsBlockSyntax* syntax = new StatementsBlockSyntax(parent);

	if (!reader.CanConsume())
	{
		Diagnostics.ReportError(SyntaxToken(TokenType::EndOfFile, L"", TextLocation()), L"Unexpected end of file - expected statement block");
		return syntax;
	}

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		// Empty block
		syntax->SemicolonToken = current;
		reader.Consume();
	}
	else if (current.Type == TokenType::OpenBrace)
	{
		syntax->OpenBraceToken = current;
		reader.Consume();

		while (reader.CanConsume())
		{
			current = reader.Current();
			if (current.Type == TokenType::CloseBrace)
			{
				syntax->CloseBraceToken = current;
				reader.Consume();
				break;
			}

			if (current.Type == TokenType::EndOfFile)
			{
				Diagnostics.ReportError(current, L"Unexpected end of file in statement block - expected '}'");
				syntax->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
				break;
			}

			if (IsKeyword(current.Type))
			{
				KeywordStatementSyntax* statement = ReadKeywordStatement(reader, syntax);
				if (statement != nullptr)
				{
					syntax->Statements.push_back(statement);
					continue; // Continue reading more statements
				}
				else
				{
					// Couldn't read statement - try to synchronize
					if (!TryMatch(reader, { TokenType::CloseBrace }, nullptr, 5))
						break;
				}
			}
			else
			{
				StatementSyntax* statement = ReadStatement(reader, syntax);
				if (statement != nullptr)
				{
					statement->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
					syntax->Statements.push_back(statement);
					continue; // Continue reading more statements
				}
				else
				{
					// Try to synchronize
					if (!TryMatch(reader, { TokenType::CloseBrace }, nullptr, 5))
						break;
				}
			}
		}
	}
	else
	{
		// Single statement block
		if (IsKeyword(current.Type))
		{
			KeywordStatementSyntax* statement = ReadKeywordStatement(reader, syntax);
			if (statement != nullptr)
				syntax->Statements.push_back(statement);
		}
		else
		{
			StatementSyntax* statement = ReadStatement(reader, syntax);
			if (statement != nullptr)
			{
				statement->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
				syntax->Statements.push_back(statement);
			}
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
	if (!reader.CanConsume())
		return nullptr;

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
			// Don't consume - let caller handle it to avoid infinite loop
			Diagnostics.ReportError(current, L"unknown/unsupported keyword");
			return nullptr;
		}
	}
}

ReturnStatementSyntax* LexicalAnalyzer::ReadReturnStatement(SourceReader& reader, SyntaxNode* parent)
{
	ReturnStatementSyntax* syntax = new ReturnStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ReturnKeyword, L"Expected return keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, parent, 0);

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

ThrowStatementSyntax* LexicalAnalyzer::ReadThrowStatement(SourceReader& reader, SyntaxNode* parent)
{
	ThrowStatementSyntax* syntax = new ThrowStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ReturnKeyword, L"Expected return keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, parent, 0);

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

BreakStatementSyntax* LexicalAnalyzer::ReadBreakStatement(SourceReader& reader, SyntaxNode* parent)
{
	BreakStatementSyntax* syntax = new BreakStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::BreakKeyword, L"Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

ContinueStatementSyntax* LexicalAnalyzer::ReadContinueStatement(SourceReader& reader, SyntaxNode* parent)
{
	ContinueStatementSyntax* syntax = new ContinueStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ContinueKeyword, L"Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
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

				syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"Expected '(' token");
				syntax->ConditionExpression = ReadStatement(reader, syntax);
				syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"Expected ')' token");
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

				syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"Expected '(' token");
				syntax->ConditionExpression = ReadStatement(reader, syntax);
				syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"Expected ')' token");
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
				Diagnostics.ReportError(current, L"Unknown token in deside statement");
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
	syntax->KeywordToken = Expect(reader, TokenType::WhileKeyword, L"Expected 'while' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

	return syntax;
}

UntilStatementSyntax* LexicalAnalyzer::ReadUntilStatement(SourceReader& reader, SyntaxNode* parent)
{
	UntilStatementSyntax* syntax = new UntilStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::UntilKeyword, L"Expected 'until' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

	return syntax;
}

ForStatementSyntax* LexicalAnalyzer::ReadForStatement(SourceReader& reader, SyntaxNode* parent)
{
	ForStatementSyntax* syntax = new ForStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::ForKeyword, L"Expected 'for' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");

	// Reading init statement
	syntax->InitializerStatement = ReadStatement(reader, syntax);
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax*>(syntax->InitializerStatement))
		Diagnostics.ReportError(keywordStatement->KeywordToken, L"Cannot use keyword statements inside for loop initializer");

	// Reading first semicolon
	syntax->FirstSemicolon = Expect(reader, TokenType::Semicolon, L"expected ';' token");

	// Reading looping consition
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);

	// Reading second semicolon
	syntax->SecondSemicolon = Expect(reader, TokenType::Semicolon, L"expected ';' token");

	// Reading after loop statement
	syntax->AfterRepeatStatement = ReadStatement(reader, syntax);
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax*>(syntax->AfterRepeatStatement))
		Diagnostics.ReportError(keywordStatement->KeywordToken, L"Cannot use keyword statements inside for loop repeater");

	// Reading close curl token
    syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");
	
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
	
	while (reader.CanConsume() && precendence != 0 && bindingPower < precendence)
	{
		leftExpr = ReadLeftDenotation(reader, parent, leftExpr);
		if (!reader.CanConsume())
			break;

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
		case TokenType::AddOperator:
		case TokenType::NotOperator:
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		{
			reader.Consume();
			UnaryExpressionSyntax* syntax = new UnaryExpressionSyntax(current, false, parent);
			syntax->Expression = ReadNullDenotation(reader, syntax);
			return syntax;
		}

		case TokenType::NullLiteral:
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
			Expect(reader, TokenType::CloseCurl, L"Expected ')' token");

			if (reader.Current().Type == TokenType::Delimeter)
				return ReadLinkedExpressionNode(reader, parent, expression, false);

			return expression;
		}

		case TokenType::OpenSquare:
			return ReadCollectionExpression(reader, parent);

		case TokenType::Identifier:
		case TokenType::FieldKeyword:
			return ReadLinkedExpressionNode(reader, parent, nullptr, true);

		case TokenType::LambdaKeyword:
			return ReadLambdaExpression(reader, parent);

		case TokenType::NewKeyword:
			return ReadObjectExpression(reader, parent);

		case TokenType::EndOfFile:
		{
			Diagnostics.ReportError(current, L"Unexpected file end in expression");

			// Return a literal expression with null value as fallback
			return new LiteralExpressionSyntax(SyntaxToken(TokenType::NullLiteral, L"null", current.Location, true), parent);
		}

		default:
		{
			// Try to consume and continue - maybe it's recoverable
			Diagnostics.ReportError(current, L"Unknown expression token");
			reader.Consume();

			// Return null literal as fallback
			return new LiteralExpressionSyntax(SyntaxToken(TokenType::Unknown, L"null", current.Location, true), parent);
		}
	}
	
	return nullptr;
}

ExpressionSyntax* LexicalAnalyzer::ReadLeftDenotation(SourceReader& reader, SyntaxNode* parent, ExpressionSyntax* leftExpr)
{
	if (!reader.CanConsume())
		return leftExpr;

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::Delimeter:
		{
			return ReadLinkedExpressionNode(reader, parent, leftExpr, false);
		}

		case TokenType::Question:
		{
			return ReadTernaryExpression(reader, leftExpr, parent);
		}
	}

	if (IsRightUnaryOperator(current.Type))
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

	// Unknown token - return left expression instead of null
	Diagnostics.ReportError(current, L"Unknown token in expression's left denotation");
	reader.Consume();
	return leftExpr;
}

ObjectExpressionSyntax* LexicalAnalyzer::ReadObjectExpression(SourceReader& reader, SyntaxNode* parent)
{
	ObjectExpressionSyntax* syntax = new ObjectExpressionSyntax(parent);
	syntax->NewToken = Expect(reader, TokenType::NewKeyword, L"Expected 'new' keyword");
	syntax->Type = ReadType(reader, syntax);
	syntax->ArgumentsList = ReadArgumentsList(reader, syntax);
	return syntax;
}

TernaryExpressionSyntax* LexicalAnalyzer::ReadTernaryExpression(SourceReader& reader, ExpressionSyntax* condition, SyntaxNode* parent)
{
	TernaryExpressionSyntax* syntax = new TernaryExpressionSyntax(parent);
	syntax->Condition = condition;
	
	syntax->QuestionToken = Expect(reader, TokenType::Question, L"Expected '?' token");
	syntax->Left = ReadExpression(reader, syntax, 0);

	syntax->QuestionToken = Expect(reader, TokenType::Colon, L"Expected ':' token");
	syntax->Right = ReadExpression(reader, syntax, 0);

	return syntax;
}

CollectionExpressionSyntax* LexicalAnalyzer::ReadCollectionExpression(SourceReader& reader, SyntaxNode* parent)
{
	CollectionExpressionSyntax* syntax = new CollectionExpressionSyntax(parent);
	syntax->OpenSquareToken = Expect(reader, TokenType::OpenSquare, L"Expected '[' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseSquare)
	{
		syntax->CloseSquareToken = checkCloser;
		reader.Consume();
		return syntax;
	}

	while (reader.CanConsume())
	{
		ExpressionSyntax* expr = ReadExpression(reader, syntax, 0);
		syntax->ValuesExpressions.push_back(expr);

		// Try to match separator with error recovery
		if (!TryMatch(reader, { TokenType::Comma, TokenType::CloseSquare }, L"Expected ',' or ']'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::CloseSquare)
		{
			syntax->CloseSquareToken = separatorToken;
			break;
		}
	}

	return syntax;
}

LambdaExpressionSyntax* LexicalAnalyzer::ReadLambdaExpression(SourceReader& reader, SyntaxNode* parent)
{
	LambdaExpressionSyntax* syntax = new LambdaExpressionSyntax(parent);
	syntax->LambdaToken = Expect(reader, TokenType::LambdaKeyword, L"Expected 'lambda' keyword");
	syntax->Params = ReadParametersList(reader, syntax);
	syntax->LambdaOperatorToken = Expect(reader, TokenType::LambdaOperator, L"Expected '=>' operator");
	syntax->Body = ReadStatementsBlock(reader, syntax);
	return syntax;
}

LinkedExpressionNode* LexicalAnalyzer::ReadLinkedExpressionNode(SourceReader& reader, SyntaxNode* parent, ExpressionSyntax* previous, bool isFirst)
{
	if (!reader.CanConsume())
		return nullptr;

	SyntaxToken delimeter;
	SyntaxToken identifier;

	if (isFirst)
	{
		delimeter = SyntaxToken(TokenType::Delimeter, L"", TextLocation(), false);
		identifier = reader.Current();
		if (!Matches(reader, { TokenType::FieldKeyword, TokenType::Identifier }))
			Diagnostics.ReportError(identifier, L"Expected identifier or 'field' keyword");

		reader.Consume();
	}
	else
	{
		delimeter = Expect(reader, TokenType::Delimeter, L"Expected '.' token");
		identifier = Expect(reader, TokenType::Identifier, L"Expected identifier");
	}

	if (!reader.CanConsume())
	{
		// If we got EOF, return a simple member access node
		return new MemberAccessExpressionSyntax(identifier, previous, parent);
	}

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::Delimeter:
		{
			MemberAccessExpressionSyntax* currentNode = new MemberAccessExpressionSyntax(identifier, previous, parent);
			currentNode->DelimeterToken = delimeter;

			LinkedExpressionNode* nextNode = ReadLinkedExpressionNode(reader, parent, currentNode, false);
			return nextNode;
		}

		case TokenType::OpenCurl:
		{
			InvokationExpressionSyntax* currentNode = new InvokationExpressionSyntax(identifier, previous, parent);
			currentNode->DelimeterToken = delimeter;
			currentNode->ArgumentsList = ReadArgumentsList(reader, currentNode);

			if (!reader.CanConsume())
				return currentNode;

			current = reader.Current();
			if (current.Type == TokenType::Delimeter)
			{
				LinkedExpressionNode* nextNode = ReadLinkedExpressionNode(reader, parent, currentNode, false);
				return nextNode;
			}

			return currentNode;
		}

		case TokenType::OpenSquare:
		{
			MemberAccessExpressionSyntax* member = new MemberAccessExpressionSyntax(identifier, previous, parent);
			member->DelimeterToken = delimeter;

			IndexatorExpressionSyntax* currentNode = new IndexatorExpressionSyntax(member, parent);
			currentNode->IndexatorList = ReadIndexatorList(reader, currentNode);

			if (!reader.CanConsume())
				return currentNode;

			current = reader.Current();
			if (current.Type == TokenType::Delimeter)
			{
				LinkedExpressionNode* nextNode = ReadLinkedExpressionNode(reader, parent, currentNode, false);
				return nextNode;
			}

			return currentNode;
		}

		default:
		{
			if (IsOperator(current.Type) || IsPunctuation(current.Type))
			{
				MemberAccessExpressionSyntax* currentNode = new MemberAccessExpressionSyntax(identifier, previous, parent);
				currentNode->DelimeterToken = delimeter;
				return currentNode;
			}

			/*
			SyntaxToken peek = reader.Peek();
			if (peek.Type == TokenType::Identifier)
			{
				Diagnostics.ReportError(current, L"Tokens must be separated with delimeter");
				return ReadLinkedExpressionNode(reader, parent, previous, false);
			}
			*/
		}
	}

	return nullptr;
}

ArgumentsListSyntax* LexicalAnalyzer::ReadArgumentsList(SourceReader& reader, SyntaxNode* parent)
{
	ArgumentsListSyntax* arguments = new ArgumentsListSyntax(parent);
	arguments->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"Expected '(' token");

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

		// Try to match separator with error recovery
		if (!TryMatch(reader, { TokenType::Comma, TokenType::CloseCurl }, L"Expected ',' or ')'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::CloseCurl)
		{
			arguments->CloseCurlToken = separatorToken;
			break;
		}
	}

	return arguments;
}

IndexatorListSyntax* LexicalAnalyzer::ReadIndexatorList(SourceReader& reader, SyntaxNode* parent)
{
	IndexatorListSyntax* arguments = new IndexatorListSyntax(parent);
	arguments->OpenSquareToken = Expect(reader, TokenType::OpenSquare, L"Exprected '[' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseSquare)
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

		// Try to match separator with error recovery
		if (!TryMatch(reader, { TokenType::Comma, TokenType::CloseSquare }, L"Expected ',' or ']'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::CloseSquare)
		{
			arguments->CloseSquareToken = separatorToken;
			break;
		}
	}

	return arguments;
}

TypeSyntax* LexicalAnalyzer::ReadType(SourceReader& reader, SyntaxNode* parent)
{
	SyntaxToken current = reader.Current();
	
	if (IsPredefinedType(current.Type))
	{
		reader.Consume();
		PredefinedTypeSyntax* predefinedType = new PredefinedTypeSyntax(current, parent);
		TypeSyntax* syntax = ReadModifiedType(reader, predefinedType, parent);
		return syntax;
	}

	if (current.Type == TokenType::DelegateKeyword)
	{
		TypeSyntax* identifierType = ReadDelegateType(reader, parent);
		return identifierType;
	}

	if (current.Type == TokenType::Identifier)
	{
		TypeSyntax* identifierType = ReadIdentifierNameType(reader, parent);
		return identifierType;
	}

	Diagnostics.ReportError(current, L"Unexpected token in type syntax");
	return nullptr;
}

TypeSyntax* LexicalAnalyzer::ReadModifiedType(SourceReader& reader, TypeSyntax* type, SyntaxNode* parent)
{
	if (!reader.CanConsume())
		return type;

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		/*
		case TokenType::Question:
		{
			reader.Consume();
			NullableTypeSyntax* nullable = new NullableTypeSyntax(type, parent);
			nullable->QuestionToken = current;
			return nullable;
		}
		*/

		case TokenType::OpenSquare:
		{
			return ReadArrayType(reader, type, parent);
		}

		case TokenType::LessOperator:
		{
			return ReadGenericType(reader, type, parent);
		}

		default:
			return type;
	}
}

TypeSyntax* LexicalAnalyzer::ReadIdentifierNameType(SourceReader& reader, SyntaxNode* parent)
{
	IdentifierNameTypeSyntax* identifier = new IdentifierNameTypeSyntax(parent);
	identifier->Identifier = Expect(reader, TokenType::Identifier, L"Expected identifier");

	TypeSyntax* modifiedSyntax = ReadModifiedType(reader, identifier, parent);
	return modifiedSyntax;
}

TypeSyntax* LexicalAnalyzer::ReadDelegateType(SourceReader& reader, SyntaxNode* parent)
{
	DelegateTypeSyntax* delegate = new DelegateTypeSyntax(parent);
	delegate->DelegateToken = Expect(reader, TokenType::DelegateKeyword, L"Excpected 'lambda' keyword");
	delegate->ReturnType = ReadType(reader, delegate);
	delegate->Params = ReadParametersList(reader, delegate);

	TypeSyntax* modifiedSyntax = ReadModifiedType(reader, delegate, parent);
	return modifiedSyntax;
}

TypeSyntax* LexicalAnalyzer::ReadArrayType(SourceReader& reader, TypeSyntax* previous, SyntaxNode* parent)
{
	ArrayTypeSyntax* array = new ArrayTypeSyntax(previous, parent);
	array->OpenSquareToken = Expect(reader, TokenType::OpenSquare, L"Expected '['");
	array->CloseSquareToken = Expect(reader, TokenType::CloseSquare, L"Expected ']'");
	array->Rank = 1;

	TypeSyntax* modifiedSyntax = ReadModifiedType(reader, array, parent);
	return modifiedSyntax;
}

TypeSyntax* LexicalAnalyzer::ReadGenericType(SourceReader& reader, TypeSyntax* previous, SyntaxNode* parent)
{
	GenericTypeSyntax* generic = new GenericTypeSyntax(previous, parent);
	generic->OpenListToken = Expect(reader, TokenType::LessOperator, L"Expected '<' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::GreaterOperator)
	{
		generic->CloseListToken = checkCloser;
		reader.Consume();
		return generic;
	}

	while (reader.CanConsume())
	{
		TypeSyntax* type = ReadType(reader, generic);
		generic->TypeArguments.push_back(type);

		// Try to match separator with error recovery
		if (!TryMatch(reader, { TokenType::Comma, TokenType::GreaterOperator }, L"Expected ',' or '>'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::CloseCurl)
		{
			generic->CloseListToken = separatorToken;
			break;
		}
	}

	TypeSyntax* modifiedSyntax = ReadModifiedType(reader, generic, parent);
	return modifiedSyntax;
}

// Smart error recovery with synchronization tokens
static const std::vector<TokenType> SynchronizationTokens = {
	TokenType::Semicolon,
	TokenType::OpenBrace,
	TokenType::CloseBrace,
	TokenType::OpenCurl,
	TokenType::CloseCurl,
	TokenType::NamespaceKeyword,
	TokenType::ClassKeyword,
	TokenType::StructKeyword,
	TokenType::EndOfFile
};

static bool IsSynchronizationToken(TokenType type)
{
	for (TokenType syncToken : SynchronizationTokens)
	{
		if (syncToken == type)
			return true;
	}
	return false;
}

static bool TrySynchronize(SourceReader& reader, const std::vector<TokenType>& expectedTokens, int maxSkips = 10)
{
	// Skip tokens until we find a synchronization point or expected token
	int skipped = 0;
	while (reader.CanConsume() && skipped < maxSkips)
	{
		SyntaxToken current = reader.Current();
		
		// Check if current token is one of expected
		for (TokenType expected : expectedTokens)
		{
			if (current.Type == expected)
				return true;
		}
		
		// Check if current token is a synchronization point
		if (IsSynchronizationToken(current.Type))
			return false;
		
		reader.Consume();
		skipped++;
	}
	
	return false;
}

SyntaxToken LexicalAnalyzer::Expect(SourceReader& reader, TokenType kind, const wchar_t* message)
{
	if (!reader.CanConsume())
	{
		if (message != nullptr)
		{
			SyntaxToken eofToken = SyntaxToken(TokenType::EndOfFile, L"", TextLocation());
			Diagnostics.ReportError(eofToken, message);
		}
		return SyntaxToken(kind, L"", TextLocation(), true);
	}

	SyntaxToken current = reader.Current();
	if (current.Type == kind)
	{
		reader.Consume();
		return current;
	}

	if (message != nullptr)
		Diagnostics.ReportError(current, message);

	// Try to synchronize - skip until we find the expected token or a sync point
	std::vector<TokenType> expected = { kind };
	if (TrySynchronize(reader, expected, 5))
	{
		// Found expected token after skipping
		current = reader.Current();
		reader.Consume();
		return current;
	}

	// Return missing token
	return SyntaxToken(kind, L"", current.Location, true);
}

bool LexicalAnalyzer::Matches(SourceReader& reader, std::initializer_list<TokenType> types)
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

// New method: Try to match one of expected tokens, with error recovery
bool LexicalAnalyzer::TryMatch(SourceReader& reader, std::initializer_list<TokenType> types, const wchar_t* errorMessage, int maxSkips)
{
	if (!reader.CanConsume())
		return false;

	if (Matches(reader, types))
		return true;

	if (errorMessage != nullptr)
	{
		SyntaxToken current = reader.Current();
		Diagnostics.ReportError(current, errorMessage);
	}

	// Try to synchronize to one of expected tokens
	std::vector<TokenType> expectedTypes(types);
	if (TrySynchronize(reader, expectedTypes, maxSkips))
		return true;

	return false;
}

TypeDeclarationSyntax* LexicalAnalyzer::make_type(MemberDeclarationInfo& info, SyntaxNode* parent)
{
	if (info.DeclareType.IsMissing)
		throw std::runtime_error("declare type is missing");

	switch (info.DeclareType.Type)
	{
		case TokenType::ClassKeyword:
			return new ClassDeclarationSyntax(info, parent);

		case TokenType::StructKeyword:
			return new StructDeclarationSyntax(info, parent);

		default:
			throw std::runtime_error("unknown type delcaration");
	}
}
