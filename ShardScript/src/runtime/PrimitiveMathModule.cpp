#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/runtime/PrimitiveMathModule.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <stdexcept>
#include <cmath>
#include <string>

using namespace shard;

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(ObjectInstance* leftInstance, TokenType opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (opToken == TokenType::AssignOperator)
	{
		assign = true;
		return GarbageCollector::CopyInstance(rightInstance);
	}

	if (leftInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool leftData = leftInstance->AsBoolean();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	if (leftInstance->Info == SymbolTable::Primitives::Integer)
	{
		int64_t leftData = leftInstance->AsInteger();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	/*
	if (leftInstance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t leftData = leftInstance->AsCharacter();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}
	*/

	if (leftInstance->Info == SymbolTable::Primitives::String)
	{
		std::wstring leftData = leftInstance->AsString();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(bool leftData, TokenType opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (rightInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool rightData = rightInstance->AsBoolean();
		switch (opToken)
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
				return ObjectInstance::FromValue(leftData && rightData);

			case TokenType::AndAssignOperator:
				assign = true;
				return ObjectInstance::FromValue(leftData && rightData);
		}
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(int64_t leftData, TokenType opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (rightInstance->Info == SymbolTable::Primitives::Integer)
	{
		int64_t rightData = rightInstance->AsInteger();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken == TokenType::AddAssignOperator;
				return ObjectInstance::FromValue(leftData + rightData);
			}

			case TokenType::SubOperator:
			case TokenType::SubAssignOperator:
			{
				assign = opToken == TokenType::SubAssignOperator;
				return ObjectInstance::FromValue(leftData - rightData);
			}

			case TokenType::MultOperator:
			case TokenType::MultAssignOperator:
			{
				assign = opToken == TokenType::MultAssignOperator;
				return ObjectInstance::FromValue(leftData * rightData);
			}

			case TokenType::DivOperator:
			case TokenType::DivAssignOperator:
			{
				assign = opToken == TokenType::DivAssignOperator;
				return ObjectInstance::FromValue(leftData / rightData);
			}

			case TokenType::ModOperator:
			case TokenType::ModAssignOperator:
			{
				assign = opToken == TokenType::ModAssignOperator;
				return ObjectInstance::FromValue(leftData % rightData);
			}

			case TokenType::PowOperator:
			case TokenType::PowAssignOperator:
			{
				assign = opToken == TokenType::PowAssignOperator;
				return ObjectInstance::FromValue(static_cast<int64_t>(pow(leftData, rightData)));
			}

			case TokenType::OrOperator:
			case TokenType::OrAssignOperator:
			{
				assign = opToken == TokenType::OrAssignOperator;
				return ObjectInstance::FromValue(leftData | rightData);
			}

			case TokenType::AndOperator:
			case TokenType::AndAssignOperator:
			{
				assign = opToken == TokenType::AndAssignOperator;
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

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(std::wstring& leftData, TokenType opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (rightInstance->Info == SymbolTable::Primitives::String)
	{
		std::wstring rightData = rightInstance->AsString();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken == TokenType::AddAssignOperator;
				std::wstring* concat = new std::wstring(leftData + rightData);
				return ObjectInstance::FromValue(*concat);
			}

			case TokenType::EqualsOperator:
				return ObjectInstance::FromValue(leftData == rightData);

			case TokenType::NotEqualsOperator:
				return ObjectInstance::FromValue(leftData != rightData);

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Integer)
	{
		int64_t rightData = rightInstance->AsInteger();
		switch (opToken)
		{
			case TokenType::MultOperator:
			case TokenType::MultAssignOperator:
			{
				if (rightData <= 0)
					return ObjectInstance::FromValue(L"");

				if (rightData == 1)
					return ObjectInstance::FromValue(leftData);

				std::wstring* result = new std::wstring();
				result->reserve(leftData.length() * rightData);

				for (int64_t i = 0; i < rightData; i++)
					*result += leftData;

				assign = opToken == TokenType::MultAssignOperator;
				return ObjectInstance::FromValue(result);
			}

			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken == TokenType::AddAssignOperator;
				std::wstring* concat = new std::wstring(leftData + std::to_wstring(rightData));
				return ObjectInstance::FromValue(*concat);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool rightData = rightInstance->AsBoolean();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken == TokenType::AddAssignOperator;
				std::wstring* concat = new std::wstring(leftData + (rightData ? L"true" : L"false"));
				return ObjectInstance::FromValue(*concat);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Boolean)
	{
		wchar_t rightData = rightInstance->AsCharacter();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken == TokenType::AddAssignOperator;
				std::wstring* concat = new std::wstring(leftData + rightData);
				return ObjectInstance::FromValue(*concat);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, TokenType opToken, bool rightDetermined)
{
	if (sourceInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool data = sourceInstance->AsBoolean();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}
	
	if (sourceInstance->Info == SymbolTable::Primitives::Integer)
	{
		int64_t data = sourceInstance->AsInteger();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}
	
	if (sourceInstance->Info == SymbolTable::Primitives::String)
	{
		std::wstring data = sourceInstance->AsString();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, int64_t data, TokenType opToken, bool rightDetermined)
{
	switch (opToken)
	{
		case TokenType::IncrementOperator:
		{
			int64_t newValue = data + 1;
			sourceInstance->WriteInteger(newValue);
			return rightDetermined ? ObjectInstance::FromValue(data) : sourceInstance;
		}

		case TokenType::DecrementOperator:
		{
			int64_t newValue = data + 1;
			sourceInstance->WriteInteger(newValue);
			return rightDetermined ? ObjectInstance::FromValue(data) : sourceInstance;
		}

		case TokenType::SubOperator:
		{
			return ObjectInstance::FromValue(data < 0 ? data : data * -1);
		}

		case TokenType::AddOperator:
		{
			return ObjectInstance::FromValue(data > 0 ? data : data * -1);
		}

		default:
			throw std::runtime_error("unsupported operation");
	}
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, bool data, TokenType opToken, bool rightDetermined)
{
	switch (opToken)
	{
		case TokenType::NotOperator:
		{
			return ObjectInstance::FromValue(!data);
		}

		default:
			throw std::runtime_error("unsupported operation");
	}
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, std::wstring& data, TokenType opToken, bool rightDetermined)
{
	throw std::runtime_error("unsupported operation");
}
