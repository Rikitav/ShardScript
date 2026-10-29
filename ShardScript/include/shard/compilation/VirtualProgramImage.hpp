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
	public:
		struct StringLiteral
		{
			std::size_t offset = 0;
			std::size_t length = 0; // code units, excluding the terminator
		};

	private:
		DataSection m_dataSection;
		BytecodeSection m_textSection;
		MethodSymbol* m_entryPoint = nullptr;
		gmt::vector<StringLiteral> m_stringLiterals;

	public:
		VirtualProgramImage() = default;
		VirtualProgramImage(const VirtualProgramImage& other) = delete;
		VirtualProgramImage& operator=(const VirtualProgramImage& other) = delete;

		BytecodeSection& get_text_section();
		const BytecodeSection& get_text_section() const;

		DataSection& get_data_section();
		const DataSection& get_data_section() const;

		// LOADCONST_STRING operands are literal ids into this table; the
		// runtime interns each literal once and reuses the instance
		std::uint32_t add_string_literal(std::size_t offset, std::size_t length);
		const StringLiteral& get_string_literal(std::uint32_t index) const;
		std::size_t get_string_literal_count() const;

		void set_entry_point(MethodSymbol* method);
		MethodSymbol* get_entry_point() const;
	};
}
