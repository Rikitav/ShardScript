#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shard
{
	using DataSection = gmt::vector<std::byte>;
	using BytecodeSection = gmt::vector<std::byte>;

	class SHARD_API VirtualProgramImage
	{
		DataSection m_dataSection;
		BytecodeSection m_textSection;
		MethodSymbol* m_entryPoint = nullptr;

	public:
		VirtualProgramImage() = default;
		VirtualProgramImage(const VirtualProgramImage& other) = delete;
		VirtualProgramImage& operator=(const VirtualProgramImage& other) = delete;

		BytecodeSection& get_text_section();
		const BytecodeSection& get_text_section() const;

		DataSection& get_data_section();
		const DataSection& get_data_section() const;

		void set_entry_point(MethodSymbol* method);
		MethodSymbol* get_entry_point() const;
	};
}
