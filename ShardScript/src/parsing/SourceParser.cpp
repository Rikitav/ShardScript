#include <shard/parsing/SourceParser.hpp>
#include <shard/lexical/SourceProvider.hpp>
#include <shard/parsing/MemberDeclarationInfo.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/analysis/TextLocation.hpp>
#include <shard/analysis/DiagnosticsContext.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/lexical/TokenType.hpp>
#include <shard/parsing/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/AttributeSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/BodyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/parsing/nodes/TypeArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/TypeParametersListSyntax.hpp>

#include <shard/parsing/nodes/Loops/ForStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/UntilStatementSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/DelegateDeclarationSyntax.hpp>

#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>

#include <shard/parsing/nodes/Statements/ConditionalClauseSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>

#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TypeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/TernaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/IsExpressionSyntax.hpp>

#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/QualifiedNameTypeSyntax.hpp>
//#include <shard/parsing/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/DelegateTypeSyntax.hpp>

#include <vector>
#include <initializer_list>
#include <set>
#include <new>
#include <cwctype>

using namespace shard;

static bool IsTypeStartToken(const shard::TokenType type);

static bool CanStartCompilationUnit(const SyntaxToken& token)
{
	return token.Type == TokenType::UsingKeyword
	    || token.Type == TokenType::NamespaceKeyword
	    || IsModifier(token.Type)
	    || IsMemberKeyword(token.Type)
	    || IsTypeKeyword(token.Type)
	    || token.Type == TokenType::FunctionKeyword
	    || token.Type == TokenType::InitKeyword
	    || token.Type == TokenType::OpenSquare;
}

static void SynchronizeToNextTopLevel(SourceProvider& reader)
{
	int braceDepth = 0;
	int parenDepth = 0;

	while (reader.CanConsume())
	{
		SyntaxToken token = reader.Current();

		if (braceDepth == 0 && parenDepth == 0 && CanStartCompilationUnit(token))
			return;

		switch (token.Type)
		{
			case TokenType::OpenBrace:
				++braceDepth;
				break;
			case TokenType::CloseBrace:
				if (braceDepth > 0)
					--braceDepth;
				break;
			case TokenType::OpenCurl:
				++parenDepth;
				break;
			case TokenType::CloseCurl:
				if (parenDepth > 0)
					--parenDepth;
				break;
			default:
				break;
		}

		reader.Consume();
	}
}

void SourceParser::FromSourceProvider(SyntaxTree& syntaxTree, SourceProvider& reader)
{
	auto unit = ReadCompilationUnit(reader);
	syntaxTree.CompilationUnits.push_back(std::move(unit));
}

std::unique_ptr<CompilationUnitSyntax> SourceParser::ReadCompilationUnit(SourceProvider& reader)
{
	ExpressionDepth = 0;
	OperatorsInExpression = 0;
	BlockDepth = 0;
	LinkedExpressionDepth = 0;
	auto unit = std::make_unique<CompilationUnitSyntax>();
	CompilationUnitSyntax* rawUnit = unit.get();

	int loopGuard = 0;
	while (reader.CanConsume())
	{
		if (++loopGuard > 10000)
		{
			Diagnostics.ReportError(reader.Current(), L"Parser loop detected - aborting compilation unit");
			throw std::runtime_error("parser loop detected");
		}

		SyntaxToken token = reader.Current();
		switch (token.Type)
		{
			case TokenType::UsingKeyword:
			{
				if (rawUnit->Namespace != nullptr || !rawUnit->Members.empty())
				{
					Diagnostics.ReportError(token, L"Using directive must be declared at the top of the compilation unit");
				}

				auto pDirective = ReadUsingDirective(reader, rawUnit);
				pDirective->Parent = rawUnit;
				rawUnit->Usings.push_back(std::move(pDirective));
				break;
			}

			case TokenType::NamespaceKeyword:
			{
				if (!rawUnit->Members.empty())
				{
					Diagnostics.ReportError(token, L"Namespace directive must be declared before any member declarations");
				}

				if (rawUnit->Namespace != nullptr)
				{
					Diagnostics.ReportError(token, L"Only one namespace declaration is allowed per compilation unit");
					reader.Consume();
					
					while (reader.CanConsume() && reader.Current().Type != TokenType::Semicolon)
						reader.Consume();

					if (reader.CanConsume())
						reader.Consume();
					
					break;
				}

				auto pNamespace = ReadNamespaceDeclaration(reader, rawUnit);
				pNamespace->Parent = rawUnit;
				rawUnit->Namespace = std::move(pNamespace);
				break;
			}

			default:
			{
				SyntaxToken peek = reader.Peek();
				if (IsMemberDeclaration(token.Type, peek.Type))
				{
					auto pMember = ReadMemberDeclaration(reader, rawUnit);
					if (pMember == nullptr)
					{
						SynchronizeToNextTopLevel(reader);
						break;
					}

					pMember->Parent = rawUnit;
					rawUnit->Members.push_back(std::move(pMember));
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

std::unique_ptr<UsingDirectiveSyntax> SourceParser::ReadUsingDirective(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<UsingDirectiveSyntax>(parent);
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

std::unique_ptr<NamespaceDeclarationSyntax> SourceParser::ReadNamespaceDeclaration(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<NamespaceDeclarationSyntax>(parent);
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
		reader.Consume(); // consume '.'
		if (!TryMatchIdentifier(reader, 5))
		{
			// Error recovery: expected identifier after '.'
			break;
		}

		SyntaxToken identifier = reader.Current();
		syntax->IdentifierTokens.push_back(identifier);
		reader.Consume();
		current = reader.Current();
	}

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Expected ';' after namespace declaration");
	return syntax;
}

std::unique_ptr<MemberDeclarationSyntax> SourceParser::ReadMemberDeclaration(SourceProvider& reader, SyntaxNode* parent)
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

			case TokenType::EnumKeyword:
				return ReadEnumDeclaration(reader, info, parent);

			default:
				Diagnostics.ReportError(info.DeclareType, L"Unsupported member keyword");
				return nullptr;
		}
	}

	// Declaration prediction: func name<T>(params) -> type { }
	if (current.Type == TokenType::FunctionKeyword)
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

		if (reader.Current().Type == TokenType::LessOperator)
			info.Generics = ReadTypeParametersList(reader, parent);

		return ReadMethodDeclaration(reader, info, parent);
	}

	// Declaration prediction: operator +(params) -> type { }
	if (current.Type == TokenType::OperatorKeyword)
	{
		reader.Consume(); // operator
		return ReadOperatorDeclaration(reader, info, parent);
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
			if (parent->Kind == SyntaxKind::InterfaceDeclaration)
			{
				if (next.Type == TokenType::AssignOperator)
				{
					Diagnostics.ReportError(next, L"Interfaces cannot contain fields");
					return nullptr;
				}

				auto syntax = std::make_unique<PropertyDeclarationSyntax>(info, parent);
				syntax->SemicolonToken = next;
				reader.Consume();

				auto getter = std::make_unique<AccessorDeclarationSyntax>(syntax.get());
				getter->KeywordToken = SyntaxToken(TokenType::GetKeyword, L"get", info.Identifier.Location, false);
				syntax->Getter = std::move(getter);

				return syntax;
			}

			return ReadFieldDeclaration(reader, info, parent);
		}
		else
		{
			Diagnostics.ReportError(next, L"Expected '{', '=>', '=' or ';' after type");
			return nullptr;
		}
	}

	if (IsPredefinedType(current.Type))
	{
		Diagnostics.ReportError(current, L"Expected 'func' keyword. In ShardScript methods are declared as 'func Name() -> Type'");
		return nullptr;
	}

	Diagnostics.ReportError(current, L"Expected member declaration");
	reader.Consume();
	return nullptr;
}

std::unique_ptr<ClassDeclarationSyntax> SourceParser::ReadClassDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ClassDeclarationSyntax>(info, parent);
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
		syntax->TypeParameters = ReadTypeParametersList(reader, syntax.get());
	}

	current = reader.Current();
	if (current.Type == TokenType::Colon)
	{
		reader.Consume();
		syntax->BaseInterfaces = ReadBaseInterfacesList(reader, syntax.get());
	}

	if (TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected class body '{' or semicolon ';'", 5))
	{
		current = reader.Current();
		if (current.Type == TokenType::OpenBrace)
		{
			ReadTypeBody(reader, syntax.get());
		}
	}

	return syntax;
}

std::unique_ptr<StructDeclarationSyntax> SourceParser::ReadStructDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<StructDeclarationSyntax>(info, parent);

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
		syntax->TypeParameters = ReadTypeParametersList(reader, syntax.get());
	}

	current = reader.Current();
	if (current.Type == TokenType::Colon)
	{
		reader.Consume();
		syntax->BaseInterfaces = ReadBaseInterfacesList(reader, syntax.get());
	}

	if (TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected struct body '{' or semicolon ';'", 5))
	{
		current = reader.Current();
		if (current.Type == TokenType::OpenBrace)
		{
			ReadTypeBody(reader, syntax.get());
		}
	}

	return syntax;
}

std::unique_ptr<ConstructorDeclarationSyntax> SourceParser::ReadConstructorDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ConstructorDeclarationSyntax>(info, parent);
	syntax->ParametersList = ReadParametersList(reader, syntax.get());

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		syntax->Semicolon = current;
		reader.Consume();
		return syntax;
	}

	syntax->Body = ReadStatementsBlock(reader, syntax.get());
	return syntax;
}

std::unique_ptr<MethodDeclarationSyntax> SourceParser::ReadMethodDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<MethodDeclarationSyntax>(info, parent);
	syntax->ParametersList = ReadParametersList(reader, syntax.get());

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::ArrowOperator)
	{
		reader.Consume(); // ->
		syntax->ReturnType = ReadType(reader, syntax.get());
	}
	else
	{
		//syntax->ReturnType = new PredefinedTypeSyntax(SyntaxToken(TokenType::VoidKeyword, L"void", TextLocation(), false), syntax);
		if (current.Type == TokenType::Identifier)
		{
			Diagnostics.ReportError(current, L"Expected '->' before return type.");
		}
		else if (current.Type == TokenType::OpenBrace)
		{
			Diagnostics.ReportError(current, L"Expected arrow and defined return type before block.");
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

	syntax->Body = ReadStatementsBlock(reader, syntax.get());
	return syntax;
}

std::unique_ptr<OperatorDeclarationSyntax> SourceParser::ReadOperatorDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	SyntaxToken operatorToken = reader.Current();
	if (operatorToken.Type == TokenType::Identifier)
	{
		TokenType mapped = GetTokenTypeFromOperatorName(operatorToken.Word);
		if (mapped != TokenType::Unknown)
			operatorToken = SyntaxToken(mapped, operatorToken.Word, operatorToken.Location, operatorToken.IsMissing);
	}

	if (!IsOverloadableOperator(operatorToken.Type))
	{
		Diagnostics.ReportError(operatorToken, L"Expected overloadable operator token");
		operatorToken = SyntaxToken(TokenType::Unknown, L"", TextLocation(), true);
	}
	reader.Consume();

	auto syntax = std::make_unique<OperatorDeclarationSyntax>(info, operatorToken, parent);
	syntax->ParametersList = ReadParametersList(reader, syntax.get());

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::ArrowOperator)
	{
		reader.Consume(); // ->
		syntax->ReturnType = ReadType(reader, syntax.get());
	}
	else
	{
		if (current.Type == TokenType::Identifier)
		{
			Diagnostics.ReportError(current, L"Expected '->' before return type.");
		}
		else if (current.Type == TokenType::OpenBrace)
		{
			Diagnostics.ReportError(current, L"Expected arrow and defined return type before block.");
		}
		else
		{
			Diagnostics.ReportError(current, L"Operator must have a return type.");
		}
	}

	current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		syntax->Semicolon = current;
		reader.Consume();
		return syntax;
	}

	syntax->Body = ReadStatementsBlock(reader, syntax.get());
	return syntax;
}

std::unique_ptr<FieldDeclarationSyntax> SourceParser::ReadFieldDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<FieldDeclarationSyntax>(info, parent);

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::AssignOperator:
		{
			syntax->InitializerAssignToken = current;
			reader.Consume();

			syntax->InitializerExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));
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

std::unique_ptr<PropertyDeclarationSyntax> SourceParser::ReadComputedPropertyDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto property = std::make_unique<PropertyDeclarationSyntax>(info, parent);
	property->ArrowToken = Expect(reader, TokenType::ArrowOperator, L"Expected '=>'");
	property->InitializerExpression = std::move(ReadExpression(reader, property.get(), 0, true));
	property->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
	return property;
}

std::vector<std::unique_ptr<TypeSyntax>> SourceParser::ReadBaseInterfacesList(SourceProvider& reader, SyntaxNode* parent)
{
    std::vector<std::unique_ptr<TypeSyntax>> interfaces;

    while (reader.CanConsume())
    {
        SyntaxToken current = reader.Current();
        if (current.Type != TokenType::Identifier)
        {
            Diagnostics.ReportError(current, L"Expected interface name");
            break;
        }

        auto type = ReadType(reader, parent);
        if (type != nullptr)
            interfaces.push_back(std::move(type));

        current = reader.Current();
        if (current.Type == TokenType::Comma)
        {
            reader.Consume();
            continue;
        }

        break;
    }

    return interfaces;
}

std::unique_ptr<InterfaceDeclarationSyntax> SourceParser::ReadInterfaceDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<InterfaceDeclarationSyntax>(info, parent);

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
		syntax->TypeParameters = ReadTypeParametersList(reader, syntax.get());
	}

	if (TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected interface body '{' or semicolon ';'", 5))
	{
		current = reader.Current();
		if (current.Type == TokenType::OpenBrace)
		{
			ReadTypeBody(reader, syntax.get());
		}
	}

	return syntax;
}

std::unique_ptr<DelegateDeclarationSyntax> SourceParser::ReadDelegateDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<DelegateDeclarationSyntax>(info, parent);

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
		syntax->TypeParameters = ReadTypeParametersList(reader, syntax.get());
	}

	syntax->ParametersList = ReadParametersList(reader, syntax.get());

	if (reader.Current().Type == TokenType::ArrowOperator)
	{
		reader.Consume();
		syntax->ReturnType = ReadType(reader, syntax.get());
	}

	syntax->Semicolon = Expect(reader, TokenType::Semicolon, L"Expected ';' token");
	return syntax;
}

std::unique_ptr<EnumDeclarationSyntax> SourceParser::ReadEnumDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<EnumDeclarationSyntax>(info, parent);

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
	if (current.Type == TokenType::Colon)
	{
		syntax->ColonToken = current;
		reader.Consume();

		SyntaxToken next = reader.Current();
		if (next.Type == TokenType::Identifier && next.Word == L"flags")
		{
			syntax->IsFlags = true;
			syntax->UnderlyingTypeToken = next;
			reader.Consume();
		}
		else if (next.Type == TokenType::IntegerKeyword || next.Type == TokenType::LongKeyword)
		{
			syntax->UnderlyingTypeToken = next;
			reader.Consume();
		}
		else
		{
			Diagnostics.ReportError(next, L"Expected 'flags', 'int' or 'long' after ':' in enum declaration");
		}
	}

	if (TryMatch(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected enum body '{' or semicolon ';'", 5))
	{
		current = reader.Current();
		if (current.Type == TokenType::OpenBrace)
		{
			ReadEnumBody(reader, syntax.get());
		}
	}

	return syntax;
}

void SourceParser::ReadEnumBody(SourceProvider& reader, EnumDeclarationSyntax* syntax)
{
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{'");

	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		if (current.Type == TokenType::EndOfFile)
		{
			Diagnostics.ReportError(current, L"Unexpected end of file in enum body - expected '}'");
			syntax->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
			break;
		}

		if (current.Type == TokenType::CloseBrace)
		{
			syntax->CloseBraceToken = current;
			reader.Consume();
			break;
		}

		if (current.Type == TokenType::Identifier)
		{
			auto field = ReadEnumFieldDeclaration(reader, syntax);
			if (field != nullptr)
			{
				field->Parent = syntax;
				syntax->Fields.push_back(std::move(field));
			}

		}
		else
		{
			Diagnostics.ReportError(current, L"Expected enum field identifier");
			reader.Consume();
		}

		current = reader.Current();
		if (current.Type == TokenType::Comma)
		{
			reader.Consume();
			continue;
		}
		else if (current.Type == TokenType::CloseBrace)
		{
			syntax->CloseBraceToken = current;
			reader.Consume();
			break;
		}
		else if (current.Type == TokenType::EndOfFile)
		{
			Diagnostics.ReportError(current, L"Unexpected end of file in enum body - expected '}'");
			syntax->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
			break;
		}
		else
		{
			Diagnostics.ReportError(current, L"Expected ',' or '}' after enum field");
			break;
		}
	}
}

std::unique_ptr<EnumFieldDeclarationSyntax> SourceParser::ReadEnumFieldDeclaration(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<EnumFieldDeclarationSyntax>(parent);

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Identifier)
	{
		syntax->IdentifierToken = current;
		reader.Consume();
	}
	else
	{
		syntax->IdentifierToken = SyntaxToken(TokenType::Identifier, L"", TextLocation(), true);
	}

	current = reader.Current();
	if (current.Type == TokenType::AssignOperator)
	{
		syntax->AssignToken = current;
		reader.Consume();
		syntax->InitializerExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));
	}

	return syntax;
}

std::unique_ptr<PropertyDeclarationSyntax> SourceParser::ReadPropertyDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<PropertyDeclarationSyntax>(info, parent);
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
			auto accessor = ReadAccessorDeclaration(reader, syntax.get());
			switch (accessor->KeywordToken.Type)
			{
				case TokenType::GetKeyword:
				{
					if (syntax->Getter != nullptr)
					{
						Diagnostics.ReportError(current, L"Duplicate get accessor");
						break;
					}

					syntax->Getter = std::move(accessor);
					break;
				}

				case TokenType::SetKeyword:
				{
					if (syntax->Setter != nullptr)
					{
						Diagnostics.ReportError(current, L"Duplicate set accessor");
						break;
					}
					
					syntax->Setter = std::move(accessor);
					break;
				}

				default:
				{
					Diagnostics.ReportError(current, L"Expected 'get' or 'set' keyword for property accessor");
					break;
				}
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

std::unique_ptr<IndexatorDeclarationSyntax> SourceParser::ReadIndexatorDeclaration(SourceProvider& reader, MemberDeclarationInfo& info, SyntaxNode* parent)
{
	auto syntax = std::make_unique<IndexatorDeclarationSyntax>(info, parent);
	//syntax->IndexKeyword = Expect(reader, TokenType::IndexerKeyword, L"Expected 'index' keyword");
	syntax->IndexKeyword = info.Identifier;
	syntax->ParametersList = ReadIndexerParametersList(reader, syntax.get());

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Colon)
	{
		reader.Consume(); // :
		syntax->ReturnType = ReadType(reader, syntax.get());
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
			auto accessor = ReadAccessorDeclaration(reader, syntax.get());
			switch (accessor->KeywordToken.Type)
			{
				case TokenType::GetKeyword:
				{
					if (syntax->Getter != nullptr)
					{
						Diagnostics.ReportError(current, L"Duplicate get accessor");
						break;
					}

					syntax->Getter = std::move(accessor);
					break;
				}

				case TokenType::SetKeyword:
				{
					if (syntax->Setter != nullptr)
					{
						Diagnostics.ReportError(current, L"Duplicate set accessor");
						break;
					}

					syntax->Setter = std::move(accessor);
					break;
				}

				default:
				{
					Diagnostics.ReportError(current, L"Expected 'get' or 'set' keyword for property accessor");
					break;
				}
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

std::unique_ptr<AccessorDeclarationSyntax> SourceParser::ReadAccessorDeclaration(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<AccessorDeclarationSyntax>(parent);
	syntax->Attributes = ReadAttributeList(reader, syntax.get());

	if (!reader.CanConsume())
	{
		Diagnostics.ReportError(SyntaxToken(TokenType::EndOfFile, L"", TextLocation()), L"Unexpected end of file after accessor attributes");
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
			syntax->Body = ReadStatementsBlock(reader, syntax.get());
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
			reader.Consume();
			continue;
		}

		// Check order
		int modifierIndex = -1;
		for (std::size_t i = 0; i < expectedOrder.size(); i++)
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

std::unique_ptr<AttributeSyntax> SourceParser::ReadAttribute(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<AttributeSyntax>(parent);
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

std::vector<std::unique_ptr<AttributeSyntax>> SourceParser::ReadAttributeList(SourceProvider& reader, SyntaxNode* parent)
{
	std::vector<std::unique_ptr<AttributeSyntax>> attributes;
	while (reader.CanConsume() && reader.Current().Type == TokenType::OpenSquare)
	{
		auto attr = ReadAttribute(reader, parent);
		if (attr != nullptr)
			attributes.push_back(std::move(attr));
	}

	return attributes;
}

std::unique_ptr<ParametersListSyntax> SourceParser::ReadIndexerParametersList(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ParametersListSyntax>(parent);
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
		auto type = ReadType(reader, syntax.get());

		auto param = std::make_unique<ParameterSyntax>(std::move(type), identifierToken, syntax.get());
		syntax->Parameters.push_back(std::move(param));

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

std::unique_ptr<ParametersListSyntax> SourceParser::ReadParametersList(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ParametersListSyntax>(parent);
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
		SyntaxToken currentToken = reader.Current();
		if (currentToken.Type == TokenType::ValueKeyword)
		{
			// 'value' is a contextual keyword; allow it as a parameter name.
			identifierToken = SyntaxToken(TokenType::Identifier, currentToken.Word, currentToken.Location, currentToken.IsMissing);
			reader.Consume();
		}
		else if (!TryMatchIdentifier(reader, 3))
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
		auto type = ReadType(reader, syntax.get());

		auto param = std::make_unique<ParameterSyntax>(std::move(type), identifierToken, syntax.get());
		syntax->Parameters.push_back(std::move(param));

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

std::unique_ptr<ParametersListSyntax> SourceParser::ReadDelegateParametersList(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ParametersListSyntax>(parent);
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
		auto type = ReadType(reader, syntax.get());
		auto param = std::make_unique<ParameterSyntax>(std::move(type), SyntaxToken(), syntax.get());
		syntax->Parameters.push_back(std::move(param));

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

std::unique_ptr<TypeParametersListSyntax> SourceParser::ReadTypeParametersList(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<TypeParametersListSyntax>(parent);
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

std::unique_ptr<TypeArgumentsListSyntax> SourceParser::ReadTypeArgumentsList(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<TypeArgumentsListSyntax>(parent);
	syntax->OpenToken = Expect(reader, TokenType::LessOperator, L"Expected '<' token");

	SyntaxToken checkCloser = reader.Current();
	if (checkCloser.Type == TokenType::GreaterOperator)
	{
		syntax->CloseToken = checkCloser;
		reader.Consume();
		return syntax;
	}

	if (checkCloser.Type == TokenType::RightShiftOperator)
	{
		// Split the >> operator into two closing > tokens so nested
		// generic types like Container<Container<int>> parse correctly.
		reader.Consume();
		SyntaxToken firstGreater(TokenType::GreaterOperator, L">", checkCloser.Location, false);
		SyntaxToken secondGreater(TokenType::GreaterOperator, L">", checkCloser.Location, false);
		syntax->CloseToken = firstGreater;
		reader.PutBackToken(secondGreater);
		return syntax;
	}

	while (reader.CanConsume())
	{
		auto type = ReadType(reader, syntax.get());
		syntax->Types.push_back(std::move(type));

		SyntaxToken separatorToken = reader.Current();
		if (separatorToken.Type == TokenType::RightShiftOperator)
		{
			reader.Consume();
			SyntaxToken firstGreater(TokenType::GreaterOperator, L">", separatorToken.Location, false);
			SyntaxToken secondGreater(TokenType::GreaterOperator, L">", separatorToken.Location, false);
			syntax->CloseToken = firstGreater;
			reader.PutBackToken(secondGreater);
			break;
		}

		// Try to match separator with error recovery
		if (!TryMatch(reader, { TokenType::Comma, TokenType::GreaterOperator }, L"Expected ',' or '>'", 3))
		{
			break;
		}

		separatorToken = reader.Current();
		reader.Consume();

		if (separatorToken.Type == TokenType::GreaterOperator)
		{
			syntax->CloseToken = separatorToken;
			break;
		}
	}

	return syntax;
}

void SourceParser::ReadTypeBody(SourceProvider& reader, TypeDeclarationSyntax* syntax)
{
	int loopGuard = 0;
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{'");

	while (reader.CanConsume())
	{
		if (++loopGuard > 10000)
		{
			Diagnostics.ReportError(reader.Current(), L"Type body parser loop detected");
			break;
		}

		SyntaxToken current = reader.Current();
		if (current.Type == TokenType::EndOfFile)
		{
			Diagnostics.ReportError(current, L"Unexpected end of file in type body - expected '}'");
			syntax->CloseBraceToken = SyntaxToken(TokenType::CloseBrace, L"", current.Location, true);
			break;
		}

		if (current.Type == TokenType::CloseBrace)
		{
			syntax->CloseBraceToken = current;
			reader.Consume();
			break;
		}

		SyntaxToken peek = reader.Peek();
		if (IsMemberDeclaration(current.Type, peek.Type))
		{
			auto pMember = ReadMemberDeclaration(reader, syntax);
			if (pMember != nullptr)
			{
				pMember->Parent = syntax;
				syntax->Members.push_back(std::move(pMember));
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

std::unique_ptr<StatementsBlockSyntax> SourceParser::ReadStatementsBlock(SourceProvider& reader, SyntaxNode* parent)
{
	struct BlockDepthGuard
	{
		int& depth;
		BlockDepthGuard(int& d) : depth(d) { ++depth; }
		~BlockDepthGuard() { --depth; }
	};

	BlockDepthGuard guard(BlockDepth);
	if (BlockDepth > MaxBlockDepth)
	{
		Diagnostics.ReportError(reader.Current(), L"Statement nesting is too deep");
		// Skip the rest of this block to avoid stack overflow.
		while (reader.CanConsume() && reader.Current().Type != TokenType::CloseBrace)
			reader.Consume();
		if (reader.CanConsume())
			reader.Consume();
		auto syntax = std::make_unique<StatementsBlockSyntax>(parent);
		return syntax;
	}

	auto syntax = std::make_unique<StatementsBlockSyntax>(parent);
	int loopGuard = 0;

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
			if (++loopGuard > 10000)
			{
				Diagnostics.ReportError(reader.Current(), L"Statement block parser loop detected");
				break;
			}

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
				auto statement = ReadKeywordStatement(reader, syntax.get());
				if (statement != nullptr)
				{
					syntax->Statements.push_back(std::move(statement));
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
				auto statement = ReadStatement(reader, syntax.get());
				if (statement != nullptr)
				{
					statement->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
					syntax->Statements.push_back(std::move(statement));
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

			if (syntax->CloseBraceToken.Type != TokenType::CloseBrace)
			{
				TextLocation location = reader.CanConsume() ? reader.Current().Location : syntax->OpenBraceToken.Location;
				Diagnostics.ReportError(SyntaxToken(TokenType::EndOfFile, L"", location), L"Statement block opened here was not closed - expected '}'");
			}
	}
	else
	{
		// Single statement block
		if (IsKeyword(current.Type))
		{
			auto statement = ReadKeywordStatement(reader, syntax.get());
			if (statement != nullptr)
				syntax->Statements.push_back(std::move(statement));
		}
		else
		{
			auto statement = ReadStatement(reader, syntax.get());
			if (statement != nullptr)
			{
				statement->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
				syntax->Statements.push_back(std::move(statement));
			}
		}
	}

	return syntax;
}

std::unique_ptr<StatementSyntax> SourceParser::ReadStatement(SourceProvider& reader, SyntaxNode* parent)
{
	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		return std::make_unique<ExpressionStatementSyntax>(nullptr, parent);
	}

	if (reader.CanPeek())
	{
		SyntaxToken peek = reader.Peek();

		// name: type = value
		if (current.Type == TokenType::Identifier && peek.Type == TokenType::Colon)
		{
			reader.Consume(); // name
			reader.Consume(); // :
			auto type = ReadType(reader, parent);
			
			if (type == nullptr)
			{
				Diagnostics.ReportError(reader.Current(), L"Expected type after ':'");
				return nullptr;
			}

			SyntaxToken name = current;
			SyntaxToken assign;
			std::unique_ptr<ExpressionSyntax> expr = nullptr;
			
			if (reader.Current().Type == TokenType::AssignOperator)
			{
				assign = reader.Current();
				reader.Consume();
				expr = ReadExpression(reader, parent, 0, true);
			}
			else
			{
				Diagnostics.ReportError(reader.Current(), L"Variable declaration is missing an initializer");
			}

			return std::make_unique<VariableStatementSyntax>(std::move(type), name, assign, std::move(expr), parent);
		}

		// name := value
		if (current.Type == TokenType::Identifier && peek.Type == TokenType::DeclareAssignOperator)
		{
			reader.Consume(); // name
			SyntaxToken walrus = reader.Current();
			reader.Consume(); // :=

			auto expr = ReadExpression(reader, parent, 0, true);
			return std::make_unique<VariableStatementSyntax>(
				std::make_unique<PredefinedTypeSyntax>(SyntaxToken(TokenType::VarKeyword, L"var", current.Location, false), parent),
				current, walrus, std::move(expr), parent);
		}

		// Wrong: type name = value (e.g. int x = 0; or var x = 0;)
		if (IsPredefinedType(current.Type) && peek.Type == TokenType::Identifier)
		{
			if (current.Type == TokenType::VarKeyword)
			{
				Diagnostics.ReportError(current,
					L"Invalid use of 'var'. For type inference use 'name := value'; "
					L"for an explicit type use 'name: type = value'.");
			}
			else
			{
				Diagnostics.ReportError(current,
					L"Invalid variable declaration syntax. Use 'name: type = value' or 'name := value'.");
			}

			reader.Consume(); // type keyword
			SyntaxToken name = reader.Current();
			reader.Consume(); // name

			std::unique_ptr<ExpressionSyntax> expr = nullptr;
			if (reader.Current().Type == TokenType::AssignOperator)
			{
				reader.Consume(); // =
				expr = ReadExpression(reader, parent, 0, true);
			}
			else
			{
				// The declaration is malformed in a way we can't recover from
				// (e.g. 'var x: int = ...'); skip to the end of the statement
				// to avoid a flood of follow-up diagnostics.
				while (reader.CanConsume() && reader.Current().Type != TokenType::Semicolon)
					reader.Consume();
			}

			return std::make_unique<VariableStatementSyntax>(
				std::make_unique<PredefinedTypeSyntax>(current, parent),
				name, SyntaxToken(), std::move(expr), parent);
		}

		/*
		// Wrong: name = value (missing type)
		if (current.Type == TokenType::Identifier && peek.Type == TokenType::AssignOperator)
		{
			Diagnostics.ReportError(current, L"Invalid variable declaration syntax. Use 'name: type = value' or 'name := value'.");

			SyntaxToken name = current;
			reader.Consume(); // name
			SyntaxToken assign = reader.Current();
			reader.Consume(); // =
			auto expr = ReadExpression(reader, parent, 0);

			return std::make_unique<VariableStatementSyntax>(
				std::make_unique<PredefinedTypeSyntax>(SyntaxToken(TokenType::VarKeyword, L"var", name.Location, false), parent),
				name, assign, std::move(expr), parent);
		}
		*/
	}

	auto expression = ReadExpression(reader, parent, 0, true);
	return std::make_unique<ExpressionStatementSyntax>(std::move(expression), parent);
}

std::unique_ptr<KeywordStatementSyntax> SourceParser::ReadKeywordStatement(SourceProvider& reader, SyntaxNode* parent)
{
	if (!reader.CanConsume())
		return nullptr;

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::ForKeyword:
		{
			bool isRangeForNoParens = false;
			bool isRangeForWithParens = false;

			if (reader.CanPeek())
			{
				isRangeForNoParens =
					reader.Peek(0).Type == TokenType::Identifier &&
					reader.Peek(1).Type == TokenType::InKeyword;

				isRangeForWithParens =
					reader.Peek(0).Type == TokenType::OpenCurl &&
					reader.Peek(1).Type == TokenType::Identifier &&
					reader.Peek(2).Type == TokenType::InKeyword;
			}

			if (isRangeForNoParens || isRangeForWithParens)
				return ReadForInStatement(reader, parent);

			return ReadForStatement(reader, parent);
		}

		case TokenType::ForeachKeyword:
			return ReadForEachStatement(reader, parent);

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
			return ReadConditionalClause(reader, parent);

		case TokenType::TryKeyword:
			return ReadTryStatement(reader, parent);

		case TokenType::DeferKeyword:
			return ReadDeferStatement(reader, parent);

		case TokenType::ThrowKeyword:
			return ReadThrowStatement(reader, parent);

		default:
		{
			// Don't consume - let caller handle it to avoid infinite loop
			Diagnostics.ReportError(current, L"unknown/unsupported keyword");
			return nullptr;
		}
	}
}

std::unique_ptr<ReturnStatementSyntax> SourceParser::ReadReturnStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ReturnStatementSyntax>(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ReturnKeyword, L"Expected 'return' keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = std::move(ReadExpression(reader, parent, 0, true));

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

std::unique_ptr<ThrowStatementSyntax> SourceParser::ReadThrowStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ThrowStatementSyntax>(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ThrowKeyword, L"Expected 'throw' keyword");

	SyntaxToken current = reader.Current();
	if (current.Type != TokenType::Semicolon)
		syntax->Expression = ReadExpression(reader, parent, 0, true);

	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

std::unique_ptr<BreakStatementSyntax> SourceParser::ReadBreakStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<BreakStatementSyntax>(parent);
	syntax->KeywordToken = Expect(reader, TokenType::BreakKeyword, L"Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

std::unique_ptr<ContinueStatementSyntax> SourceParser::ReadContinueStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ContinueStatementSyntax>(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ContinueKeyword, L"Expected return keyword");
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

std::unique_ptr<DeferStatementSyntax> SourceParser::ReadDeferStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<DeferStatementSyntax>(reader.Current(), parent);
	syntax->DeferToken = Expect(reader, TokenType::DeferKeyword, L"Expected 'defer' keyword");

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::Semicolon)
	{
		Diagnostics.ReportError(current, L"defer statement cannot be empty");
		return syntax;
	}

	syntax->Statement = ReadStatement(reader, syntax.get());
	syntax->SemicolonToken = Expect(reader, TokenType::Semicolon, L"Missing ';' token");
	return syntax;
}

std::unique_ptr<ConditionalClauseBaseSyntax> SourceParser::ReadConditionalClause(SourceProvider& reader, SyntaxNode* parent)
{
	while (reader.CanConsume())
	{
		SyntaxToken current = reader.Current();
		switch (current.Type)
		{
			case TokenType::IfKeyword:
			{
				auto syntax = std::make_unique<IfStatementSyntax>(parent);
				syntax->KeywordToken = current;
				reader.Consume();

				syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"Expected '(' token");
				syntax->ConditionExpression = ReadStatement(reader, syntax.get());
				syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"Expected ')' token");
				syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());

				if (syntax->StatementsBlock != nullptr && syntax->StatementsBlock->SemicolonToken.Type == TokenType::Semicolon)
					Diagnostics.ReportError(syntax->StatementsBlock->SemicolonToken, L"If/unless statement cannot have an empty body; use an empty block {{}} if you meant no-op");

				TokenType nextType = reader.Current().Type;
				if (nextType == TokenType::ElseKeyword)
					syntax->NextStatement = ReadConditionalClause(reader, syntax.get());

				return syntax;
			}

			case TokenType::UnlessKeyword:
			{
				auto syntax = std::make_unique<UnlessStatementSyntax>(parent);
				syntax->KeywordToken = current;
				reader.Consume();

				syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"Expected '(' token");
				syntax->ConditionExpression = ReadStatement(reader, syntax.get());
				syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"Expected ')' token");
				syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());

				if (syntax->StatementsBlock != nullptr && syntax->StatementsBlock->SemicolonToken.Type == TokenType::Semicolon)
					Diagnostics.ReportError(syntax->StatementsBlock->SemicolonToken, L"If/unless statement cannot have an empty body; use an empty block {{}} if you meant no-op");

				TokenType nextType = reader.Current().Type;
				if (nextType == TokenType::ElseKeyword)
					syntax->NextStatement = ReadConditionalClause(reader, syntax.get());

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
						auto syntax = std::make_unique<ElseStatementSyntax>(parent);
						syntax->KeywordToken = elseKeyword;
						syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());
						return syntax;
					}

					default:
					{
						if (!reader.CanConsume())
						{
							Diagnostics.ReportError(elseKeyword, L"Unexpected end of file after 'else'");
							return nullptr;
						}

						Diagnostics.ReportError(current, L"Expected 'if', 'unless', or '{' after 'else'");
						reader.Consume();
						continue;
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

std::unique_ptr<WhileStatementSyntax> SourceParser::ReadWhileStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<WhileStatementSyntax>(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::WhileKeyword, L"Expected 'while' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");
	syntax->ConditionExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());

	return syntax;
}

std::unique_ptr<UntilStatementSyntax> SourceParser::ReadUntilStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<UntilStatementSyntax>(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::UntilKeyword, L"Expected 'until' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");
	syntax->ConditionExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());

	return syntax;
}

std::unique_ptr<ForStatementSyntax> SourceParser::ReadForStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ForStatementSyntax>(parent);
	syntax->SemicolonToken = SyntaxToken(TokenType::Semicolon, L"", TextLocation(), false);
	syntax->KeywordToken = Expect(reader, TokenType::ForKeyword, L"Expected 'for' keyword");
	syntax->OpenCurlToken = Expect(reader, TokenType::OpenCurl, L"expected '(' token");

	// Reading init statement
	syntax->InitializerStatement = ReadStatement(reader, syntax.get());
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax*>(syntax->InitializerStatement.get()))
		Diagnostics.ReportError(keywordStatement->KeywordToken, L"Cannot use keyword statements inside for loop initializer");

	// Reading first semicolon
	syntax->FirstSemicolon = Expect(reader, TokenType::Semicolon, L"expected ';' token");

	// Reading looping consition
	syntax->ConditionExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));

	// Reading second semicolon
	syntax->SecondSemicolon = Expect(reader, TokenType::Semicolon, L"expected ';' token");

	// Reading after loop statement
	syntax->AfterRepeatStatement = ReadStatement(reader, syntax.get());
	if (auto keywordStatement = dynamic_cast<KeywordStatementSyntax*>(syntax->AfterRepeatStatement.get()))
		Diagnostics.ReportError(keywordStatement->KeywordToken, L"Cannot use keyword statements inside for loop repeater");

	// Reading close curl token
	syntax->CloseCurlToken = Expect(reader, TokenType::CloseCurl, L"expected ')' token");

	// Reading block
	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());
	return syntax;
}

std::unique_ptr<ForEachStatementSyntax> SourceParser::ReadForEachStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ForEachStatementSyntax>(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ForeachKeyword, L"Expected 'foreach' keyword");

	bool hasParens = reader.Current().Type == TokenType::OpenCurl;
	if (hasParens)
		Expect(reader, TokenType::OpenCurl, L"expected '(' token");

	SyntaxToken identifier = reader.Current();
	if (identifier.Type != TokenType::Identifier)
	{
		Diagnostics.ReportError(reader.Current(), L"Expected identifier after 'foreach'");
		identifier = SyntaxToken(TokenType::Identifier, L"<missing>", reader.Current().Location, false);
	}
	else
	{
		reader.Consume();
	}

	syntax->IdentifierToken = identifier;
	syntax->InKeywordToken = Expect(reader, TokenType::InKeyword, L"Expected 'in' keyword");
	syntax->RangeExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));

	if (hasParens)
		Expect(reader, TokenType::CloseCurl, L"expected ')' token");

	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());
	return syntax;
}

std::unique_ptr<ForInStatementSyntax> SourceParser::ReadForInStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ForInStatementSyntax>(parent);
	syntax->KeywordToken = Expect(reader, TokenType::ForKeyword, L"Expected 'for' keyword");

	bool hasParens = reader.Current().Type == TokenType::OpenCurl;
	if (hasParens)
		Expect(reader, TokenType::OpenCurl, L"expected '(' token");

	SyntaxToken identifier = reader.Current();
	if (identifier.Type != TokenType::Identifier)
	{
		Diagnostics.ReportError(reader.Current(), L"Expected identifier after 'for'");
		identifier = SyntaxToken(TokenType::Identifier, L"<missing>", reader.Current().Location, false);
	}
	else
	{
		reader.Consume();
	}

	syntax->IdentifierToken = identifier;
	syntax->InKeywordToken = Expect(reader, TokenType::InKeyword, L"Expected 'in' keyword");
	syntax->RangeExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));

	if (hasParens)
		Expect(reader, TokenType::CloseCurl, L"expected ')' token");

	syntax->StatementsBlock = ReadStatementsBlock(reader, syntax.get());
	return syntax;
}

std::unique_ptr<TryStatementSyntax> SourceParser::ReadTryStatement(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<TryStatementSyntax>(parent);
	syntax->TryKeywordToken = Expect(reader, TokenType::TryKeyword, L"Expected 'try' keyword");
	syntax->TryBlock = ReadStatementsBlock(reader, syntax.get());

	while (reader.CanConsume() && reader.Current().Type == TokenType::CatchKeyword)
	{
		auto clause = std::make_unique<CatchClauseSyntax>(syntax.get());
		clause->CatchKeywordToken = Expect(reader, TokenType::CatchKeyword, L"Expected 'catch' keyword");

		bool hasParens = false;
		if (reader.Current().Type == TokenType::OpenCurl)
		{
			hasParens = true;
			clause->OpenParenToken = reader.Current();
			reader.Consume();
		}

		if (TryMatchIdentifier(reader, 3))
		{
			clause->IdentifierToken = reader.Current();
			reader.Consume();
		}

		if (reader.Current().Type == TokenType::Colon)
		{
			clause->ColonToken = reader.Current();
			reader.Consume();
			clause->ExceptionType = ReadType(reader, clause.get());
		}

		if (hasParens)
			clause->CloseParenToken = Expect(reader, TokenType::CloseCurl, L"Expected ')' after catch clause");

		clause->Body = ReadStatementsBlock(reader, clause.get());

		if (clause->IdentifierToken.Type == TokenType::Unknown && clause->ExceptionType == nullptr)
			Diagnostics.ReportError(clause->CatchKeywordToken, L"Catch clause must declare an exception variable or type");

		syntax->CatchClauses.push_back(std::move(clause));
	}

	if (syntax->CatchClauses.empty())
		Diagnostics.ReportError(syntax->TryKeywordToken, L"'try' statement must have at least one 'catch' clause");

	return syntax;
}

std::unique_ptr<ExpressionSyntax> SourceParser::ReadExpression(SourceProvider& reader, SyntaxNode* parent, int bindingPower, bool resetOperatorCount)
{
	if (resetOperatorCount)
		OperatorsInExpression = 0;

	struct DepthGuard
	{
		int& depth;
		DepthGuard(int& d) : depth(d) { ++depth; }
		~DepthGuard() { --depth; }
	};

	DepthGuard guard(ExpressionDepth);
	if (ExpressionDepth > MaxExpressionDepth)
	{
		Diagnostics.ReportError(reader.Current(), L"Expression is too deeply nested");
		SyntaxToken fallbackLocation = reader.Current();
		reader.Consume();
		return std::make_unique<LiteralExpressionSyntax>(SyntaxToken(TokenType::NullLiteral, L"null", fallbackLocation.Location, true), parent);
	}

	auto leftExpr = ReadNullDenotation(reader, parent);
	if (!reader.CanConsume())
		return leftExpr;

	SyntaxToken current = reader.Current();
	int precendence = GetOperatorPrecendence(current.Type);

	while (reader.CanConsume() && precendence != 0 && bindingPower < precendence)
	{
		if (++OperatorsInExpression > MaxExpressionOperators)
		{
			Diagnostics.ReportError(reader.Current(), L"Expression contains too many operators; reduce the number of operators or split it into multiple statements");

			// Skip the remainder of this expression to avoid re-parsing the same
			// long operator chain and producing a flood of follow-up diagnostics.
			while (reader.CanConsume())
			{
				TokenType t = reader.Current().Type;
				if (t == TokenType::Semicolon || t == TokenType::CloseBrace ||
				    t == TokenType::CloseCurl || t == TokenType::CloseSquare ||
				    t == TokenType::Comma)
				{
					break;
				}
				reader.Consume();
			}
			break;
		}

		leftExpr = std::move(ReadLeftDenotation(reader, parent, std::move(leftExpr)));
		if (!reader.CanConsume())
			break;

		current = reader.Current();
		precendence = GetOperatorPrecendence(current.Type);
	}

	return leftExpr;
}

std::unique_ptr<ExpressionSyntax> SourceParser::ReadNullDenotation(SourceProvider& reader, SyntaxNode* parent)
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
			auto syntax = std::make_unique<UnaryExpressionSyntax>(current, false, parent);
			syntax->Expression = std::move(ReadNullDenotation(reader, syntax.get()));
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
			return std::make_unique<LiteralExpressionSyntax>(current, parent);
		}

		case TokenType::OpenCurl:
		{
			reader.Consume();
			auto expression = ReadExpression(reader, parent, 0);
			Expect(reader, TokenType::CloseCurl, L"Expected ')' token");

			if (reader.Current().Type == TokenType::Delimeter)
				return std::move(ReadLinkedExpressionNode(reader, parent, std::move(expression), false));

			return expression;
		}

		case TokenType::OpenSquare:
			return ReadCollectionExpression(reader, parent);

		case TokenType::StringKeyword:
		case TokenType::CharKeyword:
		case TokenType::NativeIntegerKeyword:
		case TokenType::IntegerKeyword:
		case TokenType::BooleanKeyword:
		case TokenType::ByteKeyword:
		case TokenType::DoubleKeyword:
		case TokenType::Identifier:
		case TokenType::FieldKeyword:
			return std::move(ReadLinkedExpressionNode(reader, parent, nullptr, true));

		case TokenType::LambdaKeyword:
			return std::move(ReadLambdaExpression(reader, parent));

		case TokenType::NewKeyword:
			return std::move(ReadObjectExpression(reader, parent));

		case TokenType::ThrowKeyword:
		{
			SyntaxToken throwToken = reader.Current();
			reader.Consume();
			auto syntax = std::make_unique<UnaryExpressionSyntax>(throwToken, false, parent);
			syntax->Expression = std::move(ReadNullDenotation(reader, syntax.get()));
			return syntax;
		}

		case TokenType::IfKeyword:
			return std::move(ReadIfExpression(reader, parent));

		case TokenType::SwitchKeyword:
			return std::move(ReadSwitchExpression(reader, parent));

		case TokenType::EndOfFile:
		{
			Diagnostics.ReportError(current, L"Unexpected file end in expression");

			// Return a literal expression with null value as fallback
			return std::make_unique<LiteralExpressionSyntax>(SyntaxToken(TokenType::NullLiteral, L"null", current.Location, true), parent);
		}

		default:
		{
			// Try to consume and continue - maybe it's recoverable
			Diagnostics.ReportError(current, L"Unknown expression token");
			reader.Consume();

			// Return null literal as fallback
			return std::make_unique<LiteralExpressionSyntax>(SyntaxToken(TokenType::Unknown, L"null", current.Location, true), parent);
		}
	}

	return nullptr;
}

std::unique_ptr<ExpressionSyntax> SourceParser::ReadLeftDenotation(SourceProvider& reader, SyntaxNode* parent, std::unique_ptr<ExpressionSyntax> leftExpr)
{
	if (!reader.CanConsume())
		return std::move(leftExpr);

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		case TokenType::Delimeter:
		{
			return std::move(ReadLinkedExpressionNode(reader, parent, std::move(leftExpr), false));
		}

		case TokenType::NullCoalescingOperator:
		{
			int precendce = GetOperatorPrecendence(current.Type);
			if (precendce == 0)
				return std::move(leftExpr);

			reader.Consume();
			auto syntax = std::make_unique<BinaryExpressionSyntax>(current, parent);

			syntax->Left = std::move(leftExpr);
			syntax->Right = std::move(ReadExpression(reader, syntax.get(), precendce));
			return syntax;
		}

		case TokenType::RangeOperator:
		case TokenType::RangeInclusiveOperator:
		{
			int precendce = GetOperatorPrecendence(current.Type);
			if (precendce == 0)
				return std::move(leftExpr);

			reader.Consume();
			auto syntax = std::make_unique<RangeExpressionSyntax>(parent);
			
			syntax->OperatorToken = current;
			syntax->IsInclusive = current.Type == TokenType::RangeInclusiveOperator;
			syntax->Left = std::move(leftExpr);
			syntax->Right = std::move(ReadExpression(reader, syntax.get(), precendce));
			return syntax;
		}

		case TokenType::AsOperator:
		case TokenType::IsOperator:
		{
			int precendce = GetOperatorPrecendence(current.Type);
			if (precendce == 0)
				return std::move(leftExpr);

			reader.Consume();

			if (current.Type == TokenType::AsOperator)
			{
				auto syntax = std::make_unique<CastExpressionSyntax>(current, parent);
				syntax->Expression = std::move(leftExpr);
				syntax->TargetType = ReadType(reader, syntax.get());
				return syntax;
			}
			else
			{
				auto syntax = std::make_unique<IsExpressionSyntax>(current, parent);
				syntax->Expression = std::move(leftExpr);
				syntax->TargetType = ReadType(reader, syntax.get());
				return syntax;
			}
		}
	}

	if (IsRightUnaryOperator(current.Type))
	{
		int precendce = GetOperatorPrecendence(current.Type);
		if (precendce == 0)
			return leftExpr;

		reader.Consume();
		auto syntax = std::make_unique<UnaryExpressionSyntax>(current, true, parent);
		syntax->Expression = std::move(leftExpr);
		return syntax;
	}

	if (IsBinaryOperator(current.Type) || current.Type == TokenType::AssignOperator)
	{
		int precendce = GetOperatorPrecendence(current.Type);
		if (precendce == 0)
			return leftExpr;

		reader.Consume();
		auto syntax = std::make_unique<BinaryExpressionSyntax>(current, parent);

		syntax->Left = std::move(leftExpr);
		if (syntax->Left != nullptr)
			syntax->Left->Parent = syntax.get();
		syntax->Right = std::move(ReadExpression(reader, syntax.get(), precendce));
		return syntax;
	}

	// Unknown token - return left expression instead of null
	Diagnostics.ReportError(current, L"Unknown token in expression's left denotation");
	reader.Consume();
	return std::move(leftExpr);
}

std::unique_ptr<ObjectExpressionSyntax> SourceParser::ReadObjectExpression(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<ObjectExpressionSyntax>(parent);
	syntax->NewToken = Expect(reader, TokenType::NewKeyword, L"Expected 'new' keyword");
	syntax->Type = ReadType(reader, syntax.get());
	syntax->ArgumentsList = ReadArgumentsList(reader, syntax.get());
	return syntax;
}

std::unique_ptr<TernaryExpressionSyntax> SourceParser::ReadTernaryExpression(SourceProvider& reader, std::unique_ptr<ExpressionSyntax> condition, SyntaxNode* parent)
{
	auto syntax = std::make_unique<TernaryExpressionSyntax>(parent);
	syntax->Condition = std::move(condition);

	syntax->QuestionToken = Expect(reader, TokenType::Question, L"Expected '?' token");
	syntax->Left = std::move(ReadExpression(reader, syntax.get(), 0, true));

	syntax->ColonToken = Expect(reader, TokenType::Colon, L"Expected ':' token");
	syntax->Right = std::move(ReadExpression(reader, syntax.get(), 0, true));

	return syntax;
}

std::unique_ptr<ExpressionSyntax> SourceParser::ReadCollectionExpression(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<CollectionExpressionSyntax>(parent);
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
		auto expr = ReadExpression(reader, syntax.get(), 0, true);
		syntax->ValuesExpressions.push_back(std::move(expr));

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

	// A collection containing exactly one range expression is treated as an array-range literal,
	// e.g. [1..10] produces the same value as 1..10 rather than a single-element array of ranges.
	if (syntax->ValuesExpressions.size() == 1 && syntax->ValuesExpressions[0]->Kind == SyntaxKind::RangeExpression)
	{
		auto rangeExpr = std::move(syntax->ValuesExpressions[0]);
		rangeExpr->Parent = parent;
		return rangeExpr;
	}

	return syntax;
}

std::unique_ptr<LambdaExpressionSyntax> SourceParser::ReadLambdaExpression(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<LambdaExpressionSyntax>(parent);
	syntax->LambdaToken = Expect(reader, TokenType::LambdaKeyword, L"Expected 'lambda' keyword");
	syntax->ParametersList = ReadParametersList(reader, syntax.get());

	if (reader.Current().Type == TokenType::ArrowOperator)
	{
		syntax->ReturnTypeArrowToken = reader.Current();
		reader.Consume();
		syntax->ReturnType = ReadType(reader, syntax.get());
	}
	else
	{
		syntax->LambdaOperatorToken = Expect(reader, TokenType::LambdaOperator, L"Expected '=>' or '->' operator");
	}

	syntax->Body = ReadStatementsBlock(reader, syntax.get());
	return syntax;
}

std::unique_ptr<IfExpressionSyntax> SourceParser::ReadIfExpression(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<IfExpressionSyntax>(parent);
	syntax->IfKeywordToken = Expect(reader, TokenType::IfKeyword, L"Expected 'if' keyword");
	syntax->Condition = std::move(ReadExpression(reader, syntax.get(), 0, true));
	//syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{'");
	syntax->ThenExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));
	//syntax->CloseBraceToken = Expect(reader, TokenType::CloseBrace, L"Expected '}'");

	if (reader.Current().Type == TokenType::ElseKeyword)
	{
		syntax->ElseKeywordToken = reader.Current();
		reader.Consume();
		syntax->ElseExpression = std::move(ReadExpression(reader, syntax.get(), 0, true));
	}

	return syntax;
}

std::unique_ptr<SwitchExpressionSyntax> SourceParser::ReadSwitchExpression(SourceProvider& reader, SyntaxNode* parent)
{
	auto syntax = std::make_unique<SwitchExpressionSyntax>(parent);
	syntax->SwitchKeywordToken = Expect(reader, TokenType::SwitchKeyword, L"Expected 'switch' keyword");
	syntax->Expression = std::move(ReadExpression(reader, syntax.get(), 0, true));
	syntax->OpenBraceToken = Expect(reader, TokenType::OpenBrace, L"Expected '{'");

	while (reader.CanConsume() && reader.Current().Type != TokenType::CloseBrace)
	{
		auto arm = std::make_unique<SwitchArmSyntax>(syntax.get());

		if (reader.Current().Type == TokenType::Identifier && reader.Current().Word == L"_") // '_' as default
		{
			arm->Pattern = std::make_unique<LiteralExpressionSyntax>(SyntaxToken(TokenType::Identifier, L"_", reader.Current().Location, false), arm.get());
			reader.Consume();
		}
		else
		{
			arm->Pattern = std::move(ReadExpression(reader, arm.get(), 0, true));
		}

		arm->ArrowToken = Expect(reader, TokenType::LambdaOperator, L"Expected '=>'");
		arm->Expression = std::move(ReadExpression(reader, arm.get(), 0, true));
		syntax->Arms.push_back(std::move(arm));

		if (reader.Current().Type == TokenType::Comma)
			reader.Consume();
	}

	syntax->CloseBraceToken = Expect(reader, TokenType::CloseBrace, L"Expected '}'");
	return syntax;
}

static bool IsGenericInvocation(SourceProvider& reader)
{
	// The current token is '<'; scan forward to find the matching '>' and
	// check whether it is immediately followed by '('.
	std::size_t index = 0;
	int depth = 1;

	while (true)
	{
		SyntaxToken token = reader.Peek(static_cast<int>(index));
		if (token.Type == TokenType::EndOfFile || token.Type == TokenType::Unknown)
			return false;

		if (token.Type == TokenType::LessOperator)
		{
			depth++;
		}
		else if (token.Type == TokenType::GreaterOperator)
		{
			depth--;
			if (depth == 0)
			{
				SyntaxToken next = reader.Peek(static_cast<int>(index + 1));
				return next.Type == TokenType::OpenCurl;
			}
		}

		index++;
	}
}

std::unique_ptr<LinkedExpressionNode> SourceParser::ReadLinkedExpressionNode(SourceProvider& reader, SyntaxNode* parent, std::unique_ptr<ExpressionSyntax> previous, bool isFirst)
{
	struct LinkedDepthGuard
	{
		int& depth;
		LinkedDepthGuard(int& d) : depth(d) { ++depth; }
		~LinkedDepthGuard() { --depth; }
	};

	LinkedDepthGuard guard(LinkedExpressionDepth);
	if (LinkedExpressionDepth > MaxLinkedExpressionDepth)
	{
		Diagnostics.ReportError(reader.Current(), L"Member access chain is too long");
		if (auto* linked = dynamic_cast<LinkedExpressionNode*>(previous.get()))
		{
			previous.release();
			return std::unique_ptr<LinkedExpressionNode>(linked);
		}

		return nullptr;
	}

	if (!reader.CanConsume())
		return nullptr;

	SyntaxToken delimeter;
	SyntaxToken identifier;

	if (isFirst)
	{
		delimeter = SyntaxToken(TokenType::Delimeter, L"", TextLocation(), false);
		identifier = reader.Current();
		if (!Matches(reader, { TokenType::FieldKeyword, TokenType::Identifier, TokenType::StringKeyword, TokenType::CharKeyword, TokenType::NativeIntegerKeyword, TokenType::IntegerKeyword, TokenType::BooleanKeyword, TokenType::ByteKeyword, TokenType::DoubleKeyword }))
			Diagnostics.ReportError(identifier, L"Expected identifier, primitive name or 'field' keyword");

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
		return std::make_unique<MemberAccessExpressionSyntax>(identifier, std::move(previous), parent);
	}

	std::unique_ptr<LinkedExpressionNode> currentNode = nullptr;
	SyntaxToken current = reader.Current();

	switch (current.Type)
	{
		case TokenType::OpenCurl:
		{
			auto invokation = std::make_unique<InvokationExpressionSyntax>(identifier, std::move(previous), parent);
			invokation->DelimeterToken = delimeter;
			invokation->ArgumentsList = ReadArgumentsList(reader, invokation.get());

			currentNode = std::move(invokation);
			break;
		}

		case TokenType::LessOperator:
		{
			bool genericInvocation = IsGenericInvocation(reader);
			bool genericTypeReceiver = false;
			if (!genericInvocation)
			{
				// Check whether <...> is followed by '.' (generic type as static receiver).
				// The current '<' is already consumed as current token, so start with depth=1.
				std::size_t index = 0;
				int depth = 1;
				while (true)
				{
					SyntaxToken token = reader.Peek(static_cast<int>(index));
					if (token.Type == TokenType::EndOfFile || token.Type == TokenType::Unknown)
						break;

					if (token.Type == TokenType::LessOperator)
					{
						depth++;
					}
					else if (token.Type == TokenType::GreaterOperator)
					{
						depth--;
						if (depth == 0)
						{
							SyntaxToken next = reader.Peek(static_cast<int>(index + 1));
							if (next.Type == TokenType::Delimeter)
								genericTypeReceiver = true;
							break;
						}
					}

					index++;
				}
			}

			if (!genericInvocation && !genericTypeReceiver)
			{
				currentNode = std::make_unique<MemberAccessExpressionSyntax>(identifier, std::move(previous), parent);
				currentNode->DelimeterToken = delimeter;
				return currentNode;
			}

			if (genericTypeReceiver)
			{
				auto idType = std::make_unique<IdentifierNameTypeSyntax>(parent);
				idType->Identifier = identifier;
				auto genericType = std::make_unique<GenericTypeSyntax>(std::move(idType), parent);
				genericType->Arguments = ReadTypeArgumentsList(reader, genericType.get());
				auto typeExpression = std::make_unique<TypeExpressionSyntax>(std::move(genericType), parent);
				return ReadLinkedExpressionNode(reader, parent, std::move(typeExpression), false);
			}

			auto invokation = std::make_unique<InvokationExpressionSyntax>(identifier, std::move(previous), parent);
			invokation->DelimeterToken = delimeter;
			invokation->TypeArguments = ReadTypeArgumentsList(reader, invokation.get());

			if (reader.Current().Type == TokenType::OpenCurl)
				invokation->ArgumentsList = ReadArgumentsList(reader, invokation.get());

			currentNode = std::move(invokation);
			break;
		}

		case TokenType::Delimeter:
		{
			currentNode = std::make_unique<MemberAccessExpressionSyntax>(identifier, std::move(previous), parent);
			currentNode->DelimeterToken = delimeter;

			return ReadLinkedExpressionNode(reader, parent, std::move(currentNode), false);
		}

		case TokenType::OpenSquare:
		{
			auto member = std::make_unique<MemberAccessExpressionSyntax>(identifier, std::move(previous), parent);
			member->DelimeterToken = delimeter;

			currentNode = std::move(ReadIndexatorExpressionNode(reader, parent, std::move(member), false));
			break;
		}

		default:
		{
			if (!reader.CanConsume())
				return std::move(currentNode);

			if (IsOperator(current.Type) || IsPunctuation(current.Type))
			{
				currentNode = std::make_unique<MemberAccessExpressionSyntax>(identifier, std::move(previous), parent);
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
		return std::move(currentNode);

	current = reader.Current();
	if (current.Type == TokenType::OpenSquare)
		currentNode = std::move(ReadIndexatorExpressionNode(reader, parent, std::move(currentNode), false));

	current = reader.Current();
	if (current.Type == TokenType::Delimeter)
		currentNode = std::move(ReadLinkedExpressionNode(reader, parent, std::move(currentNode), false));

	return std::move(currentNode);
}

std::unique_ptr<IndexatorExpressionSyntax> SourceParser::ReadIndexatorExpressionNode(SourceProvider& reader, SyntaxNode* parent, std::unique_ptr<ExpressionSyntax> lastNode, bool isFirst)
{
	static SyntaxToken indexerKeyword(TokenType::IndexerKeyword, L"indexer", TextLocation(), false);
	auto currentNode = std::make_unique<IndexatorExpressionSyntax>(indexerKeyword, std::move(lastNode), parent);
	currentNode->IndexatorList = ReadIndexatorList(reader, currentNode.get());

	if (!reader.CanConsume())
		return currentNode;

	SyntaxToken current = reader.Current();
	if (current.Type == TokenType::OpenSquare)
		currentNode = ReadIndexatorExpressionNode(reader, parent, std::move(currentNode), false);

	return currentNode;
}

std::unique_ptr<ArgumentsListSyntax> SourceParser::ReadArgumentsList(SourceProvider& reader, SyntaxNode* parent)
{
	auto arguments = std::make_unique<ArgumentsListSyntax>(parent);
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
		auto expr = ReadExpression(reader, arguments.get(), 0, true);
		auto argument = std::make_unique<ArgumentSyntax>(std::move(expr), arguments.get());
		arguments->Arguments.push_back(std::move(argument));

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

std::unique_ptr<IndexatorListSyntax> SourceParser::ReadIndexatorList(SourceProvider& reader, SyntaxNode* parent)
{
	auto arguments = std::make_unique<IndexatorListSyntax>(parent);
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
		auto expr = ReadExpression(reader, arguments.get(), 0, true);
		auto argument = std::make_unique<ArgumentSyntax>(std::move(expr), arguments.get());
		arguments->Arguments.push_back(std::move(argument));

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

std::unique_ptr<TypeSyntax> SourceParser::ReadType(SourceProvider& reader, SyntaxNode* parent)
{
	SyntaxToken current = reader.Current();

	if (IsPredefinedType(current.Type))
	{
		reader.Consume();
		auto predefinedType = std::make_unique<PredefinedTypeSyntax>(current, parent);
		return ReadModifiedType(reader, predefinedType.release(), parent);
	}

	if (current.Type == TokenType::DelegateKeyword)
	{
		return ReadDelegateType(reader, parent);
	}

	if (current.Type == TokenType::Identifier)
	{
		return ReadIdentifierNameType(reader, parent);
	}

	Diagnostics.ReportError(current, L"Unexpected token in type syntax");
	return nullptr;
}

std::unique_ptr<TypeSyntax> SourceParser::ReadModifiedType(SourceProvider& reader, TypeSyntax* type, SyntaxNode* parent)
{
	if (!reader.CanConsume())
		return std::unique_ptr<TypeSyntax>(type);

	SyntaxToken current = reader.Current();
	switch (current.Type)
	{
		/*
		case TokenType::Question:
		{
			reader.Consume();
			auto nullable = std::make_unique<NullableTypeSyntax>(std::unique_ptr<TypeSyntax>(type), parent);
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

		case TokenType::Delimeter:
		{
			reader.Consume();
			SyntaxToken identifier = Expect(reader, TokenType::Identifier, L"Expected identifier");
			auto qualified = std::make_unique<QualifiedNameTypeSyntax>(std::unique_ptr<TypeSyntax>(type), parent);
			qualified->Identifier = identifier;
			return ReadModifiedType(reader, qualified.release(), parent);
		}

		default:
			return std::unique_ptr<TypeSyntax>(type);
	}
}

std::unique_ptr<TypeSyntax> SourceParser::ReadIdentifierNameType(SourceProvider& reader, SyntaxNode* parent)
{
	auto identifier = std::make_unique<IdentifierNameTypeSyntax>(parent);
	identifier->Identifier = Expect(reader, TokenType::Identifier, L"Expected identifier");

	return ReadModifiedType(reader, identifier.release(), parent);
}

std::unique_ptr<TypeSyntax> SourceParser::ReadDelegateType(SourceProvider& reader, SyntaxNode* parent)
{
	auto delegate = std::make_unique<DelegateTypeSyntax>(parent);
	delegate->DelegateToken = Expect(reader, TokenType::DelegateKeyword, L"Expected 'delegate' keyword");
	delegate->ReturnType = ReadType(reader, delegate.get());
	delegate->Params = ReadDelegateParametersList(reader, delegate.get());

	return ReadModifiedType(reader, delegate.release(), parent);
}

std::unique_ptr<TypeSyntax> SourceParser::ReadArrayType(SourceProvider& reader, TypeSyntax* previous, SyntaxNode* parent)
{
	auto array = std::make_unique<ArrayTypeSyntax>(std::unique_ptr<TypeSyntax>(previous), parent);
	array->OpenSquareToken = Expect(reader, TokenType::OpenSquare, L"Expected '['");
	array->CloseSquareToken = Expect(reader, TokenType::CloseSquare, L"Expected ']'");
	array->Rank = 1;

	return ReadModifiedType(reader, array.release(), parent);
}

std::unique_ptr<TypeSyntax> SourceParser::ReadGenericType(SourceProvider& reader, TypeSyntax* previous, SyntaxNode* parent)
{
	auto generic = std::make_unique<GenericTypeSyntax>(std::unique_ptr<TypeSyntax>(previous), parent);
	generic->Arguments = ReadTypeArgumentsList(reader, generic.get());
	return ReadModifiedType(reader, generic.release(), parent);
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