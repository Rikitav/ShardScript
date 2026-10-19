#include <shard/runtime/PrimitiveMath.hpp>

#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace shard;

bool PrimitiveMath::is_double(const PrimitiveShapes& shapes, const TypeShape* shape)
{
	return shape != nullptr && shape == shapes.doubleReal;
}

bool PrimitiveMath::is_integer(const PrimitiveShapes& shapes, const TypeShape* shape)
{
	if (shape == nullptr)
		return false;

	return shape == shapes.integer
		|| shape == shapes.nint
		|| shape == shapes.byte
		|| shape == shapes.character;
}

bool PrimitiveMath::is_string(const PrimitiveShapes& shapes, const TypeShape* shape)
{
	return shape != nullptr && shape == shapes.string;
}

std::wstring_view PrimitiveMath::string_view_of(ObjectInstance value)
{
	if (value.is_null())
		return std::wstring_view();

	std::int64_t length = value.as_integer();
	const wchar_t* data = reinterpret_cast<const wchar_t*>(value.get_memory() + sizeof(std::int64_t));
	return std::wstring_view(data, static_cast<std::size_t>(length));
}

PrimitiveStatus PrimitiveMath::binary_math(OpCode op, const PrimitiveShapes& shapes, ObjectInstance left, ObjectInstance right, ScalarResult& result)
{
	const TypeShape* leftShape = left.get_shape();
	const TypeShape* rightShape = right.get_shape();

	if (is_string(shapes, leftShape) || is_string(shapes, rightShape))
		return PrimitiveStatus::NotPrimitive;

	bool doubleMath = is_double(shapes, leftShape) || is_double(shapes, rightShape);
	if (doubleMath)
	{
		double leftValue = is_double(shapes, leftShape) ? left.as_double() : static_cast<double>(left.as_integer());
		double rightValue = is_double(shapes, rightShape) ? right.as_double() : static_cast<double>(right.as_integer());

		double computed;
		switch (op)
		{
			case OpCode::MATH_ADDITION: computed = leftValue + rightValue; break;
			case OpCode::MATH_SUBTRACT: computed = leftValue - rightValue; break;
			case OpCode::MATH_MULTIPLY: computed = leftValue * rightValue; break;
			case OpCode::MATH_DIVISION: computed = leftValue / rightValue; break;
			case OpCode::MATH_MODULO: computed = std::fmod(leftValue, rightValue); break;
			case OpCode::MATH_POWER: computed = std::pow(leftValue, rightValue); break;
			default:
				return PrimitiveStatus::NotPrimitive;
		}

		result.shape = shapes.doubleReal;
		std::memcpy(&result.payload, &computed, sizeof(double));
		return PrimitiveStatus::Success;
	}

	if (!is_integer(shapes, leftShape) || !is_integer(shapes, rightShape))
		return PrimitiveStatus::NotPrimitive;

	std::int64_t leftValue = left.as_integer();
	std::int64_t rightValue = right.as_integer();

	std::int64_t computed;
	switch (op)
	{
		case OpCode::MATH_ADDITION: computed = leftValue + rightValue; break;
		case OpCode::MATH_SUBTRACT: computed = leftValue - rightValue; break;
		case OpCode::MATH_MULTIPLY: computed = leftValue * rightValue; break;
		case OpCode::MATH_DIVISION:
		{
			if (rightValue == 0)
				return PrimitiveStatus::DivideByZero;

			computed = leftValue / rightValue;
			break;
		}
		case OpCode::MATH_MODULO:
		{
			if (rightValue == 0)
				return PrimitiveStatus::DivideByZero;

			computed = leftValue % rightValue;
			break;
		}
		case OpCode::MATH_POWER:
		{
			computed = 1;
			std::int64_t base = leftValue;
			std::int64_t exponent = rightValue;
			while (exponent > 0)
			{
				if ((exponent & 1) != 0)
					computed *= base;

				base *= base;
				exponent >>= 1;
			}
			break;
		}
		case OpCode::MATH_SHL: computed = leftValue << rightValue; break;
		case OpCode::MATH_SHR: computed = leftValue >> rightValue; break;
		default:
			return PrimitiveStatus::NotPrimitive;
	}

	result.shape = shapes.integer;
	std::memcpy(&result.payload, &computed, sizeof(std::int64_t));
	return PrimitiveStatus::Success;
}

PrimitiveStatus PrimitiveMath::compare(OpCode op, const PrimitiveShapes& shapes, ObjectInstance left, ObjectInstance right, ScalarResult& result)
{
	const TypeShape* leftShape = left.get_shape();
	const TypeShape* rightShape = right.get_shape();

	bool outcome;
	if (is_string(shapes, leftShape) && is_string(shapes, rightShape))
	{
		std::wstring_view leftText = string_view_of(left);
		std::wstring_view rightText = string_view_of(right);

		switch (op)
		{
			case OpCode::COMPARE_EQUAL: outcome = leftText == rightText; break;
			case OpCode::COMPARE_NOTEQUAL: outcome = leftText != rightText; break;
			default:
				return PrimitiveStatus::NotPrimitive;
		}
	}
	else if (is_double(shapes, leftShape) || is_double(shapes, rightShape))
	{
		double leftValue = is_double(shapes, leftShape) ? left.as_double() : static_cast<double>(left.as_integer());
		double rightValue = is_double(shapes, rightShape) ? right.as_double() : static_cast<double>(right.as_integer());

		switch (op)
		{
			case OpCode::COMPARE_EQUAL: outcome = leftValue == rightValue; break;
			case OpCode::COMPARE_NOTEQUAL: outcome = leftValue != rightValue; break;
			case OpCode::COMPARE_GREATER: outcome = leftValue > rightValue; break;
			case OpCode::COMPARE_GREATER_OR_EQUAL: outcome = leftValue >= rightValue; break;
			case OpCode::COMPARE_LESS: outcome = leftValue < rightValue; break;
			case OpCode::COMPARE_LESS_OR_EQUAL: outcome = leftValue <= rightValue; break;
			default:
				return PrimitiveStatus::NotPrimitive;
		}
	}
	else
	{
		std::int64_t leftValue = left.as_integer();
		std::int64_t rightValue = right.as_integer();

		switch (op)
		{
			case OpCode::COMPARE_EQUAL: outcome = leftValue == rightValue; break;
			case OpCode::COMPARE_NOTEQUAL: outcome = leftValue != rightValue; break;
			case OpCode::COMPARE_GREATER: outcome = leftValue > rightValue; break;
			case OpCode::COMPARE_GREATER_OR_EQUAL: outcome = leftValue >= rightValue; break;
			case OpCode::COMPARE_LESS: outcome = leftValue < rightValue; break;
			case OpCode::COMPARE_LESS_OR_EQUAL: outcome = leftValue <= rightValue; break;
			default:
				return PrimitiveStatus::NotPrimitive;
		}
	}

	result.shape = shapes.boolean;
	std::uint64_t payload = outcome ? 1 : 0;
	std::memcpy(&result.payload, &payload, sizeof(bool));
	return PrimitiveStatus::Success;
}

PrimitiveStatus PrimitiveMath::negate(const PrimitiveShapes& shapes, ObjectInstance value, ScalarResult& result)
{
	if (is_double(shapes, value.get_shape()))
	{
		double computed = -value.as_double();
		result.shape = shapes.doubleReal;
		std::memcpy(&result.payload, &computed, sizeof(double));
		return PrimitiveStatus::Success;
	}

	if (is_integer(shapes, value.get_shape()))
	{
		std::int64_t computed = -value.as_integer();
		result.shape = shapes.integer;
		std::memcpy(&result.payload, &computed, sizeof(std::int64_t));
		return PrimitiveStatus::Success;
	}

	return PrimitiveStatus::NotPrimitive;
}

PrimitiveStatus PrimitiveMath::convert(const PrimitiveShapes& shapes, ObjectInstance value, gmt::Ref<StructSymbol> targetType, const TypeShape* targetShape, ScalarResult& result)
{
	if (targetShape == nullptr || targetType.is_null())
		return PrimitiveStatus::NotPrimitive;

	if (targetShape == shapes.doubleReal)
	{
		double converted;
		if (is_double(shapes, value.get_shape()))
			converted = value.as_double();
		else
			converted = static_cast<double>(value.as_integer());

		result.shape = targetShape;
		std::memcpy(&result.payload, &converted, sizeof(double));
		return PrimitiveStatus::Success;
	}

	if (targetShape == shapes.integer || targetShape == shapes.nint)
	{
		std::int64_t converted;
		if (is_double(shapes, value.get_shape()))
			converted = static_cast<std::int64_t>(value.as_double());
		else
			converted = value.as_integer();

		result.shape = targetShape;
		std::memcpy(&result.payload, &converted, sizeof(std::int64_t));
		return PrimitiveStatus::Success;
	}

	if (targetShape == shapes.character)
	{
		wchar_t converted = static_cast<wchar_t>(value.as_integer());
		result.shape = targetShape;
		std::memcpy(&result.payload, &converted, sizeof(wchar_t));
		return PrimitiveStatus::Success;
	}

	if (targetShape == shapes.boolean)
	{
		bool converted = value.as_integer() != 0;
		result.shape = targetShape;
		std::memcpy(&result.payload, &converted, sizeof(bool));
		return PrimitiveStatus::Success;
	}

	if (targetShape == shapes.byte)
	{
		std::uint8_t converted = static_cast<std::uint8_t>(value.as_integer());
		result.shape = targetShape;
		std::memcpy(&result.payload, &converted, sizeof(std::uint8_t));
		return PrimitiveStatus::Success;
	}

	return PrimitiveStatus::NotPrimitive;
}

std::wstring PrimitiveMath::to_display_string(ObjectInstance value)
{
	if (value.is_null())
		return L"null";

	const TypeShape* shape = value.get_shape();
	if (shape == nullptr)
		return L"<void>";

	gmt::Ref<StructSymbol> baseType = shape->get_base_type();
	if (baseType.is_null())
		return L"<unknown>";

	std::wstring_view name = baseType->get_name();

	if (name == L"string")
		return std::wstring(string_view_of(value));

	if (name == L"int" || name == L"nint" || name == L"byte")
		return std::to_wstring(value.as_integer());

	if (name == L"bool")
		return value.as_boolean() ? L"true" : L"false";

	if (name == L"char")
		return std::wstring(1, value.as_char());

	if (name == L"double")
	{
		wchar_t buffer[32];
		std::swprintf(buffer, 32, L"%g", value.as_double());
		return std::wstring(buffer);
	}

	return std::wstring(name);
}
