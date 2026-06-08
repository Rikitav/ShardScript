#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <cstring>
#include <shard/runtime/PrimitiveMathModule.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>

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
			int64_t value = decoder.AbsorbInt64();
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
			size_t data = decoder.AbsorbString();
			const wchar_t* str = reinterpret_cast<wchar_t*>(program.DataSection.data() + data);

			ObjectInstance* instance = garbageCollector.FromValue(str, true);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LOADVARIABLE:
		{
			uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance* instance = frame->EvalStack[slot];
			frame->PushStack(instance);
			break;
		}

		case OpCode::STOREVARIABLE:
		{
			uint16_t slot = decoder.AbsorbVariableSlot();
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

		case OpCode::LOADFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* instance = frame->PopStack();
			ObjectInstance* fieldValue = instance->GetField(field);

			frame->PushStack(fieldValue);
			garbageCollector.CollectInstance(instance);
			break;
		}

		case OpCode::STOREFIELD:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* fieldValue = frame->PopStack();
			ObjectInstance* instance = frame->PopStack();

			instance->SetField(field, fieldValue);
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
			size_t jump = decoder.AbsorbJump();
			decoder.SetCursor(jump);
			break;
		}

		case OpCode::JUMP_FALSE:
		{
			size_t jump = decoder.AbsorbJump();
			ObjectInstance* value = frame->PopStack();

			if (value->AsBoolean())
				break;

			decoder.SetCursor(jump);
			break;
		}

		case OpCode::JUMP_TRUE:
		{
			size_t jump = decoder.AbsorbJump();
			ObjectInstance* value = frame->PopStack();

			if (!value->AsBoolean())
				break;

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
	currentFrame->EvalStack.reserve(method->GetEvalStackLocalsCount() * 2);

	size_t argsCount = method->Parameters.size();
	if (!method->IsStatic)
		argsCount += 1;

	ObjectInstance* thisInstance = nullptr;
	for (size_t i = 0; i < argsCount; i++)
	{
		ObjectInstance* argument = callingFrame->PopStack();
		argument->IncrementReference();
		currentFrame->PushStack(argument);

		if (!method->IsStatic && i == 0)
			thisInstance = argument;
	}

	if (thisInstance != nullptr)
		currentFrame->WithinType = const_cast<TypeSymbol*>(thisInstance->Info);

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

			if (method->ReturnType != SymbolTable::Primitives::Void)
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
				if (method->ReturnType != SymbolTable::Primitives::Void)
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
		if (field->IsStatic)
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
	InvokeMethodInternal(ctor, currentFrame);
	PopFrame();

	return newInstance;
}

ObjectInstance* shard::VirtualMachine::InstantiateDelegate(DelegateTypeSymbol* type)
{
	ObjectInstance* newInstance = garbageCollector.AllocateInstance(type);
	return newInstance;
}

VirtualMachine::VirtualMachine(ApplicationDomain* appDomain) : domain(appDomain),
	program(domain->GetProgram()), garbageCollector(domain->GetGarbageCollector()), math(garbageCollector)
{
	AbortFlag = false;
}

CallStackFrame* VirtualMachine::CurrentFrame() const
{
	if (CallStack.empty())
		return nullptr;

	return CallStack.top();
}

CallStackFrame* VirtualMachine::PushFrame(MethodSymbol* methodSymbol)
{
	CallStackFrame* frame = new CallStackFrame(this, CurrentFrame(), nullptr, methodSymbol);	
	CallStack.push(frame);
	return frame;
}

CallStackFrame* VirtualMachine::PushFrame(MethodSymbol* methodSymbol, TypeSymbol* withinType)
{
	CallStackFrame* frame = new CallStackFrame(this, CurrentFrame(), withinType, methodSymbol);	
	CallStack.push(frame);
	return frame;
}

void VirtualMachine::PopFrame()
{
	if (CallStack.empty())
		return;

	CallStackFrame* current = CurrentFrame();
	CallStack.pop();
	delete current;
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

ObjectInstance* VirtualMachine::RunInteractive(size_t& pointer)
{
	CallStackFrame* currentFrame = CurrentFrame();
	MethodSymbol* method = currentFrame->Method;
	currentFrame->EvalStack.reserve(static_cast<size_t>(method->GetEvalStackLocalsCount()) * 2);

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
