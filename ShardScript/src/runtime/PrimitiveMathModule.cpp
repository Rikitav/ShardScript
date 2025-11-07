#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/runtime/PrimitiveMathModule.h>
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
		return GarbageCollector::CopyInstance(rightInstance);
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
				return ObjectInstance::FromValue(leftData == rightData);

			case TokenType::NotEqualsOperator:
				return ObjectInstance::FromValue(leftData != rightData);

			case TokenType::OrOperator:
				return ObjectInstance::FromValue(leftData || rightData);

			case TokenType::OrAssignOperator:
				assign = true;
				return ObjectInstance::FromValue(leftData || rightData);

			case TokenType::AndOperator:
				return ObjectInstance::FromValue(leftData & rightData);

			case TokenType::AndAssignOperator:
				assign = true;
				return ObjectInstance::FromValue(leftData & rightData);
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
				return ObjectInstance::FromValue(leftData + rightData);
			}

			case TokenType::SubOperator:
			case TokenType::SubAssignOperator:
			{
				assign = opToken.Type == TokenType::SubAssignOperator;
				return ObjectInstance::FromValue(leftData - rightData);
			}

			case TokenType::MultOperator:
			case TokenType::MultAssignOperator:
			{
				assign = opToken.Type == TokenType::MultAssignOperator;
				return ObjectInstance::FromValue(leftData * rightData);
			}

			case TokenType::DivOperator:
			case TokenType::DivAssignOperator:
			{
				assign = opToken.Type == TokenType::DivAssignOperator;
				return ObjectInstance::FromValue(leftData / rightData);
			}

			case TokenType::ModOperator:
			case TokenType::ModAssignOperator:
			{
				assign = opToken.Type == TokenType::ModAssignOperator;
				return ObjectInstance::FromValue(leftData % rightData);
			}

			case TokenType::PowOperator:
			case TokenType::PowAssignOperator:
			{
				assign = opToken.Type == TokenType::PowAssignOperator;
				return ObjectInstance::FromValue(static_cast<int>(pow(leftData, rightData)));
			}

			case TokenType::OrOperator:
			case TokenType::OrAssignOperator:
			{
				assign = opToken.Type == TokenType::OrAssignOperator;
				return ObjectInstance::FromValue(leftData | rightData);
			}

			case TokenType::AndOperator:
			case TokenType::AndAssignOperator:
			{
				assign = opToken.Type == TokenType::AndAssignOperator;
				return ObjectInstance::FromValue(leftData & rightData);
			}

			case TokenType::LessOperator:
				return ObjectInstance::FromValue(leftData < rightData);

			case TokenType::LessOrEqualsOperator:
				return ObjectInstance::FromValue(leftData <= rightData);

			case TokenType::GreaterOperator:
				return ObjectInstance::FromValue(leftData > rightData);

			case TokenType::GreaterOrEqualsOperator:
				return ObjectInstance::FromValue(leftData >= rightData);

			case TokenType::EqualsOperator:
				return ObjectInstance::FromValue(leftData == rightData);

			case TokenType::NotEqualsOperator:
				return ObjectInstance::FromValue(leftData != rightData);

			case TokenType::LeftShiftOperator:
				return ObjectInstance::FromValue(leftData << rightData);

			case TokenType::RightShiftOperator:
				return ObjectInstance::FromValue(leftData >> rightData);
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
				return ObjectInstance::FromValue(concat);
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
					return ObjectInstance::FromValue(L"");

				if (rightData == 1)
					return ObjectInstance::FromValue(leftData);

				std::wstring result;
				result.reserve(leftData.length() * rightData);

				for (int i = 0; i < rightData; ++i)
					result += leftData;

				assign = opToken.Type == TokenType::MultAssignOperator;
				return ObjectInstance::FromValue(result);
			}

			default:
				throw runtime_error("unsupported operation");
		}
	}

	throw runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, SyntaxToken opToken, bool rightDetermined)
{
	if (sourceInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool data = sourceInstance->ReadPrimitive<bool>();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}
	
	if (sourceInstance->Info == SymbolTable::Primitives::Integer)
	{
		int data = sourceInstance->ReadPrimitive<int>();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}
	
	if (sourceInstance->Info == SymbolTable::Primitives::String)
	{
		wstring data = sourceInstance->ReadPrimitive<wstring>();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}

	throw runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, int data, SyntaxToken opToken, bool rightDetermined)
{
	switch (opToken.Type)
	{
		case TokenType::IncrementOperator:
		{
			ObjectInstance* newInstance = ObjectInstance::FromValue(data + 1);
			sourceInstance = newInstance;
			return rightDetermined ? ObjectInstance::FromValue(data) : newInstance;
		}

		case TokenType::DecrementOperator:
		{
			ObjectInstance* newInstance = ObjectInstance::FromValue(data - 1);
			sourceInstance = newInstance;
			return rightDetermined ? ObjectInstance::FromValue(data) : newInstance;
		}

		default:
			throw runtime_error("unsupported operation");
	}
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, bool data, SyntaxToken opToken, bool rightDetermined)
{
	switch (opToken.Type)
	{
		case TokenType::NotOperator:
		{
			return ObjectInstance::FromValue(!data);
		}

		default:
			throw runtime_error("unsupported operation");
	}
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, wstring& data, SyntaxToken opToken, bool rightDetermined)
{
	throw runtime_error("unsupported operation");
}
