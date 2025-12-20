#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/runtime/ObjectInstance.h>

#include <string>

namespace shard::runtime
{
	class SHARD_API PrimitiveMathModule
	{
	public:
		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(shard::runtime::ObjectInstance* leftInstance, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);
		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(long leftData, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);
		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(bool leftData, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);
		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(std::wstring& leftData, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);

		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, shard::syntax::SyntaxToken opToken, bool rightDetermined);
		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, long data, shard::syntax::SyntaxToken opToken, bool rightDetermined);
		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, bool data, shard::syntax::SyntaxToken opToken, bool rightDetermined);
		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, std::wstring& data, shard::syntax::SyntaxToken opToken, bool rightDetermined);
	};
}
