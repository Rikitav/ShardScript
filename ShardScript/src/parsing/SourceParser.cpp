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

	bool try_match_identifier(SourceProvider& reader, DiagnosticsContext& diagnostics, int maxSkips)
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
					members.push_back(read_member_declaration(reader, unit));
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
	auto member = arena.emplace<MemberDeclarationSyntax>(SyntaxKind::Unknown, parent);

	detail::inline_ref_vector<AttributeSyntax, 5> attributes{};
	detail::inline_vector<SyntaxToken, 10> modifiers{};

	read_attribute_list(reader, parent, attributes.get_vector());
	read_member_modifiers(reader, modifiers.get_vector());

	if (reader.can_consume())
	{
		SyntaxToken current = reader.current();
		if (current.get_type() == TokenType::Identifier)
		{
			member->set_identifier(current);
			reader.consume();
		}
		else if (is_member_keyword(current.get_type()))
		{
			m_diagnostics.report_error(current, L"Member declarations of this kind are not supported yet");
		}
		else
		{
			m_diagnostics.report_error(current, L"Expected member declaration");
		}

		// NOTE: the full member grammar (types, parameter lists, bodies) is not
		// restored yet, skip the remaining tokens of this declaration
		detail::synchronize_to_next_top_level(reader);
	}

	member->set_attributes(attributes.commit_array(arena));
	member->set_modifiers(modifiers.commit_array(arena));
	return member;
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

void SourceParser::read_attribute_list(SourceProvider& reader, gmt::Ref<SyntaxNode> parent, std::pmr::vector<gmt::Ref<AttributeSyntax>>& attributes)
{
	while (reader.can_consume() && reader.current().get_type() == TokenType::OpenSquare)
		attributes.push_back(read_attribute(reader, parent));
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
