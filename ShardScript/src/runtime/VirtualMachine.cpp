#include <shard/runtime/VirtualMachine.h>
#include <shard/runtime/PrimitiveMathModule.h>
#include <shard/runtime/CallStackFrame.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/ArgumentsSpan.h>
#include <shard/runtime/GarbageCollector.h>

#include <shard/compilation/ByteCodeDecoder.h>
#include <shard/compilation/OperationCode.h>
#include <shard/compilation/ProgramVirtualImage.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/TokenType.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>

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
			0xBAD + 0xC0DE;
			break;
		}

		case OpCode::Halt:
		{
			AbortFlag = true;
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

			ObjectInstance* instance = ObjectInstance::FromValue(str);
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

		case OpCode::Math_Addition:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::AddOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Substraction:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::SubOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Multiplication:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::MultOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Division:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::DivOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Module:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::ModOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Math_Power:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::PowOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_Equal:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::EqualsOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_NotEqual:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::NotEqualsOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_Less:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::LessOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_LessOrEqual:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::LessOrEqualsOperator, right, assign);
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
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::GreaterOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_GreaterOrEqual:
		{
			ObjectInstance* right = frame->PopStack();
			ObjectInstance* left = frame->PopStack();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::GreaterOrEqualsOperator, right, assign);
			frame->PushStack(result);

			GarbageCollector::CollectInstance(right);
			GarbageCollector::CollectInstance(left);
			break;
		}

		case OpCode::Compare_Not:
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
				currentFrame->PushStack(prevInstance);
			}

			for (int i = 0; i < method->Parameters.size(); i++)
			{
				ObjectInstance* argument = callingFrame->PopStack();
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

			break;
		}

		case MethodHandleType::External:
		{
			try
			{
				if (method->FunctionPointer == nullptr)
					throw std::runtime_error("extern method body not resolved");

				size_t argsCount = method->Parameters.size();
				if (!method->IsStatic)
					argsCount += 1;

				std::vector<ObjectInstance*> argValues;
				for (int i = 0; i < argsCount; i++)
					argValues.push_back(callingFrame->PopStack());

				ArgumentsSpan arguments(method, argValues);
				ObjectInstance* retReg = method->FunctionPointer(this, method, arguments);

				if (method->ReturnType != SymbolTable::Primitives::Void)
				{
					if (retReg == nullptr)
						throw std::runtime_error("method returned nullptr (void), when expected instance");

					callingFrame->PushStack(retReg);
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

		newInstance->SetField(field, assignInstance);
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

	if (Program.EntryPoint == nullptr)
		throw std::runtime_error("entry point was null");
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

void VirtualMachine::InvokeMethod(MethodSymbol* method)
{
	CallStackFrame* currentFrame = PushFrame(method);
	InvokeMethodInternal(method, currentFrame);
	PopFrame();
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

void VirtualMachine::RaiseException(ObjectInstance* exceptionReg)
{

}

void VirtualMachine::Run()
{
	InvokeMethod(Program.EntryPoint);
}

void VirtualMachine::TerminateCallStack()
{
	while (!CallStack.empty())
		PopFrame();
}
