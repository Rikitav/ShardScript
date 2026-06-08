#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/analysis/TextLocation.hpp>
#include <shard/parsing/analysis/DiagnosticsContext.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxFacts.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/TypeDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/AttributeSyntax.hpp>
#include <shard/syntax/nodes/ParametersListSyntax.hpp>
#include <shard/syntax/nodes/BodyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <shard/syntax/nodes/TypeArgumentsListSyntax.hpp>
#include <shard/syntax/nodes/TypeParametersListSyntax.hpp>

#include <shard/syntax/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/syntax/nodes/Loops/UntilStatementSyntax.hpp>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>

#include <shard/syntax/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/syntax/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/syntax/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/syntax/nodes/Statements/ContinueStatementSyntax.hpp>

#include <shard/syntax/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/syntax/nodes/Expressions/TernaryExpressionSyntax.hpp>

#include <shard/syntax/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/IdentifierNameTypeSyntax.hpp>
//#include <shard/syntax/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/syntax/nodes/Types/DelegateTypeSyntax.hpp>

#include <vector>
#include <initializer_list>
#include <set>
#include <new>

using namespace shard;

void SourceParser::FromSourceProvider(SyntaxTree& syntaxTree, SourceProvider& reader)
{
	CompilationUnitSyntax *const unit = ReadCompilationUnit(reader);
	syntaxTree.CompilationUnits.push_back(unit);
}

CompilationUnitSyntax *const SourceParser::ReadCompilationUnit(SourceProvider& reader)
{
	CompilationUnitSyntax *const unit = new CompilationUnitSyntax();

	while (reader.CanConsume())
	{
		SyntaxToken token = reader.Current();
		switch (token.Type)
		{
			case TokenType::UsingKeyword:
			{
				UsingDirectiveSyntax *const pDirective = ReadUsingDirective(reader, unit);
				*const_cast<SyntaxNode**>(&pDirective->Parent) = unit;
				unit->Usings.push_back(pDirective);
				break;
			}

			case TokenType::NamespaceKeyword:
			{
				NamespaceDeclarationSyntax *const pNamespace = ReadNamespaceDeclaration(reader, unit);
				*const_cast<SyntaxNode**>(&pNamespace->Parent) = unit;
				unit->Members.push_back(pNamespace);
				break;
			}

			default:
			{
				SyntaxToken peek = reader.Peek();
				if (IsMemberDeclaration(token.Type, peek.Type))
				{
					MemberDeclarationSyntax *const pMember = ReadMemberDeclaration(reader, unit);
					*const_cast<SyntaxNode**>(&pMember->Parent) = unit;
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

UsingDirectiveSyntax *const SourceParser::ReadUsingDirective(SourceProvider& reader, SyntaxNode *const parent)
{
	UsingDirectiveSyntax *const syntax = new UsingDirectiveSyntax(parent);
	syntax->UsingKeywordToken = Expect(reader, TokenType::UsingKeyword, L"Exprected 'using' keyword");

	while (reader.CanConsume())
	{
		if (!TryMatchIdentifier(reader, 3))
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

NamespaceDeclarationSyntax *const SourceParser::ReadNamespaceDeclaration(SourceProvider& reader, SyntaxNode *const parent)
{
	NamespaceDeclarationSyntax *const syntax = new NamespaceDeclarationSyntax(parent);
	syntax->DeclareToken = Expect(reader, TokenType::NamespaceKeyword, L"Expected 'namespace' keyword");

	if (!TryMatchIdentifier(reader, 5))
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

MemberDeclarationSyntax *const SourceParser::ReadMemberDeclaration(SourceProvider& reader, SyntaxNode *const parent)
{
	MemberDeclarationInfo info;
	info.Attributes = ReadAttributeList(reader, parent);
	info.Modifiers = ReadMemberModifiers(reader);

	if (!reader.CanConsume())
		return nullptr;

	SyntaxToken current = reader.Current();

	// Declaration prediction: class, struct, interface, delegate
	if (IsTypeKeyword(current.Type))
	{
		info.DeclareType = current;
		reader.Consume();

		switch (current.Type)
		{
			case TokenType::ClassKeyword:
				return ReadClassDeclaration(reader, info, parent);

			case TokenType::StructKeyword:
				return ReadStructDeclaration(reader, info, parent);

			case TokenType::InterfaceKeyword:
				return ReadInterfaceDeclaration(reader, info, parent);

			case TokenType::DelegateKeyword:
				return ReadDelegateDeclaration(reader, info, parent);

			default:
				Diagnostics.ReportError(info.DeclareType, L"Unsupported member keyword");
				return nullptr;
		}
	}

	// Declaration prediction: fn name(params) -> type { }
	if (current.Type == TokenType::FnKeyword)
	{
		reader.Consume(); // fn
		if (!TryMatchIdentifier(reader, 3))
		{
			info.Identifier = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
		}
		else
		{
			info.Identifier = reader.Current();
			reader.Consume();
		}

		return ReadMethodDeclaration(reader, info, parent);
	}

	// Declaration prediction: init(params) { }
	if (current.Type == TokenType::InitKeyword)
	{
		reader.Consume(); // init
		info.Identifier = SyntaxToken(TokenType::InitKeyword, L"init", TextLocation(), false);
		return ReadConstructorDeclaration(reader, info, parent);
	}

	// Declaration prediction: indexer[params]: type { }
	if (current.Type == TokenType::IndexerKeyword)
	{
		reader.Consume(); // indexer
		info.Identifier = current;
		return ReadIndexatorDeclaration(reader, info, parent);
	}

	// Declaration prediction: Identifier: type ...
	if (current.Type == TokenType::Identifier)
	{
		info.Identifier = current;
		reader.Consume();

		SyntaxToken next = reader.Current();
		if (next.Type == TokenType::OpenCurl)
		{
			Diagnostics.ReportError(info.Identifier, L"Missed 'fn' keyword");
			return ReadMethodDeclaration(reader, info, parent);
		}

		if (!TryMatch(reader, { TokenType::Colon }, L"Expected ':' after identifier", 3))
			return nullptr;

		reader.Consume(); // :
		info.ReturnType = ReadType(reader, parent);
		if (info.ReturnType == nullptr)
		{
			Diagnostics.ReportError(reader.Current(), L"Expected type after ':'");
			return nullptr;
		}

		next = reader.Current();
		if (next.Type == TokenType::OpenBrace)
		{
			return ReadPropertyDeclaration(reader, info, parent);
		}
		else if (next.Type == TokenType::ArrowOperator) // =>
		{
			return ReadComputedPropertyDeclaration(reader, info, parent);
		}
		else if (next.Type == TokenType::AssignOperator || next.Type == TokenType::Semicolon)
		{
			return ReadFieldDeclaration(reader, info, parent);
		}
		else
		{
			Diagnostics.ReportError(next, L"Expected '{', '=>', '=' or ';' after type");
			return nullptr;
		}
	}

	Diagnostics.ReportError(current, L"Expected member declaration");
	reader.Consume();
	return nullptr;
}

ClassDeclarationSyntax *const SourceParser::ReadClassDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	ClassDeclarationSyntax *const syntax = new ClassDeclarationSyntax(info, parent);
	if (TryMatchIdentifier(reader, 5))
	{
		syntax->IdentifierToken = reader.Current();
		reader.Consume();
	}
	else
	{
		syntax->IdentifierToken = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
	}

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::LessOperator)
	{
		syntax->TypeParameters = ReadTypeParametersList(reader, parent);
	}

	current = reader.Current();
	if (current.Type == TokenType::Colon)
	{
		reader.Consume();
		Diagnostics.ReportError(current, L"Inheritance is not implemented yet");
	}

	if (TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected class body '{' or semicolon ';'", 5))
	{
		current = reader.Current();
		if (current.Type == TokenType::OpenBrace)
		{
			ReadTypeBody(reader, syntax);
		}
	}

	return syntax;
}

StructDeclarationSyntax *const SourceParser::ReadStructDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	StructDeclarationSyntax *const syntax = new StructDeclarationSyntax(info, parent);

	if (TryMatchIdentifier(reader, 5))
	{
		syntax->IdentifierToken = reader.Current();
		reader.Consume();
	}
	else
	{
		syntax->IdentifierToken = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
	}

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::LessOperator)
	{
		syntax->TypeParameters = ReadTypeParametersList(reader, parent);
	}

	current = reader.Current();
	if (current.Type == TokenType::Colon)
	{
		reader.Consume();
		Diagnostics.ReportError(current, L"Inheritance is not implemented yet");
	}

	if (TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected struct body '{' or semicolon ';'", 5))
	{
		current = reader.Current();
		if (current.Type == TokenType::OpenBrace)
		{
			ReadTypeBody(reader, syntax);
		}
	}

	return syntax;
}

ConstructorDeclarationSyntax *const SourceParser::ReadConstructorDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	ConstructorDeclarationSyntax *const syntax = new ConstructorDeclarationSyntax(info, parent);
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

MethodDeclarationSyntax *const SourceParser::ReadMethodDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	MethodDeclarationSyntax *const syntax = new MethodDeclarationSyntax(info, parent);
	syntax->Params = ReadParametersList(reader, syntax);

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::ArrowOperator)
	{
		reader.Consume(); // ->
		syntax->ReturnType = ReadType(reader, syntax);
	}
	else
	{
		//syntax->ReturnType = new PredefinedTypeSyntax(SyntaxToken(TokenType::VoidKeyword, L"void", TextLocation(), false), syntax);
		if (current.Type == TokenType::Identifier)
		{
			Diagnostics.ReportError(current, L"Expected '->' before return type.");
		}
		else
		{
			Diagnostics.ReportError(current, L"Function must have a return type.");
		}
	}

	current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		syntax->Semicolon = current;
		reader.Consume();
		return syntax;
	}

	syntax->Body = ReadStatementsBlock(reader, syntax);
	return syntax;
}

FieldDeclarationSyntax *const SourceParser::ReadFieldDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	FieldDeclarationSyntax *const syntax = new FieldDeclarationSyntax(info, parent);

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

		case TokenType::Semicolon:
		{
			syntax->SemicolonToken = current;
			reader.Consume();
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

PropertyDeclarationSyntax *const SourceParser::ReadComputedPropertyDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	PropertyDeclarationSyntax *const property = new PropertyDeclarationSyntax(info, parent);
	property->ArrowToken = Expect(reader, TokenType::ArrowOperator, L"Expected '=>'");
	property->InitializerExpression = ReadExpression(reader, property, 0);
	property->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
	return property;
}

InterfaceDeclarationSyntax *const SourceParser::ReadInterfaceDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	InterfaceDeclarationSyntax *const syntax = new InterfaceDeclarationSyntax(info, parent);

	if (TryMatchIdentifier(reader, 5))
	{
		syntax->IdentifierToken = reader.Current();
		reader.Consume();
	}
	else
	{
		syntax->IdentifierToken = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
	}

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::LessOperator)
	{
		syntax->TypeParameters = ReadTypeParametersList(reader, parent);
	}

	if (TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected interface body '{' or semicolon ';'", 5))
	{
		current = reader.Current();
		if (current.Type == TokenType::OpenBrace)
		{
			ReadTypeBody(reader, syntax);
		}
	}

	return syntax;
}

DelegateDeclarationSyntax *const SourceParser::ReadDelegateDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	DelegateDeclarationSyntax *const syntax = new DelegateDeclarationSyntax(info, parent);
	syntax->Params = ReadParametersList(reader, syntax);
	syntax->Semicolon = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
	return syntax;
}

PropertyDeclarationSyntax *const SourceParser::ReadPropertyDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	PropertyDeclarationSyntax *const syntax = new PropertyDeclarationSyntax(info, parent);
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{' for property accessors");

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
			Diagnostics.ReportError(current, L"Unexpected end of file in property - expected '}'");
			syntax->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
			break;
		}

		if (IsModifier(current.Type) || current.Type == TokenType::GetKeyword || current.Type == TokenType::SetKeyword || current.Type == TokenType::OpenSquare)
		{
			AccessorDeclarationSyntax *const accessor = ReadAccessorDeclaration(reader, syntax);

			if (accessor->KeywordToken.Type == TokenType::GetKeyword)
			{
				if (syntax->Getter != nullptr)
					Diagnostics.ReportError(current, L"Duplicate get accessor");
				else
					syntax->Getter = accessor;
			}
			else if (accessor->KeywordToken.Type == TokenType::SetKeyword)
			{
				if (syntax->Setter != nullptr)
					Diagnostics.ReportError(current, L"Duplicate set accessor");
				else
					syntax->Setter = accessor;
			}

			continue;
		}

		Diagnostics.ReportError(current, L"Expected accessor declaration or '}'");
		if (!TryMatch(reader, { TokenType::GetKeyword, TokenType::SetKeyword, TokenType::CloseBrace }, nullptr, 5))
			reader.Consume();
	}

	if (syntax->Getter == nullptr && syntax->Setter == nullptr)
		Diagnostics.ReportError(syntax->IdentifierToken, L"Property must have at least one accessor (get or set)");

	return syntax;
}

IndexatorDeclarationSyntax *const SourceParser::ReadIndexatorDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode *const parent)
{
	IndexatorDeclarationSyntax *const syntax = new IndexatorDeclarationSyntax(info, parent);
	//syntax->IndexKeyword = Expect(reader, TokenType::IndexerKeyword, L"Expected 'index' keyword");
	syntax->IndexKeyword = info.Identifier;
	syntax->Parameters = ReadIndexerParametersList(reader, syntax);

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Colon)
	{
		reader.Consume(); // :
		syntax->ReturnType = ReadType(reader, syntax);
	}
	else
	{
		//syntax->ReturnType = new PredefinedTypeSyntax(SyntaxToken(TokenType::VoidKeyword, L"void", TextLocation(), false), syntax);
		if (current.Type == TokenType::Identifier)
		{
			Diagnostics.ReportError(current, L"Expected ':' before return type.");
		}
		else
		{
			Diagnostics.ReportError(current, L"Indexer must have a return type.");
		}
	}

	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{' for indexer accessors");
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
			Diagnostics.ReportError(current, L"Unexpected end of file in indexer - expected '}'");
			syntax->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
			break;
		}

		if (IsModifier(current.Type) || current.Type == TokenType::GetKeyword || current.Type == TokenType::SetKeyword || current.Type == TokenType::OpenSquare)
		{
			AccessorDeclarationSyntax *const accessor = ReadAccessorDeclaration(reader, syntax);

			if (accessor->KeywordToken.Type == TokenType::GetKeyword)
			{
				if (syntax->Getter != nullptr)
					Diagnostics.ReportError(current, L"Duplicate get accessor");
				else
					syntax->Getter = accessor;
			}
			else if (accessor->KeywordToken.Type == TokenType::SetKeyword)
			{
				if (syntax->Setter != nullptr)
					Diagnostics.ReportError(current, L"Duplicate set accessor");
				else
					syntax->Setter = accessor;
			}

			continue;
		}

		Diagnostics.ReportError(current, L"Expected accessor declaration or '}'");
		if (!TryMatch(reader, { TokenType::GetKeyword, TokenType::SetKeyword, TokenType::CloseBrace }, nullptr, 5))
			reader.Consume();
	}

	if (syntax->Getter == nullptr && syntax->Setter == nullptr)
		Diagnostics.ReportError(syntax->IdentifierToken, L"Indexer must have at least one accessor (get or set)");

	return syntax;
}

AccessorDeclarationSyntax *const SourceParser::ReadAccessorDeclaration(SourceProvider& reader, SyntaxNode *const parent)
{
	AccessorDeclarationSyntax *const syntax = new AccessorDeclarationSyntax(parent);
	syntax->Attributes = ReadAttributeList(reader, parent);

	if (!reader.CanConsume())
	{
		Diagnostics.ReportError(syntax->KeywordToken, L"Unexpected end of file after accessor keyword");
		return syntax;
	}

	SyntaxToken current = reader.Current();
	if (IsModifier(current.Type))
		syntax->Modifiers = ReadMemberModifiers(reader);

	if (!TryMatch(reader, { TokenType::SetKeyword, TokenType::GetKeyword }, L"Expected 'get' or 'set' keywprd"))
		return syntax;

	syntax->KeywordToken = reader.Current();
	current = reader.Consume();

	if (!TryMatch(reader, { TokenType::Semicolon, TokenType::OpenBrace }, L"Expected ';' or '{' after accessor keyword"))
		return syntax;

	switch (current.Type)
	{
		case TokenType::Semicolon:
		{
			syntax->SemicolonToken = current;
			reader.Consume();
			break;
		}

		case TokenType::OpenBrace:
		{
			syntax->Body = ReadStatementsBlock(reader, syntax);
			break;
		}

		default:
		{
			Diagnostics.ReportError(current, L"Expected ';' or '{' after accessor keyword");
			reader.Consume();
		}
	}

	return syntax;
}

std::vector<SyntaxToken> SourceParser::ReadMemberModifiers(SourceProvider& reader)
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
		TokenType::ExportKeyword,
		TokenType::StaticKeyword,
		TokenType::ExternKeyword,
		/*
		TokenType::AbstractKeyword,
		TokenType::SealedKeyword,
		TokenType::PartialKeyword,
		*/
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

AttributeSyntax *const SourceParser::ReadAttribute(SourceProvider& reader, SyntaxNode *const parent)
{
	AttributeSyntax *const syntax = new AttributeSyntax(parent);
	syntax->OpenBracketToken = Expect(reader, TokenType::OpenSquare, L"Expected '['");
	syntax->NameToken = Expect(reader, TokenType::Identifier, L"Expected attribute name");

	if (reader.Current().Type == TokenType::OpenCurl)
	{
		syntax->OpenCurlToken = reader.Current();
		reader.Consume(); // (

		while (reader.CanConsume() && reader.Current().Type != TokenType::CloseCurl)
		{
			SyntaxToken arg = reader.Current();
			if (arg.Type == TokenType::StringLiteral)
			{
				syntax->Arguments.push_back(arg);
				reader.Consume();
			}
			else
			{
				Diagnostics.ReportError(arg, L"Expected string literal in attribute arguments");
				reader.Consume();
			}

			if (reader.Current().Type == TokenType::Comma)
			{
				reader.Consume(); // ,
			}
			else if (reader.Current().Type != TokenType::CloseCurl)
			{
				Diagnostics.ReportError(reader.Current(), L"Expected ',' or ')' in attribute arguments");
				break;
			}
		}

		syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"Expected ')'");
	}

	syntax->CloseBracketToken = Expect(reader, TokenType::CloseSquare, L"Expected ']'");
	return syntax;
}

std::vector<AttributeSyntax*> SourceParser::ReadAttributeList(SourceProvider& reader, SyntaxNode *const parent)
{
	std::vector<AttributeSyntax*> attributes;
	while (reader.CanConsume() && reader.Current().Type == TokenType::OpenSquare)
	{
		AttributeSyntax *const attr = ReadAttribute(reader, parent);
		if (attr != nullptr)
			attributes.push_back(attr);
	}

	return attributes;
}

ParametersListSyntax *const SourceParser::ReadIndexerParametersList(SourceProvider& reader, SyntaxNode *const parent)
{
	ParametersListSyntax *const syntax = new ParametersListSyntax(parent);
	syntax->OpenToken = Expect(reader, TokenType::OpenSquare, L"Expected '[' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseSquare)
	{
		syntax->CloseToken = checkCloser;
		reader.Consume();
		return syntax;
	}

	while (reader.CanConsume())
	{
		SyntaxToken identifierToken;
		if (!TryMatchIdentifier(reader, 3))
		{
			if (reader.CanConsume() && reader.Current().Type == TokenType::CloseSquare)
			{
				Diagnostics.ReportError(reader.Current(), L"Unexpected ']' - expected parameter name");
				syntax->CloseToken = reader.Current();
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

		if (identifierToken.Type == TokenType::CloseSquare)
		{
			Diagnostics.ReportError(identifierToken, L"Unexpected ']' - expected parameter name");
			syntax->CloseToken = identifierToken;
			break;
		}

		Expect(reader, TokenType::Colon, L"Expected ':' after parameter name");
		TypeSyntax* const type = ReadType(reader, syntax);

		ParameterSyntax *const param = new ParameterSyntax(type, identifierToken, syntax);
		syntax->Parameters.push_back(param);

		// Try to match separator
		if (!TryMatch(reader, { TokenType::Comma, TokenType::CloseSquare }, L"Expected ',' or ']'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::CloseSquare)
		{
			syntax->CloseToken = separatorToken;
			break;
		}
	}

	return syntax;
}

ParametersListSyntax *const SourceParser::ReadParametersList(SourceProvider& reader, SyntaxNode *const parent)
{
	ParametersListSyntax *const syntax = new ParametersListSyntax(parent);
	syntax->OpenToken = Expect(reader, TokenType::OpenCurl, L"Expected '(' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::CloseCurl)
	{
		syntax->CloseToken = checkCloser;
		reader.Consume();
		return syntax;
	}

	while (reader.CanConsume())
	{
		SyntaxToken identifierToken;
		if (!TryMatchIdentifier(reader, 3))
		{
			if (reader.CanConsume() && reader.Current().Type == TokenType::CloseCurl)
			{
				Diagnostics.ReportError(reader.Current(), L"Unexpected ')' - expected parameter name");
				syntax->CloseToken = reader.Current();
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

		Expect(reader, TokenType::Colon, L"Expected ':' after parameter name");
		TypeSyntax *const type = ReadType(reader, syntax);

		ParameterSyntax *const param = new ParameterSyntax(type, identifierToken, syntax);
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
			syntax->CloseToken = separatorToken;
			break;
		}
	}

	return syntax;
}

TypeParametersListSyntax *const SourceParser::ReadTypeParametersList(SourceProvider& reader, SyntaxNode *const parent)
{
	TypeParametersListSyntax *const syntax = new TypeParametersListSyntax(parent);
	syntax->OpenToken = Expect(reader, TokenType::LessOperator, L"Expected '<' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::GreaterOperator)
	{
		syntax->CloseToken = checkCloser;
		reader.Consume();
		return syntax;
	}

	while (reader.CanConsume())
	{
		SyntaxToken param = Expect(reader, TokenType::Identifier, L"Expected type parameter identifier");
		syntax->Types.push_back(param);

		// Try to match separator with error recovery
		if (!TryMatch(reader, { TokenType::Comma, TokenType::GreaterOperator }, L"Expected ',' or '>'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::GreaterOperator)
		{
			syntax->CloseToken = separatorToken;
			break;
		}
	}

	return syntax;
}

TypeArgumentsListSyntax *const SourceParser::ReadTypeArgumentsList(SourceProvider& reader, SyntaxNode *const parent)
{
	TypeArgumentsListSyntax *const syntax = new TypeArgumentsListSyntax(parent);
	syntax->OpenToken = Expect(reader, TokenType::LessOperator, L"Expected '<' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::GreaterOperator)
	{
		syntax->CloseToken = checkCloser;
		reader.Consume();
		return syntax;
	}

	while (reader.CanConsume())
	{
		TypeSyntax *const type = ReadType(reader, syntax);
		syntax->Types.push_back(type);

		// Try to match separator with error recovery
		if (!TryMatch(reader, { TokenType::Comma, TokenType::GreaterOperator }, L"Expected ',' or '>'", 3))
		{
			break;
		}

		SyntaxToken separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::GreaterOperator)
		{
			syntax->CloseToken = separatorToken;
			break;
		}
	}

	return syntax;
}

void SourceParser::ReadTypeBody(SourceProvider& reader, TypeDeclarationSyntax *const syntax)
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
			MemberDeclarationSyntax *const pMember = ReadMemberDeclaration(reader, syntax);
			if (pMember != nullptr)
			{
				*const_cast<SyntaxNode**>(&pMember->Parent) = syntax;
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

StatementsBlockSyntax *const SourceParser::ReadStatementsBlock(SourceProvider& reader, SyntaxNode *const parent)
{
	StatementsBlockSyntax *const syntax = new StatementsBlockSyntax(parent);

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
				KeywordStatementSyntax *const statement = ReadKeywordStatement(reader, syntax);
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
				StatementSyntax *const statement = ReadStatement(reader, syntax);
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
			KeywordStatementSyntax *const statement = ReadKeywordStatement(reader, syntax);
			if (statement != nullptr)
				syntax->Statements.push_back(statement);
		}
		else
		{
			StatementSyntax *const statement = ReadStatement(reader, syntax);
			if (statement != nullptr)
			{
				statement->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
				syntax->Statements.push_back(statement);
			}
		}
	}

	return syntax;
}

StatementSyntax *const SourceParser::ReadStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		return new StatementSyntax(SyntaxKind::ExpressionStatement, parent);
	}

	if (reader.CanPeek())
	{
		SyntaxToken peek = reader.Peek();

		// name: type = value
		if (current.Type == TokenType::Identifier && peek.Type == TokenType::Colon)
		{
			reader.Consume(); // name
			reader.Consume(); // :
			TypeSyntax *const type = ReadType(reader, parent);
			
			if (type == nullptr)
			{
				Diagnostics.ReportError(reader.Current(), L"Expected type after ':'");
				return nullptr;
			}

			SyntaxToken name = current;
			SyntaxToken assign;
			ExpressionSyntax *expr = nullptr;
			
			if (reader.Current().Type == TokenType::AssignOperator)
			{
				assign = reader.Current();
				reader.Consume();
				expr = ReadExpression(reader, parent, 0);
			}

			return new VariableStatementSyntax(type, name, assign, expr, parent);
		}

		// name := value
		if (current.Type == TokenType::Identifier && peek.Type == TokenType::DeclareAssignOperator)
		{
			reader.Consume(); // name
			SyntaxToken walrus = reader.Current();
			reader.Consume(); // :=
			ExpressionSyntax *const expr = ReadExpression(reader, parent, 0);
			return new VariableStatementSyntax(new PredefinedTypeSyntax(SyntaxToken(TokenType::VarKeyword, L"var", current.Location, false), parent), current, walrus, expr, parent);
		}
	}

	ExpressionSyntax *const expression = ReadExpression(reader, parent, 0);
	return new ExpressionStatementSyntax(expression, parent);
}

KeywordStatementSyntax *const SourceParser::ReadKeywordStatement(SourceProvider& reader, SyntaxNode *const parent)
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

		case TokenType::TryKeyword:
			return ReadTryStatement(reader, parent);

		default:
		{
			// Don't consume - let caller handle it to avoid infinite loop
			Diagnostics.ReportError(current, L"unknown/unsupported keyword");
			return nullptr;
		}
	}
}

ReturnStatementSyntax *const SourceParser::ReadReturnStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	ReturnStatementSyntax *const syntax = new ReturnStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ReturnKeyword, L"Expected return keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, parent, 0);

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

ThrowStatementSyntax *const SourceParser::ReadThrowStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	ThrowStatementSyntax *const syntax = new ThrowStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ReturnKeyword, L"Expected return keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, parent, 0);

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

BreakStatementSyntax *const SourceParser::ReadBreakStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	BreakStatementSyntax *const syntax = new BreakStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::BreakKeyword, L"Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

ContinueStatementSyntax *const SourceParser::ReadContinueStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	ContinueStatementSyntax *const syntax = new ContinueStatementSyntax(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ContinueKeyword, L"Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

ConditionalClauseBaseSyntax *const SourceParser::ReadConditionalClause(SourceProvider& reader, SyntaxNode *const parent)
{
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		switch (current.Type)
		{
			case TokenType::IfKeyword:
			{
				IfStatementSyntax *const syntax = new IfStatementSyntax(parent);
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
				UnlessStatementSyntax *const syntax = new UnlessStatementSyntax(parent);
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
						ElseStatementSyntax *const syntax = new ElseStatementSyntax(parent);
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

WhileStatementSyntax *const SourceParser::ReadWhileStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	WhileStatementSyntax *const syntax = new WhileStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::WhileKeyword, L"Expected 'while' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

	return syntax;
}

UntilStatementSyntax *const SourceParser::ReadUntilStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	UntilStatementSyntax *const syntax = new UntilStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::UntilKeyword, L"Expected 'until' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);

	return syntax;
}

ForStatementSyntax *const SourceParser::ReadForStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	ForStatementSyntax *const syntax = new ForStatementSyntax(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::ForKeyword, L"Expected 'for' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");

	// Reading init statement
	syntax->InitializerStatement = ReadStatement(reader, syntax);
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax *const>(syntax->InitializerStatement))
		Diagnostics.ReportError(keywordStatement->KeywordToken, L"Cannot use keyword statements inside for loop initializer");

	// Reading first semicolon
	syntax->FirstSemicolon = Expect(reader, TokenType::Semicolon, L"expected ';' token");

	// Reading looping consition
	syntax->ConditionExpression = ReadExpression(reader, syntax, 0);

	// Reading second semicolon
	syntax->SecondSemicolon = Expect(reader, TokenType::Semicolon, L"expected ';' token");

	// Reading after loop statement
	syntax->AfterRepeatStatement = ReadStatement(reader, syntax);
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax *const>(syntax->AfterRepeatStatement))
		Diagnostics.ReportError(keywordStatement->KeywordToken, L"Cannot use keyword statements inside for loop repeater");

	// Reading close curl token
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");

	// Reading block
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax);
	return syntax;
}

TryStatementSyntax *const SourceParser::ReadTryStatement(SourceProvider& reader, SyntaxNode *const parent)
{
	TryStatementSyntax *const syntax = new TryStatementSyntax(parent);
	syntax->TryKeywordToken = Expect(reader, TokenType::TryKeyword, L"Expected 'try' keyword");
	syntax->TryBlock = ReadStatementsBlock(reader, syntax);

	while (reader.CanConsume() && reader.Current().Type == TokenType::CatchKeyword)
	{
		CatchClauseSyntax *const clause = new CatchClauseSyntax(syntax);
		clause->CatchKeywordToken = Expect(reader, TokenType::CatchKeyword, L"Expected 'catch' keyword");

		if (TryMatchIdentifier(reader, 3))
		{
			clause->IdentifierToken = reader.Current();
			reader.Consume();
		}

		if (reader.Current().Type == TokenType::Colon)
		{
			clause->ColonToken = reader.Current();
			reader.Consume();
			clause->ExceptionType = ReadType(reader, clause);
		}

		clause->Body = ReadStatementsBlock(reader, clause);
		syntax->CatchClauses.push_back(clause);
	}

	return syntax;
}

ExpressionSyntax *const SourceParser::ReadExpression(SourceProvider& reader, SyntaxNode *const parent, int bindingPower)
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

ExpressionSyntax *const SourceParser::ReadNullDenotation(SourceProvider& reader, SyntaxNode *const parent)
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
			UnaryExpressionSyntax *const syntax = new UnaryExpressionSyntax(current, false, parent);
			syntax->Expression = ReadNullDenotation(reader, syntax);
			return syntax;
		}

		case TokenType::NullLiteral:
		case TokenType::BooleanLiteral:
		case TokenType::NumberLiteral:
		case TokenType::DoubleLiteral:
		case TokenType::StringLiteral:
		case TokenType::CharLiteral:
		{
			reader.Consume();
			return new LiteralExpressionSyntax(current, parent);
		}

		case TokenType::OpenCurl:
		{
			reader.Consume();
			ExpressionSyntax *const expression = ReadExpression(reader, parent, 0);
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

		case TokenType::ThrowKeyword:
		{
			SyntaxToken throwToken = reader.Current();
			reader.Consume();
			UnaryExpressionSyntax *const syntax = new UnaryExpressionSyntax(throwToken, false, parent);
			syntax->Expression = ReadNullDenotation(reader, syntax);
			return syntax;
		}

		case TokenType::IfKeyword:
			return ReadIfExpression(reader, parent);

		case TokenType::SwitchKeyword:
			return ReadSwitchExpression(reader, parent);

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

ExpressionSyntax *const SourceParser::ReadLeftDenotation(SourceProvider& reader, SyntaxNode *const parent, ExpressionSyntax *const leftExpr)
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

		case TokenType::NullCoalescingOperator:
		{
			int precendce = GetOperatorPrecendence(current.Type);
			if (precendce == 0)
				return leftExpr;

			reader.Consume();
			BinaryExpressionSyntax *const syntax = new BinaryExpressionSyntax(current, parent);
			*const_cast<SyntaxNode**>(&leftExpr->Parent) = syntax;
			syntax->Left = leftExpr;
			syntax->Right = ReadExpression(reader, syntax, precendce);
			return syntax;
		}
	}

	if (IsRightUnaryOperator(current.Type))
	{
		int precendce = GetOperatorPrecendence(current.Type);
		if (precendce == 0)
			return leftExpr;

		reader.Consume();
		UnaryExpressionSyntax *const syntax = new UnaryExpressionSyntax(current, true, parent);
		syntax->Expression = leftExpr;
		return syntax;
	}

	if (IsBinaryOperator(current.Type) || current.Type == TokenType::AssignOperator)
	{
		int precendce = GetOperatorPrecendence(current.Type);
		if (precendce == 0)
			return leftExpr;

		reader.Consume();
		BinaryExpressionSyntax *const syntax = new BinaryExpressionSyntax(current, parent);
		*const_cast<SyntaxNode**>(&leftExpr->Parent) = syntax;

		syntax->Left = leftExpr;
		syntax->Right = ReadExpression(reader, syntax, precendce);
		return syntax;
	}

	// Unknown token - return left expression instead of null
	Diagnostics.ReportError(current, L"Unknown token in expression's left denotation");
	reader.Consume();
	return leftExpr;
}

ObjectExpressionSyntax *const SourceParser::ReadObjectExpression(SourceProvider& reader, SyntaxNode *const parent)
{
	ObjectExpressionSyntax *const syntax = new ObjectExpressionSyntax(parent);
	syntax->NewToken = Expect(reader, TokenType::NewKeyword, L"Expected 'new' keyword");
	syntax->Type = ReadType(reader, syntax);
	syntax->ArgumentsList = ReadArgumentsList(reader, syntax);
	return syntax;
}

TernaryExpressionSyntax *const SourceParser::ReadTernaryExpression(SourceProvider& reader, ExpressionSyntax *const condition, SyntaxNode *const parent)
{
	TernaryExpressionSyntax *const syntax = new TernaryExpressionSyntax(parent);
	syntax->Condition = condition;

	syntax->QuestionToken = Expect(reader, TokenType::Question, L"Expected '?' token");
	syntax->Left = ReadExpression(reader, syntax, 0);

	syntax->QuestionToken = Expect(reader, TokenType::Colon, L"Expected ':' token");
	syntax->Right = ReadExpression(reader, syntax, 0);

	return syntax;
}

CollectionExpressionSyntax *const SourceParser::ReadCollectionExpression(SourceProvider& reader, SyntaxNode *const parent)
{
	CollectionExpressionSyntax *const syntax = new CollectionExpressionSyntax(parent);
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
		ExpressionSyntax *const expr = ReadExpression(reader, syntax, 0);
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

LambdaExpressionSyntax *const SourceParser::ReadLambdaExpression(SourceProvider& reader, SyntaxNode *const parent)
{
	LambdaExpressionSyntax *const syntax = new LambdaExpressionSyntax(parent);
	syntax->LambdaToken = Expect(reader, TokenType::LambdaKeyword, L"Expected 'lambda' keyword");
	syntax->Params = ReadParametersList(reader, syntax);
	syntax->LambdaOperatorToken = Expect(reader, TokenType::LambdaOperator, L"Expected '=>' operator");
	syntax->Body = ReadStatementsBlock(reader, syntax);
	return syntax;
}

IfExpressionSyntax *const SourceParser::ReadIfExpression(SourceProvider& reader, SyntaxNode *const parent)
{
	IfExpressionSyntax *const syntax = new IfExpressionSyntax(parent);
	syntax->IfKeywordToken = Expect(reader, TokenType::IfKeyword, L"Expected 'if' keyword");
	syntax->Condition = ReadExpression(reader, syntax, 0);
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{'");
	syntax->ThenExpression = ReadExpression(reader, syntax, 0);
	syntax->CloseBraceToken = Expect(reader, TokenType::CloseBrace, L"Expected '}'");

	if (reader.Current().Type == TokenType::ElseKeyword)
	{
		syntax->ElseKeywordToken = reader.Current();
		reader.Consume();
		syntax->ElseExpression = ReadExpression(reader, syntax, 0);
	}

	return syntax;
}

SwitchExpressionSyntax *const SourceParser::ReadSwitchExpression(SourceProvider& reader, SyntaxNode *const parent)
{
	SwitchExpressionSyntax *const syntax = new SwitchExpressionSyntax(parent);
	syntax->SwitchKeywordToken = Expect(reader, TokenType::SwitchKeyword, L"Expected 'switch' keyword");
	syntax->Expression = ReadExpression(reader, syntax, 0);
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{'");

	while (reader.CanConsume() && reader.Current().Type != TokenType::CloseBrace)
	{
		SwitchArmSyntax *const arm = new SwitchArmSyntax(syntax);

		if (reader.Current().Type == TokenType::Identifier && reader.Current().Word == L"_") // '_' as default
		{
			arm->Pattern = new LiteralExpressionSyntax(SyntaxToken(TokenType::Identifier, L"_", reader.Current().Location, false), arm);
			reader.Consume();
		}
		else
		{
			arm->Pattern = ReadExpression(reader, arm, 0);
		}

		arm->ArrowToken = Expect(reader, TokenType::LambdaOperator, L"Expected '=>'");
		arm->Expression = ReadExpression(reader, arm, 0);
		syntax->Arms.push_back(arm);

		if (reader.Current().Type == TokenType::Comma)
			reader.Consume();
	}

	syntax->CloseBraceToken = Expect(reader, TokenType::CloseBrace, L"Expected '}'");
	return syntax;
}

LinkedExpressionNode *const SourceParser::ReadLinkedExpressionNode(SourceProvider& reader, SyntaxNode *const parent, ExpressionSyntax *const previous, bool isFirst)
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

	LinkedExpressionNode* currentNode = nullptr;
	SyntaxToken current = reader.Current();

	switch (current.Type)
	{
		case TokenType::OpenCurl:
		{
			InvokationExpressionSyntax* invokation = new InvokationExpressionSyntax(identifier, previous, parent);
			invokation->DelimeterToken = delimeter;
			invokation->ArgumentsList = ReadArgumentsList(reader, invokation);

			currentNode = invokation;
			break;
		}

		case TokenType::Delimeter:
		{
			MemberAccessExpressionSyntax* currentNode = new MemberAccessExpressionSyntax(identifier, previous, parent);
			currentNode->DelimeterToken = delimeter;

			LinkedExpressionNode *const nextNode = ReadLinkedExpressionNode(reader, parent, currentNode, false);
			return nextNode;
		}

		case TokenType::OpenSquare:
		{
			MemberAccessExpressionSyntax *const member = new MemberAccessExpressionSyntax(identifier, previous, parent);
			member->DelimeterToken = delimeter;

			currentNode = ReadIndexatorExpressionNode(reader, parent, member, false);
			break;
		}

		default:
		{
			if (!reader.CanConsume())
				return currentNode;

			if (IsOperator(current.Type) || IsPunctuation(current.Type))
			{
				MemberAccessExpressionSyntax *const currentNode = new MemberAccessExpressionSyntax(identifier, previous, parent);
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

	if (!reader.CanConsume())
		return currentNode;

	current = reader.Current();
	if (current.Type == TokenType::OpenSquare)
		currentNode = ReadIndexatorExpressionNode(reader, parent, currentNode, false);

	current = reader.Current();
	if (current.Type == TokenType::Delimeter)
		currentNode = ReadLinkedExpressionNode(reader, parent, currentNode, false);

	return currentNode;
}

IndexatorExpressionSyntax* const SourceParser::ReadIndexatorExpressionNode(SourceProvider& reader, SyntaxNode* const parent, ExpressionSyntax* const lastNode, bool isFirst)
{
	static SyntaxToken indexerKeyword(TokenType::IndexerKeyword, L"indexer", TextLocation(), false);
	IndexatorExpressionSyntax* currentNode = new IndexatorExpressionSyntax(indexerKeyword, lastNode, parent);
	currentNode->IndexatorList = ReadIndexatorList(reader, currentNode);

	if (!reader.CanConsume())
		return currentNode;

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::OpenSquare)
		currentNode = ReadIndexatorExpressionNode(reader, parent, currentNode, false);

	return currentNode;
}

ArgumentsListSyntax *const SourceParser::ReadArgumentsList(SourceProvider& reader, SyntaxNode *const parent)
{
	ArgumentsListSyntax *const arguments = new ArgumentsListSyntax(parent);
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
		ExpressionSyntax *const expr = ReadExpression(reader, arguments, 0);
		ArgumentSyntax *const argument = new ArgumentSyntax(expr, arguments);
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

IndexatorListSyntax *const SourceParser::ReadIndexatorList(SourceProvider& reader, SyntaxNode *const parent)
{
	IndexatorListSyntax *const arguments = new IndexatorListSyntax(parent);
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
		ExpressionSyntax *const expr = ReadExpression(reader, arguments, 0);
		ArgumentSyntax *const argument = new ArgumentSyntax(expr, arguments);
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

TypeSyntax *const SourceParser::ReadType(SourceProvider& reader, SyntaxNode *const parent)
{
	SyntaxToken current = reader.Current();

	if (IsPredefinedType(current.Type))
	{
		reader.Consume();
		PredefinedTypeSyntax *const predefinedType = new PredefinedTypeSyntax(current, parent);
		TypeSyntax *const syntax = ReadModifiedType(reader, predefinedType, parent);
		return syntax;
	}

	if (current.Type == TokenType::DelegateKeyword)
	{
		TypeSyntax *const identifierType = ReadDelegateType(reader, parent);
		return identifierType;
	}

	if (current.Type == TokenType::Identifier)
	{
		TypeSyntax *const identifierType = ReadIdentifierNameType(reader, parent);
		return identifierType;
	}

	Diagnostics.ReportError(current, L"Unexpected token in type syntax");
	return nullptr;
}

TypeSyntax *const SourceParser::ReadModifiedType(SourceProvider& reader, TypeSyntax *const type, SyntaxNode *const parent)
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
			NullableTypeSyntax *const nullable = new NullableTypeSyntax(type, parent);
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

TypeSyntax *const SourceParser::ReadIdentifierNameType(SourceProvider& reader, SyntaxNode *const parent)
{
	IdentifierNameTypeSyntax *const identifier = new IdentifierNameTypeSyntax(parent);
	identifier->Identifier = Expect(reader, TokenType::Identifier, L"Expected identifier");

	TypeSyntax *const modifiedSyntax = ReadModifiedType(reader, identifier, parent);
	return modifiedSyntax;
}

TypeSyntax *const SourceParser::ReadDelegateType(SourceProvider& reader, SyntaxNode *const parent)
{
	DelegateTypeSyntax *const delegate = new DelegateTypeSyntax(parent);
	delegate->DelegateToken = Expect(reader, TokenType::DelegateKeyword, L"Excpected 'lambda' keyword");
	delegate->ReturnType = ReadType(reader, delegate);
	delegate->Params = ReadParametersList(reader, delegate);

	TypeSyntax *const modifiedSyntax = ReadModifiedType(reader, delegate, parent);
	return modifiedSyntax;
}

TypeSyntax *const SourceParser::ReadArrayType(SourceProvider& reader, TypeSyntax *const previous, SyntaxNode *const parent)
{
	ArrayTypeSyntax *const array = new ArrayTypeSyntax(previous, parent);
	array->OpenSquareToken = Expect(reader, TokenType::OpenSquare, L"Expected '['");
	array->CloseSquareToken = Expect(reader, TokenType::CloseSquare, L"Expected ']'");
	array->Rank = 1;

	TypeSyntax *const modifiedSyntax = ReadModifiedType(reader, array, parent);
	return modifiedSyntax;
}

TypeSyntax *const SourceParser::ReadGenericType(SourceProvider& reader, TypeSyntax *const previous, SyntaxNode *const parent)
{
	GenericTypeSyntax *const generic = new GenericTypeSyntax(previous, parent);
	generic->Arguments = ReadTypeArgumentsList(reader, generic);
	TypeSyntax *const modifiedSyntax = ReadModifiedType(reader, generic, parent);
	return modifiedSyntax;
}

// Smart error recovery with synchronization tokens
static const std::vector<shard::TokenType> SynchronizationTokens = {
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

static bool IsSynchronizationToken(shard::TokenType type)
{
	for (shard::TokenType syncToken : SynchronizationTokens)
	{
		if (syncToken == type)
			return true;
	}

	return false;
}

static bool TrySynchronize(SourceProvider& reader, const std::vector<shard::TokenType>& expectedTokens, int maxSkips = 10)
{
	// Skip tokens until we find a synchronization point or expected token
	int skipped = 0;
	while (reader.CanConsume() && skipped < maxSkips)
	{
		SyntaxToken current = reader.Current();

		// Check if current token is one of expected
		for (shard::TokenType expected : expectedTokens)
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

SyntaxToken SourceParser::Expect(SourceProvider& reader, TokenType kind, const wchar_t* message)
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

bool SourceParser::Matches(SourceProvider& reader, std::initializer_list<TokenType> types)
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

bool SourceParser::TryMatch(SourceProvider& reader, std::initializer_list<TokenType> types, const wchar_t* errorMessage, int maxSkips)
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

bool SourceParser::TryMatchIdentifier(shard::SourceProvider& reader, int maxSkips)
{
	if (!reader.CanConsume())
		return false;

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Identifier)
		return true;

	if (IsReservedIdentifier(current.Type))
	{
		Diagnostics.ReportError(current, L"Identifier cannot be a reserved keyword.");
		return false;
	}

	// Try to synchronize to one of expected tokens
	std::vector<TokenType> expectedTypes({ TokenType::Identifier });
	if (TrySynchronize(reader, expectedTypes, maxSkips))
		return true;

	return false;
}