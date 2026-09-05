#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/NativeAsync.hpp>
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
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <exception>

using namespace shard;

namespace 
{
	static void ExecuteDeferExpression(VirtualMachine* vm, CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t target)
	{
		std::size_t savedIP = decoder.Index();
		decoder.SetCursor(target);

		while (true)
		{
			if (decoder.IsEOF())
				throw std::runtime_error("Deferred expression ran past end of bytecode");

			OpCode op = decoder.AbsorbOpCode();
			vm->ProcessCode(frame, decoder, op);

			if (frame->interrupted())
			{
				return;
			}

			if (op == OpCode::DEFER_BREAK)
				break;
		}

		decoder.SetCursor(savedIP);
	}

	static ClassSymbol* GetExceptionClassDefinition(TypeSymbol* type)
	{
		if (type == nullptr)
			return nullptr;

		if (type->Kind == SyntaxKind::GenericType)
			return static_cast<ClassSymbol*>(static_cast<GenericTypeSymbol*>(type)->UnderlayingType);

		if (type->Kind == SyntaxKind::ClassDeclaration)
			return static_cast<ClassSymbol*>(type);

		return nullptr;
	}

	static FieldSymbol* FindThrowableBackingField(TypeSymbol* type, MethodSymbol* interfaceGetter)
	{
		if (type == nullptr || interfaceGetter == nullptr)
			return nullptr;

		ClassSymbol* definition = GetExceptionClassDefinition(type);
		if (definition == nullptr)
			return nullptr;

		auto it = definition->InterfaceMethodMap.find(interfaceGetter);
		if (it == definition->InterfaceMethodMap.end())
			return nullptr;

		MethodSymbol* implementation = it->second;
		if (implementation == nullptr || implementation->Parent == nullptr)
			return nullptr;

		if (implementation->Parent->Kind != SyntaxKind::PropertyDeclaration)
			return nullptr;

		PropertySymbol* property = static_cast<PropertySymbol*>(implementation->Parent);
		return property->BackingField;
	}

	static void VerifyInstanceNotNull(ObjectInstance instance, const char* operation)
	{
		if (instance.IsNullInstance())
		{
			throw std::runtime_error("Cannot perform operation on null instance.");
		}
	}

	static bool DrainDefersTo(VirtualMachine* vm, CallStackFrame* frame, ByteCodeDecoder& decoder, std::size_t targetSize, GarbageCollector& gc)
	{
		while (frame->DeferStack.size() > targetSize)
		{
			std::size_t target = frame->DeferStack.back();
			frame->DeferStack.pop_back();

			frame->DeferDrainDepth++;
			try
			{
				ExecuteDeferExpression(vm, frame, decoder, target);
			}
			catch (...)
			{
				frame->DeferDrainDepth--;
				throw;
			}

			frame->DeferDrainDepth--;
			if (frame->interrupted())
				return false;
		}

		return true;
	}

	static bool HandleExceptionInFrame(VirtualMachine* vm, CallStackFrame* frame, ByteCodeDecoder& decoder, GarbageCollector& gc)
	{
		ObjectInstance exception = frame->InterruptionRegister;
		if (exception.IsNullInstance())
			throw std::runtime_error("Exception was raised without an exception object");

		frame->DrainEvalReferences(gc);

		frame->InterruptionReason = FrameInterruptionReason::None;
		frame->InterruptionRegister = ObjectInstance();

		while (!frame->ExceptionHandlers.empty())
		{
			CallStackFrame::ExceptionHandlerFrame handler = frame->ExceptionHandlers.back();
			frame->ExceptionHandlers.pop_back();

			if (!DrainDefersTo(vm, frame, decoder, handler.DeferStackBase, gc))
				return true;

			decoder.SetCursor(handler.HandlerOffset);
			frame->PushReference(exception);
			exception.IncrementReference();
			
			frame->InterruptionReason = FrameInterruptionReason::None;
			frame->InterruptionRegister = exception;
			return true;
		}

		if (!DrainDefersTo(vm, frame, decoder, 0, gc))
			return true;

		frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
		frame->InterruptionRegister = exception;
		exception.IncrementReference();
		return false;
	}

	static bool IsPendingTask(ObjectInstance task, GarbageCollector& gc)
	{
		if (task.IsNullInstance() || !gc.IsTaskLike(task))
			return false;

		const TypeSymbol* info = task.getInfo();
		FieldSymbol* stateField = nullptr;
		if (info->Name == L"Task")
			stateField = SymbolTable::StandardTypes::Task_StateField;
		else if (info->Name == L"ValueTask")
			stateField = SymbolTable::StandardTypes::ValueTask_StateField;

		if (stateField == nullptr)
			return false;

		return GetTaskState(task, stateField) == AsyncState::PENDING;
	}

	static void BindTaskToFrame(ObjectInstance task, CallStackFrame* frame, GarbageCollector& gc)
	{
		if (task.IsNullInstance() || frame == nullptr || !gc.IsTaskLike(task))
			return;

		if (gc.GetFrameOwner(task).get() == frame)
			return;

		gc.ReleaseFrameOwner(task);
		if (!IsPendingTask(task, gc))
			return;

		gc.BindToFrame(task, frame->shared_from_this());
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

	static void HaltTaskObject(ObjectInstance task, GarbageCollector& gc)
	{
		if (task.IsNullInstance() || !gc.IsTaskLike(task))
			return;

		const TypeSymbol* info = task.getInfo();
		ObjectInstance exception = CreateRuntimeException(gc,
			L"Task halted because the virtual machine has stopped");

		if (info->Name == L"Task")
		{
			SetTaskState(task, SymbolTable::StandardTypes::Task_StateField, AsyncState::FAULTED, gc);
			task.SetField(SymbolTable::StandardTypes::Task_ExceptionField->SlotIndex, exception);
		}
		else if (info->Name == L"ValueTask")
		{
			SetTaskState(task, SymbolTable::StandardTypes::ValueTask_StateField, AsyncState::FAULTED, gc);
			task.SetField(SymbolTable::StandardTypes::ValueTask_ExceptionField->SlotIndex, exception);
		}

		gc.ReleaseFrameOwner(task);
	}
}

void VirtualMachine::ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode)
{
	auto primitiveShape = [&](TypeSymbol* primitive) -> TypeShape*
	{
		return program.TypeShapes->GetOrCreateShape(primitive);
	};

	auto pushPrimitiveResult = [&](ObjectInstance result) -> void
	{
		if (!result.IsNullInstance() && result.getShape() != nullptr && !result.getShape()->IsReferenceType())
		{
			TypeShape* shape = result.getShape();
			frame->PushInline(shape, result.getMemory());
			garbageCollector.DestroyInstance(result);
			return;
		}

		frame->PushReference(result);
	};

	auto executeBinary = [&](TokenType token) -> void
	{
		ObjectInstance right = frame->PopValue();
		ObjectInstance left = frame->PopValue();

		if (right.IsNullInstance() || left.IsNullInstance())
		{
			bool equal = false;
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Boolean), &equal);
			return;
		}

		if (token == TokenType::DivOperator || token == TokenType::ModOperator)
		{
			const TypeSymbol* rightType = right.getInfo();
			if (rightType != nullptr && (rightType == TYPE_INT || rightType == TYPE_CHAR || rightType == TYPE_BYTE || rightType == TYPE_NINT || rightType->Kind == SyntaxKind::EnumDeclaration))
			{
				if (right.AsInteger() == 0)
					throw std::runtime_error("DivideByZeroException");
			}
		}

		ObjectInstance result = primitiveMath.ExecuteBinary(token, left, right);
		if (result.IsNullInstance())
			result = InvokeOperatorMethod(left, token, right);

		pushPrimitiveResult(result);
		CallStackFrame::DiscardValue(right, garbageCollector);
		CallStackFrame::DiscardValue(left, garbageCollector);
	};

	auto executeUnary = [&](TokenType token) -> void
	{
		ObjectInstance operand = frame->PopValue();

		if (operand.IsNullInstance())
			throw std::runtime_error("Cannot perform operation on null instance");

		ObjectInstance result = primitiveMath.ExecuteUnary(token, operand);
		if (result.IsNullInstance())
			result = InvokeOperatorMethod(operand, token);

		pushPrimitiveResult(result);
		CallStackFrame::DiscardValue(operand, garbageCollector);
	};

	switch (opCode)
	{
		case OpCode::NOP:
		{
			break;
		}

		case OpCode::HALT:
		{
			AbortFlag = true;
			break;
		}

		case OpCode::POP:
		{
			ObjectInstance value = frame->PopValue();
			CallStackFrame::DiscardValue(value, garbageCollector);
			break;
		}

		case OpCode::CALLMETHODSYMBOL:
		{
			MethodSymbol* methodSymbol = decoder.AbsorbMethodSymbol();
			InvokeMethod(methodSymbol);
			break;
		}

		case OpCode::CALLDELEGATE:
		{
			decoder.AbsordDelegateTypeSymbol();
			ObjectInstance delegateInstance = frame->PopStack();
			if (delegateInstance.IsNullInstance())
				throw std::runtime_error("Cannot invoke a null delegate");

			MethodSymbol* target = delegateInstance.getInfo()->Methods.at(0);
			if (target == nullptr)
				throw std::runtime_error("Delegate has no target method");

			if (target->Linking == LINK_INSTANCE)
				frame->PushStack(delegateInstance);   // becomes 'this' for the closure method

			InvokeMethod(target);
			break;
		}

		case OpCode::CALLINTERFACE:
		{
			MethodSymbol* interfaceMethod = decoder.AbsorbMethodSymbol();
			ObjectInstance thisInstance = frame->PeekStack();

			if (thisInstance.IsNullInstance())
				throw std::runtime_error("Cannot invoke interface method on a null reference");

			TypeSymbol* concreteType = const_cast<TypeSymbol*>(thisInstance.getInfo());
			MethodSymbol* implementation = concreteType->FindInterfaceImplementation(interfaceMethod);

			if (implementation == nullptr)
				throw std::runtime_error("Interface method implementation not found");

			InvokeMethod(implementation);
			break;
		}

		case OpCode::ISINSTANCE:
		{
			TypeSymbol* targetType = decoder.AbsorbTypeSymbol();
			ObjectInstance value = frame->PopValue();

			bool result = false;
			if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
			{
				result = value.getShape() != nullptr && SemanticModel::IsAssignableTo(targetType, value.getShape()->BaseType);
			}
			else if (!value.IsNullInstance())
			{
				TypeSymbol* instanceType = const_cast<TypeSymbol*>(value.getInfo());
				result = SemanticModel::IsAssignableTo(targetType, instanceType);
			}

			CallStackFrame::DiscardValue(value, garbageCollector);
			bool asBool = result;
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Boolean), &asBool);
			break;
		}

		case OpCode::CAST_AS:
		{
			TypeSymbol* targetType = decoder.AbsorbTypeSymbol();
			targetType = frame->ResolveType(targetType);
			ObjectInstance value = frame->PopValue();

			bool compatible = false;
			if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
			{
				compatible = value.getShape() != nullptr && SemanticModel::IsAssignableTo(targetType, value.getShape()->BaseType);
			}
			else if (!value.IsNullInstance())
			{
				TypeSymbol* instanceType = const_cast<TypeSymbol*>(value.getInfo());
				compatible = SemanticModel::IsAssignableTo(targetType, instanceType);
			}

			if (compatible)
			{
				frame->PushCopy(value);
			}
			else
			{
				frame->PushReference(ObjectInstance());
				CallStackFrame::DiscardValue(value, garbageCollector);
			}

			break;
		}

		case OpCode::CAST:
		{
			TypeSymbol* targetType = decoder.AbsorbTypeSymbol();
			targetType = frame->ResolveType(targetType);
			ObjectInstance value = frame->PopValue();

			if (targetType == SymbolTable::Primitives::Any)
			{
				if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
				{
					frame->PushCopy(value);
				}
				else
				{
					frame->PushReference(value.IsNullInstance() ? ObjectInstance() : value);
				}
				break;
			}

			if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
			{
				if (value.getShape() == nullptr || !SemanticModel::IsAssignableTo(targetType, value.getShape()->BaseType))
					throw std::runtime_error("Invalid cast");

				frame->PushCopy(value);
				break;
			}

			if (value.IsNullInstance())
			{
				if (targetType->Inlining == TypeInlining::ByValue)
					throw std::runtime_error("Cannot cast null to a value type");

				frame->PushReference(ObjectInstance());
				break;
			}

			TypeSymbol* instanceType = const_cast<TypeSymbol*>(value.getInfo());
			if (SemanticModel::IsAssignableTo(targetType, instanceType))
			{
				frame->PushReference(value);
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
			ObjectInstance value = frame->PopValue();

			if (value.IsNullInstance())
				throw std::runtime_error("Cannot cast null to a primitive type");

			ObjectInstance result = primitiveMath.ExecuteCast(targetType, value);
			if (result.IsNullInstance())
				throw std::runtime_error("Unsupported primitive cast");

			pushPrimitiveResult(result);
			CallStackFrame::DiscardValue(value, garbageCollector);
			break;
		}

		case OpCode::LOADCONST_NULL:
		{
			frame->PushReference(ObjectInstance());
			break;
		}

		case OpCode::LOADCONST_BOOLEAN:
		{
			bool value = decoder.AbsorbBoolean();
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Boolean), &value);
			break;
		}

		case OpCode::LOADCONST_INTEGER8:
		{
			std::uint8_t value = decoder.AbsorbUInt8();
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Byte), &value);
			break;
		}

		case OpCode::LOADCONST_INTEGER64:
		{
			std::int64_t value = decoder.AbsorbInt64();
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Integer), &value);
			break;
		}

		case OpCode::LOADCONST_NATIVE_INTEGER:
		{
			std::intptr_t value = decoder.AbsorbIntPtr();
			frame->PushInline(primitiveShape(SymbolTable::Primitives::NativeInteger), &value);
			break;
		}

		case OpCode::LOADCONST_RATIONAL64:
		{
			double value = decoder.AbsorbDouble64();
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Double), &value);
			break;
		}

		case OpCode::LOADCONST_CHAR:
		{
			wchar_t value = decoder.AbsorbChar16();
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Char), &value);
			break;
		}

		case OpCode::LOADCONST_STRING:
		{
			std::size_t data = decoder.AbsorbString();
			const wchar_t* str = reinterpret_cast<wchar_t*>(program.DataSection.data() + data);

			ObjectInstance instance = garbageCollector.InternString(str);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LOAD_LOCAL:
		{
			std::uint16_t slot = decoder.AbsorbVariableSlot();
			frame->PushCopy(frame->GetLocal(slot));
			break;
		}

		case OpCode::STORE_LOCAL:
		{
			std::uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance value = frame->PopValue();
			frame->SetLocal(slot, value, garbageCollector);
			break;
		}

		case OpCode::LOAD_ARG:
		{
			std::uint16_t slot = decoder.AbsorbVariableSlot();
			frame->PushCopy(frame->GetLocal(slot));
			break;
		}

		case OpCode::STORE_ARG:
		{
			std::uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance value = frame->PopValue();
			frame->SetLocal(slot, value, garbageCollector);
			break;
		}

		case OpCode::NEWOBJECT:
		{
			TypeSymbol* type = decoder.AbsorbTypeSymbol();
			type = frame->ResolveType(type);
			ConstructorSymbol* ctor = decoder.AbsorbConstructorSymbol();

			ObjectInstance instance = InstantiateObject(type, ctor, true);
			if (!instance.IsNullInstance())
				frame->PushStack(instance);
			break;
		}

		case OpCode::NEWDELEGATE:
		{
			DelegateTypeSymbol* type = decoder.AbsordDelegateTypeSymbol();
			MethodSymbol* target = type->AnonymousSymbol;

			ObjectInstance instance;
			if (target != nullptr && target->Linking == LINK_INSTANCE)
			{
				instance = frame->PopStack();
				if (instance.IsNullInstance())
					throw std::runtime_error("Cannot create delegate with null receiver");
			}
			else
			{
				instance = InstantiateDelegate(type);
			}

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
			ObjectInstance target = frame->PopValue();

			VerifyInstanceNotNull(target, "member");

			ObjectInstance fieldValue = target.GetField(slot);

			if (fieldValue.getInfo() != nullptr && !fieldValue.getInfo()->IsReferenceType())
			{
				frame->PushInline(fieldValue.getShape(), fieldValue.getMemory());
			}
			else
			{
				frame->PushReference(fieldValue);
			}

			CallStackFrame::DiscardValue(target, garbageCollector);
			break;
		}

		case OpCode::STOREFIELD:
		{
			std::uint32_t slot = decoder.AbsorbFieldSlot();
			ObjectInstance fieldValue = frame->PopValue();
			ObjectInstance target = frame->PopValue();

			VerifyInstanceNotNull(target, "member");

			target.SetField(slot, fieldValue);

			CallStackFrame::DiscardValue(fieldValue, garbageCollector);
			CallStackFrame::DiscardValue(target, garbageCollector);
			break;
		}

		case OpCode::LOADSTATICFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance fieldValue = garbageCollector.GetStaticField(field);

			frame->PushStack(fieldValue);
			break;
		}

		case OpCode::LOADENUMFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			TypeSymbol* enumType = static_cast<TypeSymbol*>(field->Parent);

			std::int64_t value = static_cast<std::int64_t>(field->EnumValue);
			frame->PushInline(primitiveShape(enumType), &value);
			break;
		}

		case OpCode::STORESTATICFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance fieldValue = frame->PopStack();

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

			std::vector<ObjectInstance> elements;
			elements.reserve(length);

			for (std::size_t i = 0; i < length; ++i)
				elements.push_back(frame->PopValue());

			ObjectInstance instance = garbageCollector.AllocateInstance(type);
			std::uint64_t payloadLength = static_cast<std::uint64_t>(length);
			instance.WriteMemory(0, sizeof(std::int64_t), &payloadLength);

			for (std::size_t i = 0; i < length; ++i)
			{
				instance.SetElement(i, elements[i]);
				CallStackFrame::DiscardValue(elements[i], garbageCollector);
			}

			frame->PushReference(instance);
			break;
		}

		case OpCode::NEWARRAY_DYNAMIC:
		{
			TypeSymbol* operand = decoder.AbsorbTypeSymbol();
			TypeSymbol* resolved = frame->ResolveType(operand);
			if (resolved == nullptr || resolved->Kind != SyntaxKind::ArrayType)
				throw std::runtime_error("NEWARRAY_DYNAMIC expects an array type");

			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(resolved);
			TypeSymbol* elementType = frame->ResolveType(arrayType->UnderlayingType);

			ObjectInstance sizeValue = frame->PopValue();
			std::int64_t length = sizeValue.AsInteger();
			CallStackFrame::DiscardValue(sizeValue, garbageCollector);

			ObjectInstance instance = garbageCollector.AllocateArray(arrayType, elementType, static_cast<std::size_t>(length));
			frame->PushReference(instance);
			break;
		}

		case OpCode::CREATERANGE:
		{
			TypeSymbol* operand = decoder.AbsorbTypeSymbol();
			TypeSymbol* resolved = frame->ResolveType(operand);
			if (resolved == nullptr || resolved->Kind != SyntaxKind::ArrayType)
				throw std::runtime_error("CREATERANGE expects an array type");

			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(resolved);
			TypeSymbol* elementType = frame->ResolveType(arrayType->UnderlayingType);

			ObjectInstance inclusiveValue = frame->PopValue();
			bool inclusive = inclusiveValue.AsBoolean();
			CallStackFrame::DiscardValue(inclusiveValue, garbageCollector);

			ObjectInstance upperValue = frame->PopValue();
			std::int64_t upper = upperValue.AsInteger();
			CallStackFrame::DiscardValue(upperValue, garbageCollector);

			ObjectInstance lowerValue = frame->PopValue();
			std::int64_t lower = lowerValue.AsInteger();
			CallStackFrame::DiscardValue(lowerValue, garbageCollector);

			std::int64_t diff = upper - lower;
			std::int64_t length = diff + (inclusive ? 1 : 0);

			if (diff < 0)
				length = -diff + (inclusive ? 1 : 0);

			if (length < 0)
				length = 0;

			std::int64_t step = (diff < 0) ? -1 : 1;

			ObjectInstance arrayInstance = garbageCollector.AllocateArray(arrayType, elementType, static_cast<std::size_t>(length));
			for (std::int64_t i = 0; i < length; i++)
			{
				ObjectInstance valueInstance = garbageCollector.FromInteger(lower + step * i);
				arrayInstance.SetElement(static_cast<std::size_t>(i), valueInstance);
				valueInstance.DecrementReference();
			}

			frame->PushStack(arrayInstance);
			break;
		}

		case OpCode::LOADARRAYELEMENT:
		{
			ObjectInstance indexValue = frame->PopValue();
			ObjectInstance arrayInstance = frame->PopStack();
			VerifyInstanceNotNull(arrayInstance, "indexer");

			std::int64_t index = indexValue.AsInteger();
			std::size_t length = arrayInstance.GetArrayLength();
			if (index < 0 || static_cast<std::size_t>(index) >= length)
				throw std::runtime_error("Array index out of range");

			ObjectInstance element = arrayInstance.GetElement(static_cast<std::size_t>(index));

			if (element.getInfo() != nullptr && !element.getInfo()->IsReferenceType())
			{
				const ArrayTypeSymbol* arrayInfo = static_cast<const ArrayTypeSymbol*>(arrayInstance.getInfo());
				TypeShape* elementShape = program.TypeShapes->GetOrCreateShape(frame->ResolveType(arrayInfo->UnderlayingType));
				frame->PushInline(elementShape, element.getMemory());
			}
			else
			{
				element.IncrementReference();
				frame->PushReference(element);
			}

			CallStackFrame::DiscardValue(indexValue, garbageCollector);
			garbageCollector.CollectInstance(arrayInstance);
			break;
		}

		case OpCode::STOREARRAYELEMENT:
		{
			ObjectInstance valueValue = frame->PopValue();
			ObjectInstance indexValue = frame->PopValue();

			ObjectInstance arrayInstance = frame->PopStack();
			VerifyInstanceNotNull(arrayInstance, "indexer");

			std::int64_t index = indexValue.AsInteger();
			std::size_t length = arrayInstance.GetArrayLength();

			if (index < 0 || static_cast<std::size_t>(index) >= length)
				throw std::runtime_error("Array index out of range");

			arrayInstance.SetElement(static_cast<std::size_t>(index), valueValue);

			CallStackFrame::DiscardValue(valueValue, garbageCollector);
			CallStackFrame::DiscardValue(indexValue, garbageCollector);
			break;
		}

		case OpCode::ARRAYLENGTH:
		{
			ObjectInstance arrayInstance = frame->PopStack();
			VerifyInstanceNotNull(arrayInstance, "indexer");

			std::int64_t length = static_cast<std::int64_t>(arrayInstance.GetArrayLength());
			frame->PushInline(primitiveShape(SymbolTable::Primitives::Integer), &length);
			garbageCollector.CollectInstance(arrayInstance);
			break;
		}

		case OpCode::DUP:
		{
			ObjectInstance value = frame->TopValue();
			if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
			{
				frame->PushInline(value.getShape(), value.getMemory());
				break;
			}

			if (!value.IsNullInstance())
				value.IncrementReference();

			frame->PushReference(value);
			break;
		}

		case OpCode::MATH_ADDITION:
		{
			executeBinary(TokenType::AddOperator);
			break;
		}

		case OpCode::MATH_SUBTRACT:
		{
			executeBinary(TokenType::SubOperator);
			break;
		}

		case OpCode::MATH_MULTIPLY:
		{
			executeBinary(TokenType::MultOperator);
			break;
		}

		case OpCode::MATH_DIVISION:
		{
			executeBinary(TokenType::DivOperator);
			break;
		}

		case OpCode::MATH_MODULO:
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

		case OpCode::MATH_SHL:
		{
			executeBinary(TokenType::LeftShiftOperator);
			break;
		}

		case OpCode::MATH_SHR:
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

		case OpCode::COMPARE_LESS_OR_EQUAL:
		{
			executeBinary(TokenType::LessOrEqualsOperator);
			break;
		}

		case OpCode::COMPARE_GREATER:
		{
			executeBinary(TokenType::GreaterOperator);
			break;
		}

		case OpCode::COMPARE_GREATER_OR_EQUAL:
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
			ObjectInstance value = frame->PopValue();

			bool asBool = value.AsBoolean();
			CallStackFrame::DiscardValue(value, garbageCollector);

			if (!asBool)
				decoder.SetCursor(jump);

			break;
		}

		case OpCode::JUMP_TRUE:
		{
			std::size_t jump = decoder.AbsorbJump();
			ObjectInstance value = frame->PopValue();

			bool asBool = value.AsBoolean();
			CallStackFrame::DiscardValue(value, garbageCollector);

			if (asBool)
				decoder.SetCursor(jump);

			break;
		}

		case OpCode::BR_NULL:
		{
			std::size_t jump = decoder.AbsorbJump();
			ObjectInstance value = frame->PopValue();

			if (value.getInfo() != nullptr && !value.getInfo()->IsReferenceType())
			{
				CallStackFrame::DiscardValue(value, garbageCollector);
				break;
			}

			bool isNull = value.IsNullInstance();
			if (!isNull)
				garbageCollector.CollectInstance(value);

			if (isNull)
				decoder.SetCursor(jump);

			break;
		}

		case OpCode::JUMP_TABLE:
		{
			std::uint32_t count = decoder.AbsorbUInt32();
			std::size_t base = decoder.GetCursor();
			ObjectInstance value = frame->PopValue();

			std::int64_t index = value.AsInteger();
			CallStackFrame::DiscardValue(value, garbageCollector);

			if (index < 0 || static_cast<std::uint64_t>(index) >= count)
			{
				decoder.SetCursor(base + count * sizeof(std::size_t));
			}
			else
			{
				std::size_t target = decoder.PeekJumpAt(base + static_cast<std::size_t>(index) * sizeof(std::size_t));
				decoder.SetCursor(target);
			}

			break;
		}

		case OpCode::RETURN:
		{
			decoder.Return();
			break;
		}

		case OpCode::THROW:
		{
			ObjectInstance thrown = frame->PopValue();

			ObjectInstance exception;
			if (thrown.getInfo() != nullptr && !thrown.getInfo()->IsReferenceType())
			{
				exception = garbageCollector.AllocateInstance(thrown.getShape());
				exception.WriteMemory(0, thrown.getShape()->Size, thrown.getMemory());
				for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(thrown.getShape()->Slots.size()); ++slot)
				{
					TypeShape* fieldShape = thrown.getShape()->GetFieldShape(slot);
					if (fieldShape == nullptr || !fieldShape->IsReferenceType())
						continue;

					ObjectInstance fieldValue = exception.GetField(slot);
					if (!fieldValue.IsNullInstance())
						fieldValue.IncrementReference();
				}
			}
			else
			{
				exception = thrown;
			}

			if (exception.IsNullInstance())
				throw std::runtime_error("Cannot throw null exception");

			TypeSymbol* exceptionType = const_cast<TypeSymbol*>(exception.getInfo());
			FieldSymbol* stackTraceField = FindThrowableBackingField(exceptionType, TRAIT_THROWABLE_getStackTrace);

			if (stackTraceField != nullptr)
			{
				ObjectInstance currentTrace = exception.GetField(stackTraceField->SlotIndex);
				bool needsTrace = currentTrace.IsNullInstance();

				if (!needsTrace && currentTrace.AsStringLength() == 0)
					needsTrace = true;

				if (needsTrace)
				{
					ObjectInstance traceInstance = garbageCollector.FromString(GetStackTrace());
					exception.SetField(stackTraceField->SlotIndex, traceInstance);
				}
			}

			exception.IncrementReference();
			frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
			frame->InterruptionRegister = exception;
			frame->CurrentException = exception;
			break;
		}

		case OpCode::RETHROW:
		{
			ObjectInstance exception = frame->CurrentException;
			if (exception.IsNullInstance())
				throw std::runtime_error("Cannot rethrow outside of catch block");

			exception.IncrementReference();
			frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
			frame->InterruptionRegister = exception;
			break;
		}

		case OpCode::ENTER_TRY:
		{
			std::size_t handlerOffset = decoder.AbsorbJump();
			frame->ExceptionHandlers.push_back({handlerOffset, frame->DeferStack.size()});
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
			if (!frame->CurrentException.IsNullInstance())
			{
				garbageCollector.CollectInstance(frame->CurrentException);
				frame->CurrentException = ObjectInstance();
			}
			break;
		}

		case OpCode::LOAD_CURRENT_EXCEPTION:
		{
			frame->PushStack(frame->CurrentException);
			break;
		}

		case OpCode::STORE_CURRENT_EXCEPTION:
		{
			ObjectInstance exception = frame->PopStack();

			if (!frame->CurrentException.IsNullInstance() && !exception.IsNullInstance() &&
				frame->CurrentException.getMemory() != exception.getMemory())
			{
				frame->CurrentException.DecrementReference();
				garbageCollector.CollectInstance(frame->CurrentException);
			}

			frame->CurrentException = exception;
			if (!exception.IsNullInstance())
				exception.IncrementReference();

			break;
		}

		case OpCode::DEFER:
		{
			std::size_t target = decoder.AbsorbJump();
			frame->DeferStack.push_back(target);
			break;
		}

		case OpCode::DEFER_BREAK:
		{
			if (frame->DeferDrainDepth == 0)
				throw std::runtime_error("DEFER_BREAK reached outside of a deferred expression drain");

			break;
		}

		case OpCode::DEFER_DRAIN:
		{
			std::size_t count = decoder.AbsorbJump();
			std::size_t savedIP = decoder.Index();

			frame->DeferDrainDepth++;
			try
			{
				for (std::size_t i = 0; i < count; ++i)
				{
					if (frame->DeferStack.empty())
						throw std::runtime_error("DEFER_DRAIN: not enough registered defers");

					std::size_t target = frame->DeferStack.back();
					frame->DeferStack.pop_back();
					ExecuteDeferExpression(this, frame, decoder, target);

					if (frame->interrupted())
					{
						return;
					}
				}
			}
			catch (...)
			{
				frame->DeferDrainDepth--;
				throw;
			}
			frame->DeferDrainDepth--;

			decoder.SetCursor(savedIP);
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

ObjectInstance VirtualMachine::CreateRuntimeException(TypeSymbol* type, const std::wstring& message, const std::wstring& stackTrace)
{
	if (type == nullptr)
		throw std::runtime_error("CreateRuntimeException: type is null");

	ClassSymbol* classDef = GetExceptionClassDefinition(type);
	if (classDef == nullptr)
		throw std::runtime_error("CreateRuntimeException: type is not a class");

	ConstructorSymbol* ctor = nullptr;
	for (ConstructorSymbol* candidate : classDef->Constructors)
	{
		if (candidate->Parameters.empty())
		{
			ctor = candidate;
			break;
		}
	}

	if (ctor == nullptr)
		throw std::runtime_error("CreateRuntimeException: exception type has no parameterless constructor");

	ObjectInstance instance = InstantiateObject(type, ctor);

	FieldSymbol* messageField = FindThrowableBackingField(type, TRAIT_THROWABLE_getMessage);
	if (messageField != nullptr)
	{
		ObjectInstance msgInstance = garbageCollector.FromString(message);
		instance.SetField(messageField->SlotIndex, msgInstance);
	}

	FieldSymbol* stackTraceField = FindThrowableBackingField(type, TRAIT_THROWABLE_getStackTrace);
	if (stackTraceField != nullptr)
	{
		ObjectInstance traceInstance = garbageCollector.FromString(stackTrace);
		instance.SetField(stackTraceField->SlotIndex, traceInstance);
	}

	return instance;
}

ObjectInstance VirtualMachine::CreateRuntimeException(const std::exception& err)
{
	const undefined_behaviour* contractViolation = dynamic_cast<const undefined_behaviour*>(&err);
	if (contractViolation != nullptr)
	{
		std::wstring stackTrace = contractViolation->stack_trace();
		if (stackTrace.empty())
			stackTrace = GetStackTrace();

		return CreateRuntimeException(SymbolTable::StandardTypes::UndefinedBehaviour, contractViolation->message(), stackTrace);
	}

	const runtime_exception* typed = dynamic_cast<const runtime_exception*>(&err);
	if (typed != nullptr)
	{
		TypeSymbol* type = typed->exception_type();
		if (type == nullptr)
			type = SymbolTable::StandardTypes::RuntimeException;

		std::wstring stackTrace = typed->stack_trace();
		if (stackTrace.empty())
			stackTrace = GetStackTrace();

		return CreateRuntimeException(type, typed->message(), stackTrace);
	}

	std::wstring message;
	const char* what = err.what();
	if (what != nullptr)
		message = std::wstring(what, what + std::strlen(what));

	return CreateRuntimeException(SymbolTable::StandardTypes::RuntimeException, message, GetStackTrace());
}

void VirtualMachine::InvokeMethodInternal(MethodSymbol* method, CallStackFrame* currentFrame)
{
	if (AbortFlag)
		throw std::runtime_error("Execution aborted by host.");

	CallStackFrame* callingFrame = currentFrame->PreviousFrame;

	std::size_t argsCount = method->GetEvalStackArgumentsCount();
	ObjectInstance thisInstance;

	for (std::size_t i = 0; i < argsCount; i++)
	{
		ObjectInstance argument = callingFrame->PopValue();
		currentFrame->SetLocal(static_cast<std::uint16_t>(i), argument, garbageCollector);

		if (method->Linking == LINK_INSTANCE && i == 0 && argument.getInfo() != nullptr && argument.getInfo()->IsReferenceType())
			thisInstance = argument;
	}

	if (!thisInstance.IsNullInstance())
	{
		TypeShape* thisShape = thisInstance.getShape();
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
					if (!HandleExceptionInFrame(this, currentFrame, decoder, garbageCollector))
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
				{
					std::string methodName(method->FullName.begin(), method->FullName.end());
					throw std::runtime_error("extern method body not resolved: " + methodName);
				}

				std::vector<ObjectInstance> argumentValues(argsCount);
				currentFrame->CopyArgumentPayloads(argumentValues.data(), argsCount);
				ArgumentsSpan args(argumentValues.data(), argsCount);
				
				TypeShape* returnShape = currentFrame->ReturnShape();
				const bool returnsValue = returnShape != nullptr;
				const bool returnsReference = method->ReturnType != nullptr &&
					method->ReturnType != SymbolTable::Primitives::Void && !returnsValue;

				TypeShape* slotHeader = returnsValue ? returnShape : nullptr;
				std::memcpy(currentFrame->ReturnSlotMemory(), &slotHeader, sizeof(slotHeader));

				CallState context
				{
					.Domain = *domain,
					.Program = program,
					.Runtimer = *this,
					.Collector = garbageCollector,

					.Frame = currentFrame,
					.Method = method,
					.Args = args,

					.ReturnTarget = { currentFrame->ReturnSlotMemory() + CallStackFrame::SlotHeaderBytes, returnShape }
				};

				method->FunctionPointer(context);

				if (currentFrame->InterruptionReason != FrameInterruptionReason::ExceptionRaised)
				{
					if (returnsReference)
					{
						ObjectInstance retReg;
						std::memcpy(&retReg, currentFrame->ReturnSlotMemory() + CallStackFrame::SlotHeaderBytes, sizeof(retReg));
						if (retReg.IsNullInstance())
							retReg = ObjectInstance();

						callingFrame->PushReference(retReg);
						BindTaskToFrame(retReg, callingFrame, garbageCollector);
					}
					else if (returnsValue)
					{
						ObjectInstance payload = callingFrame->PushInlineUninitialized(returnShape);
						std::memcpy(payload.getMemory(), currentFrame->ReturnSlotMemory() + CallStackFrame::SlotHeaderBytes, returnShape->Size);
					}
				}
			}
			catch (const std::exception& err)
			{
				ObjectInstance exception = CreateRuntimeException(err);

				currentFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				currentFrame->InterruptionRegister = exception;
				currentFrame->CurrentException = exception;
				exception.IncrementReference();
			}

			break;
		}
	}

	if (currentFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
	{
		ObjectInstance exception = currentFrame->InterruptionRegister;
		if (callingFrame != nullptr && !exception.IsNullInstance())
		{
			callingFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
			callingFrame->InterruptionRegister = exception;
			callingFrame->CurrentException = exception;
			exception.IncrementReference();
		}
	}
	else
	{
		ObjectInstance returnedValue;
		bool hasReturnedValue = false;
		if (method->HandleType != MethodHandleType::External &&
			method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void &&
			currentFrame->InterruptionReason != FrameInterruptionReason::ExceptionRaised)
		{
			if (currentFrame->EvalCount() > 0)
			{
				returnedValue = currentFrame->PopValue();
				hasReturnedValue = true;

				if (returnedValue.getInfo() != nullptr && !returnedValue.getInfo()->IsReferenceType())
				{
					ObjectInstance payload = callingFrame->PushInlineUninitialized(returnedValue.getShape());
					std::memcpy(payload.getMemory(), returnedValue.getMemory(), returnedValue.getShape()->Size);
				}
				else
				{
					if (!returnedValue.IsNullInstance())
						returnedValue.IncrementReference();

					callingFrame->PushReference(returnedValue);
					BindTaskToFrame(returnedValue, callingFrame, garbageCollector);
				}
			}
		}

		bool skippedReturnedValue = false;
		while (currentFrame->EvalCount() != 0)
		{
			ObjectInstance top = currentFrame->PopValue();

			if (!skippedReturnedValue && hasReturnedValue &&
				returnedValue.getInfo() != nullptr && returnedValue.getInfo()->IsReferenceType() &&
				top.getInfo() != nullptr && top.getInfo()->IsReferenceType() &&
				top.getMemory() == returnedValue.getMemory())
			{
				skippedReturnedValue = true;
				continue;
			}

			CallStackFrame::ReleaseValue(top, garbageCollector);
		}

		currentFrame->DrainLocalReferences(garbageCollector);
	}
}

ObjectInstance VirtualMachine::InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor, bool inPlace)
{
	CallStackFrame* callingFrame = CurrentFrame();

	auto resolver = MakeFrameResolver(callingFrame);

	TypeSymbol* baseType = type;
	std::vector<TypeSymbol*> genericArgs;
	bool isGeneric = SemanticModel::TryResolveGenericArguments(type, resolver, baseType, genericArgs);

	const bool constructInPlace = inPlace && baseType != nullptr && !baseType->IsReferenceType();

	ObjectInstance newInstance;
	std::byte* inlinePayload = nullptr;
	TypeShape* inlineShape = nullptr;

	if (constructInPlace)
	{
		inlineShape = isGeneric
			? program.TypeShapes->GetOrCreateShape(baseType, genericArgs)
			: program.TypeShapes->GetOrCreateShape(SemanticModel::ResolveRuntimeTypeArgument(type, resolver));

		ObjectInstance payload = callingFrame->PushInlineUninitialized(inlineShape);
		inlinePayload = payload.getMemory();
	}
	else
	{
		newInstance = isGeneric
			? garbageCollector.AllocateGeneric(baseType, genericArgs)
			: garbageCollector.AllocateInstance(SemanticModel::ResolveRuntimeTypeArgument(type, resolver));

		inlineShape = newInstance.getShape();
		inlinePayload = newInstance.getMemory();
		callingFrame->PushStack(newInstance);
	}

	if (inlineShape != nullptr)
	{
		for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(inlineShape->Slots.size()); ++slot)
		{
			TypeShape* fieldShape = inlineShape->GetFieldShape(slot);
			std::size_t fieldOffset = inlineShape->GetOffset(slot);
			std::size_t fieldSize = fieldShape != nullptr
				? (fieldShape->IsReferenceType() ? sizeof(void*) : fieldShape->Size)
				: sizeof(void*);

			std::memset(inlinePayload + fieldOffset, 0, fieldSize);
		}
	}

	for (TypeSymbol*& pendingArg : PendingTypeArguments)
		pendingArg = SemanticModel::ResolveRuntimeTypeArgument(pendingArg, resolver);

	CallStackFrame* currentFrame = PushFrame(ctor);
	if (isGeneric && currentFrame->TypeArguments.empty())
		currentFrame->TypeArguments = genericArgs;

	if (!constructInPlace)
		newInstance.IncrementReference();

	InvokeMethodInternal(ctor, currentFrame);
	PopFrame();

	if (constructInPlace)
	{
		ObjectInstance resultPayload = callingFrame->PushInlineUninitialized(inlineShape);
		std::memmove(resultPayload.getMemory(), inlinePayload, inlineShape->Size);
		return ObjectInstance();
	}

	newInstance.DecrementReference();
	return newInstance;
}

ObjectInstance VirtualMachine::InstantiateDelegate(DelegateTypeSymbol* type)
{
	ObjectInstance newInstance = garbageCollector.AllocateInstance(type);
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

ObjectInstance VirtualMachine::InvokeOperatorMethod(ObjectInstance leftInstance, TokenType opToken, ObjectInstance rightInstance)
{
	std::wstring opName = GetOperatorMethodName(opToken);
	if (opName.empty())
		throw std::runtime_error("operator is not overloadable");

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(leftInstance.getInfo());
	std::vector<TypeSymbol*> paramTypes =
	{
		ownerType,
		const_cast<TypeSymbol*>(rightInstance.getInfo())
	};

	OperatorSymbol* method = ownerType->FindOperator(opToken, paramTypes);
	if (method == nullptr)
		throw std::runtime_error("operator overload not found");

	InvokeMethod(method, { rightInstance, leftInstance });
	return CurrentFrame()->PopValue();
}

ObjectInstance VirtualMachine::InvokeOperatorMethod(ObjectInstance sourceInstance, TokenType opToken)
{
	std::wstring opName = GetOperatorMethodName(opToken);
	if (opName.empty())
		throw std::runtime_error("operator is not overloadable");

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(sourceInstance.getInfo());
	OperatorSymbol* method = ownerType->FindOperator(opToken, { ownerType });
	if (method == nullptr)
		throw std::runtime_error("operator overload not found");

	InvokeMethod(method, { sourceInstance });
	return CurrentFrame()->PopValue();
}

CallStackFrame* VirtualMachine::CurrentFrame() const
{
	if (CallStack.empty())
		return nullptr;

	return CallStack.back().get();
}

CallStackFrame* VirtualMachine::PushFrame(MethodSymbol* methodSymbol)
{
	auto frame = CallStackFrame::Create(this, CurrentFrame(), methodSymbol, PendingTypeArguments);
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

	auto frame = std::move(CallStack.back());
	CallStack.pop_back();

	if (frame->PendingTaskCount > 0)
		frame->PreviousFrame = nullptr;
}

void VirtualMachine::InvokeMethod(MethodSymbol* method) const
{
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);

	CallStackFrame* currentFrame = vm->PushFrame(method);
	vm->InvokeMethodInternal(method, currentFrame);
	vm->PopFrame();
}

void VirtualMachine::InvokeMethod(MethodSymbol* method, std::initializer_list<ObjectInstance> args) const
{
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);

	CallStackFrame* callingFrame = vm->CurrentFrame();
	CallStackFrame* currentFrame = vm->PushFrame(method);

	for (ObjectInstance argValue : args)
		callingFrame->PushCopy(argValue);

	vm->InvokeMethodInternal(method, currentFrame);
	vm->PopFrame();
}

ObjectInstance VirtualMachine::InvokeMethod(MethodSymbol* method, ObjectInstance* args, std::size_t count) const
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
		callingFrame->PushCopy(args[i]);

	vm->InvokeMethodInternal(method, currentFrame);

	ObjectInstance result;
	if (method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void && callingFrame->EvalCount() > 0)
		result = callingFrame->PopValue();

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

void VirtualMachine::SetPendingTypeArguments(const std::vector<TypeSymbol*>& args) const
{
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);
	vm->PendingTypeArguments = args;
}

void VirtualMachine::RaiseException(ObjectInstance exceptionReg) const
{
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);
	CallStackFrame* frame = vm->CurrentFrame();
	if (frame == nullptr || exceptionReg.IsNullInstance())
		return;

	exceptionReg.IncrementReference();
	frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
	frame->InterruptionRegister = exceptionReg;
	frame->CurrentException = exceptionReg;
}

std::wstring VirtualMachine::GetThrowablePropertyValue(ObjectInstance exception, AccessorSymbol* interfacePropertyAccessor) const
{
	if (exception.IsNullInstance() || interfacePropertyAccessor == nullptr)
		return L"";

	TypeSymbol* exceptionType = const_cast<TypeSymbol*>(exception.getInfo());
	if (exceptionType == nullptr)
		return L"";

	MethodSymbol* implementation = exceptionType->FindInterfaceImplementation(interfacePropertyAccessor);
	if (implementation == nullptr)
		return L"";

	VirtualMachine* vm = const_cast<VirtualMachine*>(this);
	vm->InvokeMethod(implementation, { exception });

	CallStackFrame* frame = vm->CurrentFrame();
	if (frame == nullptr || frame->EvalCount() == 0)
		return L"";

	ObjectInstance result = frame->PopValue();
	if (result.IsNullInstance())
		return L"";

	if (result.getInfo() == nullptr || result.getInfo() != SymbolTable::Primitives::String)
		return L"";

	const wchar_t* data = result.AsString();
	std::wstring value = data != nullptr ? std::wstring(data) : L"";

	result.DecrementReference();

	return value;
}

void VirtualMachine::HaltFireAndForgetTasks()
{
	EventLoop& loop = domain->GetEventLoop();
	const std::vector<ObjectInstance>& rooted = loop.GetRootedTasks();
	std::vector<ObjectInstance> toHalt(rooted.begin(), rooted.end());

	for (ObjectInstance task : toHalt)
	{
		if (task.IsNullInstance() || !garbageCollector.IsFireAndForget(task))
			continue;

		void* nativeState = garbageCollector.GetAsyncNativeState(task);
		if (nativeState != nullptr)
		{
			static_cast<detail::AsyncScopeState*>(nativeState)->Halt();
		}
		else
		{
			HaltTaskObject(task, garbageCollector);
			loop.UnrootTask(task);
		}
	}
}

void VirtualMachine::Run()
{
	if (program.EntryPoint == nullptr)
		throw std::runtime_error("entry point was null");

	if (!UnhandledException.IsNullInstance())
	{
		garbageCollector.CollectInstance(UnhandledException);
		UnhandledException = ObjectInstance();
	}

	UnhandledExceptionMessage.clear();
	UnhandledExceptionStackTrace.clear();

	AbortFlag = false;
	CallStackFrame* entryFrame = PushFrame(program.EntryPoint);
	InvokeMethodInternal(program.EntryPoint, entryFrame);

	HaltFireAndForgetTasks();

	if (entryFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
	{
		ObjectInstance exception = entryFrame->InterruptionRegister;
		if (!exception.IsNullInstance())
		{
			exception.IncrementReference();
			UnhandledException = exception;

			if (SemanticModel::IsAssignableTo(TRAIT_THROWABLE, exception.getInfo()))
			{
				UnhandledExceptionMessage = GetThrowablePropertyValue(exception, TRAIT_THROWABLE_getMessage);
				UnhandledExceptionStackTrace = GetThrowablePropertyValue(exception, TRAIT_THROWABLE_getStackTrace);
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
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);
	vm->AbortFlag = true;
}

ObjectInstance VirtualMachine::RunInteractive(std::size_t& pointer)
{
	CallStackFrame* currentFrame = CurrentFrame();
	MethodSymbol* method = currentFrame->Method;

	ByteCodeDecoder decoder = ByteCodeDecoder(method->ExecutableByteCode);
	decoder.SetCursor(pointer);

	while (!decoder.IsEOF())
	{
		if (AbortFlag)
			throw std::runtime_error("Execution aborted by host.");

		OpCode opCode = decoder.AbsorbOpCode();
		if (opCode == OpCode::POP && decoder.IsEOF())
			continue;

		ProcessCode(currentFrame, decoder, opCode);
	}

	pointer = decoder.Index();
	if (currentFrame->EvalCount() > 0)
	{
		ObjectInstance retReg = currentFrame->PopValue();
		return retReg;
	}

	return ObjectInstance();
}

void VirtualMachine::TerminateCallStack()
{
	while (!CallStack.empty())
		PopFrame();
}
