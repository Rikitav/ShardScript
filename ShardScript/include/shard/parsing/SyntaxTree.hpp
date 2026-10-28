#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>
#include <shard/parsing/nodes/TranslationUnitSyntax.hpp>

#include <vector>
#include <span>

namespace shard
{
	class SHARD_API SyntaxTree final
	{
		gmt::Arena m_arena;
		gmt::vector<TranslationUnitSyntax*> m_units;

	public:
		SyntaxTree() = default;
		~SyntaxTree() = default;

		SyntaxTree(const SyntaxTree&) = delete;
		SyntaxTree& operator=(const SyntaxTree&) = delete;

		SyntaxTree(SyntaxTree&&) = delete;
		SyntaxTree& operator=(SyntaxTree&&) = delete;

		inline gmt::Arena& get_arena() { return m_arena; }
		inline const gmt::Arena& get_arena() const { return m_arena; }

		std::span<TranslationUnitSyntax* const> get_units() const;

		void add_unit(TranslationUnitSyntax* unit);
	};
}
