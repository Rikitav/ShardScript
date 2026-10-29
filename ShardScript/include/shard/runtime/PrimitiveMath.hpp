#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <shard/compilation/OperationCode.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace shard
{
	class StructSymbol;

	enum class PrimitiveStatus
	{
		Success,
		NotPrimitive,
		DivideByZero
	};

	struct PrimitiveShapes
	{
		const TypeShape* boolean;
		const TypeShape* integer;
		const TypeShape* doubleReal;
		const TypeShape* character;
		const TypeShape* nint;
		const TypeShape* byte;
		const TypeShape* string;
	};

	struct ScalarResult
	{
		const TypeShape* shape;
		std::uint64_t payload;
	};

	class SHARD_API PrimitiveMath
	{
		static bool is_double(const PrimitiveShapes& shapes, const TypeShape* shape);
		static bool is_integer(const PrimitiveShapes& shapes, const TypeShape* shape);
		static bool is_string(const PrimitiveShapes& shapes, const TypeShape* shape);

		static std::wstring_view string_view_of(ObjectInstance value);

	public:
		static PrimitiveStatus binary_math(OpCode op, const PrimitiveShapes& shapes, ObjectInstance left, ObjectInstance right, ScalarResult& result);
		static PrimitiveStatus compare(OpCode op, const PrimitiveShapes& shapes, ObjectInstance left, ObjectInstance right, ScalarResult& result);
		static PrimitiveStatus negate(const PrimitiveShapes& shapes, ObjectInstance value, ScalarResult& result);
		static PrimitiveStatus convert(const PrimitiveShapes& shapes, ObjectInstance value, StructSymbol* targetType, const TypeShape* targetShape, ScalarResult& result);

		// canonical primitive shapes are compared by identity; the
		// one-argument form is the name-based fallback used by the C API
		static gmt::wstring to_display_string(const PrimitiveShapes& shapes, ObjectInstance value);
		static gmt::wstring to_display_string(ObjectInstance value);
	};
}
