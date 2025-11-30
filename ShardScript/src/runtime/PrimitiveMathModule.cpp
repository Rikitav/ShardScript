#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/runtime/PrimitiveMathModule.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <stdexcept>
#include <cmath>
#include <string>

using namespace shard::syntax;
using namespace shard::runtime;
using namespace shard::parsing;
using namespace shard::parsing::semantic;

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
		std::wstring leftData = leftInstance->ReadPrimitive<std::wstring>();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance, assign);
	}

	throw std::runtime_error("unknown primitive");
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
				return ObjectInstance::FromValue(leftData && rightData);

			case TokenType::AndAssignOperator:
				assign = true;
				return ObjectInstance::FromValue(leftData & rightData);
		}
	}

	throw std::runtime_error("unknown primitive");
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

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(std::wstring& leftData, SyntaxToken opToken, ObjectInstance* rightInstance, bool& assign)
{
	if (rightInstance->Info == SymbolTable::Primitives::String)
	{
		std::wstring rightData = rightInstance->ReadPrimitive<std::wstring>();
		switch (opToken.Type)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken.Type == TokenType::AddAssignOperator;
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

				std::wstring* result = new std::wstring();
				result->reserve(leftData.length() * rightData);

				for (int i = 0; i < rightData; ++i)
					*result += leftData;

				assign = opToken.Type == TokenType::MultAssignOperator;
				return ObjectInstance::FromValue(result);
			}

			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken.Type == TokenType::AddAssignOperator;
				std::wstring* concat = new std::wstring(leftData + std::to_wstring(rightData));
				return ObjectInstance::FromValue(*concat);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool rightData = rightInstance->ReadPrimitive<bool>();
		switch (opToken.Type)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken.Type == TokenType::AddAssignOperator;
				std::wstring* concat = new std::wstring(leftData + (rightData ? L"true" : L"false"));
				return ObjectInstance::FromValue(*concat);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Boolean)
	{
		wchar_t rightData = rightInstance->ReadPrimitive<wchar_t>();
		switch (opToken.Type)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				assign = opToken.Type == TokenType::AddAssignOperator;
				std::wstring* concat = new std::wstring(leftData + rightData);
				return ObjectInstance::FromValue(*concat);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	throw std::runtime_error("unknown primitive");
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
		std::wstring data = sourceInstance->ReadPrimitive<std::wstring>();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}

	throw std::runtime_error("unknown primitive");
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

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, bool data, SyntaxToken opToken, bool rightDetermined)
{
	switch (opToken.Type)
	{
		case TokenType::NotOperator:
		{
			return ObjectInstance::FromValue(!data);
		}

		default:
			throw std::runtime_error("unsupported operation");
	}
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance*& sourceInstance, std::wstring& data, SyntaxToken opToken, bool rightDetermined)
{
	throw std::runtime_error("unsupported operation");
}
