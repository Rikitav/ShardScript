#pragma once
#include <shard/syntax/SyntaxToken.h>
#include <shard/runtime/ObjectInstance.h>
#include <string>

namespace shard::runtime
{
	class PrimitiveMathModule
	{
	public:
		static shard::runtime::ObjectInstance* CreateInstanceFromValue(int value);
		static shard::runtime::ObjectInstance* CreateInstanceFromValue(bool value);
		static shard::runtime::ObjectInstance* CreateInstanceFromValue(wchar_t value);
		static shard::runtime::ObjectInstance* CreateInstanceFromValue(const wchar_t* value);
		static shard::runtime::ObjectInstance* CreateInstanceFromValue(std::wstring& value);

		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(shard::runtime::ObjectInstance* leftInstance, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);
		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(int leftData, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);
		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(bool leftData, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);
		static shard::runtime::ObjectInstance* EvaluateBinaryOperator(std::wstring& leftData, shard::syntax::SyntaxToken opToken, shard::runtime::ObjectInstance* rightInstance, bool& assign);

		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, shard::syntax::SyntaxToken opToken, bool rightDetermined);
		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, int data, shard::syntax::SyntaxToken opToken, bool rightDetermined);
		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, bool data, shard::syntax::SyntaxToken opToken, bool rightDetermined);
		static shard::runtime::ObjectInstance* EvaluateUnaryOperator(shard::runtime::ObjectInstance*& sourceInstance, std::wstring& data, shard::syntax::SyntaxToken opToken, bool rightDetermined);
	};
}
