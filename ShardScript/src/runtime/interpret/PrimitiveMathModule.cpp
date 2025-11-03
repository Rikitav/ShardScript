#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/runtime/interpreter/PrimitiveMathModule.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <stdexcept>
#include <cmath>
#include <string>
#include <source_location>

using namespace std;
using namespace shard::syntax;
using namespace shard::runtime;
using namespace shard::parsing;
using namespace shard::parsing::semantic;

ObjectInstance* PrimitiveMathModule::CreateInstanceFromValue(bool value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Boolean);
	instance->WritePrimitive(value);
	return instance;
}

ObjectInstance* PrimitiveMathModule::CreateInstanceFromValue(int value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
	instance->WritePrimitive(value);
	return instance;
}

ObjectInstance* PrimitiveMathModule::CreateInstanceFromValue(wchar_t value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Char);
	instance->WritePrimitive(value);
	return instance;
}

ObjectInstance* PrimitiveMathModule::CreateInstanceFromValue(wstring& value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String);
	instance->WritePrimitive<wstring>(value);
	return instance;
}

/*
bool PrimitiveMathModule::IsPrimitiveValuesExpressionEvaluatable(int leftTypeCode, TokenType type, int rightTypeCode)
{
	switch (leftTypeCode)
	{
		case TYPE_CODE_BOOLEAN:
		{
			switch (rightTypeCode)
			{
				case TYPE_CODE_BOOLEAN:
				{
					switch (type)
					{
						case TokenType::OrOperator:
						case TokenType::OrAssignOperator:
						case TokenType::AndOperator:
						case TokenType::AndAssignOperator:
							return true;

						default:
							return false;
					}
				}

				default:
					return false;
			}
		}

		case TYPE_CODE_INTEGER:
		{
			switch (rightTypeCode)
			{
				case TYPE_CODE_INTEGER:
				{
					switch (type)
					{
						case TokenType::EqualsOperator:
						case TokenType::NotEqualsOperator:
						case TokenType::LessOperator:
						case TokenType::LessOrEqualsOperator:
						case TokenType::GreaterOperator:
						case TokenType::GreaterOrEqualsOperator:

						case TokenType::OrOperator:
						case TokenType::AndOperator:
						case TokenType::LeftShiftOperator:
						case TokenType::RightShiftOperator:

						case TokenType::OrAssignOperator:
						case TokenType::AndAssignOperator:

						case TokenType::AddOperator:
						case TokenType::SubOperator:
						case TokenType::MultOperator:
						case TokenType::DivOperator:
						case TokenType::ModOperator:
						case TokenType::PowOperator:

						case TokenType::AddAssignOperator:
						case TokenType::SubAssignOperator:
						case TokenType::MultAssignOperator:
						case TokenType::DivAssignOperator:
						case TokenType::ModAssignOperator:
						case TokenType::PowAssignOperator:
							return true;

						default:
							return false;
					}
				}

				default:
					return false;
			}
		}

		case TYPE_CODE_STRING:
		{
			switch (rightTypeCode)
			{
				case TYPE_CODE_STRING:
				{
					switch (type)
					{
						case TokenType::AddOperator:
						case TokenType::AddAssignOperator:
							return true;
					
						default:
							return false;
					}
				}

				case TYPE_CODE_INTEGER:
				{
					switch (type)
					{
						case TokenType::MultOperator:
						case TokenType::MultAssignOperator:
							return true;

						default:
							return false;
					}
				}

				default:
					return false;
			}
		}
	}

	return false;
}
*/

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(ObjectInstance* leftInstance, SyntaxToken opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (opToken.Type == TokenType::AssignOperator)
	{
		assign = true;
		return rightInstance;
	}

	if (leftInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool leftData = leftInstance->ReadPrimitive<bool>();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	if (leftInstance->Info == SymbolTable::Primitives::Integer)
	{
		int leftData = leftInstance->ReadPrimitive<int>();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	if (leftInstance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t leftData = leftInstance->ReadPrimitive<wchar_t>();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	if (leftInstance->Info == SymbolTable::Primitives::String)
	{
		wstring leftData = leftInstance->ReadPrimitive<wstring>();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	throw runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(bool leftData, SyntaxToken opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (rightInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool rightData = rightInstance->ReadPrimitive<bool>();
		switch (opToken.Type)
		{
			case TokenType::EqualsOperator:
				return CreateInstanceFromValue(leftData == rightData);

			case TokenType::NotEqualsOperator:
				return CreateInstanceFromValue(leftData != rightData);

			case TokenType::OrOperator:
				return CreateInstanceFromValue(leftData || rightData);

			case TokenType::OrAssignOperator:
				assign = true;
				return CreateInstanceFromValue(leftData || rightData);

			case TokenType::AndOperator:
				return CreateInstanceFromValue(leftData & rightData);

			case TokenType::AndAssignOperator:
				assign = true;
				return CreateInstanceFromValue(leftData & rightData);
		}
	}

	throw runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(int leftData, SyntaxToken opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (rightInstance->Info == SymbolTable::Primitives::Integer)
	{
		int rightData = rightInstance->ReadPrimitive<int>();
		switch (opToken.Type)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken.Type == TokenType::AddAssignOperator;
				return CreateInstanceFromValue(leftData + rightData);
			}

			case TokenType::SubOperator:
			case TokenType::SubAssignOperator:
			{
				assign = opToken.Type == TokenType::SubAssignOperator;
				return CreateInstanceFromValue(leftData - rightData);
			}

			case TokenType::MultOperator:
			case TokenType::MultAssignOperator:
			{
				assign = opToken.Type == TokenType::MultAssignOperator;
				return CreateInstanceFromValue(leftData * rightData);
			}

			case TokenType::DivOperator:
			case TokenType::DivAssignOperator:
			{
				assign = opToken.Type == TokenType::DivAssignOperator;
				return CreateInstanceFromValue(leftData / rightData);
			}

			case TokenType::ModOperator:
			case TokenType::ModAssignOperator:
			{
				assign = opToken.Type == TokenType::ModAssignOperator;
				return CreateInstanceFromValue(leftData % rightData);
			}

			case TokenType::PowOperator:
			case TokenType::PowAssignOperator:
			{
				assign = opToken.Type == TokenType::PowAssignOperator;
				return CreateInstanceFromValue(static_cast<int>(pow(leftData, rightData)));
			}

			case TokenType::OrOperator:
			case TokenType::OrAssignOperator:
			{
				assign = opToken.Type == TokenType::OrAssignOperator;
				return CreateInstanceFromValue(leftData | rightData);
			}

			case TokenType::AndOperator:
			case TokenType::AndAssignOperator:
			{
				assign = opToken.Type == TokenType::AndAssignOperator;
				return CreateInstanceFromValue(leftData & rightData);
			}

			case TokenType::LessOperator:
				return CreateInstanceFromValue(leftData < rightData);

			case TokenType::LessOrEqualsOperator:
				return CreateInstanceFromValue(leftData <= rightData);

			case TokenType::GreaterOperator:
				return CreateInstanceFromValue(leftData > rightData);

			case TokenType::GreaterOrEqualsOperator:
				return CreateInstanceFromValue(leftData >= rightData);

			case TokenType::EqualsOperator:
				return CreateInstanceFromValue(leftData == rightData);

			case TokenType::NotEqualsOperator:
				return CreateInstanceFromValue(leftData != rightData);

			case TokenType::LeftShiftOperator:
				return CreateInstanceFromValue(leftData << rightData);

			case TokenType::RightShiftOperator:
				return CreateInstanceFromValue(leftData >> rightData);
		}
	}

	throw runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(wstring& leftData, SyntaxToken opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (rightInstance->Info == SymbolTable::Primitives::String)
	{
		wstring rightData = rightInstance->ReadPrimitive<wstring>();
		switch (opToken.Type)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken.Type == TokenType::AddAssignOperator;
				wstring concat = leftData + rightData;
				return CreateInstanceFromValue(concat);
			}

			default:
				throw runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Integer)
	{
		int rightData = rightInstance->ReadPrimitive<int>();
		switch (opToken.Type)
		{
			case TokenType::MultOperator:
			case TokenType::MultAssignOperator:
			{
				if (rightData <= 0)
					return CreateInstanceFromValue(L"");

				if (rightData == 1)
					return CreateInstanceFromValue(leftData);

				std::wstring result;
				result.reserve(leftData.length() * rightData);

				for (int i = 0; i < rightData; ++i)
					result += leftData;

				assign = opToken.Type == TokenType::MultAssignOperator;
				return CreateInstanceFromValue(result);
			}

			default:
				throw runtime_error("unsupported operation");
		}
	}

	throw runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* instance, SyntaxToken opToken, bool rightDetermined)
{
	if (instance->Info == SymbolTable::Primitives::Boolean)
	{
		bool data = instance->ReadPrimitive<bool>();
		return EvaluateUnaryOperator(instance, data, opToken, rightDetermined);
	}

	if (instance->Info == SymbolTable::Primitives::Integer)
	{
		int data = instance->ReadPrimitive<int>();
		return EvaluateUnaryOperator(instance, data, opToken, rightDetermined);
	}

	if (instance->Info == SymbolTable::Primitives::String)
	{
		wstring data = instance->ReadPrimitive<wstring>();
		return EvaluateUnaryOperator(instance, data, opToken, rightDetermined);
	}

	throw runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* sourceInstance, int data, SyntaxToken opToken, bool rightDetermined)
{
	switch (opToken.Type)
	{
		case TokenType::IncrementOperator:
		{
			ObjectInstance* newInstance = CreateInstanceFromValue(data + 1);
			newInstance->CopyTo(sourceInstance);
			return rightDetermined ? CreateInstanceFromValue(data) : newInstance;
		}

		case TokenType::DecrementOperator:
		{
			ObjectInstance* newInstance = CreateInstanceFromValue(data - 1);
			newInstance->CopyTo(sourceInstance);
			return rightDetermined ? CreateInstanceFromValue(data) : newInstance;
		}

		default:
			throw runtime_error("unsupported operation");
	}
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* sourceInstance, bool data, SyntaxToken opToken, bool rightDetermined)
{
	switch (opToken.Type)
	{
		case TokenType::NotOperator:
		{
			return CreateInstanceFromValue(!data);
		}

		default:
			throw runtime_error("unsupported operation");
	}
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* sourceInstance, wstring& data, SyntaxToken opToken, bool rightDetermined)
{
	switch (opToken.Type)
	{
		case TokenType::NotOperator:
		{
			return CreateInstanceFromValue(data.size() == 0);
		}

		default:
			throw runtime_error("unsupported operation");
	}
}
