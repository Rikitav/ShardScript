#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/compilation/ProgramDisassembler.hpp>

#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <shard/lexical/TokenType.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/parsing/SyntaxFacts.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <shard/ApplicationDomain.hpp>

#include <cstring>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <exception>

using namespace shard;

void VirtualMachine::ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode)
{
	auto executeBinary = [&](TokenType token) -> void
	{
		ObjectInstance* right = frame->PopStack();
		ObjectInstance* left = frame->PopStack();

		if (right == nullptr || left == nullptr)
			throw std::runtime_error("Cannot perform operation on nullptr instance");

		if (right == GarbageCollector::NullInstance || left == GarbageCollector::NullInstance)
		{
			ObjectInstance* result = garbageCollector.FromValue(right == left);
			frame->PushStack(result);
			return;
		}

		ObjectInstance* result = primitiveMath.ExecuteBinary(token, left, right);
		if (result == nullptr)
			result = InvokeOperatorMethod(left, token, right);

		frame->PushStack(result);
		garbageCollector.CollectInstance(right);
		garbageCollector.CollectInstance(left);
	};

	auto executeUnary = [&](TokenType token) -> void
	{
		ObjectInstance* operand = frame->PopStack();

		if (operand == nullptr)
			throw std::runtime_error("Cannot perform operation on nullptr instance");

		if (operand == GarbageCollector::NullInstance)
			throw std::runtime_error("Cannot perform operation on null instance");

		ObjectInstance* result = primitiveMath.ExecuteUnary(token, operand);
		if (result == nullptr)
			result = InvokeOperatorMethod(operand, token);

		frame->PushStack(result);
		garbageCollector.CollectInstance(operand);
	};

	switch (opCode)
	{
		case OpCode::NOP:
		{
			// 0xBAD + 0xC0DE;
			break;
		}

		case OpCode::HALT:
		{
			AbortFlag = true;
			break;
		}

		case OpCode::POPSTACK:
		{
			ObjectInstance* pop = frame->PopStack();
			garbageCollector.CollectInstance(pop);
			break;
		}

		case OpCode::CALLMETHODSYMBOL:
		{
			MethodSymbol* methodSymbol = decoder.AbsorbMethodSymbol();
			PendingTypeArguments.clear();
			InvokeMethod(methodSymbol);
			break;
		}

		case OpCode::CALLGENERICMETHOD:
		{
			MethodSymbol* methodSymbol = decoder.AbsorbMethodSymbol();
			InvokeMethod(methodSymbol);
			break;
		}

		case OpCode::CALLDELEGATE:
		{
			ObjectInstance* delegateInstance = frame->PopStack();
			if (delegateInstance == nullptr || delegateInstance == garbageCollector.NullInstance)
				throw std::runtime_error("Cannot invoke a null delegate");

			MethodSymbol* target = delegateInstance->DelegateTarget;
			if (target == nullptr)
				throw std::runtime_error("Delegate has no target method");

			InvokeMethod(target);
			break;
		}

		case OpCode::CALLINTERFACE:
		{
			MethodSymbol* interfaceMethod = decoder.AbsorbMethodSymbol();
			ObjectInstance* thisInstance = frame->PeekStack();

			if (thisInstance == nullptr || thisInstance == garbageCollector.NullInstance)
				throw std::runtime_error("Cannot invoke interface method on a null reference");

			TypeSymbol* concreteType = const_cast<TypeSymbol*>(thisInstance->getInfo());
			MethodSymbol* implementation = concreteType->FindInterfaceImplementation(interfaceMethod);

			if (implementation == nullptr)
				throw std::runtime_error("Interface method implementation not found");

			InvokeMethod(implementation);
			break;
		}

		case OpCode::ISINSTANCE:
		{
			TypeSymbol* targetType = decoder.AbsorbTypeSymbol();
			ObjectInstance* instance = frame->PopStack();

			bool result = false;
			if (instance != nullptr && instance != garbageCollector.NullInstance)
			{
				TypeSymbol* instanceType = const_cast<TypeSymbol*>(instance->getInfo());
				result = SemanticModel::IsAssignableTo(targetType, instanceType);
				garbageCollector.CollectInstance(instance);
			}

			frame->PushStack(garbageCollector.FromValue(result));
			break;
		}

		case OpCode::CASTINTERFACE:
		{
			TypeSymbol* targetType = decoder.AbsorbTypeSymbol();
			targetType = frame->ResolveType(targetType);
			ObjectInstance* instance = frame->PopStack();

			bool compatible = false;
			if (instance != nullptr && instance != garbageCollector.NullInstance)
			{
				TypeSymbol* instanceType = const_cast<TypeSymbol*>(instance->getInfo());
				compatible = SemanticModel::IsAssignableTo(targetType, instanceType);
			}

			if (compatible)
			{
				frame->PushStack(instance);
			}
			else
			{
				frame->PushStack(garbageCollector.NullInstance);
				garbageCollector.CollectInstance(instance);
			}

			break;
		}

		case OpCode::CAST:
		{
			TypeSymbol* targetType = decoder.AbsorbTypeSymbol();
			targetType = frame->ResolveType(targetType);
			ObjectInstance* instance = frame->PopStack();

			if (targetType == SymbolTable::Primitives::Any)
			{
				frame->PushStack(instance != nullptr ? instance : garbageCollector.NullInstance);
				break;
			}

			if (instance == nullptr || instance == garbageCollector.NullInstance)
			{
				if (targetType->Inlining == TypeInlining::ByValue)
					throw std::runtime_error("Cannot cast null to a value type");

				frame->PushStack(garbageCollector.NullInstance);
				break;
			}

			TypeSymbol* instanceType = const_cast<TypeSymbol*>(instance->getInfo());
			if (SemanticModel::IsAssignableTo(targetType, instanceType))
			{
				frame->PushStack(instance);
			}
			else
			{
				throw std::runtime_error("Invalid cast");
			}

			break;
		}

		case OpCode::CASTPRIMITIVE:
		{
			TypeSymbol* targetType = decoder.AbsorbTypeSymbol();
			targetType = frame->ResolveType(targetType);
			ObjectInstance* instance = frame->PopStack();

			if (instance == nullptr || instance == garbageCollector.NullInstance)
				throw std::runtime_error("Cannot cast null to a primitive type");

			ObjectInstance* result = primitiveMath.ExecuteCast(targetType, instance);
			if (result == nullptr)
				throw std::runtime_error("Unsupported primitive cast");

			garbageCollector.CollectInstance(instance);
			frame->PushStack(result);
			break;
		}

		case OpCode::LOADCONST_NULL:
		{
			frame->PushStack(garbageCollector.NullInstance);
			break;
		}

		case OpCode::LOADCONST_BOOLEAN:
		{
			bool value = decoder.AbsorbBoolean();
			ObjectInstance* instance = garbageCollector.FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LOADCONST_INTEGER64:
		{
			std::int64_t value = decoder.AbsorbInt64();
			ObjectInstance* instance = garbageCollector.FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LOADCONST_RATIONAL64:
		{
			double value = decoder.AbsorbDouble64();
			ObjectInstance* instance = garbageCollector.FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LOADCONST_CHAR:
		{
			wchar_t value = decoder.AbsorbChar16();
			ObjectInstance* instance = garbageCollector.FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LOADCONST_STRING:
		{
			std::size_t data = decoder.AbsorbString();
			const wchar_t* str = reinterpret_cast<wchar_t*>(program.DataSection.data() + data);

			ObjectInstance* instance = garbageCollector.FromValue(str, true);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LOADVARIABLE:
		{
			std::uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance* instance = frame->EvalStack[slot];
			frame->PushStack(instance);
			break;
		}

		case OpCode::STOREVARIABLE:
		{
			std::uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance* instance = frame->PopStack();

			if (slot >= frame->EvalStack.size())
			{
				frame->EvalStack.resize(slot + 1, nullptr);
			}

			ObjectInstance* oldVar = frame->EvalStack[slot];
			if (oldVar != nullptr)
			{
				oldVar->DecrementReference();
				garbageCollector.CollectInstance(oldVar);
			}

			instance->IncrementReference();
			frame->EvalStack[slot] = instance;
			break;
		}

		case OpCode::NEWOBJECT:
		{
			TypeSymbol* type = decoder.AbsorbTypeSymbol();
			type = frame->ResolveType(type);
			ConstructorSymbol* ctor = decoder.AbsorbConstructorSymbol();

			ObjectInstance* instance = InstantiateObject(type, ctor);
			frame->PushStack(instance);
			break;
		}

		case OpCode::NEWDELEGATE:
		{
			DelegateTypeSymbol* type = decoder.AbsordDelegateTypeSymbol();

			ObjectInstance* instance = InstantiateDelegate(type);
			if (instance != nullptr)
				instance->DelegateTarget = type->AnonymousSymbol;

			frame->PushStack(instance);
			break;
		}

		case OpCode::LOAD_TYPEARGUMENT:
		{
			std::uint16_t index = decoder.AbsorbUInt16();
			TypeSymbol* type = decoder.AbsorbTypeSymbol();
			if (PendingTypeArguments.size() <= index)
				PendingTypeArguments.resize(index + 1);

			PendingTypeArguments[index] = type;
			break;
		}

		case OpCode::LOADFIELD:
		{
			std::uint32_t slot = decoder.AbsorbFieldSlot();
			ObjectInstance* instance = frame->PopStack();
			ObjectInstance* fieldValue = instance->GetField(slot, frame);

			frame->PushStack(fieldValue);
			garbageCollector.CollectInstance(instance);
			break;
		}

		case OpCode::STOREFIELD:
		{
			std::uint32_t slot = decoder.AbsorbFieldSlot();
			ObjectInstance* fieldValue = frame->PopStack();
			ObjectInstance* instance = frame->PopStack();

			instance->SetField(slot, fieldValue, frame);
			garbageCollector.CollectInstance(fieldValue);
			garbageCollector.CollectInstance(instance);
			break;
		}

		case OpCode::LOADSTATICFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* fieldValue = garbageCollector.GetStaticField(field);

			frame->PushStack(fieldValue);
			break;
		}

		case OpCode::LOADENUMFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			TypeSymbol* enumType = static_cast<TypeSymbol*>(field->Parent);

			ObjectInstance* instance = garbageCollector.AllocateInstance(enumType);
			instance->WriteInteger(field->EnumValue);
			frame->PushStack(instance);
			break;
		}

		case OpCode::STORESTATICFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* fieldValue = frame->PopStack();

			garbageCollector.SetStaticField(field, fieldValue);
			garbageCollector.CollectInstance(fieldValue);
			break;
		}

		case OpCode::NEWARRAY:
		{
			TypeSymbol* type = decoder.AbsorbTypeSymbol();
			type = frame->ResolveType(type);
			if (type == nullptr || type->Kind != SyntaxKind::ArrayType)
				throw std::runtime_error("NEWARRAY expects an array type");

			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
			std::size_t length = arrayType->Length;
			std::vector<ObjectInstance*> elements;
			elements.reserve(length);
			for (std::size_t i = 0; i < length; ++i)
				elements.push_back(frame->PopStack());

			ObjectInstance* instance = garbageCollector.AllocateInstance(type);
			for (std::size_t i = 0; i < length; ++i)
			{
				std::size_t index = i;
				instance->SetElement(index, elements[i], frame);
				garbageCollector.CollectInstance(elements[i]);
			}

			frame->PushStack(instance);
			break;
		}

		case OpCode::NEWDYNAMICARRAY:
		{
			TypeSymbol* elementType = decoder.AbsorbTypeSymbol();
			elementType = frame->ResolveType(elementType);

			ObjectInstance* sizeInstance = frame->PopStack();
			std::int64_t length = sizeInstance->AsInteger();
			garbageCollector.CollectInstance(sizeInstance);

			ObjectInstance* instance = garbageCollector.AllocateArray(elementType, static_cast<std::size_t>(length));
			frame->PushStack(instance);
			break;
		}

		case OpCode::CREATERANGE:
		{
			TypeSymbol* elementType = decoder.AbsorbTypeSymbol();
			elementType = frame->ResolveType(elementType);

			ObjectInstance* inclusiveInstance = frame->PopStack();
			bool inclusive = inclusiveInstance->AsBoolean();
			garbageCollector.CollectInstance(inclusiveInstance);

			ObjectInstance* upperInstance = frame->PopStack();
			std::int64_t upper = upperInstance->AsInteger();
			garbageCollector.CollectInstance(upperInstance);

			ObjectInstance* lowerInstance = frame->PopStack();
			std::int64_t lower = lowerInstance->AsInteger();
			garbageCollector.CollectInstance(lowerInstance);

			std::int64_t diff = upper - lower;
			std::int64_t length = diff + (inclusive ? 1 : 0);

			if (diff < 0)
				length = -diff + (inclusive ? 1 : 0);

			if (length < 0)
				length = 0;

			std::int64_t step = (diff < 0) ? -1 : 1;

			ObjectInstance* arrayInstance = garbageCollector.AllocateArray(elementType, static_cast<std::size_t>(length));
			for (std::int64_t i = 0; i < length; i++)
			{
				ObjectInstance* valueInstance = garbageCollector.FromValue(lower + step * i);
				arrayInstance->SetElement(static_cast<std::size_t>(i), valueInstance, frame);
				valueInstance->DecrementReference();
			}

			frame->PushStack(arrayInstance);
			break;
		}

		case OpCode::LOADARRAYELEMENT:
		{
			ObjectInstance* indexInstance = frame->PopStack();
			ObjectInstance* arrayInstance = frame->PopStack();

			std::int64_t index = indexInstance->AsInteger();
			ObjectInstance* element = arrayInstance->GetElement(static_cast<std::size_t>(index), frame);

			frame->PushStack(element);

			garbageCollector.CollectInstance(indexInstance);
			break;
		}

		case OpCode::STOREARRAYELEMENT:
		{
			ObjectInstance* valueInstance = frame->PopStack();
			ObjectInstance* indexInstance = frame->PopStack();
			ObjectInstance* arrayInstance = frame->PopStack();

			std::int64_t index = indexInstance->AsInteger();
			arrayInstance->SetElement(static_cast<std::size_t>(index), valueInstance, frame);

			garbageCollector.CollectInstance(valueInstance);
			garbageCollector.CollectInstance(indexInstance);
			break;
		}

		case OpCode::ARRAYLENGTH:
		{
			ObjectInstance* arrayInstance = frame->PopStack();
			std::int64_t length = static_cast<std::int64_t>(arrayInstance->GetArrayLength());
			frame->PushStack(garbageCollector.FromValue(length));
			garbageCollector.CollectInstance(arrayInstance);
			break;
		}

		case OpCode::CREATEDUPLICATE:
		{
			ObjectInstance* instance = frame->PeekStack();
			ObjectInstance* duplicate = garbageCollector.CopyInstance(instance);

			frame->PushStack(duplicate);
			break;
		}

		case OpCode::MATH_ADDITION:
		{
			executeBinary(TokenType::AddOperator);
			break;
		}

		case OpCode::MATH_SUBSTRACTION:
		{
			executeBinary(TokenType::SubOperator);
			break;
		}

		case OpCode::MATH_MULTIPLICATION:
		{
			executeBinary(TokenType::MultOperator);
			break;
		}

		case OpCode::MATH_DIVISION:
		{
			executeBinary(TokenType::DivOperator);
			break;
		}

		case OpCode::MATH_MODULE:
		{
			executeBinary(TokenType::ModOperator);
			break;
		}

		case OpCode::MATH_POWER:
		{
			executeBinary(TokenType::PowOperator);
			break;
		}

		case OpCode::MATH_NEGATIVE:
		{
			executeUnary(TokenType::SubOperator);
			break;
		}

		case OpCode::MATH_POSITIVE:
		{
			executeUnary(TokenType::AddOperator);
			break;
		}

		case OpCode::MATH_LEFTSHIFT:
		{
			executeBinary(TokenType::LeftShiftOperator);
			break;
		}

		case OpCode::MATH_RIGHTSHIFT:
		{
			executeBinary(TokenType::RightShiftOperator);
			break;
		}

		case OpCode::COMPARE_EQUAL:
		{
			executeBinary(TokenType::EqualsOperator);
			break;
		}

		case OpCode::COMPARE_NOTEQUAL:
		{
			executeBinary(TokenType::NotEqualsOperator);
			break;
		}

		case OpCode::COMPARE_LESS:
		{
			executeBinary(TokenType::LessOperator);
			break;
		}

		case OpCode::COMPARE_LESSOREQUAL:
		{
			executeBinary(TokenType::LessOrEqualsOperator);
			break;
		}

		case OpCode::COMPARE_GREATER:
		{
			executeBinary(TokenType::GreaterOperator);
			break;
		}

		case OpCode::COMPARE_GREATEROREQUAL:
		{
			executeBinary(TokenType::GreaterOrEqualsOperator);
			break;
		}

		case OpCode::LOGICAL_NOT:
		{
			executeUnary(TokenType::NotOperator);
			break;
		}

		case OpCode::LOGICAL_OR:
		{
			executeBinary(TokenType::OrOperator);
			break;
		}

		case OpCode::LOGICAL_AND:
		{
			executeBinary(TokenType::AndOperator);
			break;
		}

		case OpCode::JUMP:
		{
			std::size_t jump = decoder.AbsorbJump();
			decoder.SetCursor(jump);
			break;
		}

		case OpCode::JUMP_FALSE:
		{
			std::size_t jump = decoder.AbsorbJump();
			ObjectInstance* value = frame->PopStack();

			bool asBool = value->AsBoolean();
			garbageCollector.DestroyInstance(value);

			if (!asBool)
				decoder.SetCursor(jump);

			break;
		}

		case OpCode::JUMP_TRUE:
		{
			std::size_t jump = decoder.AbsorbJump();
			ObjectInstance* value = frame->PopStack();

			bool asBool = value->AsBoolean();
			garbageCollector.DestroyInstance(value);

			if (asBool)
				decoder.SetCursor(jump);

			break;
		}

		case OpCode::RETURN:
		{
			decoder.Return();
			break;
		}

		case OpCode::THROW:
		{
			ObjectInstance* exception = frame->PopStack();
			if (exception == nullptr || exception == garbageCollector.NullInstance)
				throw std::runtime_error("Cannot throw null exception");

			exception->IncrementReference();
			frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
			frame->InterruptionRegister = exception;
			frame->CurrentException = exception;
			break;
		}

		case OpCode::RETHROW:
		{
			ObjectInstance* exception = frame->CurrentException;
			if (exception == nullptr)
				throw std::runtime_error("Cannot rethrow outside of catch block");

			exception->IncrementReference();
			frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
			frame->InterruptionRegister = exception;
			break;
		}

		case OpCode::ENTER_TRY:
		{
			std::size_t handlerOffset = decoder.AbsorbJump();
			frame->ExceptionHandlers.push_back(handlerOffset);
			break;
		}

		case OpCode::LEAVE_TRY:
		{
			if (!frame->ExceptionHandlers.empty())
				frame->ExceptionHandlers.pop_back();
			break;
		}

		case OpCode::END_CATCH:
		{
			if (frame->CurrentException != nullptr)
			{
				garbageCollector.CollectInstance(frame->CurrentException);
				frame->CurrentException = nullptr;
			}
			break;
		}

		default:
			throw std::runtime_error("CRITICAL SHIT! UNKNOWN OPCODE");
	}
}

std::wstring VirtualMachine::GetStackTrace() const
{
	std::wstring result;
	for (const auto& framePtr : CallStack)
	{
		CallStackFrame* frame = framePtr.get();
		if (frame == nullptr || frame->Method == nullptr)
			continue;

		if (!result.empty())
			result += L"\n";

		result += frame->Method->FullName;
	}

	return result;
}

ObjectInstance* VirtualMachine::CreateRuntimeException(const std::exception& err)
{
	ClassSymbol* runtimeExceptionType = SymbolTable::StandardTypes::RuntimeException;
	if (runtimeExceptionType == nullptr)
		throw std::runtime_error("RuntimeException type was not initialized");

	ConstructorSymbol* ctor = nullptr;
	for (ConstructorSymbol* candidate : runtimeExceptionType->Constructors)
	{
		if (candidate->Parameters.empty())
		{
			ctor = candidate;
			break;
		}
	}

	if (ctor == nullptr)
		throw std::runtime_error("RuntimeException has no parameterless constructor");

	ObjectInstance* instance = InstantiateObject(runtimeExceptionType, ctor);

	if (SymbolTable::StandardTypes::RuntimeExceptionMessageField != nullptr)
	{
		const char* what = err.what();
		std::wstring msg(what, what + std::strlen(what));
		ObjectInstance* msgInstance = garbageCollector.FromValue(msg);
		instance->SetField(SymbolTable::StandardTypes::RuntimeExceptionMessageField->SlotIndex, msgInstance, nullptr);
	}

	if (SymbolTable::StandardTypes::RuntimeExceptionStackTraceField != nullptr)
	{
		std::wstring trace = GetStackTrace();
		ObjectInstance* traceInstance = garbageCollector.FromValue(trace);
		instance->SetField(SymbolTable::StandardTypes::RuntimeExceptionStackTraceField->SlotIndex, traceInstance, nullptr);
	}

	return instance;
}

static bool HandleExceptionInFrame(CallStackFrame* frame, ByteCodeDecoder& decoder, GarbageCollector& gc)
{
	ObjectInstance* exception = frame->InterruptionRegister;
	if (exception == nullptr)
		throw std::runtime_error("Exception was raised without an exception object");

	while (!frame->EvalStack.empty())
	{
		ObjectInstance* top = frame->PopStack();
		if (top != nullptr)
			gc.DestroyInstance(top);
	}

	while (!frame->ExceptionHandlers.empty())
	{
		std::size_t handlerOffset = frame->ExceptionHandlers.back();
		frame->ExceptionHandlers.pop_back();

		decoder.SetCursor(handlerOffset);
		frame->EvalStack.push_back(exception);
		exception->IncrementReference();

		frame->CurrentException = exception;
		frame->InterruptionReason = FrameInterruptionReason::None;
		frame->InterruptionRegister = nullptr;
		return true;
	}

	return false;
}

void VirtualMachine::InvokeMethodInternal(MethodSymbol* method, CallStackFrame* currentFrame)
{
	if (AbortFlag)
		throw std::runtime_error("Execution aborted by host.");

	CallStackFrame* callingFrame = currentFrame->PreviousFrame;
	currentFrame->EvalStack.reserve(static_cast<std::size_t>(method->GetEvalStackLocalsCount()));

	std::size_t argsCount = method->GetEvalStackArgumentsCount();
	ObjectInstance* thisInstance = nullptr;

	for (std::size_t i = 0; i < argsCount; i++)
	{
		ObjectInstance* argument = callingFrame->PopStack();
		argument->IncrementReference();
		currentFrame->PushStack(argument);

		if (method->Linking == LINK_INSTANCE && i == 0)
			thisInstance = argument;
	}

	if (thisInstance != nullptr)
	{
		TypeShape* thisShape = thisInstance->getShape();
		if (currentFrame->TypeArguments.empty() && thisShape != nullptr && thisShape->HasGenericArguments())
		{
			currentFrame->TypeArguments = thisShape->GenericArguments;
		}
	}

	switch (method->HandleType)
	{
		case MethodHandleType::None:
		{
			throw std::runtime_error("Method handle type was not resolved");
		}

		case MethodHandleType::Lambda:
		case MethodHandleType::Body:
		{
			ByteCodeDecoder decoder = ByteCodeDecoder(method->ExecutableByteCode);
			while (true)
			{
				if (AbortFlag)
					throw std::runtime_error("Execution aborted by host.");

				if (currentFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
				{
					if (!HandleExceptionInFrame(currentFrame, decoder, garbageCollector))
						break;

					continue;
				}

				if (decoder.IsEOF())
					break;

				OpCode opCode = decoder.AbsorbOpCode();
				ProcessCode(currentFrame, decoder, opCode);
			}

			break;
		}

		case MethodHandleType::External:
		{
			try
			{
				if (method->FunctionPointer == nullptr)
					throw std::runtime_error("extern method body not resolved");

				ArgumentsSpan args(currentFrame->EvalStack.data(), argsCount);
				CallState context
				{
					.Domain = *domain,
					.Program = program,
					.Runtimer = *this,
					.Collector = garbageCollector,

					.Frame = currentFrame,
					.Method = method,
					.Args = args
				};

				ObjectInstance* retReg = method->FunctionPointer(context);
				if (method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void)
				{
					if (retReg == nullptr)
						throw std::runtime_error("method returned nullptr (void), when expected instance");

					callingFrame->PushStack(retReg);
				}
			}
			catch (const std::exception& err)
			{
				ObjectInstance* exception = CreateRuntimeException(err);

				currentFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				currentFrame->InterruptionRegister = exception;
				currentFrame->CurrentException = exception;
				exception->IncrementReference();
			}

			break;
		}
	}

	if (currentFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
	{
		ObjectInstance* exception = currentFrame->InterruptionRegister;
		if (callingFrame != nullptr && exception != nullptr)
		{
			callingFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
			callingFrame->InterruptionRegister = exception;
			callingFrame->CurrentException = exception;
			exception->IncrementReference();
		}
	}
	else
	{
		ObjectInstance* returnedValue = nullptr;
		if (method->HandleType != MethodHandleType::External &&
			method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void &&
			currentFrame->InterruptionReason != FrameInterruptionReason::ExceptionRaised)
		{
			if (!currentFrame->EvalStack.empty())
			{
				returnedValue = currentFrame->PopStack();
				callingFrame->PushStack(returnedValue);
			}
		}

		bool skippedReturnedValue = false;
		while (currentFrame->EvalStack.size() != 0)
		{
			ObjectInstance* top = currentFrame->PopStack();
			if (top == nullptr)
				continue;

			if (!skippedReturnedValue && top == returnedValue)
			{
				skippedReturnedValue = true;
				continue;
			}

			garbageCollector.DestroyInstance(top);
		}
	}
}

static SemanticModel::TypeParameterResolver MakeFrameResolver(CallStackFrame* frame)
{
	return [frame](TypeParameterSymbol* param) -> TypeSymbol*
	{
		if (frame == nullptr || param == nullptr)
			return nullptr;

		return frame->ResolveType(param);
	};
}

ObjectInstance* VirtualMachine::InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor)
{
	CallStackFrame* callingFrame = CurrentFrame();

	auto resolver = MakeFrameResolver(callingFrame);

	TypeSymbol* baseType = type;
	std::vector<TypeSymbol*> genericArgs;
	bool isGeneric = SemanticModel::TryResolveGenericArguments(type, resolver, baseType, genericArgs);

	ObjectInstance* newInstance = isGeneric
		? garbageCollector.AllocateGeneric(baseType, genericArgs)
		: garbageCollector.AllocateInstance(SemanticModel::ResolveRuntimeTypeArgument(type, resolver));

	// Zero-initialize all field slots using the type shape.
	TypeShape* shape = newInstance->getShape();
	if (shape != nullptr)
	{
		for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(shape->Slots.size()); ++slot)
		{
			TypeShape* fieldShape = shape->GetFieldShape(slot);
			std::size_t fieldOffset = shape->GetOffset(slot);
			std::size_t fieldSize = fieldShape != nullptr
				? (fieldShape->IsReferenceType() ? sizeof(void*) : fieldShape->Size)
				: sizeof(void*);

			void* offset = newInstance->OffsetMemory(fieldOffset, fieldSize);
			std::memset(offset, 0, fieldSize);
		}
	}

	// Resolve pending type arguments for the constructor frame as well.
	for (TypeSymbol*& pendingArg : PendingTypeArguments)
		pendingArg = SemanticModel::ResolveRuntimeTypeArgument(pendingArg, resolver);

	callingFrame->PushStack(newInstance);

	CallStackFrame* currentFrame = PushFrame(ctor);
	if (isGeneric && currentFrame->TypeArguments.empty())
		currentFrame->TypeArguments = genericArgs;

	newInstance->IncrementReference();

	InvokeMethodInternal(ctor, currentFrame);
	PopFrame();

	newInstance->DecrementReference();
	return newInstance;
}

ObjectInstance* VirtualMachine::InstantiateDelegate(DelegateTypeSymbol* type)
{
	ObjectInstance* newInstance = garbageCollector.AllocateInstance(type);
	return newInstance;
}

VirtualMachine::VirtualMachine(ApplicationDomain* appDomain) :
	domain(appDomain),
	program(domain->GetProgram()),
	garbageCollector(domain->GetGarbageCollector()),
	primitiveMath(garbageCollector)
{
	AbortFlag = false;
}

ObjectInstance* VirtualMachine::InvokeOperatorMethod(ObjectInstance* leftInstance, TokenType opToken, ObjectInstance* rightInstance)
{
	std::wstring opName = GetOperatorMethodName(opToken);
	if (opName.empty())
		throw std::runtime_error("operator is not overloadable");

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(leftInstance->getInfo());
	std::vector<TypeSymbol*> paramTypes =
	{
		ownerType,
		const_cast<TypeSymbol*>(rightInstance->getInfo())
	};

	OperatorSymbol* method = ownerType->FindOperator(opToken, paramTypes);
	if (method == nullptr)
		throw std::runtime_error("operator overload not found");

	InvokeMethod(method, { rightInstance, leftInstance });
	return CurrentFrame()->PopStack();
}

ObjectInstance* VirtualMachine::InvokeOperatorMethod(ObjectInstance* sourceInstance, TokenType opToken)
{
	std::wstring opName = GetOperatorMethodName(opToken);
	if (opName.empty())
		throw std::runtime_error("operator is not overloadable");

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(sourceInstance->getInfo());
	OperatorSymbol* method = ownerType->FindOperator(opToken, { ownerType });
	if (method == nullptr)
		throw std::runtime_error("operator overload not found");

	InvokeMethod(method, { sourceInstance });
	return CurrentFrame()->PopStack();
}

CallStackFrame* VirtualMachine::CurrentFrame() const
{
	if (CallStack.empty())
		return nullptr;

	return CallStack.back().get();
}

CallStackFrame* VirtualMachine::PushFrame(MethodSymbol* methodSymbol)
{
	auto frame = std::make_unique<CallStackFrame>(this, CurrentFrame(), methodSymbol);
	frame->TypeArguments = std::move(PendingTypeArguments);
	PendingTypeArguments.clear();

	CallStackFrame* rawFrame = frame.get();
	CallStack.push_back(std::move(frame));
	return rawFrame;
}

void VirtualMachine::PopFrame()
{
	if (CallStack.empty())
		return;

	CallStack.pop_back();
}

void VirtualMachine::InvokeMethod(MethodSymbol* method) const
{
	// hehe
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);

	CallStackFrame* currentFrame = vm->PushFrame(method);
	vm->InvokeMethodInternal(method, currentFrame);
	vm->PopFrame();
}

void VirtualMachine::InvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance*> args) const
{
	// hehe
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);

	CallStackFrame* callingFrame = vm->CurrentFrame();
	CallStackFrame* currentFrame = vm->PushFrame(method);

	for (ObjectInstance* argValue : args)
		callingFrame->PushStack(argValue);

	vm->InvokeMethodInternal(method, currentFrame);
	vm->PopFrame();
}

ObjectInstance* VirtualMachine::InvokeMethod(MethodSymbol* method, ObjectInstance** args, std::size_t count) const
{
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);

	bool pushedRootFrame = false;
	CallStackFrame* callingFrame = vm->CurrentFrame();
	if (callingFrame == nullptr)
	{
		MethodSymbol* rootMethod = program.EntryPoint != nullptr ? program.EntryPoint : method;
		callingFrame = vm->PushFrame(rootMethod);
		pushedRootFrame = true;
	}

	CallStackFrame* currentFrame = vm->PushFrame(method);

	for (std::size_t i = 0; i < count; i++)
		callingFrame->PushStack(args[i]);

	vm->InvokeMethodInternal(method, currentFrame);

	ObjectInstance* result = nullptr;
	if (method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void && !callingFrame->EvalStack.empty())
		result = callingFrame->PopStack();

	vm->PopFrame();

	if (pushedRootFrame)
		vm->PopFrame();

	return result;
}

void VirtualMachine::SetPendingTypeArguments(std::initializer_list<TypeSymbol*> args) const
{
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);
	vm->PendingTypeArguments.clear();
	vm->PendingTypeArguments.reserve(args.size());

	for (TypeSymbol* arg : args)
		vm->PendingTypeArguments.push_back(arg);
}

void VirtualMachine::RaiseException(ObjectInstance* exceptionReg) const
{
	// TODO: implement method
}

static std::wstring GetThrowablePropertyValue(VirtualMachine& vm, ObjectInstance* exception, AccessorSymbol* interfacePropertyAccessor)
{
	if (exception == nullptr || interfacePropertyAccessor == nullptr)
		return L"";

	TypeSymbol* exceptionType = const_cast<TypeSymbol*>(exception->getInfo());
	if (exceptionType == nullptr)
		return L"";

	MethodSymbol* implementation = exceptionType->FindInterfaceImplementation(interfacePropertyAccessor);
	if (implementation == nullptr)
		return L"";

	vm.InvokeMethod(implementation, { exception });

	CallStackFrame* frame = vm.CurrentFrame();
	if (frame == nullptr || frame->EvalStack.empty())
		return L"";

	ObjectInstance* result = frame->PopStack();
	if (result == nullptr)
		return L"";

	if (result->getInfo() == nullptr || result->getInfo() != SymbolTable::Primitives::String)
		return L"";

	const wchar_t* data = result->AsString();
	std::wstring value = data != nullptr ? std::wstring(data) : L"";

	if (result != GarbageCollector::NullInstance)
		result->DecrementReference();

	return value;
}

void VirtualMachine::Run()
{
	if (program.EntryPoint == nullptr)
		throw std::runtime_error("entry point was null");

	if (UnhandledException != nullptr)
	{
		garbageCollector.CollectInstance(UnhandledException);
		UnhandledException = nullptr;
	}

	UnhandledExceptionMessage.clear();
	UnhandledExceptionStackTrace.clear();

	AbortFlag = false;
	CallStackFrame* entryFrame = PushFrame(program.EntryPoint);
	InvokeMethodInternal(program.EntryPoint, entryFrame);

	if (entryFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
	{
		ObjectInstance* exception = entryFrame->InterruptionRegister;
		if (exception != nullptr)
		{
			exception->IncrementReference();
			UnhandledException = exception;

			if (SemanticModel::IsAssignableTo(TRAIT_THROWABLE, exception->getInfo()))
			{
				UnhandledExceptionMessage = GetThrowablePropertyValue(*this, exception, TRAIT_THROWABLE_getMessage);
				UnhandledExceptionStackTrace = GetThrowablePropertyValue(*this, exception, TRAIT_THROWABLE_getStackTrace);
			}

			if (UnhandledExceptionStackTrace.empty())
				UnhandledExceptionStackTrace = GetStackTrace();
		}

		PopFrame();
		return;
	}

	PopFrame();
}

void VirtualMachine::Abort() const
{
	// hehe
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);
	vm->AbortFlag = true;
}

ObjectInstance* VirtualMachine::RunInteractive(std::size_t& pointer)
{
	CallStackFrame* currentFrame = CurrentFrame();
	MethodSymbol* method = currentFrame->Method;
	currentFrame->EvalStack.reserve(static_cast<std::size_t>(method->GetEvalStackLocalsCount()) * 2);

	ByteCodeDecoder decoder = ByteCodeDecoder(method->ExecutableByteCode);
	decoder.SetCursor(pointer);

	while (!decoder.IsEOF())
	{
		if (AbortFlag)
			throw std::runtime_error("Execution aborted by host.");

		OpCode opCode = decoder.AbsorbOpCode();
		if (opCode == OpCode::POPSTACK && decoder.IsEOF())
			continue;

		ProcessCode(currentFrame, decoder, opCode);
	}

	pointer = decoder.Index();
	if (currentFrame->EvalStack.size() > method->GetEvalStackLocalsCount())
	{
		ObjectInstance* retReg = currentFrame->PopStack();
		return retReg;
	}

	return nullptr;
}

void VirtualMachine::TerminateCallStack()
{
	while (!CallStack.empty())
		PopFrame();
}
