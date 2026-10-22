#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Containers.hpp>

#include <optional>
#include <string>
#include <vector>

namespace shard::semver
{
	// A semantic version Major.Minor.Patch with an optional prerelease string;
	// build metadata is not modeled
	struct SemanticVersion
	{
		int major = 0;
		int minor = 0;
		int patch = 0;
		gmt::wstring prerelease;

		bool operator==(const SemanticVersion& other) const;
		bool operator!=(const SemanticVersion& other) const;
		bool operator<(const SemanticVersion& other) const;
		bool operator>(const SemanticVersion& other) const;
		bool operator<=(const SemanticVersion& other) const;
		bool operator>=(const SemanticVersion& other) const;
	};

	// Parses "MAJOR.MINOR.PATCH" or "MAJOR.MINOR.PATCH-prerelease"
	SHARD_API std::optional<SemanticVersion> parse_version(const gmt::wstring& text);

	// A version range expression such as "1.2.3", "^1.2.3", ">=1.0.0 <2.0.0"
	class SHARD_API VersionExpression
	{
	public:
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

			Operator op = Operator::Equal;
			SemanticVersion version;
		};

	private:
		gmt::vector<Constraint> m_constraints;

		VersionExpression() = default;

	public:
		static std::optional<VersionExpression> parse(const gmt::wstring& text);
		bool satisfies(const SemanticVersion& version) const;
	};
}
