#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/PrimitiveMathModule.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/ArgumentsSpan.hpp>
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

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <initializer_list>
#include <string>

using namespace shard;

void VirtualMachine::ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode)
{
	switch (opCode)
	{
		case OpCode::Nop:
		{
			// 0xBAD + 0xC0DE;
			break;
		}

		case OpCode::Halt:
		{
			AbortFlag = true;
			break;
		}

		case OpCode::PopStack:
		{
			ObjectInstance* pop = frame->PopStack();
			GarbageCollector::CollectInstance(pop);
			break;
		}

		case OpCode::CallMethodSymbol:
		{
			MethodSymbol* methodSymbol = decoder.AbsorbMethodSymbol();
			InvokeMethod(methodSymbol);
			break;
		}

		case OpCode::LoadConst_Null:
		{
			frame->PushStack(GarbageCollector::NullInstance);
			break;
		}

		case OpCode::LoadConst_Boolean:
		{
			bool value = decoder.AbsorbBoolean();
			ObjectInstance* instance = ObjectInstance::FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LoadConst_Integer64:
		{
			int64_t value = decoder.AbsorbInt64();
			ObjectInstance* instance = ObjectInstance::FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LoadConst_Rational64:
		{
			double value = decoder.AbsorbDouble64();
			ObjectInstance* instance = ObjectInstance::FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LoadConst_Char:
		{
			wchar_t value = decoder.AbsorbChar16();
			ObjectInstance* instance = ObjectInstance::FromValue(value);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LoadConst_String:
		{
			size_t data = decoder.AbsorbString();
			const wchar_t* str = reinterpret_cast<wchar_t*>(Program.DataSection.data() + data);

			ObjectInstance* instance = ObjectInstance::FromValue(str, true);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LoadVariable:
		{
			uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance* instance = frame->EvalStack[slot];
			frame->PushStack(instance);
			break;
		}

		case OpCode::StoreVariable:
		{
			uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance* instance = frame->PopStack();

			ObjectInstance* oldVar = frame->EvalStack[slot];
			if (oldVar != nullptr)
			{
				oldVar->DecrementReference();
				GarbageCollector::CollectInstance(oldVar);
			}

			instance->IncrementReference();
			frame->EvalStack[slot] = instance;
			break;
		}

		case OpCode::NewObject:
		{
			TypeSymbol* type = decoder.AbsorbTypeSymbol();
			ConstructorSymbol* ctor = decoder.AbsorbConstructorSymbol();

			ObjectInstance* instance = InstantiateObject(type, ctor);
			frame->PushStack(instance);
			break;
		}

		case OpCode::LoadField:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* instance = frame->PopStack();
			ObjectInstance* fieldValue = instance->GetField(field);

			frame->PushStack(fieldValue);
			GarbageCollector::CollectInstance(instance);
			break;
		}

		case OpCode::StoreField:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* fieldValue = frame->PopStack();
			ObjectInstance* instance = frame->PopStack();

			instance->SetField(field, fieldValue);
			GarbageCollector::CollectInstance(fieldValue);
			GarbageCollector::CollectInstance(instance);
			break;
		}

		case OpCode::LoadStaticField:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* fieldValue = GarbageCollector::GetStaticField(this, field);

			frame->PushStack(fieldValue);
			break;
		}

		case OpCode::StoreStaticField:
		{
			FieldSymbol* field = decoder.AbsorbFieldSymbol();
			ObjectInstance* fieldValue = frame->PopStack();

			GarbageCollector::SetStaticField(this, field, fieldValue);
			GarbageCollector::CollectInstance(fieldValue);
			break;
		}

		case OpCode::CreateDuplicate:
		{
			ObjectInstance* instance = frame->PeekStack();
			ObjectInstance* duplicate = GarbageCollector::CopyInstance(instance);

			frame->PushStack(duplicate);
			break;
		}

		case OpCode::Math_Addition:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::AddOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Substraction:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::SubOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Multiplication:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::MultOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Division:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::DivOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Module:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::ModOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Power:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::PowOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_Equal:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::EqualsOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_NotEqual:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::NotEqualsOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_Less:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::LessOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_LessOrEqual:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::LessOrEqualsOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_Greater:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::GreaterOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_GreaterOrEqual:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::GreaterOrEqualsOperator, right);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Logical_Not:
		{
			ObjectInstance* right = frame->PopStack();

			bool rightDetermined = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateUnaryOperator(right, TokenType::NotOperator, rightDetermined);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			break;
		}

		case OpCode::Jump:
		{
			size_t jump = decoder.AbsorbJump();
			decoder.SetCursor(jump);
			break;
		}

		case OpCode::Jump_False:
		{
			size_t jump = decoder.AbsorbJump();
			ObjectInstance* value = frame->PopStack();

			if (value->AsBoolean())
				break;

			decoder.SetCursor(jump);
			break;
		}

		case OpCode::Jump_True:
		{
			size_t jump = decoder.AbsorbJump();
			ObjectInstance* value = frame->PopStack();

			if (!value->AsBoolean())
				break;

			decoder.SetCursor(jump);
			break;
		}

		case OpCode::Return:
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
	switch (method->HandleType)
	{
		case MethodHandleType::None:
		{
			throw std::runtime_error("Method handle type was not resolved");
		}

		case MethodHandleType::Lambda:
		case MethodHandleType::Body:
		{
			if (!method->IsStatic)
			{
				ObjectInstance* prevInstance = callingFrame->PopStack();
				prevInstance->IncrementReference();
				currentFrame->PushStack(prevInstance);
			}

			for (int i = 0; i < method->Parameters.size(); i++)
			{
				ObjectInstance* argument = callingFrame->PopStack();
				argument->IncrementReference();
				currentFrame->PushStack(argument);
			}

			// pushing variables slots
			size_t variablesCount = method->EvalStackLocalsCount - method->Parameters.size();
			if (!method->IsStatic)
				variablesCount -= 1;

			for (int i = 0; i < variablesCount; i++)
				currentFrame->PushStack(nullptr);

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

			for (int i = 0; i < method->EvalStackLocalsCount; i++)
			{
				ObjectInstance* local = currentFrame->PopStack();
				local->DecrementReference();
				GarbageCollector::CollectInstance(local);
			}

			if (!method->IsStatic)
			{
				ObjectInstance* prevInstance = currentFrame->PopStack();
				prevInstance->DecrementReference();
				GarbageCollector::CollectInstance(prevInstance);
			}

			break;
		}

		case MethodHandleType::External:
		{
			try
			{
				if (method->FunctionPointer == nullptr)
					throw std::runtime_error("extern method body not resolved");

				if (!method->IsStatic)
				{
					ObjectInstance* prevInstance = callingFrame->PopStack();
					prevInstance->IncrementReference();
					currentFrame->PushStack(prevInstance);
				}

				size_t argsCount = method->Parameters.size();
				if (!method->IsStatic)
					argsCount += 1;

				std::vector<ObjectInstance*> argValues;
				for (int i = 0; i < argsCount; i++)
				{
					ObjectInstance* arg = callingFrame->PopStack();
					arg->IncrementReference();
					argValues.push_back(arg);
				}

				ArgumentsSpan arguments(method, argValues);
				ObjectInstance* retReg = method->FunctionPointer(this, method, arguments);

				if (method->ReturnType != SymbolTable::Primitives::Void)
				{
					if (retReg == nullptr)
						throw std::runtime_error("method returned nullptr (void), when expected instance");

					callingFrame->PushStack(retReg);
				}

				for (ObjectInstance* arg : argValues)
				{
					arg->DecrementReference();
					GarbageCollector::CollectInstance(arg);
				}
			}
			catch (const std::runtime_error& err)
			{
				std::string description = err.what();
				std::wstring wdescription = std::wstring(description.begin(), description.end());

				currentFrame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				currentFrame->InterruptionRegister = ObjectInstance::FromValue(wdescription);
				currentFrame->InterruptionRegister->IncrementReference();
			}

			break;
		}
	}

	/*
	switch (currentFrame->InterruptionReason)
	{
		case FrameInterruptionReason::ExceptionRaised:
		{
			RaiseException(currentFrame->InterruptionRegister);
			break;
		}

		case FrameInterruptionReason::ValueReturned:
		{
			ObjectInstance* retReg = currentFrame->InterruptionRegister;
			callingFrame->PushStack(retReg);
			retReg->DecrementReference();
			break;
		}
	}
	*/
}

ObjectInstance* VirtualMachine::InstantiateObject(TypeSymbol* type, ConstructorSymbol* ctor)
{
	GenericTypeSymbol* genericInfo = nullptr;
	TypeSymbol* withinType = type;

	CallStackFrame* callingFrame = CurrentFrame();
	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(type);

	if (type->Kind == SyntaxKind::GenericType)
	{
		genericInfo = static_cast<GenericTypeSymbol*>(type);
		withinType = genericInfo->UnderlayingType;
	}

	CallStackFrame* currentFrame = PushFrame(ctor);
	InvokeMethodInternal(ctor, currentFrame);
	PopFrame();

	return newInstance;

	// TODO: add field initialization
	/*
	for (FieldSymbol* field : withinType->Fields)
	{
		ObjectInstance* assignInstance = nullptr;
		TypeSymbol* fieldType = field->ReturnType;

		if (fieldType->Kind == SyntaxKind::TypeParameter)
			fieldType = genericInfo->SubstituteTypeParameters(fieldType);

		if (field->DefaultValueExpression != nullptr)
		{
			assignInstance = AbstractInterpreter::EvaluateExpression(field->DefaultValueExpression);
		}
		else
		{
			assignInstance = fieldType->IsReferenceType
				? GarbageCollector::NullInstance
				: GarbageCollector::AllocateInstance(fieldType);
		}

		newInstance->SetField(fieldInstance);
	}
	*/

	/*
	newInstance->IncrementReference();
	InboundVariablesContext* arguments = CreateArgumentsContext(expression->ArgumentsList->Arguments, expression->CtorSymbol->Parameters, expression->CtorSymbol->IsStatic, newInstance);
	ExecuteMethod(expression->CtorSymbol, withinType, arguments);
	newInstance->DecrementReference();

	AbstractInterpreter::PopFrame();
	return newInstance;
	*/
}

VirtualMachine::VirtualMachine(ProgramVirtualImage& program) : Program(program)
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

void VirtualMachine::Run() const
{
	if (Program.EntryPoint == nullptr)
		throw std::runtime_error("entry point was null");

	// hehe
	VirtualMachine* vm = const_cast<VirtualMachine*>(this);

	vm->AbortFlag = false;
	InvokeMethod(Program.EntryPoint);
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

	ByteCodeDecoder decoder = ByteCodeDecoder(method->ExecutableByteCode);
	decoder.SetCursor(pointer);

	while (!decoder.IsEOF())
	{
		if (AbortFlag)
			throw std::runtime_error("Execution aborted by host.");

		OpCode opCode = decoder.AbsorbOpCode();
		if (opCode == OpCode::PopStack && decoder.IsEOF())
			continue;

		ProcessCode(currentFrame, decoder, opCode);
	}

	pointer = decoder.Index();
	if (currentFrame->EvalStack.size() > method->EvalStackLocalsCount)
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
