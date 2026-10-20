#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shard
{
	using DataSection = std::vector<std::byte>;
	using BytecodeSection = std::vector<std::byte>;

	class SHARD_API VirtualProgramImage
	{
		DataSection m_dataSection;
		BytecodeSection m_textSection;

		// method directory for disassembly; bytecode operands reference symbols
		// by raw arena offset instead, resolved against the SymbolTable arena
		std::vector<gmt::Ref<MethodSymbol>> m_methods;
		gmt::Ref<MethodSymbol> m_entryPoint;

	public:
		VirtualProgramImage() = default;
		VirtualProgramImage(const VirtualProgramImage& other) = delete;
		VirtualProgramImage& operator=(const VirtualProgramImage& other) = delete;

		BytecodeSection& get_text_section();
		const BytecodeSection& get_text_section() const;

		DataSection& get_data_section();
		const DataSection& get_data_section() const;

		void register_method(gmt::Ref<MethodSymbol> method);
		const std::vector<gmt::Ref<MethodSymbol>>& get_methods() const;

		void set_entry_point(gmt::Ref<MethodSymbol> method);
		gmt::Ref<MethodSymbol> get_entry_point() const;
	};
}
