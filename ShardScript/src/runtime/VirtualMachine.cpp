#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/runtime/PrimitiveMathModule.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ProgramDisassembler.hpp>

#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/TokenType.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>

#include <shard/ApplicationDomain.hpp>

#include <iostream>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <exception>

using namespace shard;

void VirtualMachine::ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode)
{
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
			InvokeMethod(methodSymbol);
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
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* instance = frame->PopStack();
			ObjectInstance* fieldValue = instance->GetField(field, frame);

			frame->PushStack(fieldValue);
			garbageCollector.CollectInstance(instance);
			break;
		}

		case OpCode::STOREFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* fieldValue = frame->PopStack();
			ObjectInstance* instance = frame->PopStack();

			instance->SetField(field, fieldValue, frame);
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
			ObjectInstance* instance = garbageCollector.AllocateInstance(type);
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

			std::int64_t length = upper - lower + (inclusive ? 1 : 0);
			if (length < 0)
				length = 0;

			ObjectInstance* arrayInstance = garbageCollector.AllocateArray(elementType, static_cast<std::size_t>(length));
			for (std::int64_t i = 0; i < length; i++)
			{
				ObjectInstance* valueInstance = garbageCollector.FromValue(lower + i);
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
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::AddOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::MATH_SUBSTRACTION:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::SubOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::MATH_MULTIPLICATION:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::MultOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::MATH_DIVISION:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::DivOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::MATH_MODULE:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::ModOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::MATH_POWER:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::PowOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::COMPARE_EQUAL:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::EqualsOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::COMPARE_NOTEQUAL:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::NotEqualsOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::COMPARE_LESS:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::LessOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::COMPARE_LESSOREQUAL:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::LessOrEqualsOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::COMPARE_GREATER:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::GreaterOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::COMPARE_GREATEROREQUAL:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = math.EvaluateBinaryOperator(left, TokenType::GreaterOrEqualsOperator, right);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
			garbageCollector.CollectInstance(left);
			break;
		}

		case OpCode::LOGICAL_NOT:
		{
			ObjectInstance* right = frame->PopStack();

			bool rightDetermined = false;
			ObjectInstance* result = math.EvaluateUnaryOperator(right, TokenType::NotOperator, rightDetermined);
			frame->PushStack(result);

			garbageCollector.CollectInstance(right);
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

		default:
			throw std::runtime_error("CRITICAL SHIT! UNKNOWN OPCODE");
	}
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
		currentFrame->WithinType = const_cast<TypeSymbol*>(thisInstance->getInfo());
		if (currentFrame->TypeArguments.empty() && thisInstance->getInfo()->Kind == SyntaxKind::GenericType)
		{
			GenericTypeSymbol* genericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(thisInstance->getInfo()));
			TypeSymbol* underlyingType = genericInfo->UnderlayingType;

			currentFrame->TypeArguments.resize(underlyingType->TypeParameters.size());
			for (std::size_t i = 0; i < underlyingType->TypeParameters.size(); i++)
				currentFrame->TypeArguments[i] = genericInfo->SubstituteTypeParameters(underlyingType->TypeParameters[i]);
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
			while (!decoder.IsEOF())
			{
				if (AbortFlag)
					throw std::runtime_error("Execution aborted by host.");

				OpCode opCode = decoder.AbsorbOpCode();
				ProcessCode(currentFrame, decoder, opCode);
			}

			if (method->ReturnType != nullptr && method->ReturnType != SymbolTable::Primitives::Void)
			{
				callingFrame->PushStack(currentFrame->PopStack());
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
				std::string description = err.what();
				std::wstring wdescription = std::wstring(description.begin(), description.end());

				currentFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				currentFrame->InterruptionRegister = garbageCollector.FromValue(wdescription);
				currentFrame->InterruptionRegister->IncrementReference();
			}

			break;
		}
	}

	while (currentFrame->EvalStack.size() != 0)
		garbageCollector.DestroyInstance(currentFrame->PopStack());
}

ObjectInstance* VirtualMachine::InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor)
{
	GenericTypeSymbol* genericInfo = nullptr;

	CallStackFrame* callingFrame = CurrentFrame();
	ObjectInstance* newInstance = garbageCollector.AllocateInstance(type);

	if (type->Kind == SyntaxKind::GenericType)
		genericInfo = static_cast<GenericTypeSymbol*>(type);

	TypeSymbol* fieldOwnerType = genericInfo != nullptr ? genericInfo->UnderlayingType : type;
	for (FieldSymbol* field : fieldOwnerType->Fields)
	{
		if (field->Linking == LINK_STATIC)
			continue;

		TypeSymbol* fieldType = field->ReturnType;
		if (fieldType->Kind == SyntaxKind::TypeParameter && genericInfo != nullptr)
			fieldType = genericInfo->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(fieldType));

		if (fieldType == nullptr)
			continue;

		if (fieldType->IsReferenceType)
		{
			void* offset = newInstance->OffsetMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*));
			memset(offset, 0, sizeof(ObjectInstance*));
		}
		else
		{
			void* offset = newInstance->OffsetMemory(field->MemoryBytesOffset, fieldType->GetInlineSize());
			memset(offset, 0, fieldType->GetInlineSize());
		}
	}

	callingFrame->PushStack(newInstance);
	
	CallStackFrame* currentFrame = PushFrame(ctor, type);
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
	math(garbageCollector)
{
	AbortFlag = false;
}

CallStackFrame* VirtualMachine::CurrentFrame() const
{
	if (CallStack.empty())
		return nullptr;

	return CallStack.top().get();
}

CallStackFrame* VirtualMachine::PushFrame(MethodSymbol* methodSymbol)
{
	auto frame = std::make_unique<CallStackFrame>(this, CurrentFrame(), nullptr, methodSymbol);
	frame->TypeArguments = std::move(PendingTypeArguments);
	PendingTypeArguments.clear();
	CallStackFrame* rawFrame = frame.get();
	CallStack.push(std::move(frame));
	return rawFrame;
}

CallStackFrame* VirtualMachine::PushFrame(MethodSymbol* methodSymbol, TypeSymbol* withinType)
{
	auto frame = std::make_unique<CallStackFrame>(this, CurrentFrame(), withinType, methodSymbol);
	frame->TypeArguments = std::move(PendingTypeArguments);
	PendingTypeArguments.clear();
	CallStackFrame* rawFrame = frame.get();
	CallStack.push(std::move(frame));
	return rawFrame;
}

void VirtualMachine::PopFrame()
{
	if (CallStack.empty())
		return;

	CallStack.pop();
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

void VirtualMachine::RaiseException(ObjectInstance* exceptionReg) const
{
	// TODO: implement method
}

void VirtualMachine::Run()
{
	if (program.EntryPoint == nullptr)
		throw std::runtime_error("entry point was null");

	AbortFlag = false;
	InvokeMethod(program.EntryPoint);
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
	ProgramDisassembler disassembler;
	disassembler.Disassemble(std::wcout, program);

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
