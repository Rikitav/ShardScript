#pragma once

#include <optional>
#include <string>
#include <vector>

namespace shard::semver
{
    /// <summary>
    /// A semantic version represented as Major.Minor.Patch with an optional
    /// prerelease string.  Build metadata is not modeled.
    /// </summary>
    struct SemanticVersion
    {
        int Major = 0;
        int Minor = 0;
        int Patch = 0;
        std::wstring Prerelease;

        bool operator==(const SemanticVersion& other) const;
        bool operator!=(const SemanticVersion& other) const;
        bool operator<(const SemanticVersion& other) const;
        bool operator>(const SemanticVersion& other) const;
        bool operator<=(const SemanticVersion& other) const;
        bool operator>=(const SemanticVersion& other) const;
    };

    /// <summary>
    /// Parses a version string of the form "MAJOR.MINOR.PATCH" or
    /// "MAJOR.MINOR.PATCH-prerelease".  Returns std::nullopt on failure.
    /// </summary>
    std::optional<SemanticVersion> ParseVersion(const std::wstring& text);

    /// <summary>
    /// A version range expression such as "1.2.3", "^1.2.3", ">=1.0.0 <2.0.0".
    /// </summary>
    class VersionExpression
    {
    public:
        /// <summary>
        /// Parses a version expression.  Returns std::nullopt on failure.
        /// </summary>
        static std::optional<VersionExpression> Parse(const std::wstring& text);

        /// <summary>
        /// Returns true when the supplied version satisfies this expression.
        /// </summary>
        bool Satisfies(const SemanticVersion& version) const;

        struct Constraint
        {
            enum class Operator
            {
                Equal,          // = or no operator (exact)
                Greater,        // >
                GreaterOrEqual, // >=
                Less,           // <
                LessOrEqual,    // <=
                Compatible,     // ^
                Approximately   // ~
            };

            Operator Op = Operator::Equal;
            SemanticVersion Version;
        };

    private:
        std::vector<Constraint> constraints;

        VersionExpression() = default;
    };
} // namespace shard::semver
