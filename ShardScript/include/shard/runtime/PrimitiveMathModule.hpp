#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/lexical/TokenType.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

namespace shard
{
	class GarbageCollector;
	class ObjectInstance;

	/// <summary>
	/// Inline implementation of primitive operators for the built-in engine types.
	/// <para>
	/// The VM calls this module before falling back to user-defined operator overloads.
	/// Returning <c>nullptr</c> means "not handled — use the normal overload resolution path".
	/// </para>
	/// </summary>
	class SHARD_API PrimitiveMathModule
	{
	public:
		PrimitiveMathModule(GarbageCollector& garbageCollector);

		ObjectInstance* ExecuteBinary(TokenType opToken, ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteUnary(TokenType opToken, ObjectInstance* operand) const;
		ObjectInstance* ExecuteCast(TypeSymbol* targetType, ObjectInstance* source) const;

	private:
		GarbageCollector& gc;

		static bool IsNumericType(TypeSymbol* type);
		static bool IsIntegralType(TypeSymbol* type);
		static bool IsPrimitiveType(TypeSymbol* type);

		static std::int64_t AsInteger(ObjectInstance* instance);
		static double AsDouble(ObjectInstance* instance);
		static bool AsBoolean(ObjectInstance* instance);
		static wchar_t AsCharacter(ObjectInstance* instance);

		ObjectInstance* FromInteger(std::int64_t value) const;
		ObjectInstance* FromDouble(double value) const;
		ObjectInstance* FromBoolean(bool value) const;
		ObjectInstance* FromCharacter(wchar_t value) const;

		std::wstring ToString(ObjectInstance* instance) const;

		ObjectInstance* ExecuteMathAddition(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteMathSubtraction(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteMathMultiplication(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteMathDivision(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteMathModulo(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteMathPower(ObjectInstance* left, ObjectInstance* right) const;

		ObjectInstance* ExecuteBitwiseOr(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteBitwiseAnd(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteShiftLeft(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteShiftRight(ObjectInstance* left, ObjectInstance* right) const;

		ObjectInstance* ExecuteCompareEqual(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteCompareNotEqual(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteCompareLess(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteCompareLessOrEqual(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteCompareGreater(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteCompareGreaterOrEqual(ObjectInstance* left, ObjectInstance* right) const;

		ObjectInstance* ExecuteLogicalNot(ObjectInstance* operand) const;
		ObjectInstance* ExecuteLogicalOr(ObjectInstance* left, ObjectInstance* right) const;
		ObjectInstance* ExecuteLogicalAnd(ObjectInstance* left, ObjectInstance* right) const;
	};
}
