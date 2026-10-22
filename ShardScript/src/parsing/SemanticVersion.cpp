#include <shard/parsing/SemanticVersion.hpp>

#include <gmt/Containers.hpp>

#include <cwctype>

using namespace shard::semver;

namespace
{
	bool is_space(wchar_t chr)
	{
		return std::iswspace(static_cast<wint_t>(chr)) != 0;
	}

	gmt::wstring trim(const gmt::wstring& text)
	{
		std::size_t start = 0;
		while (start < text.size() && is_space(text[start]))
			++start;

		std::size_t end = text.size();
		while (end > start && is_space(text[end - 1]))
			--end;

		return text.substr(start, end - start);
	}

	std::optional<int> parse_int(const gmt::wstring& text, std::size_t& pos)
	{
		if (pos >= text.size() || !std::iswdigit(static_cast<wint_t>(text[pos])))
			return std::nullopt;

		int value = 0;
		while (pos < text.size() && std::iswdigit(static_cast<wint_t>(text[pos])))
		{
			value = value * 10 + (text[pos] - L'0');
			++pos;
		}

		return value;
	}

	std::optional<VersionExpression::Constraint> parse_constraint(gmt::wstring text)
	{
		text = trim(text);
		if (text.empty())
			return std::nullopt;

		VersionExpression::Constraint constraint;

		std::size_t pos = 0;
		if (text[pos] == L'^')
		{
			constraint.op = VersionExpression::Constraint::Operator::Compatible;
			++pos;
		}
		else if (text[pos] == L'~')
		{
			constraint.op = VersionExpression::Constraint::Operator::Approximately;
			++pos;
		}
		else if (text[pos] == L'>')
		{
			++pos;
			if (pos < text.size() && text[pos] == L'=')
			{
				constraint.op = VersionExpression::Constraint::Operator::GreaterOrEqual;
				++pos;
			}
			else
			{
				constraint.op = VersionExpression::Constraint::Operator::Greater;
			}
		}
		else if (text[pos] == L'<')
		{
			++pos;
			if (pos < text.size() && text[pos] == L'=')
			{
				constraint.op = VersionExpression::Constraint::Operator::LessOrEqual;
				++pos;
			}
			else
			{
				constraint.op = VersionExpression::Constraint::Operator::Less;
			}
		}
		else if (text[pos] == L'=')
		{
			++pos;
		}

		std::optional<SemanticVersion> version = parse_version(text.substr(pos));
		if (!version.has_value())
			return std::nullopt;

		constraint.version = version.value();
		return constraint;
	}
}

bool SemanticVersion::operator==(const SemanticVersion& other) const
{
	return major == other.major && minor == other.minor && patch == other.patch;
}

bool SemanticVersion::operator!=(const SemanticVersion& other) const
{
	return !(*this == other);
}

bool SemanticVersion::operator<(const SemanticVersion& other) const
{
	if (major != other.major)
		return major < other.major;

	if (minor != other.minor)
		return minor < other.minor;

	return patch < other.patch;
}

bool SemanticVersion::operator>(const SemanticVersion& other) const
{
	return other < *this;
}

bool SemanticVersion::operator<=(const SemanticVersion& other) const
{
	return !(other < *this);
}

bool SemanticVersion::operator>=(const SemanticVersion& other) const
{
	return !(*this < other);
}

std::optional<SemanticVersion> shard::semver::parse_version(const gmt::wstring& text)
{
	gmt::wstring input = trim(text);
	if (input.empty())
		return std::nullopt;

	std::size_t pos = 0;

	std::optional<int> major = parse_int(input, pos);
	if (!major.has_value() || pos >= input.size() || input[pos] != L'.')
		return std::nullopt;

	++pos;

	std::optional<int> minor = parse_int(input, pos);
	if (!minor.has_value() || pos >= input.size() || input[pos] != L'.')
		return std::nullopt;

	++pos;

	std::optional<int> patch = parse_int(input, pos);
	if (!patch.has_value())
		return std::nullopt;

	SemanticVersion version;
	version.major = major.value();
	version.minor = minor.value();
	version.patch = patch.value();

	if (pos < input.size())
	{
		if (input[pos] != L'-')
			return std::nullopt;

		++pos;
		if (pos >= input.size())
			return std::nullopt;

		version.prerelease = input.substr(pos);
	}

	return version;
}

std::optional<VersionExpression> VersionExpression::parse(const gmt::wstring& text)
{
	VersionExpression expression;

	std::size_t start = 0;
	while (start < text.size())
	{
		while (start < text.size() && (is_space(text[start]) || text[start] == L','))
			++start;

		if (start >= text.size())
			break;

		std::size_t end = start;
		while (end < text.size() && !is_space(text[end]) && text[end] != L',')
			++end;

		std::optional<Constraint> constraint = parse_constraint(text.substr(start, end - start));
		if (!constraint.has_value())
			return std::nullopt;

		expression.m_constraints.push_back(constraint.value());
		start = end;
	}

	if (expression.m_constraints.empty())
		return std::nullopt;

	return expression;
}

bool VersionExpression::satisfies(const SemanticVersion& version) const
{
	for (const Constraint& constraint : m_constraints)
	{
		switch (constraint.op)
		{
			case Constraint::Operator::Equal:
			{
				if (version != constraint.version)
					return false;

				break;
			}

			case Constraint::Operator::Greater:
			{
				if (!(version > constraint.version))
					return false;

				break;
			}

			case Constraint::Operator::GreaterOrEqual:
			{
				if (!(version >= constraint.version))
					return false;

				break;
			}

			case Constraint::Operator::Less:
			{
				if (!(version < constraint.version))
					return false;

				break;
			}

			case Constraint::Operator::LessOrEqual:
			{
				if (!(version <= constraint.version))
					return false;

				break;
			}

			case Constraint::Operator::Compatible:
			{
				// ^1.2.3 := >=1.2.3 <2.0.0
				// ^0.2.3 := >=0.2.3 <0.3.0
				// ^0.0.3 := >=0.0.3 <0.0.4
				if (version < constraint.version)
					return false;

				SemanticVersion upper = constraint.version;
				if (constraint.version.major > 0)
				{
					upper.minor = 0;
					upper.patch = 0;
					++upper.major;
				}
				else if (constraint.version.minor > 0)
				{
					upper.patch = 0;
					++upper.minor;
				}
				else
				{
					++upper.patch;
				}

				if (!(version < upper))
					return false;

				break;
			}

			case Constraint::Operator::Approximately:
			{
				// ~1.2.3 := >=1.2.3 <1.3.0
				if (version < constraint.version)
					return false;

				SemanticVersion upper = constraint.version;
				upper.patch = 0;
				++upper.minor;

				if (!(version < upper))
					return false;

				break;
			}
		}
	}

	return true;
}
