#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/LiteralSymbol.hpp>

#include <cerrno>
#include <charconv>
#include <clocale>
#include <cstdlib>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>

#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <Windows.h>
#endif

using namespace shard;

namespace
{
	struct NumericParseResult
	{
		bool success = false;
		std::wstring error;
	};

	bool is_valid_integer_digit(wchar_t symbol, int base)
	{
		if (symbol >= L'0' && symbol <= L'1')
			return base >= 2;

		if (symbol >= L'2' && symbol <= L'9')
			return base >= 10;

		if ((symbol >= L'a' && symbol <= L'f') || (symbol >= L'A' && symbol <= L'F'))
			return base >= 16;

		return false;
	}

	bool try_parse_base_prefix(std::wstring_view word, int& base, std::size_t& prefixLength)
	{
		base = 10;
		prefixLength = 0;

		if (word.size() < 2 || word[0] != L'0')
			return false;

		switch (word[1])
		{
			case L'x': case L'X': base = 16; prefixLength = 2; return true;
			case L'd': case L'D': base = 10; prefixLength = 2; return true;
			case L'b': case L'B': base = 2; prefixLength = 2; return true;
			default: return false;
		}
	}

	bool try_parse_volume_suffix(std::wstring_view word, std::uint64_t& multiplier, std::size_t& suffixLength)
	{
		multiplier = 1;
		suffixLength = 0;

		if (word.size() < 2)
			return false;

		wchar_t first = word[word.size() - 2];
		wchar_t second = word[word.size() - 1];

		if (second != L'B' && second != L'b')
			return false;

		switch (first)
		{
			case L'k': case L'K': multiplier = 1ULL << 10; break;
			case L'm': case L'M': multiplier = 1ULL << 20; break;
			case L'g': case L'G': multiplier = 1ULL << 30; break;
			case L't': case L'T': multiplier = 1ULL << 40; break;
			case L'p': case L'P': multiplier = 1ULL << 50; break;
			default: return false;
		}

		suffixLength = 2;
		return true;
	}

	double wstod_independent(std::wstring_view str, std::size_t* pos = nullptr)
	{
		if (str.empty())
			throw std::invalid_argument("wstod_independent: empty string");

		std::wstring terminated(str);
		wchar_t* endptr = nullptr;
		const wchar_t* start = terminated.c_str();
		errno = 0;
		double value = 0.0;

#ifdef _WIN32
		static _locale_t cLocale = _create_locale(LC_NUMERIC, "C");
		value = _wcstod_l(start, &endptr, cLocale);
#else
		value = std::wcstod(start, &endptr);
#endif

		if (endptr == start)
			throw std::invalid_argument("wstod_independent: no conversion could be performed");

		if (errno == ERANGE)
			throw std::out_of_range("wstod_independent: argument out of range");

		if (pos)
			*pos = static_cast<std::size_t>(endptr - start);

		return value;
	}

	NumericParseResult parse_integer_literal(std::wstring_view word, std::int64_t& outValue)
	{
		int base;
		std::size_t prefixLength;
		try_parse_base_prefix(word, base, prefixLength);

		std::size_t numLength = word.size() - prefixLength;
		std::uint64_t multiplier = 1;
		std::size_t suffixLength = 0;

		if (try_parse_volume_suffix(word.substr(prefixLength), multiplier, suffixLength))
			numLength -= suffixLength;

		std::wstring_view numPart = word.substr(prefixLength, numLength);

		for (wchar_t symbol : numPart)
		{
			if (!is_valid_integer_digit(symbol, base))
				return { false, L"Invalid characters in number" };
		}

		std::wstring terminated(numPart);
		try
		{
			std::size_t pos = 0;
			long long raw = std::stoll(terminated, &pos, base);

			if (pos != terminated.size())
				return { false, L"Invalid number format" };

			if (multiplier > 1)
			{
				if (raw > (std::numeric_limits<std::int64_t>::max)() / static_cast<long long>(multiplier))
					return { false, L"Multiplication overflow" };

				raw *= static_cast<long long>(multiplier);
			}

			outValue = static_cast<std::int64_t>(raw);
			return { true, L"" };
		}
		catch (const std::out_of_range&)
		{
			return { false, L"Number out of range" };
		}
		catch (const std::exception&)
		{
			return { false, L"Invalid number format" };
		}
	}

	NumericParseResult parse_double_literal(std::wstring_view word, double& outValue)
	{
		int base;
		std::size_t prefixLength;
		if (try_parse_base_prefix(word, base, prefixLength))
			return { false, L"Floating point number cannot have base prefix" };

		std::uint64_t multiplier;
		std::size_t suffixLength;
		if (try_parse_volume_suffix(word, multiplier, suffixLength))
			return { false, L"Floating point number cannot have suffix" };

		try
		{
			std::size_t pos = 0;
			double value = wstod_independent(word, &pos);

			if (pos != word.size())
				return { false, L"Invalid characters in floating point number" };

			outValue = value;
			return { true, L"" };
		}
		catch (const std::out_of_range&)
		{
			return { false, L"Number out of range" };
		}
		catch (const std::exception&)
		{
			return { false, L"Invalid floating point number format" };
		}
	}
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_literal_expression(const LiteralExpressionSyntax* node)
{
	const SyntaxToken literal = node->get_literal();
	SymbolTable& table = get_symbol_table();

	std::optional<gmt::Ref<const LiteralExpressionSyntax>> nodeRef = ref_from(node);
	if (!nodeRef.has_value())
		return gmt::nullref;

	gmt::Ref<LiteralSymbol> symbol = lookup_symbol<LiteralSymbol>(*nodeRef);
	if (symbol.is_null())
		symbol = table.bind_symbol<LiteralSymbol>(*nodeRef, literal.get_type(), gmt::Ref<SyntaxSymbol>(gmt::nullref));

	switch (literal.get_type())
	{
		case TokenType::NullLiteral:
		{
			return gmt::nullref;
		}

		case TokenType::BooleanLiteral:
		{
			symbol->set_boolean_value(literal.get_lexeme() == L"true");
			return table.get_boolean_primitive();
		}

		case TokenType::StringLiteral:
		{
			symbol->set_string_value(gmt::intern(literal.get_lexeme()));
			return table.get_string_primitive();
		}

		case TokenType::DoubleLiteral:
		{
			return analyze_double_literal(node);
		}

		case TokenType::NumberLiteral:
		{
			return analyze_number_literal(node);
		}

		case TokenType::CharLiteral:
		{
			std::wstring_view lexeme = literal.get_lexeme();
			if (lexeme.empty())
				get_diagnostics().report_error(node->get_location(), L"empty Char literal");
			else if (lexeme.size() > 1)
				get_diagnostics().report_error(node->get_location(), L"invalid Char literal length");
			else
				symbol->set_char_value(lexeme[0]);

			return table.get_char_primitive();
		}

		default:
			return gmt::nullref;
	}
}

void ExpressionBinder::visit_literal_expression(const LiteralExpressionSyntax* node)
{
	gmt::Ref<StructSymbol> type = analyze_literal_expression(node);

	std::optional<gmt::Ref<const LiteralExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);

	if (type.is_null() && node->get_literal().get_type() != TokenType::NullLiteral)
		get_diagnostics().report_error(node->get_location(), L"Unsupported literal type");
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_number_literal(const LiteralExpressionSyntax* node)
{
	SymbolTable& table = get_symbol_table();

	std::optional<gmt::Ref<const LiteralExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<LiteralSymbol> symbol = nodeRef.has_value() ? gmt::Ref<LiteralSymbol>(lookup_symbol<LiteralSymbol>(*nodeRef)) : gmt::Ref<LiteralSymbol>(gmt::nullref);
	if (symbol.is_null())
		return table.get_integer_primitive();

	std::int64_t value = 0;
	NumericParseResult result = parse_integer_literal(node->get_literal().get_lexeme(), value);
	if (!result.success)
		get_diagnostics().report_error(node->get_location(), result.error);
	else
		symbol->set_integer_value(value);

	gmt::Ref<StructSymbol> expectedType = resolve_left_denotation();
	bool expectedIsByte = expectedType == table.get_byte_primitive();
	bool expectedIsNint = expectedType == table.get_nint_primitive();
	bool matchesExpectedType = expectedIsByte || expectedIsNint;
	if (!expectedType.is_null() && matchesExpectedType)
	{
		symbol->set_bound_type(expectedType);
		return expectedType;
	}

	return table.get_integer_primitive();
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_double_literal(const LiteralExpressionSyntax* node)
{
	std::optional<gmt::Ref<const LiteralExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<LiteralSymbol> symbol = nodeRef.has_value() ? gmt::Ref<LiteralSymbol>(lookup_symbol<LiteralSymbol>(*nodeRef)) : gmt::Ref<LiteralSymbol>(gmt::nullref);
	if (symbol.is_null())
		return get_symbol_table().get_double_primitive();

	double value = 0.0;
	NumericParseResult result = parse_double_literal(node->get_literal().get_lexeme(), value);
	if (!result.success)
		get_diagnostics().report_error(node->get_location(), result.error);
	else
		symbol->set_double_value(value);

	return get_symbol_table().get_double_primitive();
}

void ExpressionBinder::visit_collection_expression(const CollectionExpressionSyntax* node)
{
	const std::span<const gmt::Ref<ExpressionSyntax>> values = node->get_values().as_span();

	for (const gmt::Ref<ExpressionSyntax>& value : values)
		visit_expression(value);

	gmt::Ref<StructSymbol> elementType = gmt::nullref;
	for (const gmt::Ref<ExpressionSyntax>& value : values)
	{
		if (elementType.is_null())
			elementType = get_expression_type(value);
	}

	for (const gmt::Ref<ExpressionSyntax>& value : values)
	{
		gmt::Ref<StructSymbol> valueType = get_expression_type(value);
		if (!valueType.is_null() && !SemanticModel::are_types_equal(valueType, elementType))
			get_diagnostics().report_error(node->get_location(), L"Element have type different from collection's target type");
	}

	std::optional<gmt::Ref<const CollectionExpressionSyntax>> nodeRef = ref_from(node);
	if (elementType.is_null())
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, gmt::nullref);
		return;
	}

	gmt::Ref<ArrayTypeSymbol> arrayType = m_factory.create_array_type(elementType, values.size());
	if (nodeRef.has_value())
	{
		get_symbol_table().bind_symbol<ArrayTypeSymbol>(*nodeRef, arrayType);
		set_expression_type(*nodeRef, arrayType);
	}
}

void ExpressionBinder::visit_range_expression(const RangeExpressionSyntax* node)
{
	SymbolTable& table = get_symbol_table();

	visit_expression(node->get_left());
	visit_expression(node->get_right());

	gmt::Ref<StructSymbol> leftType = get_expression_type(node->get_left());
	gmt::Ref<StructSymbol> rightType = get_expression_type(node->get_right());

	bool leftIsInteger = leftType == table.get_integer_primitive();
	bool rightIsInteger = rightType == table.get_integer_primitive();
	if (!leftIsInteger || !rightIsInteger)
		get_diagnostics().report_error(node->get_location(), L"Range bounds must be integers");

	std::optional<gmt::Ref<const RangeExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, table.get_standard_type(StandardTypes::RangeEnumerable).as<StructSymbol>());
}
