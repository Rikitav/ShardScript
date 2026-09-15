#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/parsing/SourceParser.hpp>

#include <iterator>
#include <unordered_set>

using namespace shard;

namespace detail
{
	template<typename T>
	gmt::Span<T> commit_array(gmt::Arena& arena, const std::pmr::vector<T>& list)
	{
		gmt::Span<T> span = arena.allocate_array<T>(list.size());
		std::copy(list.begin(), list.end(), span.data());
		return span;
	}

	template<typename T, std::size_t Capacity>
	struct inline_vector
	{
	private:
		alignas(T) std::byte m_buffer[Capacity * sizeof(T)];

		std::pmr::monotonic_buffer_resource m_pool;
		std::pmr::vector<T> m_list;

	public:
		inline_vector() :
			m_pool(m_buffer, sizeof(m_buffer)),
			m_list(&m_pool)
		{ }

		inline std::pmr::vector<T>& get_vector()
		{
			return m_list;
		}

		inline void push_back(const T& val)
		{
			m_list.push_back(val);
		}

		inline gmt::Span<T> commit_array(gmt::Arena& arena)
		{
			return detail::commit_array(arena, m_list);
		}
	};

	template<typename T, std::size_t Capacity>
	using inline_ref_vector = inline_vector<gmt::Ref<T>, Capacity>;

	static void synchronize_to_next_top_level(SourceProvider& reader)
	{
		int braceDepth = 0;
		int parenDepth = 0;

		while (reader.can_consume())
		{
			SyntaxToken token = reader.current();
			if (braceDepth == 0 && parenDepth == 0 && can_start_compilation_unit(token.get_type()))
				return;

			switch (token.get_type())
			{
				case TokenType::OpenBrace:
				{
					++braceDepth;
					break;
				}

				case TokenType::CloseBrace:
				{
					if (braceDepth > 0)
						--braceDepth;

					break;
				}

				case TokenType::OpenCurl:
				{
					++parenDepth;
					break;
				}

				case TokenType::CloseCurl:
				{
					if (parenDepth > 0)
						--parenDepth;

					break;
				}

				default:
					break;
			}

			reader.consume();
		}
	}

	static bool try_synchronize(SourceProvider& reader, std::initializer_list<TokenType> expectedTokens, int maxSkips)
	{
		// skip tokens until we find a synchronization point or expected token
		int skipped = 0;
		while (reader.can_consume() && skipped < maxSkips)
		{
			SyntaxToken current = reader.current();

			for (TokenType expected : expectedTokens)
			{
				if (current.get_type() == expected)
					return true;
			}

			if (is_synchronization_token(current.get_type()))
				return false;

			reader.consume();
			skipped++;
		}

		return false;
	}

	static bool try_match_identifier(SourceProvider& reader, DiagnosticsContext& diagnostics, int maxSkips)
	{
		if (!reader.can_consume())
			return false;

		SyntaxToken current = reader.current();
		if (current.get_type() == TokenType::Identifier)
			return true;

		if (is_reserved_identifier(current.get_type()))
		{
			diagnostics.report_error(current, L"Identifier cannot be a reserved keyword.");
			return false;
		}

		// try to synchronize to an identifier
		return detail::try_synchronize(reader, { TokenType::Identifier }, maxSkips);
	}
}

SourceParser::SourceParser(SyntaxTree& syntaxTree, DiagnosticsContext& diagnostics) :
	m_syntaxTree(syntaxTree),
	m_diagnostics(diagnostics)
{ }

SyntaxToken SourceParser::expect(SourceProvider& reader, TokenType type, const wchar_t* message)
{
	if (!reader.can_consume())
	{
		if (message != nullptr)
		{
			SyntaxToken eofToken(TokenType::EndOfFile, L"", TextLocation());
			m_diagnostics.report_error(eofToken, message);
		}

		return SyntaxToken(type, L"", TextLocation(), true);
	}

	SyntaxToken current = reader.current();
	if (current.get_type() == type)
	{
		reader.consume();
		return current;
	}

	if (message != nullptr)
		m_diagnostics.report_error(current, message);

	// try to synchronize - skip until we find the expected token or a sync point
	if (detail::try_synchronize(reader, { type }, 5))
	{
		current = reader.current();
		reader.consume();
		return current;
	}

	// return missing token
	return SyntaxToken(type, L"", current.get_location(), true);
}

bool SourceParser::matches(SourceProvider& reader, std::initializer_list<TokenType> types)
{
	if (!reader.can_consume())
		return false;

	SyntaxToken current = reader.current();
	for (const TokenType& type : types)
	{
		if (current.get_type() == type)
			return true;
	}

	return false;
}

bool SourceParser::try_match(SourceProvider& reader, std::initializer_list<TokenType> types, const wchar_t* errorMessage, int maxSkips)
{
	if (!reader.can_consume())
		return false;

	if (matches(reader, types))
		return true;

	if (errorMessage != nullptr)
		m_diagnostics.report_error(reader.current(), errorMessage);

	// try to synchronize to one of expected tokens
	return detail::try_synchronize(reader, types, maxSkips);
}

void SourceParser::FromSourceProvider(SourceProvider& reader)
{
	auto unit = read_compilation_unit(reader);
	m_syntaxTree.add_unit(unit);
}

gmt::Ref<TranslationUnitSyntax> SourceParser::read_compilation_unit(SourceProvider& reader)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto unit = arena.emplace<TranslationUnitSyntax>();

	detail::inline_ref_vector<UsingDirectiveSyntax, 10> usings{};
	detail::inline_ref_vector<MemberDeclarationSyntax, 10> members{};

	int loopGuard = 0;
	while (reader.can_consume())
	{
		if (++loopGuard > max_loop_iterations)
		{
			m_diagnostics.report_error(reader.current(), L"Parser loop detected - aborting compilation unit");
			break;
		}

		SyntaxToken token = reader.current();
		switch (token.get_type())
		{
			case TokenType::UsingKeyword:
			{
				if (unit->has_namespace() || unit->get_members().size() != 0)
					m_diagnostics.report_error(token, L"Using directive must be declared at the top of the compilation unit");

				usings.push_back(read_using_directive(reader, unit));
				break;
			}

			case TokenType::NamespaceKeyword:
			{
				if (unit->get_members().size() != 0)
					m_diagnostics.report_error(token, L"Namespace directive must be declared before any member declarations");

				if (unit->has_namespace())
				{
					m_diagnostics.report_error(token, L"Only one namespace declaration is allowed per compilation unit");
					reader.consume();

					while (reader.can_consume() && reader.current().get_type() != TokenType::Semicolon)
						reader.consume();

					if (reader.can_consume())
						reader.consume();

					break;
				}

				auto directive = read_namespace_directive(reader, unit);
				unit->set_namespace(directive);
				break;
			}

			default:
			{
				if (can_start_member_declaration(token.get_type()))
				{
					gmt::Ref<MemberDeclarationSyntax> member = read_member_declaration(reader, unit);
					if (!member.is_null())
						members.push_back(member);

					break;
				}

				m_diagnostics.report_error(token, L"Unknown token in compilation unit declaration");
				reader.consume();
				break;
			}
		}
	}

	unit->set_usings(usings.commit_array(arena));
	unit->set_members(members.commit_array(arena));
	return unit;
}

gmt::Ref<UsingDirectiveSyntax> SourceParser::read_using_directive(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<UsingDirectiveSyntax>(parent);
	syntax->set_using_keyword(expect(reader, TokenType::UsingKeyword, L"Expected 'using' keyword"));

	detail::inline_vector<SyntaxToken, 10> qualifier{};
	while (reader.can_consume())
	{
		if (!detail::try_match_identifier(reader, m_diagnostics, 3))
		{
			if (try_match(reader, { TokenType::Semicolon }, nullptr, 10))
			{
				syntax->set_semicolon(reader.current());
				reader.consume();
			}

			break;
		}

		qualifier.push_back(reader.current());
		reader.consume();

		if (!try_match(reader, { TokenType::Delimeter, TokenType::Semicolon }, L"Expected separator token '.' or closing token ';'", 3))
			break;

		SyntaxToken separatorToken = reader.current();
		reader.consume();

		if (separatorToken.get_type() == TokenType::Semicolon)
		{
			syntax->set_semicolon(separatorToken);
			break;
		}
	}

	syntax->set_qualifier(qualifier.commit_array(arena));
	return syntax;
}

gmt::Ref<NamespaceDirectiveSyntax> SourceParser::read_namespace_directive(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<NamespaceDirectiveSyntax>(parent);
	syntax->set_namespace_keyword(expect(reader, TokenType::NamespaceKeyword, L"Expected 'namespace' keyword"));

	detail::inline_vector<SyntaxToken, 10> qualifier{};

	if (!detail::try_match_identifier(reader, m_diagnostics, 5))
	{
		// create missing identifier if we couldn't recover
		qualifier.push_back(SyntaxToken(TokenType::Identifier, L"", TextLocation(), true));
	}
	else
	{
		qualifier.push_back(reader.current());
		reader.consume();
	}

	while (reader.can_consume() && reader.current().get_type() == TokenType::Delimeter)
	{
		reader.consume(); // consume '.'
		if (!detail::try_match_identifier(reader, m_diagnostics, 5))
			break; // error recovery: expected identifier after '.'

		qualifier.push_back(reader.current());
		reader.consume();
	}

	syntax->set_semicolon(expect(reader, TokenType::Semicolon, L"Expected ';' after namespace declaration"));
	syntax->set_qualifier(qualifier.commit_array(arena));
	return syntax;
}

gmt::Ref<MemberDeclarationSyntax> SourceParser::read_member_declaration(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();

	gmt::Ref<AttributesListSyntax> attributes = read_attributes_list(reader, parent);

	detail::inline_vector<SyntaxToken, 10> modifiers{};
	read_member_modifiers(reader, modifiers.get_vector());

	if (!reader.can_consume())
	{
		SyntaxToken eofToken(TokenType::EndOfFile, L"", TextLocation());
		m_diagnostics.report_error(eofToken, L"Expected member declaration");
		return gmt::Ref<MemberDeclarationSyntax>();
	}

	SyntaxToken current = reader.current();
	gmt::Ref<MemberDeclarationSyntax> member;

	switch (current.get_type())
	{
		case TokenType::ClassKeyword:
		{
			member = read_class_declaration(reader, parent);
			break;
		}

		case TokenType::FunctionKeyword:
		{
			member = read_function_declaration(reader, parent);
			break;
		}

		default:
		{
			m_diagnostics.report_error(current, L"Expected member declaration");

			// error recovery: skip the remaining tokens of this declaration
			detail::synchronize_to_next_top_level(reader);
			return gmt::Ref<MemberDeclarationSyntax>();
		}
	}

	member->set_attributes(attributes);
	member->set_modifiers(modifiers.commit_array(arena));
	return member;
}

gmt::Ref<ClassDeclarationSyntax> SourceParser::read_class_declaration(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ClassDeclarationSyntax>(parent);
	syntax->set_declare_token(expect(reader, TokenType::ClassKeyword, L"Expected 'class' keyword"));

	if (detail::try_match_identifier(reader, m_diagnostics, 5))
	{
		syntax->set_identifier(reader.current());
		reader.consume();
	}
	else
	{
		syntax->set_identifier(SyntaxToken(TokenType::Identifier, L"", TextLocation(), true));
	}

	if (reader.can_consume() && reader.current().get_type() == TokenType::LessOperator)
		syntax->set_type_parameters(read_generic_type_parameters(reader, syntax));

	if (reader.can_consume() && reader.current().get_type() == TokenType::WhereKeyword)
		syntax->set_where_clauses(read_where_clauses(reader, syntax));

	if (reader.can_consume() && reader.current().get_type() == TokenType::Colon)
	{
		syntax->set_base_type_colon(reader.current());
		reader.consume();

		if (reader.can_consume() && is_predefined_type(reader.current().get_type()))
			syntax->set_base_types(read_base_types(reader, syntax));
	}

	if (try_match(reader, { TokenType::OpenBrace, TokenType::Semicolon }, L"Expected class body '{' or semicolon ';'", 5))
	{
		if (reader.current().get_type() == TokenType::OpenBrace)
		{
			syntax->set_open_bracket(reader.current());
			reader.consume();

			detail::inline_ref_vector<MemberDeclarationSyntax, 10> members{};

			int loopGuard = 0;
			while (reader.can_consume())
			{
				if (++loopGuard > max_loop_iterations)
				{
					m_diagnostics.report_error(reader.current(), L"Parser loop detected - aborting class body");
					break;
				}

				if (reader.current().get_type() == TokenType::CloseBrace)
					break;

				if (!can_start_member_declaration(reader.current().get_type()))
				{
					m_diagnostics.report_error(reader.current(), L"Unexpected token in class body");
					reader.consume();
					continue;
				}

				gmt::Ref<MemberDeclarationSyntax> member = read_member_declaration(reader, syntax);
				if (!member.is_null())
					members.push_back(member);
			}

			syntax->set_close_bracket(expect(reader, TokenType::CloseBrace, L"Expected '}'"));
			syntax->set_members(members.commit_array(arena));
		}
		else
		{
			syntax->set_semicolon(reader.current());
			reader.consume();
		}
	}

	return syntax;
}

gmt::Ref<FunctionDeclarationSyntax> SourceParser::read_function_declaration(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<FunctionDeclarationSyntax>(parent);
	syntax->set_declare_token(expect(reader, TokenType::FunctionKeyword, L"Expected 'func' keyword"));

	if (detail::try_match_identifier(reader, m_diagnostics, 5))
	{
		syntax->set_identifier(reader.current());
		reader.consume();
	}
	else
	{
		syntax->set_identifier(SyntaxToken(TokenType::Identifier, L"", TextLocation(), true));
	}

	if (reader.can_consume() && reader.current().get_type() == TokenType::LessOperator)
		syntax->set_type_parameters(read_generic_type_parameters(reader, syntax));

	syntax->set_parameters_list(read_method_parameters(reader, syntax));

	if (reader.can_consume() && reader.current().get_type() == TokenType::ArrowOperator)
	{
		reader.consume(); // consume '->'
		syntax->set_return_type(read_type(reader, syntax));

		if (syntax->get_return_type().is_null())
			m_diagnostics.report_error(reader.current(), L"Expected type after '->'");
	}

	if (reader.can_consume() && reader.current().get_type() == TokenType::WhereKeyword)
		syntax->set_where_clauses(read_where_clauses(reader, syntax));

	if (reader.can_consume() && reader.current().get_type() == TokenType::Semicolon)
	{
		syntax->set_semicolon(reader.current());
		reader.consume();
		return syntax;
	}

	syntax->set_body(read_body(reader, syntax));
	if (!syntax->get_body().is_null() && syntax->get_body().get()->get_kind() == SyntaxKind::ArrowClause)
		syntax->set_semicolon(expect(reader, TokenType::Semicolon, L"Missing ';' token"));

	return syntax;
}

gmt::Ref<TypeSyntax> SourceParser::read_type(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	if (!reader.can_consume())
		return gmt::Ref<TypeSyntax>();

	SyntaxToken current = reader.current();
	if (is_predefined_type(current.get_type()))
	{
		auto syntax = m_syntaxTree.get_arena().emplace<PredefinedTypeSyntax>(parent);
		syntax->set_type_token(current);
		reader.consume();
		return syntax;
	}

	m_diagnostics.report_error(current, L"Unexpected token in type syntax");
	return gmt::Ref<TypeSyntax>();
}

gmt::Ref<ParameterSyntax> SourceParser::read_parameter(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ParameterSyntax>(parent);

	if (detail::try_match_identifier(reader, m_diagnostics, 3))
	{
		syntax->set_identifier(reader.current());
		reader.consume();
	}
	else
	{
		syntax->set_identifier(SyntaxToken(TokenType::Identifier, L"", TextLocation(), true));
	}

	if (!try_match(reader, { TokenType::Colon }, L"Expected ':' after parameter name", 3))
		return syntax;

	reader.consume(); // consume ':'
	syntax->set_type(read_type(reader, syntax));

	if (syntax->get_type().is_null())
		m_diagnostics.report_error(reader.current(), L"Expected type after ':'");

	return syntax;
}

gmt::Ref<ParametersListSyntax> SourceParser::read_method_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ParametersListSyntax>(parent);
	syntax->set_open_token(expect(reader, TokenType::OpenCurl, L"Expected '(' token"));

	detail::inline_ref_vector<ParameterSyntax, 4> parameters{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::CloseCurl)
	{
		syntax->set_close_token(reader.current());
		reader.consume();
	}
	else
	{
		while (reader.can_consume())
		{
			gmt::Ref<ParameterSyntax> parameter = read_parameter(reader, syntax);
			if (!parameter.is_null())
				parameters.push_back(parameter);

			if (!try_match(reader, { TokenType::Comma, TokenType::CloseCurl }, L"Expected ',' or ')'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::CloseCurl)
			{
				syntax->set_close_token(separatorToken);
				break;
			}
		}
	}

	syntax->set_parameters(parameters.commit_array(arena));
	return syntax;
}

gmt::Ref<ParametersListSyntax> SourceParser::read_indexer_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ParametersListSyntax>(parent);
	syntax->set_open_token(expect(reader, TokenType::OpenSquare, L"Expected '[' token"));

	detail::inline_ref_vector<ParameterSyntax, 4> parameters{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::CloseSquare)
	{
		syntax->set_close_token(reader.current());
		reader.consume();
	}
	else
	{
		while (reader.can_consume())
		{
			gmt::Ref<ParameterSyntax> parameter = read_parameter(reader, syntax);
			if (!parameter.is_null())
				parameters.push_back(parameter);

			if (!try_match(reader, { TokenType::Comma, TokenType::CloseSquare }, L"Expected ',' or ']'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::CloseSquare)
			{
				syntax->set_close_token(separatorToken);
				break;
			}
		}
	}

	syntax->set_parameters(parameters.commit_array(arena));
	return syntax;
}

gmt::Ref<ParametersListSyntax> SourceParser::read_lambda_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ParametersListSyntax>(parent);
	syntax->set_open_token(expect(reader, TokenType::OrOperator, L"Expected '|' token"));

	detail::inline_ref_vector<ParameterSyntax, 4> parameters{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::OrOperator)
	{
		syntax->set_close_token(reader.current());
		reader.consume();
	}
	else
	{
		while (reader.can_consume())
		{
			gmt::Ref<ParameterSyntax> parameter = read_parameter(reader, syntax);
			if (!parameter.is_null())
				parameters.push_back(parameter);

			if (!try_match(reader, { TokenType::Comma, TokenType::OrOperator }, L"Expected ',' or '|'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::OrOperator)
			{
				syntax->set_close_token(separatorToken);
				break;
			}
		}
	}

	syntax->set_parameters(parameters.commit_array(arena));
	return syntax;
}

gmt::Ref<TypeParameterSyntax> SourceParser::read_type_parameter(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<TypeParameterSyntax>(parent);

	if (detail::try_match_identifier(reader, m_diagnostics, 3))
	{
		syntax->set_identifier(reader.current());
		reader.consume();
	}
	else
	{
		syntax->set_identifier(SyntaxToken(TokenType::Identifier, L"", TextLocation(), true));
	}

	return syntax;
}

gmt::Ref<TypeParametersListSyntax> SourceParser::read_generic_type_parameters(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<TypeParametersListSyntax>(parent);
	syntax->set_open_token(expect(reader, TokenType::LessOperator, L"Expected '<' token"));

	detail::inline_ref_vector<TypeParameterSyntax, 4> typeParameters{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::GreaterOperator)
	{
		syntax->set_close_token(reader.current());
		reader.consume();
	}
	else
	{
		while (reader.can_consume())
		{
			gmt::Ref<TypeParameterSyntax> typeParameter = read_type_parameter(reader, syntax);
			if (!typeParameter.is_null())
				typeParameters.push_back(typeParameter);

			if (!try_match(reader, { TokenType::Comma, TokenType::GreaterOperator }, L"Expected ',' or '>'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::GreaterOperator)
			{
				syntax->set_close_token(separatorToken);
				break;
			}
		}
	}

	syntax->set_parameters(typeParameters.commit_array(arena));
	return syntax;
}

gmt::Ref<ArgumentSyntax> SourceParser::read_argument(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ArgumentSyntax>(parent);

	syntax->set_expression(read_expression(reader, syntax));
	if (syntax->get_expression().is_null())
		m_diagnostics.report_error(reader.current(), L"Expected argument expression");

	return syntax;
}

gmt::Ref<ArgumentsListSyntax> SourceParser::read_arguments_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ArgumentsListSyntax>(parent);
	syntax->set_open_token(expect(reader, TokenType::OpenCurl, L"Expected '(' token"));

	detail::inline_ref_vector<ArgumentSyntax, 4> arguments{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::CloseCurl)
	{
		syntax->set_close_token(reader.current());
		reader.consume();
	}
	else
	{
		while (reader.can_consume())
		{
			gmt::Ref<ArgumentSyntax> argument = read_argument(reader, syntax);
			if (!argument.is_null())
				arguments.push_back(argument);

			if (!try_match(reader, { TokenType::Comma, TokenType::CloseCurl }, L"Expected ',' or ')'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::CloseCurl)
			{
				syntax->set_close_token(separatorToken);
				break;
			}
		}
	}

	syntax->set_arguments(arguments.commit_array(arena));
	return syntax;
}

gmt::Ref<TypeArgumentsListSyntax> SourceParser::read_type_arguments_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<TypeArgumentsListSyntax>(parent);
	syntax->set_open_token(expect(reader, TokenType::LessOperator, L"Expected '<' token"));

	detail::inline_ref_vector<TypeSyntax, 4> types{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::GreaterOperator)
	{
		syntax->set_close_token(reader.current());
		reader.consume();
	}
	else
	{
		while (reader.can_consume())
		{
			gmt::Ref<TypeSyntax> type = read_type(reader, syntax);
			if (!type.is_null())
				types.push_back(type);

			if (!try_match(reader, { TokenType::Comma, TokenType::GreaterOperator }, L"Expected ',' or '>'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::GreaterOperator)
			{
				syntax->set_close_token(separatorToken);
				break;
			}
		}
	}

	syntax->set_types(types.commit_array(arena));
	return syntax;
}

gmt::Ref<BaseTypesListSyntax> SourceParser::read_base_types(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<BaseTypesListSyntax>(parent);

	detail::inline_ref_vector<TypeSyntax, 4> baseTypes{};
	while (reader.can_consume() && is_predefined_type(reader.current().get_type()))
	{
		gmt::Ref<TypeSyntax> baseType = read_type(reader, syntax);
		if (baseType.is_null())
			break;

		baseTypes.push_back(baseType);

		if (reader.can_consume() && reader.current().get_type() == TokenType::Comma)
			reader.consume();
		else
			break;
	}

	syntax->set_types(baseTypes.commit_array(arena));
	return syntax;
}

gmt::Ref<WhereClauseSyntax> SourceParser::read_where_clause(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<WhereClauseSyntax>(parent);
	syntax->set_where_keyword(expect(reader, TokenType::WhereKeyword, L"Expected 'where' keyword"));

	if (detail::try_match_identifier(reader, m_diagnostics, 3))
	{
		syntax->set_identifier(reader.current());
		reader.consume();
	}
	else
	{
		syntax->set_identifier(SyntaxToken(TokenType::Identifier, L"", TextLocation(), true));
	}

	syntax->set_colon(expect(reader, TokenType::Colon, L"Expected ':' after type parameter"));

	detail::inline_ref_vector<TypeSyntax, 4> constraintTypes{};
	while (reader.can_consume() && is_predefined_type(reader.current().get_type()))
	{
		gmt::Ref<TypeSyntax> constraintType = read_type(reader, syntax);
		if (constraintType.is_null())
			break;

		constraintTypes.push_back(constraintType);

		if (reader.can_consume() && reader.current().get_type() == TokenType::Comma)
			reader.consume();
		else
			break;
	}

	syntax->set_constraint_types(constraintTypes.commit_array(arena));
	return syntax;
}

gmt::Ref<WhereClausesListSyntax> SourceParser::read_where_clauses(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<WhereClausesListSyntax>(parent);

	detail::inline_ref_vector<WhereClauseSyntax, 2> clauses{};
	while (reader.can_consume() && reader.current().get_type() == TokenType::WhereKeyword)
		clauses.push_back(read_where_clause(reader, syntax));

	syntax->set_clauses(clauses.commit_array(arena));
	return syntax;
}

gmt::Ref<BodySyntax> SourceParser::read_body(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	if (!reader.can_consume())
	{
		SyntaxToken eofToken(TokenType::EndOfFile, L"", TextLocation());
		m_diagnostics.report_error(eofToken, L"Expected method body");
		return gmt::Ref<BodySyntax>();
	}

	TokenType type = reader.current().get_type();
	if (type == TokenType::OpenBrace)
		return read_statements_block(reader, parent);

	if (type == TokenType::LambdaOperator)
		return read_arrow_clause(reader, parent);

	m_diagnostics.report_error(reader.current(), L"Expected method body");
	return gmt::Ref<BodySyntax>();
}

gmt::Ref<StatementsBlockSyntax> SourceParser::read_statements_block(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<StatementsBlockSyntax>(parent);

	++m_blockDepth;
	if (m_blockDepth > max_block_depth)
	{
		m_diagnostics.report_error(reader.current(), L"Statement nesting is too deep");

		// skip the rest of this block to avoid stack overflow
		syntax->set_open_bracket(expect(reader, TokenType::OpenBrace, L"Expected '{'"));
		while (reader.can_consume() && reader.current().get_type() != TokenType::CloseBrace)
			reader.consume();

		if (reader.can_consume())
			reader.consume();

		--m_blockDepth;
		return syntax;
	}

	syntax->set_open_bracket(expect(reader, TokenType::OpenBrace, L"Expected '{'"));

	detail::inline_ref_vector<StatementSyntax, 10> statements{};

	int loopGuard = 0;
	while (reader.can_consume())
	{
		if (++loopGuard > max_loop_iterations)
		{
			m_diagnostics.report_error(reader.current(), L"Parser loop detected - aborting statement block");
			break;
		}

		if (reader.current().get_type() == TokenType::CloseBrace)
			break;

		gmt::Ref<StatementSyntax> statement = read_statement(reader, syntax);
		if (!statement.is_null())
			statements.push_back(statement);
		else
			reader.consume(); // error recovery: guarantee progress
	}

	syntax->set_close_bracket(expect(reader, TokenType::CloseBrace, L"Expected '}'"));
	syntax->set_statements(statements.commit_array(arena));
	--m_blockDepth;
	return syntax;
}

gmt::Ref<ArrowClauseSyntax> SourceParser::read_arrow_clause(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ArrowClauseSyntax>(parent);

	syntax->set_arrow_token(reader.current());
	reader.consume(); // consume '=>'

	syntax->set_expression(read_expression(reader, syntax));
	return syntax;
}

gmt::Ref<StatementSyntax> SourceParser::read_statement(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	if (!reader.can_consume())
		return gmt::Ref<StatementSyntax>();

	// keyword statement kinds (loops, branchings, ...) are dispatched here

	if (reader.current().get_type() == TokenType::Semicolon)
	{
		// empty statement
		gmt::Arena& arena = m_syntaxTree.get_arena();
		auto syntax = arena.emplace<ExpressionStatementSyntax>(parent);
		syntax->set_semicolon(reader.current());
		reader.consume();
		return syntax;
	}

	// fall through to an expression statement when no other statement kind matched
	return read_expression_statement(reader, parent);
}

gmt::Ref<ExpressionStatementSyntax> SourceParser::read_expression_statement(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ExpressionStatementSyntax>(parent);

	syntax->set_expression(read_expression(reader, syntax));
	if (syntax->get_expression().is_null())
	{
		// error already reported by read_expression; skip the offending token to guarantee progress
		if (reader.can_consume())
			reader.consume();

		return syntax;
	}

	syntax->set_semicolon(expect(reader, TokenType::Semicolon, L"Expected ';'"));
	return syntax;
}

gmt::Ref<ExpressionSyntax> SourceParser::read_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	if (!reader.can_consume())
		return gmt::Ref<ExpressionSyntax>();

	switch (reader.current().get_type())
	{
		case TokenType::NullLiteral:
		case TokenType::CharLiteral:
		case TokenType::StringLiteral:
		case TokenType::BooleanLiteral:
		case TokenType::NumberLiteral:
		case TokenType::DoubleLiteral:
		case TokenType::ByteLiteral:
			return read_literal_expression(reader, parent);

		default:
			m_diagnostics.report_error(reader.current(), L"Expected expression");
			return gmt::Ref<ExpressionSyntax>();
	}
}

gmt::Ref<LiteralExpressionSyntax> SourceParser::read_literal_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<LiteralExpressionSyntax>(parent);

	syntax->set_literal(reader.current());
	reader.consume();

	return syntax;
}

gmt::Ref<AttributeSyntax> SourceParser::read_attribute(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<AttributeSyntax>(parent);
	syntax->set_open_bracket(expect(reader, TokenType::OpenSquare, L"Expected '['"));
	syntax->set_name(expect(reader, TokenType::Identifier, L"Expected attribute name"));

	if (reader.can_consume() && reader.current().get_type() == TokenType::OpenCurl)
	{
		syntax->set_open_curl(reader.current());
		reader.consume(); // consume '('

		alignas(SyntaxToken) std::byte inlineArguments[2 * sizeof(SyntaxToken)];
		std::pmr::monotonic_buffer_resource argumentsPool(inlineArguments, sizeof(inlineArguments));
		std::pmr::vector<SyntaxToken> arguments(&argumentsPool);

		while (reader.can_consume() && reader.current().get_type() != TokenType::CloseCurl)
		{
			SyntaxToken argument = reader.current();
			if (argument.get_type() == TokenType::StringLiteral)
			{
				arguments.push_back(argument);
				reader.consume();
			}
			else
			{
				m_diagnostics.report_error(argument, L"Expected string literal in attribute arguments");
				reader.consume();
			}

			if (reader.current().get_type() == TokenType::Comma)
			{
				reader.consume(); // consume ','
			}
			else if (reader.current().get_type() != TokenType::CloseCurl)
			{
				m_diagnostics.report_error(reader.current(), L"Expected ',' or ')' in attribute arguments");
				break;
			}
		}

		syntax->set_close_curl(expect(reader, TokenType::CloseCurl, L"Expected ')'"));
		syntax->set_arguments(detail::commit_array(arena, arguments));
	}

	syntax->set_close_bracket(expect(reader, TokenType::CloseSquare, L"Expected ']'"));
	return syntax;
}

gmt::Ref<AttributesListSyntax> SourceParser::read_attributes_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	if (!reader.can_consume() || reader.current().get_type() != TokenType::OpenSquare)
		return gmt::Ref<AttributesListSyntax>();

	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<AttributesListSyntax>(parent);

	detail::inline_ref_vector<AttributeSyntax, 5> attributes{};
	while (reader.can_consume() && reader.current().get_type() == TokenType::OpenSquare)
		attributes.push_back(read_attribute(reader, syntax));

	syntax->set_attributes(attributes.commit_array(arena));
	return syntax;
}

static std::pair<int, uint32_t> get_modifier_meta(TokenType type) noexcept
{
	switch (type)
	{
	case TokenType::PublicKeyword:    return { 1, 1u << 0 };
	case TokenType::PrivateKeyword:   return { 1, 1u << 1 };
	case TokenType::ProtectedKeyword: return { 1, 1u << 2 };
	case TokenType::InternalKeyword:  return { 1, 1u << 3 };
	case TokenType::StaticKeyword:    return { 2, 1u << 4 };
	case TokenType::AsyncKeyword:     return { 3, 1u << 5 };
	default:                          return { 0, 0 };
	}
};

void SourceParser::read_member_modifiers(SourceProvider& reader, std::pmr::vector<SyntaxToken>& modifiers)
{
	// expected order: access -> static -> async/extern
	uint32_t seenMask = 0;
	int currentGroupOrder = 0;

	while (reader.can_consume())
	{
		SyntaxToken current = reader.current();
		const auto [groupOrder, maskBit] = get_modifier_meta(current.get_type());

		if (groupOrder == 0)
			break;

		reader.consume();
		if (seenMask & maskBit)
		{
			m_diagnostics.report_error(current, L"Duplicate modifier");
			continue;
		}

		seenMask |= maskBit;
		if (groupOrder < currentGroupOrder)
		{
			m_diagnostics.report_error(current, L"Modifier out of order");
			continue;
		}

		currentGroupOrder = groupOrder;
		modifiers.push_back(current);
	}
}
