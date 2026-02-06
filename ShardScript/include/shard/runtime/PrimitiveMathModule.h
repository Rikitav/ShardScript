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
		static shard::ObjectInstance* EvaluateBinaryOperator(shard::ObjectInstance* leftInstance, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(int64_t leftData, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(bool leftData, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(std::wstring& leftData, TokenType opToken, shard::ObjectInstance* rightInstance, bool& assign);

		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, TokenType opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, int64_t data, TokenType opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, bool data, TokenType opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, std::wstring& data, TokenType opToken, bool rightDetermined);
	};
}
