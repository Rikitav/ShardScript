#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/runtime/ObjectInstance.h>

#include <string>

namespace shard
{
	class SHARD_API PrimitiveMathModule
	{
	public:
		static ObjectInstance* EvaluateBinaryOperator(ObjectInstance* leftInstance, TokenType opToken, ObjectInstance* rightInstance);
		static ObjectInstance* EvaluateBinaryOperator(int64_t leftData, TokenType opToken, ObjectInstance* rightInstance);
		static ObjectInstance* EvaluateBinaryOperator(bool leftData, TokenType opToken, ObjectInstance* rightInstance);
		static ObjectInstance* EvaluateBinaryOperator(wchar_t leftData, TokenType opToken, ObjectInstance* rightInstance);
		static ObjectInstance* EvaluateBinaryOperator(const wchar_t* leftData, TokenType opToken, ObjectInstance* rightInstance);

		static ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, TokenType opToken, bool rightDetermined);
		static ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, int64_t data, TokenType opToken, bool rightDetermined);
		static ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, bool data, TokenType opToken, bool rightDetermined);
		static ObjectInstance* EvaluateUnaryOperator(ObjectInstance* sourceInstance, const wchar_t* data, TokenType opToken, bool rightDetermined);
	};
}
