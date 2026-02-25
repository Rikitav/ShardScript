#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxToken.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <string>

namespace shard
{
	class SHARD_API PrimitiveMathModule
	{
		GarbageCollector& garbageCollector;

	public:
		PrimitiveMathModule(GarbageCollector& gc);

		ObjectInstance* EvaluateBinaryOperator(ObjectInstance* leftInstance, TokenType opToken, ObjectInstance* rightInstance);
		ObjectInstance* EvaluateBinaryOperator(int64_t leftData, TokenType opToken, ObjectInstance* rightInstance);
		ObjectInstance* EvaluateBinaryOperator(bool leftData, TokenType opToken, ObjectInstance* rightInstance);
		ObjectInstance* EvaluateBinaryOperator(wchar_t leftData, TokenType opToken, ObjectInstance* rightInstance);
		ObjectInstance* EvaluateBinaryOperator(const wchar_t* leftData, TokenType opToken, ObjectInstance* rightInstance);

		ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, TokenType opToken, bool rightDetermined);
		ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, int64_t data, TokenType opToken, bool rightDetermined);
		ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, bool data, TokenType opToken, bool rightDetermined);
		ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, const wchar_t* data, TokenType opToken, bool rightDetermined);
	};
}
