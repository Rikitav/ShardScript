#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>

namespace shard
{
	class SHARD_API ArrayTypeSymbol final : public StructSymbol
	{
		StructSymbol* m_underlayingType = nullptr;
		std::size_t m_length = 0;

	public:
		ArrayTypeSymbol(StructSymbol* underlaying_type, SyntaxSymbol* parent);
		virtual ~ArrayTypeSymbol() = default;

		ArrayTypeSymbol(const ArrayTypeSymbol& other) = delete;
		ArrayTypeSymbol& operator=(const ArrayTypeSymbol& other) = delete;

		ArrayTypeSymbol(ArrayTypeSymbol&& other) = delete;
		ArrayTypeSymbol& operator=(ArrayTypeSymbol&& other) = delete;

		StructSymbol* get_underlaying_type() const;

		std::size_t get_length() const;
		void set_length(std::size_t length);
	};
}