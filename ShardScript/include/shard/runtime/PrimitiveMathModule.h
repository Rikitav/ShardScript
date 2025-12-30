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
		static shard::ObjectInstance* EvaluateBinaryOperator(shard::ObjectInstance* leftInstance, shard::SyntaxToken opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(long leftData, shard::SyntaxToken opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(bool leftData, shard::SyntaxToken opToken, shard::ObjectInstance* rightInstance, bool& assign);
		static shard::ObjectInstance* EvaluateBinaryOperator(std::wstring& leftData, shard::SyntaxToken opToken, shard::ObjectInstance* rightInstance, bool& assign);

		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, shard::SyntaxToken opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, long data, shard::SyntaxToken opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, bool data, shard::SyntaxToken opToken, bool rightDetermined);
		static shard::ObjectInstance* EvaluateUnaryOperator(shard::ObjectInstance*& sourceInstance, std::wstring& data, shard::SyntaxToken opToken, bool rightDetermined);
	};
}
