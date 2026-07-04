#include <shard/runtime/PrimitiveMathModule.hpp>

#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <cmath>
#include <cwchar>
#include <string>
#include <stdexcept>
#include <cstdint>

using namespace shard;

namespace
{
	static TypeSymbol* TypeOf(ObjectInstance* instance)
	{
		if (instance == nullptr)
			return nullptr;

		return const_cast<TypeSymbol*>(instance->getInfo());
	}

	static bool IsNullInstance(ObjectInstance* instance)
	{
		if (instance == nullptr)
			return true;

		if (instance == GarbageCollector::NullInstance)
			return true;

		TypeSymbol* type = TypeOf(instance);
		return type == TYPE_NULL;
	}
}

PrimitiveMathModule::PrimitiveMathModule(GarbageCollector& garbageCollector)
	: gc(garbageCollector)
{
}

bool PrimitiveMathModule::IsNumericType(TypeSymbol* type)
{
	return type == TYPE_INT || type == TYPE_DOUBLE || type == TYPE_CHAR || type == TYPE_BOOL;
}

bool PrimitiveMathModule::IsIntegralType(TypeSymbol* type)
{
	return type == TYPE_INT || type == TYPE_CHAR;
}

bool PrimitiveMathModule::IsPrimitiveType(TypeSymbol* type)
{
	return type == TYPE_BOOL || type == TYPE_INT || type == TYPE_DOUBLE
		|| type == TYPE_CHAR || type == TYPE_STRING;
}

std::int64_t PrimitiveMathModule::AsInteger(ObjectInstance* instance)
{
	TypeSymbol* type = TypeOf(instance);
	if (type == TYPE_INT || type->Kind == SyntaxKind::EnumDeclaration)
		return instance->AsInteger();

	if (type == TYPE_DOUBLE)
		return static_cast<std::int64_t>(instance->AsDouble());

	if (type == TYPE_BOOL)
		return instance->AsBoolean() ? 1 : 0;

	if (type == TYPE_CHAR)
		return static_cast<std::int64_t>(instance->AsCharacter());

	return 0;
}

double PrimitiveMathModule::AsDouble(ObjectInstance* instance)
{
	TypeSymbol* type = TypeOf(instance);
	if (type == TYPE_DOUBLE)
		return instance->AsDouble();

	if (type == TYPE_INT || type->Kind == SyntaxKind::EnumDeclaration)
		return static_cast<double>(instance->AsInteger());

	if (type == TYPE_BOOL)
		return instance->AsBoolean() ? 1.0 : 0.0;

	if (type == TYPE_CHAR)
		return static_cast<double>(instance->AsCharacter());

	return 0.0;
}

bool PrimitiveMathModule::AsBoolean(ObjectInstance* instance)
{
	TypeSymbol* type = TypeOf(instance);
	if (type == TYPE_BOOL)
		return instance->AsBoolean();

	if (type == TYPE_INT || type->Kind == SyntaxKind::EnumDeclaration)
		return instance->AsInteger() != 0;

	if (type == TYPE_DOUBLE)
		return instance->AsDouble() != 0.0;

	if (type == TYPE_CHAR)
		return instance->AsCharacter() != L'\0';

	return false;
}

wchar_t PrimitiveMathModule::AsCharacter(ObjectInstance* instance)
{
	TypeSymbol* type = TypeOf(instance);
	if (type == TYPE_CHAR)
		return instance->AsCharacter();

	if (type == TYPE_INT || type->Kind == SyntaxKind::EnumDeclaration)
		return static_cast<wchar_t>(instance->AsInteger());

	if (type == TYPE_DOUBLE)
		return static_cast<wchar_t>(static_cast<std::int64_t>(instance->AsDouble()));

	if (type == TYPE_BOOL)
		return instance->AsBoolean() ? L'1' : L'\0';

	return L'\0';
}

ObjectInstance* PrimitiveMathModule::FromInteger(std::int64_t value) const
{
	return gc.FromValue(value);
}

ObjectInstance* PrimitiveMathModule::FromDouble(double value) const
{
	return gc.FromValue(value);
}

ObjectInstance* PrimitiveMathModule::FromBoolean(bool value) const
{
	return gc.FromValue(value);
}

ObjectInstance* PrimitiveMathModule::FromCharacter(wchar_t value) const
{
	return gc.FromValue(value);
}

std::wstring PrimitiveMathModule::ToString(ObjectInstance* instance) const
{
	TypeSymbol* type = TypeOf(instance);
	if (type == TYPE_STRING)
		return instance->AsString();

	if (type == TYPE_INT || type->Kind == SyntaxKind::EnumDeclaration)
		return std::to_wstring(instance->AsInteger());

	if (type == TYPE_DOUBLE)
		return std::to_wstring(instance->AsDouble());

	if (type == TYPE_BOOL)
		return instance->AsBoolean() ? L"true" : L"false";

	if (type == TYPE_CHAR)
	{
		wchar_t ch = instance->AsCharacter();
		return std::wstring(&ch, 1);
	}

	return L"";
}

ObjectInstance* PrimitiveMathModule::ExecuteBinary(TokenType opToken, ObjectInstance* left, ObjectInstance* right) const
{
	switch (opToken)
	{
		case TokenType::AddOperator: return ExecuteMathAddition(left, right);
		case TokenType::SubOperator: return ExecuteMathSubtraction(left, right);
		case TokenType::MultOperator: return ExecuteMathMultiplication(left, right);
		case TokenType::DivOperator: return ExecuteMathDivision(left, right);
		case TokenType::ModOperator: return ExecuteMathModulo(left, right);
		case TokenType::PowOperator: return ExecuteMathPower(left, right);

		case TokenType::OrOperator: return ExecuteBitwiseOr(left, right);
		case TokenType::AndOperator: return ExecuteBitwiseAnd(left, right);
		case TokenType::LeftShiftOperator: return ExecuteShiftLeft(left, right);
		case TokenType::RightShiftOperator: return ExecuteShiftRight(left, right);

		case TokenType::EqualsOperator: return ExecuteCompareEqual(left, right);
		case TokenType::NotEqualsOperator: return ExecuteCompareNotEqual(left, right);
		case TokenType::LessOperator: return ExecuteCompareLess(left, right);
		case TokenType::LessOrEqualsOperator: return ExecuteCompareLessOrEqual(left, right);
		case TokenType::GreaterOperator: return ExecuteCompareGreater(left, right);
		case TokenType::GreaterOrEqualsOperator: return ExecuteCompareGreaterOrEqual(left, right);

		default: return nullptr;
	}
}

ObjectInstance* PrimitiveMathModule::ExecuteUnary(TokenType opToken, ObjectInstance* operand) const
{
	TypeSymbol* type = TypeOf(operand);
	if (!IsNumericType(type))
	{
		if (opToken == TokenType::NotOperator)
			return ExecuteLogicalNot(operand);

		return nullptr;
	}

	switch (opToken)
	{
		case TokenType::NotOperator:
			return ExecuteLogicalNot(operand);

		case TokenType::SubOperator:
		{
			if (type == TYPE_DOUBLE)
				return FromDouble(-AsDouble(operand));

			return FromInteger(-AsInteger(operand));
		}

		case TokenType::AddOperator:
		{
			if (type == TYPE_DOUBLE)
				return FromDouble(+AsDouble(operand));

			return FromInteger(+AsInteger(operand));
		}

		default: return nullptr;
	}
}

ObjectInstance* PrimitiveMathModule::ExecuteCast(TypeSymbol* targetType, ObjectInstance* source) const
{
	if (targetType == nullptr || source == nullptr)
		return nullptr;

	if (targetType == TYPE_ANY)
		return source;

	TypeSymbol* sourceType = TypeOf(source);

	// Reference casts are intentionally left to the VM.
	if (targetType->Inlining == TypeInlining::ByReference || sourceType->Inlining == TypeInlining::ByReference)
		return nullptr;

	if (targetType == TYPE_INT)
		return FromInteger(AsInteger(source));

	if (targetType == TYPE_DOUBLE)
		return FromDouble(AsDouble(source));

	if (targetType == TYPE_BOOL)
		return FromBoolean(AsBoolean(source));

	if (targetType == TYPE_CHAR)
		return FromCharacter(AsCharacter(source));

	if (targetType->Kind == SyntaxKind::EnumDeclaration)
	{
		ObjectInstance* result = gc.AllocateInstance(targetType);
		result->WriteInteger(AsInteger(source));
		return result;
	}

	return nullptr;
}

// ---------------------------------------------------------------------------
// Arithmetic
// ---------------------------------------------------------------------------

ObjectInstance* PrimitiveMathModule::ExecuteMathAddition(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (lt == TYPE_STRING || rt == TYPE_STRING)
	{
		std::wstring result = ToString(left) + ToString(right);
		return gc.FromValue(result);
	}

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
		return FromDouble(AsDouble(left) + AsDouble(right));

	return FromInteger(AsInteger(left) + AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteMathSubtraction(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
		return FromDouble(AsDouble(left) - AsDouble(right));

	return FromInteger(AsInteger(left) - AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteMathMultiplication(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
		return FromDouble(AsDouble(left) * AsDouble(right));

	return FromInteger(AsInteger(left) * AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteMathDivision(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
	{
		double divisor = AsDouble(right);
		if (divisor == 0.0)
			throw std::runtime_error("Division by zero");
		return FromDouble(AsDouble(left) / divisor);
	}

	std::int64_t divisor = AsInteger(right);
	if (divisor == 0)
		throw std::runtime_error("Division by zero");

	return FromInteger(AsInteger(left) / divisor);
}

ObjectInstance* PrimitiveMathModule::ExecuteMathModulo(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
	{
		double divisor = AsDouble(right);
		if (divisor == 0.0)
			throw std::runtime_error("Modulo by zero");

		return FromDouble(std::fmod(AsDouble(left), divisor));
	}

	std::int64_t divisor = AsInteger(right);
	if (divisor == 0)
		throw std::runtime_error("Modulo by zero");
	return FromInteger(AsInteger(left) % divisor);
}

ObjectInstance* PrimitiveMathModule::ExecuteMathPower(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	double result = std::pow(AsDouble(left), AsDouble(right));
	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
		return FromDouble(result);

	return FromInteger(static_cast<std::int64_t>(result));
}

// ---------------------------------------------------------------------------
// Bitwise / shift
// ---------------------------------------------------------------------------

ObjectInstance* PrimitiveMathModule::ExecuteBitwiseOr(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (lt == TYPE_BOOL && rt == TYPE_BOOL)
		return FromBoolean(AsBoolean(left) || AsBoolean(right));

	if (!IsIntegralType(lt) || !IsIntegralType(rt))
		return nullptr;

	return FromInteger(AsInteger(left) | AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteBitwiseAnd(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (lt == TYPE_BOOL && rt == TYPE_BOOL)
		return FromBoolean(AsBoolean(left) && AsBoolean(right));

	if (!IsIntegralType(lt) || !IsIntegralType(rt))
		return nullptr;

	return FromInteger(AsInteger(left) & AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteShiftLeft(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (!IsIntegralType(lt) || !IsIntegralType(rt))
		return nullptr;

	return FromInteger(AsInteger(left) << AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteShiftRight(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (!IsIntegralType(lt) || !IsIntegralType(rt))
		return nullptr;

	return FromInteger(AsInteger(left) >> AsInteger(right));
}

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------

ObjectInstance* PrimitiveMathModule::ExecuteCompareEqual(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (IsNullInstance(left) || IsNullInstance(right))
		return FromBoolean(left == right);

	if (lt == TYPE_STRING && rt == TYPE_STRING)
	{
		std::size_t leftLength = *reinterpret_cast<const std::int64_t*>(left->getMemory());
		std::size_t rightLength = *reinterpret_cast<const std::int64_t*>(right->getMemory());
		if (leftLength != rightLength)
			return FromBoolean(false);

		return FromBoolean(std::wmemcmp(left->AsString(), right->AsString(), leftLength) == 0);
	}

	if (lt->Inlining == TypeInlining::ByReference && rt->Inlining == TypeInlining::ByReference)
		return FromBoolean(left == right);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
		return FromBoolean(AsDouble(left) == AsDouble(right));

	return FromBoolean(AsInteger(left) == AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteCompareNotEqual(ObjectInstance* left, ObjectInstance* right) const
{
	ObjectInstance* result = ExecuteCompareEqual(left, right);
	if (result == nullptr)
		return nullptr;

	return FromBoolean(!result->AsBoolean());
}

ObjectInstance* PrimitiveMathModule::ExecuteCompareLess(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (IsNullInstance(left) || IsNullInstance(right))
		return nullptr;

	if (lt == TYPE_STRING && rt == TYPE_STRING)
		return FromBoolean(std::wcscmp(left->AsString(), right->AsString()) < 0);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
		return FromBoolean(AsDouble(left) < AsDouble(right));

	return FromBoolean(AsInteger(left) < AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteCompareLessOrEqual(ObjectInstance* left, ObjectInstance* right) const
{
	ObjectInstance* less = ExecuteCompareLess(left, right);
	if (less != nullptr && less->AsBoolean())
		return less;

	return ExecuteCompareEqual(left, right);
}

ObjectInstance* PrimitiveMathModule::ExecuteCompareGreater(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (IsNullInstance(left) || IsNullInstance(right))
		return nullptr;

	if (lt == TYPE_STRING && rt == TYPE_STRING)
		return FromBoolean(std::wcscmp(left->AsString(), right->AsString()) > 0);

	if (!IsNumericType(lt) || !IsNumericType(rt))
		return nullptr;

	if (lt == TYPE_DOUBLE || rt == TYPE_DOUBLE)
		return FromBoolean(AsDouble(left) > AsDouble(right));

	return FromBoolean(AsInteger(left) > AsInteger(right));
}

ObjectInstance* PrimitiveMathModule::ExecuteCompareGreaterOrEqual(ObjectInstance* left, ObjectInstance* right) const
{
	ObjectInstance* greater = ExecuteCompareGreater(left, right);
	if (greater != nullptr && greater->AsBoolean())
		return greater;

	return ExecuteCompareEqual(left, right);
}

// ---------------------------------------------------------------------------
// Logical
// ---------------------------------------------------------------------------

ObjectInstance* PrimitiveMathModule::ExecuteLogicalNot(ObjectInstance* operand) const
{
	TypeSymbol* type = TypeOf(operand);
	if (type != TYPE_BOOL)
		return nullptr;

	return FromBoolean(!AsBoolean(operand));
}

ObjectInstance* PrimitiveMathModule::ExecuteLogicalOr(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (lt == TYPE_BOOL && rt == TYPE_BOOL)
		return FromBoolean(AsBoolean(left) || AsBoolean(right));

	return ExecuteBitwiseOr(left, right);
}

ObjectInstance* PrimitiveMathModule::ExecuteLogicalAnd(ObjectInstance* left, ObjectInstance* right) const
{
	TypeSymbol* lt = TypeOf(left);
	TypeSymbol* rt = TypeOf(right);

	if (lt == TYPE_BOOL && rt == TYPE_BOOL)
		return FromBoolean(AsBoolean(left) && AsBoolean(right));

	return ExecuteBitwiseAnd(left, right);
}
