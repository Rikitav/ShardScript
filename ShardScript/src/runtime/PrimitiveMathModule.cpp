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

static const wchar_t* concatStrings(const wchar_t* left, const wchar_t* right)
{
	if (left == nullptr || right == nullptr)
		throw std::invalid_argument("Input strings cannot be null");

	size_t leftLen = wcslen(left);
	size_t rightLen = wcslen(right);

	size_t leftSize = leftLen * sizeof(wchar_t);
	size_t rightSize = rightLen * sizeof(wchar_t);
	size_t totalSize = leftSize + rightSize + sizeof(wchar_t); // + L'\0'

	wchar_t* result = static_cast<wchar_t*>(malloc(totalSize));
	if (result == nullptr)
		throw std::runtime_error("Failed to allocate space for new string");

	memcpy(result, left, leftSize);
	memcpy(result + leftLen, right, rightSize + sizeof(wchar_t)); // + L'\0'

	return result;
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(ObjectInstance* leftInstance, TokenType opToken, ObjectInstance* rightInstance)
{
	if (leftInstance->Info == SymbolTable::Primitives::Boolean)
	{
		bool leftData = leftInstance->AsBoolean();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance);
	}

	if (leftInstance->Info == SymbolTable::Primitives::Integer)
	{
		int64_t leftData = leftInstance->AsInteger();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance);
	}

	if (leftInstance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t leftData = leftInstance->AsCharacter();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance);
	}

	if (leftInstance->Info == SymbolTable::Primitives::String)
	{
		const wchar_t* leftData = leftInstance->AsString();
		return EvaluateBinaryOperator(leftData, opToken, rightInstance);
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(bool leftData, TokenType opToken, ObjectInstance* rightInstance)
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
				return ObjectInstance::FromValue(leftData || rightData);

			case TokenType::AndOperator:
				return ObjectInstance::FromValue(leftData && rightData);

			case TokenType::AndAssignOperator:
				return ObjectInstance::FromValue(leftData && rightData);
		}
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(int64_t leftData, TokenType opToken, ObjectInstance* rightInstance)
{
	if (rightInstance->Info == SymbolTable::Primitives::Integer)
	{
		int64_t rightData = rightInstance->AsInteger();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
				return ObjectInstance::FromValue(leftData + rightData);

			case TokenType::SubOperator:
			case TokenType::SubAssignOperator:
				return ObjectInstance::FromValue(leftData - rightData);

			case TokenType::MultOperator:
			case TokenType::MultAssignOperator:
				return ObjectInstance::FromValue(leftData * rightData);

			case TokenType::DivOperator:
			case TokenType::DivAssignOperator:
				return ObjectInstance::FromValue(leftData / rightData);

			case TokenType::ModOperator:
			case TokenType::ModAssignOperator:
				return ObjectInstance::FromValue(leftData % rightData);

			case TokenType::PowOperator:
			case TokenType::PowAssignOperator:
				return ObjectInstance::FromValue(static_cast<int64_t>(pow(leftData, rightData)));

			case TokenType::OrOperator:
			case TokenType::OrAssignOperator:
				return ObjectInstance::FromValue(leftData | rightData);

			case TokenType::AndOperator:
			case TokenType::AndAssignOperator:
				return ObjectInstance::FromValue(leftData & rightData);

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

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(wchar_t leftData, TokenType opToken, ObjectInstance* rightInstance)
{
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

				size_t stringSize = rightData * sizeof(wchar_t);
				size_t totalSize = stringSize * rightData + sizeof(wchar_t);

				wchar_t* result = static_cast<wchar_t*>(malloc(totalSize));
				if (result == nullptr)
					throw std::runtime_error("Failed to allocate space for new string");

				for (int64_t i = 0; i < rightData; i++)
					memcpy(result + i, &leftData, sizeof(wchar_t));

				result[leftData * rightData] = L'\0';
				return ObjectInstance::FromValue(result, false);
			}

			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				wchar_t temp[] = { rightData, L'\0' };
				std::wstring asStr = std::to_wstring(rightData);

				const wchar_t* result = concatStrings(temp, asStr.data());
				return ObjectInstance::FromValue(result, false);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t rightData = rightInstance->AsCharacter();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				wchar_t* result = new wchar_t[] { leftData, rightData, L'\0' };
				return ObjectInstance::FromValue(result, false);
			}

			case TokenType::EqualsOperator:
				return ObjectInstance::FromValue(leftData == rightData);

			case TokenType::NotEqualsOperator:
				return ObjectInstance::FromValue(leftData != rightData);

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::String)
	{
		const wchar_t* rightData = rightInstance->AsString();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				wchar_t temp[] = { leftData, L'\0' };
				const wchar_t* str = concatStrings(temp, rightData);
				return ObjectInstance::FromValue(str, false);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateBinaryOperator(const wchar_t* leftData, TokenType opToken, ObjectInstance* rightInstance)
{
	if (rightInstance->Info == SymbolTable::Primitives::String)
	{
		const wchar_t* rightData = rightInstance->AsString();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				const wchar_t* str = concatStrings(leftData, rightData);
				return ObjectInstance::FromValue(str, false);
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

				size_t length = wcslen(leftData);
				size_t stringSize = length * sizeof(wchar_t);
				size_t totalSize = stringSize * rightData + sizeof(wchar_t);

				wchar_t* result = static_cast<wchar_t*>(malloc(totalSize));
				if (result == nullptr)
					throw std::runtime_error("Failed to allocate space for new string");

				for (int64_t i = 0; i < rightData; i++)
					memcpy(result + i * length, leftData, stringSize);

				result[length * rightData] = L'\0';
				return ObjectInstance::FromValue(result, false);
			}

			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				const wchar_t* result = concatStrings(leftData, std::to_wstring(rightData).data());
				return ObjectInstance::FromValue(result, false);
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
				const wchar_t* dataStr = rightData ? L"true" : L"false";
				const wchar_t* concat = concatStrings(leftData, dataStr);
				return ObjectInstance::FromValue(concat, false);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	if (rightInstance->Info == SymbolTable::Primitives::Char)
	{
		wchar_t rightData = rightInstance->AsCharacter();
		switch (opToken)
		{
			case TokenType::AddOperator:
			case TokenType::AddAssignOperator:
			{
				wchar_t temp[] = { rightData, L'\0' };
				const wchar_t* concat = concatStrings(leftData, temp);
				return ObjectInstance::FromValue(concat, false);
			}

			default:
				throw std::runtime_error("unsupported operation");
		}
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* sourceInstance, TokenType opToken, bool rightDetermined)
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
		const wchar_t* data = sourceInstance->AsString();
		return EvaluateUnaryOperator(sourceInstance, data, opToken, rightDetermined);
	}

	throw std::runtime_error("unknown primitive");
}

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* sourceInstance, int64_t data, TokenType opToken, bool rightDetermined)
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

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* sourceInstance, bool data, TokenType opToken, bool rightDetermined)
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

ObjectInstance* PrimitiveMathModule::EvaluateUnaryOperator(ObjectInstance* sourceInstance, const wchar_t* data, TokenType opToken, bool rightDetermined)
{
	throw std::runtime_error("unsupported operation");
}
