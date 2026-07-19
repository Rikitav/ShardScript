#include <utilities/SemanticVersion.hpp>

#include <cwctype>
#include <optional>

namespace shard::semver
{
    static bool IsSpace(wchar_t c)
    {
        return std::iswspace(static_cast<wint_t>(c)) != 0;
    }

    static std::wstring Trim(const std::wstring& text)
    {
        std::size_t start = 0;
        while (start < text.size() && IsSpace(text[start]))
            ++start;

        std::size_t end = text.size();
        while (end > start && IsSpace(text[end - 1]))
            --end;

        return text.substr(start, end - start);
    }

    static std::optional<int> ParseInt(const std::wstring& text, std::size_t& pos)
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

    bool SemanticVersion::operator==(const SemanticVersion& other) const
    {
        return Major == other.Major && Minor == other.Minor && Patch == other.Patch;
    }

    bool SemanticVersion::operator!=(const SemanticVersion& other) const
    {
        return !(*this == other);
    }

    bool SemanticVersion::operator<(const SemanticVersion& other) const
    {
        if (Major != other.Major) return Major < other.Major;
        if (Minor != other.Minor) return Minor < other.Minor;
        return Patch < other.Patch;
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

    std::optional<SemanticVersion> ParseVersion(const std::wstring& text)
    {
        std::wstring input = Trim(text);
        if (input.empty())
            return std::nullopt;

        std::size_t pos = 0;

        auto major = ParseInt(input, pos);
        if (!major.has_value() || pos >= input.size() || input[pos] != L'.')
            return std::nullopt;
        ++pos;

        auto minor = ParseInt(input, pos);
        if (!minor.has_value() || pos >= input.size() || input[pos] != L'.')
            return std::nullopt;
        ++pos;

        auto patch = ParseInt(input, pos);
        if (!patch.has_value())
            return std::nullopt;

        SemanticVersion version;
        version.Major = major.value();
        version.Minor = minor.value();
        version.Patch = patch.value();

        if (pos < input.size())
        {
            if (input[pos] != L'-')
                return std::nullopt;

            ++pos;
            if (pos >= input.size())
                return std::nullopt;

            version.Prerelease = input.substr(pos);
        }

        return version;
    }

    static std::optional<VersionExpression::Constraint> ParseConstraint(std::wstring text)
    {
        text = Trim(text);
        if (text.empty())
            return std::nullopt;

        VersionExpression::Constraint constraint;
        constraint.Op = VersionExpression::Constraint::Operator::Equal;

        std::size_t pos = 0;
        if (text[pos] == L'^')
        {
            constraint.Op = VersionExpression::Constraint::Operator::Compatible;
            ++pos;
        }
        else if (text[pos] == L'~')
        {
            constraint.Op = VersionExpression::Constraint::Operator::Approximately;
            ++pos;
        }
        else if (text[pos] == L'>')
        {
            ++pos;
            if (pos < text.size() && text[pos] == L'=')
            {
                constraint.Op = VersionExpression::Constraint::Operator::GreaterOrEqual;
                ++pos;
            }
            else
            {
                constraint.Op = VersionExpression::Constraint::Operator::Greater;
            }
        }
        else if (text[pos] == L'<')
        {
            ++pos;
            if (pos < text.size() && text[pos] == L'=')
            {
                constraint.Op = VersionExpression::Constraint::Operator::LessOrEqual;
                ++pos;
            }
            else
            {
                constraint.Op = VersionExpression::Constraint::Operator::Less;
            }
        }
        else if (text[pos] == L'=')
        {
            ++pos;
        }

        auto version = ParseVersion(text.substr(pos));
        if (!version.has_value())
            return std::nullopt;

        constraint.Version = version.value();
        return constraint;
    }

    std::optional<VersionExpression> VersionExpression::Parse(const std::wstring& text)
    {
        VersionExpression expression;

        std::size_t start = 0;
        while (start < text.size())
        {
            // Skip whitespace and commas between constraints.
            while (start < text.size() && (IsSpace(text[start]) || text[start] == L','))
                ++start;

            if (start >= text.size())
                break;

            std::size_t end = start;
            while (end < text.size() && !IsSpace(text[end]) && text[end] != L',')
                ++end;

            auto constraint = ParseConstraint(text.substr(start, end - start));
            if (!constraint.has_value())
                return std::nullopt;

            expression.constraints.push_back(constraint.value());
            start = end;
        }

        if (expression.constraints.empty())
            return std::nullopt;

        return expression;
    }

    bool VersionExpression::Satisfies(const SemanticVersion& version) const
    {
        for (const auto& constraint : constraints)
        {
            switch (constraint.Op)
            {
                case Constraint::Operator::Equal:
                {
                    if (version != constraint.Version)
                        return false;

                    break;
                }

                case Constraint::Operator::Greater:
                {
                    if (!(version > constraint.Version))
                        return false;

                    break;
                }

                case Constraint::Operator::GreaterOrEqual:
                {
                    if (!(version >= constraint.Version))
                        return false;

                    break;
                }

                case Constraint::Operator::Less:
                {
                    if (!(version < constraint.Version))
                        return false;

                    break;
                }

                case Constraint::Operator::LessOrEqual:
                {
                    if (!(version <= constraint.Version))
                        return false;

                    break;
                }

                case Constraint::Operator::Compatible:
                {
                    // ^1.2.3 := >=1.2.3 <2.0.0
                    // ^0.2.3 := >=0.2.3 <0.3.0
                    // ^0.0.3 := >=0.0.3 <0.0.4
                    if (version < constraint.Version)
                        return false;

                    SemanticVersion upper = constraint.Version;
                    if (constraint.Version.Major > 0)
                    {
                        upper.Minor = 0;
                        upper.Patch = 0;
                        ++upper.Major;
                    }
                    else if (constraint.Version.Minor > 0)
                    {
                        upper.Patch = 0;
                        ++upper.Minor;
                    }
                    else
                    {
                        ++upper.Patch;
                    }

                    if (!(version < upper))
                        return false;

                    break;
                }

                case Constraint::Operator::Approximately:
                {
                    // ~1.2.3 := >=1.2.3 <1.3.0
                    if (version < constraint.Version)
                        return false;

                    SemanticVersion upper = constraint.Version;
                    upper.Patch = 0;
                    ++upper.Minor;

                    if (!(version < upper))
                        return false;

                    break;
                }
            }
        }

        return true;
    }
} // namespace shard::semver
