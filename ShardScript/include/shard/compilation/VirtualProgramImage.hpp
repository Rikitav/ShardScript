#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace shard
{
	using DataSection = std::vector<std::byte>;
	using BytecodeSection = std::vector<std::byte>;

	class SHARD_API VirtualProgramImage
	{
		DataSection m_dataSection;
		BytecodeSection m_textSection;

		std::vector<gmt::Ref<SyntaxSymbol>> m_symbols;
		std::unordered_map<std::size_t, std::uint32_t> m_symbolHandles;
		gmt::Ref<MethodSymbol> m_entryPoint;

	public:
		VirtualProgramImage() = default;
		VirtualProgramImage(const VirtualProgramImage& other) = delete;
		VirtualProgramImage& operator=(const VirtualProgramImage& other) = delete;

		BytecodeSection& get_text_section();
		const BytecodeSection& get_text_section() const;

		DataSection& get_data_section();
		const DataSection& get_data_section() const;

		std::uint32_t intern_symbol(gmt::Ref<SyntaxSymbol> symbol);
		gmt::Ref<SyntaxSymbol> resolve_symbol(std::uint32_t handle) const;
		const std::vector<gmt::Ref<SyntaxSymbol>>& get_symbols() const;

		void set_entry_point(gmt::Ref<MethodSymbol> method);
		gmt::Ref<MethodSymbol> get_entry_point() const;
	};
}
