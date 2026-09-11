#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>

#include <vector>
#include <span>

namespace shard
{
	class SHARD_API SyntaxTree final
	{
		std::vector<TranslationUnitSyntax> m_units;
	
	public:
		SyntaxTree() = default;
		~SyntaxTree() = default;

		SyntaxTree(const SyntaxTree&) = delete;
		SyntaxTree& operator=(const SyntaxTree&) = delete;
	
		std::span<const TranslationUnitSyntax> get_units() const;
	
		void add_unit(TranslationUnitSyntax& unit);
	};
}
