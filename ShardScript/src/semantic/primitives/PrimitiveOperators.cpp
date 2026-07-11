#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/PrimitiveOperators.hpp>

#include <shard/semantic/SymbolFactory.hpp>
#include <shard/parsing/SyntaxFacts.hpp>
#include <shard/lexical/TokenType.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

using namespace shard;

static const wchar_t* concatStrings(const wchar_t* left, const wchar_t* right)
{
	if (left == nullptr || right == nullptr)
		throw std::invalid_argument("Input strings cannot be null");

	std::size_t leftLen = wcslen(left);
	std::size_t rightLen = wcslen(right);

	std::size_t leftSize = leftLen * sizeof(wchar_t);
	std::size_t rightSize = rightLen * sizeof(wchar_t);
	std::size_t totalSize = leftSize + rightSize + sizeof(wchar_t);

	wchar_t* result = static_cast<wchar_t*>(malloc(totalSize));
	if (result == nullptr)
		throw std::runtime_error("Failed to allocate space for new string");

	std::memcpy(result, left, leftSize);
	std::memcpy(result + leftLen, right, rightSize + sizeof(wchar_t));

	return result;
}

// -----------------------------------------------------------------------------
// Integer operators
// -----------------------------------------------------------------------------

static ObjectInstance* integer_op_AddOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left + right);
}

static ObjectInstance* integer_op_SubOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left - right);
}

static ObjectInstance* integer_op_MultOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left * right);
}

static ObjectInstance* integer_op_DivOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left / right);
}

static ObjectInstance* integer_op_ModOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left % right);
}

static ObjectInstance* integer_op_PowOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(static_cast<std::int64_t>(pow(left, right)));
}

static ObjectInstance* integer_op_OrOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left | right);
}

static ObjectInstance* integer_op_AndOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left & right);
}

static ObjectInstance* integer_op_LeftShiftOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left << right);
}

static ObjectInstance* integer_op_RightShiftOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left >> right);
}

static ObjectInstance* integer_op_EqualsOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left == right);
}

static ObjectInstance* integer_op_NotEqualsOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left != right);
}

static ObjectInstance* integer_op_LessOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left < right);
}

static ObjectInstance* integer_op_LessOrEqualsOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left <= right);
}

static ObjectInstance* integer_op_GreaterOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left > right);
}

static ObjectInstance* integer_op_GreaterOrEqualsOperator(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	std::int64_t right = context.Args[1]->AsInteger();
	return context.Collector.FromValue(left >= right);
}

static ObjectInstance* integer_op_IncrementOperator(const CallState& context)
{
	std::int64_t value = context.Args[0]->AsInteger();
	return context.Collector.FromValue(value + 1);
}

static ObjectInstance* integer_op_DecrementOperator(const CallState& context)
{
	std::int64_t value = context.Args[0]->AsInteger();
	return context.Collector.FromValue(value - 1);
}

static ObjectInstance* integer_op_UnaryNegation(const CallState& context)
{
	std::int64_t value = context.Args[0]->AsInteger();
	return context.Collector.FromValue(-value);
}

static ObjectInstance* integer_op_UnaryPositive(const CallState& context)
{
	std::int64_t value = context.Args[0]->AsInteger();
	return context.Collector.FromValue(value);
}

// -----------------------------------------------------------------------------
// Double operators
// -----------------------------------------------------------------------------

static ObjectInstance* double_op_AddOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left + right);
}

static ObjectInstance* double_op_SubOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left - right);
}

static ObjectInstance* double_op_MultOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left * right);
}

static ObjectInstance* double_op_DivOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left / right);
}

static ObjectInstance* double_op_ModOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(std::fmod(left, right));
}

static ObjectInstance* double_op_PowOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(std::pow(left, right));
}

static ObjectInstance* double_op_EqualsOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left == right);
}

static ObjectInstance* double_op_NotEqualsOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left != right);
}

static ObjectInstance* double_op_LessOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left < right);
}

static ObjectInstance* double_op_LessOrEqualsOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left <= right);
}

static ObjectInstance* double_op_GreaterOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left > right);
}

static ObjectInstance* double_op_GreaterOrEqualsOperator(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	double right = context.Args[1]->AsDouble();
	return context.Collector.FromValue(left >= right);
}

static ObjectInstance* double_op_UnaryNegation(const CallState& context)
{
	double value = context.Args[0]->AsDouble();
	return context.Collector.FromValue(-value);
}

static ObjectInstance* double_op_UnaryPositive(const CallState& context)
{
	double value = context.Args[0]->AsDouble();
	return context.Collector.FromValue(value);
}

// -----------------------------------------------------------------------------
// Boolean operators
// -----------------------------------------------------------------------------

static ObjectInstance* boolean_op_EqualsOperator(const CallState& context)
{
	bool left = context.Args[0]->AsBoolean();
	bool right = context.Args[1]->AsBoolean();
	return context.Collector.FromValue(left == right);
}

static ObjectInstance* boolean_op_NotEqualsOperator(const CallState& context)
{
	bool left = context.Args[0]->AsBoolean();
	bool right = context.Args[1]->AsBoolean();
	return context.Collector.FromValue(left != right);
}

static ObjectInstance* boolean_op_OrOperator(const CallState& context)
{
	bool left = context.Args[0]->AsBoolean();
	bool right = context.Args[1]->AsBoolean();
	return context.Collector.FromValue(left || right);
}

static ObjectInstance* boolean_op_AndOperator(const CallState& context)
{
	bool left = context.Args[0]->AsBoolean();
	bool right = context.Args[1]->AsBoolean();
	return context.Collector.FromValue(left && right);
}

static ObjectInstance* boolean_op_NotOperator(const CallState& context)
{
	bool value = context.Args[0]->AsBoolean();
	return context.Collector.FromValue(!value);
}

// -----------------------------------------------------------------------------
// Char operators
// -----------------------------------------------------------------------------

static ObjectInstance* char_op_AddOperator(const CallState& context)
{
	wchar_t left = context.Args[0]->AsCharacter();
	TypeSymbol* rightType = const_cast<TypeSymbol*>(context.Args[1]->getInfo());

	if (rightType == SymbolTable::Primitives::Char)
	{
		wchar_t right = context.Args[1]->AsCharacter();
		wchar_t* result = new wchar_t[] { left, right, L'\0' };
		return context.Collector.FromValue(result, false);
	}

	if (rightType == SymbolTable::Primitives::Integer)
	{
		std::int64_t right = context.Args[1]->AsInteger();
		wchar_t temp[] = { left, L'\0' };
		const wchar_t* result = concatStrings(temp, std::to_wstring(right).data());
		return context.Collector.FromValue(result, false);
	}

	if (rightType == SymbolTable::Primitives::String)
	{
		const wchar_t* right = context.Args[1]->AsString();
		wchar_t temp[] = { left, L'\0' };
		const wchar_t* result = concatStrings(temp, right);
		return context.Collector.FromValue(result, false);
	}

	throw std::runtime_error("unsupported char add operation");
}

static ObjectInstance* char_op_MultOperator(const CallState& context)
{
	wchar_t left = context.Args[0]->AsCharacter();
	std::int64_t right = context.Args[1]->AsInteger();

	if (right <= 0)
		return context.Collector.FromValue(L"");

	if (right == 1)
		return context.Collector.FromValue(left);

	std::size_t stringSize = sizeof(wchar_t);
	std::size_t totalSize = stringSize * right + sizeof(wchar_t);

	wchar_t* result = static_cast<wchar_t*>(malloc(totalSize));
	if (result == nullptr)
		throw std::runtime_error("Failed to allocate space for new string");

	for (std::int64_t i = 0; i < right; i++)
		std::memcpy(result + i, &left, sizeof(wchar_t));

	result[right] = L'\0';
	return context.Collector.FromValue(result, false);
}

static ObjectInstance* char_op_EqualsOperator(const CallState& context)
{
	wchar_t left = context.Args[0]->AsCharacter();
	wchar_t right = context.Args[1]->AsCharacter();
	return context.Collector.FromValue(left == right);
}

static ObjectInstance* char_op_NotEqualsOperator(const CallState& context)
{
	wchar_t left = context.Args[0]->AsCharacter();
	wchar_t right = context.Args[1]->AsCharacter();
	return context.Collector.FromValue(left != right);
}

static ObjectInstance* char_op_IncrementOperator(const CallState& context)
{
	wchar_t value = context.Args[0]->AsCharacter();
	return context.Collector.FromValue(static_cast<wchar_t>(value + 1));
}

static ObjectInstance* char_op_DecrementOperator(const CallState& context)
{
	wchar_t value = context.Args[0]->AsCharacter();
	return context.Collector.FromValue(static_cast<wchar_t>(value - 1));
}

// -----------------------------------------------------------------------------
// String operators
// -----------------------------------------------------------------------------

static ObjectInstance* string_op_AddOperator(const CallState& context)
{
	const wchar_t* left = context.Args[0]->AsString();
	TypeSymbol* rightType = const_cast<TypeSymbol*>(context.Args[1]->getInfo());

	if (rightType == SymbolTable::Primitives::String)
	{
		const wchar_t* right = context.Args[1]->AsString();
		const wchar_t* result = concatStrings(left, right);
		return context.Collector.FromValue(result, false);
	}

	if (rightType == SymbolTable::Primitives::Integer)
	{
		std::int64_t right = context.Args[1]->AsInteger();
		const wchar_t* result = concatStrings(left, std::to_wstring(right).data());
		return context.Collector.FromValue(result, false);
	}

	if (rightType == SymbolTable::Primitives::Boolean)
	{
		bool right = context.Args[1]->AsBoolean();
		const wchar_t* dataStr = right ? L"true" : L"false";
		const wchar_t* result = concatStrings(left, dataStr);
		return context.Collector.FromValue(result, false);
	}

	if (rightType == SymbolTable::Primitives::Char)
	{
		wchar_t right = context.Args[1]->AsCharacter();
		wchar_t temp[] = { right, L'\0' };
		const wchar_t* result = concatStrings(left, temp);
		return context.Collector.FromValue(result, false);
	}

	throw std::runtime_error("unsupported string add operation");
}

static ObjectInstance* string_op_MultOperator(const CallState& context)
{
	const wchar_t* left = context.Args[0]->AsString();
	std::int64_t right = context.Args[1]->AsInteger();

	if (right <= 0)
		return context.Collector.FromValue(L"");

	if (right == 1)
		return context.Collector.FromValue(left);

	std::size_t length = wcslen(left);
	std::size_t stringSize = length * sizeof(wchar_t);
	std::size_t totalSize = stringSize * right + sizeof(wchar_t);

	wchar_t* result = static_cast<wchar_t*>(malloc(totalSize));
	if (result == nullptr)
		throw std::runtime_error("Failed to allocate space for new string");

	for (std::int64_t i = 0; i < right; i++)
		std::memcpy(result + i * length, left, stringSize);

	result[length * right] = L'\0';
	return context.Collector.FromValue(result, false);
}

static ObjectInstance* string_op_EqualsOperator(const CallState& context)
{
	const wchar_t* left = context.Args[0]->AsString();
	const wchar_t* right = context.Args[1]->AsString();
	return context.Collector.FromValue(left == right);
}

static ObjectInstance* string_op_NotEqualsOperator(const CallState& context)
{
	const wchar_t* left = context.Args[0]->AsString();
	const wchar_t* right = context.Args[1]->AsString();
	return context.Collector.FromValue(left != right);
}

// -----------------------------------------------------------------------------
// Mixed-type operators (left operand decides owner type)
// -----------------------------------------------------------------------------

static ObjectInstance* integer_op_AddOperator_String(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	const wchar_t* right = context.Args[1]->AsString();
	const wchar_t* result = concatStrings(std::to_wstring(left).data(), right);
	return context.Collector.FromValue(result, false);
}

static ObjectInstance* integer_op_AddOperator_Char(const CallState& context)
{
	std::int64_t left = context.Args[0]->AsInteger();
	wchar_t right = context.Args[1]->AsCharacter();
	wchar_t temp[] = { right, L'\0' };
	const wchar_t* result = concatStrings(std::to_wstring(left).data(), temp);
	return context.Collector.FromValue(result, false);
}

static ObjectInstance* boolean_op_AddOperator_String(const CallState& context)
{
	bool left = context.Args[0]->AsBoolean();
	const wchar_t* right = context.Args[1]->AsString();
	const wchar_t* leftStr = left ? L"true" : L"false";
	const wchar_t* result = concatStrings(leftStr, right);
	return context.Collector.FromValue(result, false);
}

static ObjectInstance* double_op_AddOperator_String(const CallState& context)
{
	double left = context.Args[0]->AsDouble();
	const wchar_t* right = context.Args[1]->AsString();
	const wchar_t* result = concatStrings(std::to_wstring(left).data(), right);
	return context.Collector.FromValue(result, false);
}

// -----------------------------------------------------------------------------
// Registration helpers
// -----------------------------------------------------------------------------

static OperatorSymbol* RegisterOperator(TypeSymbol* type, TypeSymbol* returnType, const wchar_t* name, MethodSymbolDelegate callback, const std::vector<TypeSymbol*>& paramTypes, SymbolFactory& factory)
{
	shard::TokenType opToken = GetTokenTypeFromOperatorName(std::wstring(name));
	OperatorSymbol* op = factory.Operator(std::wstring(name), opToken, returnType, callback, paramTypes);
	type->OnSymbolDeclared(op);
	return op;
}

static void RegisterIntegerOperators(SymbolFactory& factory)
{
	TypeSymbol* type = SymbolTable::Primitives::Integer;
	TypeSymbol* retBool = SymbolTable::Primitives::Boolean;
	TypeSymbol* retInt = SymbolTable::Primitives::Integer;
	TypeSymbol* retStr = SymbolTable::Primitives::String;

	RegisterOperator(type, retInt, L"op_AddOperator", &integer_op_AddOperator, { type, type }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &integer_op_AddOperator_String, { type, retStr }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &integer_op_AddOperator_Char, { type, SymbolTable::Primitives::Char }, factory);

	RegisterOperator(type, retInt, L"op_SubOperator", &integer_op_SubOperator, { type, type }, factory);
	RegisterOperator(type, retInt, L"op_MultOperator", &integer_op_MultOperator, { type, type }, factory);
	RegisterOperator(type, retInt, L"op_DivOperator", &integer_op_DivOperator, { type, type }, factory);
	RegisterOperator(type, retInt, L"op_ModOperator", &integer_op_ModOperator, { type, type }, factory);
	RegisterOperator(type, retInt, L"op_PowOperator", &integer_op_PowOperator, { type, type }, factory);

	RegisterOperator(type, retInt, L"op_OrOperator", &integer_op_OrOperator, { type, type }, factory);
	RegisterOperator(type, retInt, L"op_AndOperator", &integer_op_AndOperator, { type, type }, factory);
	RegisterOperator(type, retInt, L"op_LeftShiftOperator", &integer_op_LeftShiftOperator, { type, type }, factory);
	RegisterOperator(type, retInt, L"op_RightShiftOperator", &integer_op_RightShiftOperator, { type, type }, factory);

	RegisterOperator(type, retBool, L"op_EqualsOperator", &integer_op_EqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_NotEqualsOperator", &integer_op_NotEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_LessOperator", &integer_op_LessOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_LessOrEqualsOperator", &integer_op_LessOrEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_GreaterOperator", &integer_op_GreaterOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_GreaterOrEqualsOperator", &integer_op_GreaterOrEqualsOperator, { type, type }, factory);

	RegisterOperator(type, retInt, L"op_IncrementOperator", &integer_op_IncrementOperator, { type }, factory);
	RegisterOperator(type, retInt, L"op_DecrementOperator", &integer_op_DecrementOperator, { type }, factory);
	RegisterOperator(type, retInt, L"op_SubOperator", &integer_op_UnaryNegation, { type }, factory);
	RegisterOperator(type, retInt, L"op_AddOperator", &integer_op_UnaryPositive, { type }, factory);
}

static void RegisterBooleanOperators(SymbolFactory& factory)
{
	TypeSymbol* type = SymbolTable::Primitives::Boolean;
	TypeSymbol* retBool = SymbolTable::Primitives::Boolean;
	TypeSymbol* retStr = SymbolTable::Primitives::String;

	RegisterOperator(type, retStr, L"op_AddOperator", &boolean_op_AddOperator_String, { type, retStr }, factory);

	RegisterOperator(type, retBool, L"op_EqualsOperator", &boolean_op_EqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_NotEqualsOperator", &boolean_op_NotEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_OrOperator", &boolean_op_OrOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_AndOperator", &boolean_op_AndOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_NotOperator", &boolean_op_NotOperator, { type }, factory);
}

static void RegisterCharOperators(SymbolFactory& factory)
{
	TypeSymbol* type = SymbolTable::Primitives::Char;
	TypeSymbol* retBool = SymbolTable::Primitives::Boolean;
	TypeSymbol* retStr = SymbolTable::Primitives::String;
	TypeSymbol* retChar = SymbolTable::Primitives::Char;
	TypeSymbol* retInt = SymbolTable::Primitives::Integer;

	RegisterOperator(type, retStr, L"op_AddOperator", &char_op_AddOperator, { type, type }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &char_op_AddOperator, { type, retInt }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &char_op_AddOperator, { type, retStr }, factory);
	RegisterOperator(type, retStr, L"op_MultOperator", &char_op_MultOperator, { type, retInt }, factory);
	RegisterOperator(type, retBool, L"op_EqualsOperator", &char_op_EqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_NotEqualsOperator", &char_op_NotEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retChar, L"op_IncrementOperator", &char_op_IncrementOperator, { type }, factory);
	RegisterOperator(type, retChar, L"op_DecrementOperator", &char_op_DecrementOperator, { type }, factory);
}

static void RegisterStringOperators(SymbolFactory& factory)
{
	TypeSymbol* type = SymbolTable::Primitives::String;
	TypeSymbol* retBool = SymbolTable::Primitives::Boolean;
	TypeSymbol* retStr = SymbolTable::Primitives::String;
	TypeSymbol* retInt = SymbolTable::Primitives::Integer;
	TypeSymbol* retChar = SymbolTable::Primitives::Char;

	RegisterOperator(type, retStr, L"op_AddOperator", &string_op_AddOperator, { type, type }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &string_op_AddOperator, { type, retInt }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &string_op_AddOperator, { type, retBool }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &string_op_AddOperator, { type, retChar }, factory);
	RegisterOperator(type, retStr, L"op_MultOperator", &string_op_MultOperator, { type, retInt }, factory);
	RegisterOperator(type, retBool, L"op_EqualsOperator", &string_op_EqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_NotEqualsOperator", &string_op_NotEqualsOperator, { type, type }, factory);
}

static void RegisterDoubleOperators(SymbolFactory& factory)
{
	TypeSymbol* type = SymbolTable::Primitives::Double;
	TypeSymbol* retBool = SymbolTable::Primitives::Boolean;
	TypeSymbol* retDbl = SymbolTable::Primitives::Double;
	TypeSymbol* retStr = SymbolTable::Primitives::String;

	RegisterOperator(type, retDbl, L"op_AddOperator", &double_op_AddOperator, { type, type }, factory);
	RegisterOperator(type, retStr, L"op_AddOperator", &double_op_AddOperator_String, { type, retStr }, factory);

	RegisterOperator(type, retDbl, L"op_SubOperator", &double_op_SubOperator, { type, type }, factory);
	RegisterOperator(type, retDbl, L"op_MultOperator", &double_op_MultOperator, { type, type }, factory);
	RegisterOperator(type, retDbl, L"op_DivOperator", &double_op_DivOperator, { type, type }, factory);
	RegisterOperator(type, retDbl, L"op_ModOperator", &double_op_ModOperator, { type, type }, factory);
	RegisterOperator(type, retDbl, L"op_PowOperator", &double_op_PowOperator, { type, type }, factory);

	RegisterOperator(type, retBool, L"op_EqualsOperator", &double_op_EqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_NotEqualsOperator", &double_op_NotEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_LessOperator", &double_op_LessOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_LessOrEqualsOperator", &double_op_LessOrEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_GreaterOperator", &double_op_GreaterOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_GreaterOrEqualsOperator", &double_op_GreaterOrEqualsOperator, { type, type }, factory);

	RegisterOperator(type, retDbl, L"op_SubOperator", &double_op_UnaryNegation, { type }, factory);
	RegisterOperator(type, retDbl, L"op_AddOperator", &double_op_UnaryPositive, { type }, factory);
}

// -----------------------------------------------------------------------------
// Byte operators
// -----------------------------------------------------------------------------

static ObjectInstance* byte_op_AddOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(static_cast<std::uint8_t>(left + right));
}

static ObjectInstance* byte_op_SubOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(static_cast<std::uint8_t>(left - right));
}

static ObjectInstance* byte_op_MultOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(static_cast<std::uint8_t>(left * right));
}

static ObjectInstance* byte_op_DivOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(static_cast<std::uint8_t>(left / right));
}

static ObjectInstance* byte_op_ModOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(static_cast<std::uint8_t>(left % right));
}

static ObjectInstance* byte_op_EqualsOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(left == right);
}

static ObjectInstance* byte_op_NotEqualsOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(left != right);
}

static ObjectInstance* byte_op_LessOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(left < right);
}

static ObjectInstance* byte_op_LessOrEqualsOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(left <= right);
}

static ObjectInstance* byte_op_GreaterOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(left > right);
}

static ObjectInstance* byte_op_GreaterOrEqualsOperator(const CallState& context)
{
	std::uint8_t left = context.Args[0]->AsByte();
	std::uint8_t right = context.Args[1]->AsByte();
	return context.Collector.FromValue(left >= right);
}

static ObjectInstance* byte_op_IncrementOperator(const CallState& context)
{
	std::uint8_t value = context.Args[0]->AsByte();
	return context.Collector.FromValue(static_cast<std::uint8_t>(value + 1));
}

static ObjectInstance* byte_op_DecrementOperator(const CallState& context)
{
	std::uint8_t value = context.Args[0]->AsByte();
	return context.Collector.FromValue(static_cast<std::uint8_t>(value - 1));
}

static void RegisterByteOperators(SymbolFactory& factory)
{
	TypeSymbol* type = SymbolTable::Primitives::Byte;
	TypeSymbol* retBool = SymbolTable::Primitives::Boolean;
	TypeSymbol* retByte = SymbolTable::Primitives::Byte;

	RegisterOperator(type, retByte, L"op_AddOperator", &byte_op_AddOperator, { type, type }, factory);
	RegisterOperator(type, retByte, L"op_SubOperator", &byte_op_SubOperator, { type, type }, factory);
	RegisterOperator(type, retByte, L"op_MultOperator", &byte_op_MultOperator, { type, type }, factory);
	RegisterOperator(type, retByte, L"op_DivOperator", &byte_op_DivOperator, { type, type }, factory);
	RegisterOperator(type, retByte, L"op_ModOperator", &byte_op_ModOperator, { type, type }, factory);

	RegisterOperator(type, retBool, L"op_EqualsOperator", &byte_op_EqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_NotEqualsOperator", &byte_op_NotEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_LessOperator", &byte_op_LessOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_LessOrEqualsOperator", &byte_op_LessOrEqualsOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_GreaterOperator", &byte_op_GreaterOperator, { type, type }, factory);
	RegisterOperator(type, retBool, L"op_GreaterOrEqualsOperator", &byte_op_GreaterOrEqualsOperator, { type, type }, factory);

	RegisterOperator(type, retByte, L"op_IncrementOperator", &byte_op_IncrementOperator, { type }, factory);
	RegisterOperator(type, retByte, L"op_DecrementOperator", &byte_op_DecrementOperator, { type }, factory);
}

void shard::RegisterPrimitiveOperators(SymbolFactory& factory)
{
	RegisterIntegerOperators(factory);
	RegisterBooleanOperators(factory);
	RegisterCharOperators(factory);
	RegisterStringOperators(factory);
	RegisterDoubleOperators(factory);
	RegisterByteOperators(factory);
}
