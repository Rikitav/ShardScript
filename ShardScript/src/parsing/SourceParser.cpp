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
		if (is_identifier_like(current.get_type()))
			return true;

		if (is_reserved_identifier(current.get_type()))
		{
			diagnostics.report_error(current, L"Identifier cannot be a reserved keyword.");
			return false;
		}

		// try to synchronize to an identifier
		return detail::try_synchronize(reader, { TokenType::Identifier }, maxSkips);
	}

	static bool can_follow_type(TokenType type) noexcept
	{
		switch (type)
		{
			case TokenType::Delimeter:			// member access
			case TokenType::OpenCurl:			// invocation
			case TokenType::CloseCurl:
			case TokenType::CloseSquare:
			case TokenType::CloseBrace:
			case TokenType::Colon:
			case TokenType::Semicolon:
			case TokenType::Comma:
			case TokenType::Question:
			case TokenType::EqualsOperator:
			case TokenType::NotEqualsOperator:
			case TokenType::OrOperator:
			case TokenType::AndOperator:
			case TokenType::PipeOperator:
			case TokenType::AmpersandOperator:
			case TokenType::IsOperator:
			case TokenType::AsOperator:
				return true;

			default:
				return false;
		}
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
		return gmt::nullref;
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
			return gmt::nullref;
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
	if (!syntax->get_body().is_null() && syntax->get_body().as_ptr()->get_kind() == SyntaxKind::ArrowClause)
		syntax->set_semicolon(expect(reader, TokenType::Semicolon, L"Missing ';' token"));

	return syntax;
}

gmt::Ref<TypeSyntax> SourceParser::read_type(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	if (!reader.can_consume())
		return gmt::nullref;

	gmt::Ref<TypeSyntax> base;
	SyntaxToken current = reader.current();

	if (is_predefined_type(current.get_type()))
	{
		gmt::Arena& arena = m_syntaxTree.get_arena();
		auto predefined = arena.emplace<PredefinedTypeSyntax>(parent);
		predefined->set_type_token(current);

		reader.consume();
		base = predefined;
	}
	else if (current.get_type() == TokenType::Identifier)
	{
		base = read_identifier_name_type(reader, parent);

		// type context: '<' after a name is always a generic argument list
		if (reader.can_consume() && reader.current().get_type() == TokenType::LessOperator)
			base = read_generic_type(reader, parent, base);
	}
	else
	{
		m_diagnostics.report_error(current, L"Unexpected token in type syntax");
		return gmt::nullref;
	}

	// array and nullable suffixes
	while (reader.can_consume())
	{
		gmt::Arena& arena = m_syntaxTree.get_arena();

		if (reader.current().get_type() == TokenType::OpenSquare && reader.peek().get_type() == TokenType::CloseSquare)
		{
			auto arrayType = arena.emplace<ArrayTypeSyntax>(parent);
			arrayType->set_underlaying_type(base);
			arrayType->set_open_bracket(reader.current());

			reader.consume();
			arrayType->set_close_bracket(reader.current());

			reader.consume();
			base = arrayType;
		}
		else if (reader.current().get_type() == TokenType::Question)
		{
			auto nullableType = arena.emplace<NullableTypeSyntax>(parent);
			nullableType->set_underlaying_type(base);
			nullableType->set_question_token(reader.current());

			reader.consume();
			base = nullableType;
		}
		else
		{
			break;
		}
	}

	return base;
}

gmt::Ref<TypeSyntax> SourceParser::read_identifier_name_type(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();

	// current is expected to be the identifier token
	auto name = arena.emplace<IdentifierNameTypeSyntax>(parent);
	name->set_identifier(reader.current());
	reader.consume();

	gmt::Ref<TypeSyntax> identifier = name;

	// namespace-qualified name: Foo::Bar::Baz
	while (reader.can_consume() && reader.current().get_type() == TokenType::NamespaceQualifier)
	{
		auto qualified = arena.emplace<QualifiedNameTypeSyntax>(parent);
		qualified->set_qualifier_token(reader.current());
		qualified->set_left(identifier);
		reader.consume();

		if (detail::try_match_identifier(reader, m_diagnostics, 3))
		{
			qualified->set_identifier(reader.current());
			reader.consume();
		}
		else
		{
			qualified->set_identifier(SyntaxToken(TokenType::Identifier, L"", TextLocation(), true));
		}

		identifier = qualified;
	}

	return identifier;
}

gmt::Ref<GenericTypeSyntax> SourceParser::read_generic_type(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, gmt::Ref<TypeSyntax> underlayingType)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto generic = arena.emplace<GenericTypeSyntax>(parent);
	
	generic->set_underlaying_type(underlayingType);
	generic->set_type_arguments(read_type_arguments_list(reader, generic));
	return generic;
}

bool SourceParser::scan_generic_type_arguments(SourceProvider& reader)
{
	// current token is the opening '<'; peek(offset - 1) is the token right after it
	int depth = 1;
	int offset = 1;

	while (offset < max_loop_iterations)
	{
		SyntaxToken token = reader.peek(offset - 1);
		TokenType type = token.get_type();

		if (type == TokenType::LessOperator)
		{
			++depth;
			++offset;
			continue;
		}
		
		if (type == TokenType::GreaterOperator)
		{
			--depth;
			if (depth == 0)
				return detail::can_follow_type(reader.peek(offset).get_type());

			++offset;
			continue;
		}
		
		if (is_valid_generic_type_token(type) || is_predefined_type(type))
		{
			// valid inside a type argument list
			++offset;
			continue;
		}

		return false;
	}

	return false;
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
	SyntaxToken openToken = expect(reader, TokenType::PipeOperator, L"Expected '|' token");
	syntax->set_open_token(openToken);

	detail::inline_ref_vector<ParameterSyntax, 4> parameters{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::PipeOperator)
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

			if (!try_match(reader, { TokenType::Comma, TokenType::PipeOperator }, L"Expected ',' or '|'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::PipeOperator)
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
	while (reader.can_consume())
	{
		TokenType currentType = reader.current().get_type();
		if (currentType != TokenType::Identifier && !is_predefined_type(currentType))
			break;

		gmt::Ref<TypeSyntax> baseType = read_type(reader, syntax);
		if (baseType.is_null())
			break;

		baseTypes.push_back(baseType);
		if (reader.can_consume() && reader.current().get_type() != TokenType::Comma)
			break;

		reader.consume();
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
	while (reader.can_consume())
	{
		TokenType currentType = reader.current().get_type();
		if (currentType != TokenType::Identifier && !is_predefined_type(currentType))
			break;

		gmt::Ref<TypeSyntax> constraintType = read_type(reader, syntax);
		if (constraintType.is_null())
			break;

		constraintTypes.push_back(constraintType);
		if (reader.can_consume() && reader.current().get_type() != TokenType::Comma)
			break;

		reader.consume();
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
		return gmt::nullref;
	}

	TokenType type = reader.current().get_type();
	if (type == TokenType::OpenBrace)
		return read_statements_block(reader, parent);

	if (type == TokenType::LambdaOperator)
		return read_arrow_clause(reader, parent);

	m_diagnostics.report_error(reader.current(), L"Expected method body");
	return gmt::nullref;
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
		return gmt::nullref;

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

	// variable declarations 'name: Type = expr' / 'name := expr' - two-token lookahead
	if (is_identifier_like(reader.current().get_type()) && reader.can_peek())
	{
		TokenType peekType = reader.peek().get_type();
		if (peekType == TokenType::Colon || peekType == TokenType::DeclareAssignOperator)
			return read_variable_statement(reader, parent);
	}

	// fall through to an expression statement when no other statement kind matched
	return read_expression_statement(reader, parent);
}

gmt::Ref<VariableStatementSyntax> SourceParser::read_variable_statement(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<VariableStatementSyntax>(parent);

	// current is the variable name, the caller confirmed ':' or ':=' via peek
	syntax->set_identifier(reader.current());
	reader.consume();

	if (reader.current().get_type() == TokenType::Colon)
	{
		reader.consume(); // consume ':'
		syntax->set_type(read_type(reader, syntax));

		if (syntax->get_type().is_null())
			m_diagnostics.report_error(reader.current(), L"Expected type after ':'");
	}
	else
	{
		// ':=' - the type is inferred by semantic analysis
		syntax->set_assign_token(reader.current());
		reader.consume();
	}

	if (syntax->get_assign_token().is_missing()
		&& reader.can_consume()
		&& reader.current().get_type() == TokenType::AssignOperator)
	{
		syntax->set_assign_token(reader.current());
		reader.consume();
	}

	if (reader.can_consume() && reader.current().get_type() != TokenType::Semicolon)
	{
		syntax->set_expression(read_expression(reader, syntax, 0));
	}
	else
	{
		m_diagnostics.report_error(reader.current(), L"Variable declaration is missing an initializer");
	}

	syntax->set_semicolon(expect(reader, TokenType::Semicolon, L"Expected ';'"));
	return syntax;
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

gmt::Ref<ExpressionSyntax> SourceParser::read_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, int parentPrecedence)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();

	++m_expressionDepth;
	if (m_expressionDepth > max_expression_depth)
	{
		m_diagnostics.report_error(reader.current(), L"Expression is too deeply nested");
		--m_expressionDepth;
		return gmt::nullref;
	}

	gmt::Ref<ExpressionSyntax> left = read_operand(reader, parent);
	while (!left.is_null() && reader.can_consume())
	{
		SyntaxToken operation = reader.current();
		TokenType operationType = operation.get_type();

		// type-testing operators take a type on the right
		if (operationType == TokenType::IsOperator || operationType == TokenType::AsOperator)
		{
			int precedence = get_operator_precendence(operationType);
			if (precedence <= parentPrecedence)
				break;

			reader.consume();

			gmt::Ref<TypeSyntax> targetType = read_type(reader, parent);
			if (targetType.is_null())
				break;

			if (operationType == TokenType::AsOperator)
			{
				auto cast = arena.emplace<CastExpressionSyntax>(parent);
				cast->set_operator_token(operation);
				cast->set_expression(left);
				cast->set_target_type(targetType);
				left = cast;
			}
			else
			{
				auto isExpression = arena.emplace<IsExpressionSyntax>(parent);
				isExpression->set_operator_token(operation);
				isExpression->set_expression(left);
				isExpression->set_target_type(targetType);
				left = isExpression;
			}

			continue;
		}

		if (!is_binary_operator(operationType))
			break;

		int precedence = get_operator_precendence(operationType);
		if (precedence <= parentPrecedence)
			break;

		reader.consume();

		gmt::Ref<ExpressionSyntax> right = read_expression(reader, parent, precedence);
		if (right.is_null())
			break;

		if (operationType == TokenType::RangeOperator || operationType == TokenType::RangeInclusiveOperator)
		{
			auto range = arena.emplace<RangeExpressionSyntax>(parent);
			range->set_operator_token(operation);
			range->set_left(left);
			range->set_right(right);

			left = range;
			continue;
		}

		auto binary = arena.emplace<BinaryExpressionSyntax>(parent);
		binary->set_left(left);
		binary->set_operator_token(operation);
		binary->set_right(right);
		left = binary;
	}

	--m_expressionDepth;
	return left;
}

gmt::Ref<ExpressionSyntax> SourceParser::read_operand(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	if (!reader.can_consume())
	{
		SyntaxToken eofToken(TokenType::EndOfFile, L"", TextLocation());
		m_diagnostics.report_error(eofToken, L"Expected expression");
		return gmt::nullref;
	}

	SyntaxToken current = reader.current();

	// prefix unary operators bind the tightest: '-a * b' is '(-a) * b'
	if (is_left_unary_operator(current.get_type()))
		return read_unary_expression(reader, parent);

	switch (current.get_type())
	{
		case TokenType::NullLiteral:
		case TokenType::CharLiteral:
		case TokenType::StringLiteral:
		case TokenType::BooleanLiteral:
		case TokenType::NumberLiteral:
		case TokenType::DoubleLiteral:
		case TokenType::ByteLiteral:
			return read_literal_expression(reader, parent);

		case TokenType::OpenCurl: // parenthesized grouping
		{
			reader.consume();
			gmt::Ref<ExpressionSyntax> inner = read_expression(reader, parent, 0);
			expect(reader, TokenType::CloseCurl, L"Expected ')'");
			return inner;
		}

		case TokenType::AwaitKeyword:
			return read_await_expression(reader, parent);

		case TokenType::IfKeyword: // ternary if-expression 'if cond a else b'
			return read_if_expression(reader, parent);

		case TokenType::OpenSquare: // collection expression '[1, 2, 3]'
			return read_collection_expression(reader, parent);

		case TokenType::NewKeyword: // object creation 'new T(args)'
			return read_object_expression(reader, parent);

		case TokenType::PipeOperator: // lambda '|args| => expr' / '|args| { }' - at operand position '|' can only be a lambda parameter list
			return read_lambda_expression(reader, parent);
	}

	if (is_identifier_like(current.get_type()))
	{
		// identifier-led operand: name, namespace-qualified name, generic instantiation, then the member access / invocation chain
		gmt::Ref<TypeSyntax> name = read_identifier_name_type(reader, parent);

		// in expression context '<' is generic only when the scan confirms it
		if (reader.can_consume() && reader.current().get_type() == TokenType::LessOperator && scan_generic_type_arguments(reader))
			name = read_generic_type(reader, parent, name);

		return read_linked_expression(reader, parent, name);
	}
	m_diagnostics.report_error(current, L"Expected expression");
	return gmt::nullref;
}

gmt::Ref<InvokationExpressionSyntax> SourceParser::read_invokation_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, gmt::Ref<ExpressionSyntax> previous, const SyntaxToken& identifier, const SyntaxToken& delimeter)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto invocation = arena.emplace<InvokationExpressionSyntax>(parent);

	invocation->set_previous(previous);
	invocation->set_identifier(identifier);
	invocation->set_delimeter_token(delimeter);
	invocation->set_arguments(read_arguments_list(reader, invocation));
	return invocation;
}

gmt::Ref<LiteralExpressionSyntax> SourceParser::read_literal_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<LiteralExpressionSyntax>(parent);

	syntax->set_literal(reader.current());
	reader.consume();

	return syntax;
}

gmt::Ref<UnaryExpressionSyntax> SourceParser::read_unary_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<UnaryExpressionSyntax>(parent);

	syntax->set_operator_token(reader.current());
	reader.consume();

	// operand binds the tightest: '-a * b' is '(-a) * b', '-a[i]' is '-(a[i])'
	syntax->set_operand(read_operand(reader, parent));
	return syntax;
}

gmt::Ref<AwaitExpressionSyntax> SourceParser::read_await_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<AwaitExpressionSyntax>(parent);

	syntax->set_await_keyword(reader.current());
	reader.consume();

	// operand binds the tightest: 'await x + y' is '(await x) + y'
	syntax->set_expression(read_operand(reader, parent));
	return syntax;
}

gmt::Ref<IfExpressionSyntax> SourceParser::read_if_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<IfExpressionSyntax>(parent);

	syntax->set_if_keyword(reader.current());
	reader.consume(); // consume 'if'

	syntax->set_condition(read_expression(reader, syntax, 0));
	syntax->set_then_expression(read_expression(reader, syntax, 0));

	if (reader.can_consume() && reader.current().get_type() == TokenType::ElseKeyword)
	{
		syntax->set_else_keyword(reader.current());
		reader.consume();

		syntax->set_else_expression(read_expression(reader, syntax, 0));
	}

	return syntax;
}

gmt::Ref<CollectionExpressionSyntax> SourceParser::read_collection_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<CollectionExpressionSyntax>(parent);
	syntax->set_open_bracket(expect(reader, TokenType::OpenSquare, L"Expected '['"));

	detail::inline_ref_vector<ExpressionSyntax, 8> values{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::CloseSquare)
	{
		syntax->set_close_bracket(reader.current());
		reader.consume();
	}
	else
	{
		while (reader.can_consume())
		{
			gmt::Ref<ExpressionSyntax> value = read_expression(reader, syntax, 0);
			if (!value.is_null())
				values.push_back(value);

			if (!try_match(reader, { TokenType::Comma, TokenType::CloseSquare }, L"Expected ',' or ']'", 3))
				break;

			SyntaxToken separatorToken = reader.current();
			reader.consume();

			if (separatorToken.get_type() == TokenType::CloseSquare)
			{
				syntax->set_close_bracket(separatorToken);
				break;
			}
		}
	}

	syntax->set_values(values.commit_array(arena));
	return syntax;
}

gmt::Ref<ObjectExpressionSyntax> SourceParser::read_object_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ObjectExpressionSyntax>(parent);

	syntax->set_new_token(expect(reader, TokenType::NewKeyword, L"Expected 'new' keyword"));
	syntax->set_type(read_type(reader, syntax));

	if (syntax->get_type().is_null())
	{
		m_diagnostics.report_error(reader.current(), L"Expected type after 'new'");
		return syntax;
	}

	if (reader.can_consume() && reader.current().get_type() == TokenType::OpenSquare)
	{
		// fixed-size array creation 'new Type[x]'
		reader.consume(); // consume '['
		syntax->set_array_size(read_expression(reader, syntax, 0));
		expect(reader, TokenType::CloseSquare, L"Expected ']'");
	}
	else
	{
		syntax->set_arguments(read_arguments_list(reader, syntax));
	}

	return syntax;
}

gmt::Ref<LambdaExpressionSyntax> SourceParser::read_lambda_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<LambdaExpressionSyntax>(parent);

	syntax->set_parameters(read_lambda_parameters(reader, syntax));

	if (reader.can_consume() && reader.current().get_type() == TokenType::LambdaOperator)
	{
		syntax->set_body(read_arrow_clause(reader, syntax));
	}
	else if (reader.can_consume() && reader.current().get_type() == TokenType::OpenBrace)
	{
		syntax->set_body(read_statements_block(reader, syntax));
	}
	else
	{
		m_diagnostics.report_error(reader.current(), L"Expected '=>' or '{' after lambda parameters");
	}

	return syntax;
}

gmt::Ref<ExpressionSyntax> SourceParser::read_linked_expression(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, gmt::Ref<ExpressionSyntax> previous)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	gmt::Ref<ExpressionSyntax> expression = previous;

	while (reader.can_consume())
	{
		switch (reader.current().get_type())
		{
			case TokenType::Delimeter: // member access '.'
			{
				SyntaxToken delimeter = reader.current();
				reader.consume();

				if (!reader.can_consume())
				{
					m_diagnostics.report_error(delimeter, L"Expected member name after '.'");
					return expression;
				}

				SyntaxToken memberName = reader.current();
				TokenType memberNameType = memberName.get_type();
				if (!is_identifier_like(memberNameType))
				{
					m_diagnostics.report_error(memberName, L"Expected member name after '.'");
					return expression;
				}

				reader.consume();

				if (reader.can_consume() && reader.current().get_type() == TokenType::OpenCurl)
				{
					// 'a.b(...)' - the invocation absorbs the member name
					expression = read_invokation_expression(reader, parent, expression, memberName, delimeter);
				}
				else
				{
					auto memberAccess = arena.emplace<MemberAccessExpressionSyntax>(parent);
					memberAccess->set_previous(expression);
					memberAccess->set_delimeter_token(delimeter);
					memberAccess->set_identifier(memberName);
					expression = memberAccess;
				}

				break;
			}

			case TokenType::OpenCurl: // invocation 'f(...)' or on a call result 'g()(...)'
			{
				SyntaxToken identifier(TokenType::Identifier, L"", TextLocation(), true);
				gmt::Ref<ExpressionSyntax> receiver = expression;

				ExpressionSyntax* raw = expression.as_ptr();
				if (raw->get_kind() == SyntaxKind::IdentifierNameType)
				{
					identifier = static_cast<IdentifierNameTypeSyntax*>(raw)->get_identifier();
					receiver = gmt::nullref;
				}
				else if (raw->get_kind() == SyntaxKind::QualifiedNameType)
				{
					identifier = static_cast<QualifiedNameTypeSyntax*>(raw)->get_identifier();
				}

				expression = read_invokation_expression(reader, parent, receiver, identifier, SyntaxToken());
				break;
			}

			case TokenType::OpenSquare: // indexer access 'a[i]'
			{
				auto indexator = arena.emplace<IndexatorExpressionSyntax>(parent);
				indexator->set_previous(expression);
				indexator->set_delimeter_token(reader.current());
				indexator->set_arguments(read_indexer_arguments_list(reader, indexator));
				
				expression = indexator;
				break;
			}

			case TokenType::IncrementOperator:
			case TokenType::DecrementOperator: // postfix 'a++' / 'a--'
			{
				auto unary = arena.emplace<UnaryExpressionSyntax>(parent);
				unary->set_operand(expression);
				unary->set_operator_token(reader.current());
				unary->set_is_postfix(true);

				expression = unary;
				reader.consume();
				break;
			}

			default:
				return expression;
		}
	}

	return expression;
}

gmt::Ref<ArgumentsListSyntax> SourceParser::read_indexer_arguments_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent)
{
	gmt::Arena& arena = m_syntaxTree.get_arena();
	auto syntax = arena.emplace<ArgumentsListSyntax>(parent);
	syntax->set_open_token(expect(reader, TokenType::OpenSquare, L"Expected '[' token"));

	detail::inline_ref_vector<ArgumentSyntax, 4> arguments{};

	if (reader.can_consume() && reader.current().get_type() == TokenType::CloseSquare)
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

	syntax->set_arguments(arguments.commit_array(arena));
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
		return gmt::nullref;

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
