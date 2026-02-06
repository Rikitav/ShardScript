#include <shard/runtime/VirtualMachine.h>
#include <shard/runtime/PrimitiveMathModule.h>

#include <shard/compilation/ByteCodeDecoder.h>

#include <vector>
#include <stdexcept>

using namespace shard;

CallStackFrame* VirtualMachine::CurrentFrame()
{
	if (CallStack.empty())
		return nullptr;

	return CallStack.top();
}

CallStackFrame* VirtualMachine::PushFrame(MethodSymbol* methodSymbol)
{

	CallStackFrame* frame = new CallStackFrame(CurrentFrame(), nullptr, methodSymbol);
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

void VirtualMachine::InvokeMethod(CallStackFrame* frame)
{
	switch (frame->Method->HandleType)
	{
		case MethodHandleType::None:
		{
			throw std::runtime_error("Method handle type was not resolved");
		}

		case MethodHandleType::Lambda:
		case MethodHandleType::Body:
		{
			// pushing variables slots
			for (int i = 0; i < frame->Method->EvalStackLocalsCount; i++)
				frame->EvalStack.push_back(nullptr);

			ByteCodeDecoder decoder = ByteCodeDecoder(frame->Method->ExecutableByteCode);
			while (!decoder.IsEOF())
			{
				OpCode opCode = decoder.AbsorbOpCode();
				ProcessCode(frame, decoder, opCode);
			}

			break;
		}

		case MethodHandleType::External:
		{
			// unsupported
			break;

			/*
			try
			{
				if (methodSymbol->FunctionPointer == nullptr)
					throw std::runtime_error("extern method body not resolved");

				ObjectInstance* retReg = method->FunctionPointer(method, argumentsContext);
				if (retReg != nullptr)
				{
					frame->InterruptionReason = FrameInterruptionReason::ValueReturned;
					frame->InterruptionRegister = retReg;
					frame->InterruptionRegister->IncrementReference();
				}
				else
				{
					if (method->ReturnType != SymbolTable::Primitives::Void)
						throw std::runtime_error("method returned nullptr (void), when expected instance");
				}
			}
			catch (const std::runtime_error& err)
			{
				std::string description = err.what();
				std::wstring wdescription = std::wstring(description.begin(), description.end());

				frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				frame->InterruptionRegister = ObjectInstance::FromValue(wdescription);
				frame->InterruptionRegister->IncrementReference();
			}

			break;
			*/
		}
	}

	switch (frame->InterruptionReason)
	{
		case FrameInterruptionReason::ExceptionRaised:
		{
			RaiseException(frame->InterruptionRegister);
			break;
		}

		case FrameInterruptionReason::ValueReturned:
		{
			ObjectInstance* retReg = frame->InterruptionRegister;
			frame->PreviousFrame->EvalStack.push_back(retReg);
			retReg->DecrementReference();
			break;
		}
	}
}

void VirtualMachine::ProcessCode(CallStackFrame* frame, ByteCodeDecoder& decoder, const OpCode opCode)
{
	switch (opCode)
	{
		case OpCode::CallMethodSymbol:
		{
			MethodSymbol* methodSymbol = decoder.AbsorbMethodSymbol();
			CallStackFrame* callingFrame = PushFrame(methodSymbol);

			if (!methodSymbol->IsStatic)
			{
				ObjectInstance* prevInstance = frame->EvalStack.back(); frame->EvalStack.pop_back();
				callingFrame->EvalStack.push_back(prevInstance);
			}

			for (int i = 0; i < methodSymbol->Parameters.size(); i++)
			{
				ObjectInstance* argument = frame->EvalStack.back(); frame->EvalStack.pop_back();
				callingFrame->EvalStack.push_back(argument);
			}

			InvokeMethod(callingFrame);
			PopFrame();
			break;
		}

		case OpCode::LoadConst_Integer64:
		{
			int64_t value = decoder.AbsorbInt64();
			ObjectInstance* instance = ObjectInstance::FromValue(value);
			frame->EvalStack.push_back(instance);
			break;
		}

		case OpCode::LoadVariable:
		{
			uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance* instance = frame->EvalStack[slot];
			frame->EvalStack.push_back(instance);
			break;
		}

		case OpCode::StoreVariable:
		{
			uint16_t slot = decoder.AbsorbVariableSlot();
			ObjectInstance* instance = frame->EvalStack.back(); frame->EvalStack.pop_back();
			frame->EvalStack[slot] = instance;
			break;
		}

		case OpCode::Math_Addition:
		{
			ObjectInstance* right = frame->EvalStack.back(); frame->EvalStack.pop_back();
			ObjectInstance* left = frame->EvalStack.back(); frame->EvalStack.pop_back();

			bool assign = false;
			ObjectInstance* result = PrimitiveMathModule::EvaluateBinaryOperator(left, TokenType::AddOperator, right, assign);
			frame->EvalStack.push_back(result);
			break;
		}

		default:
			throw std::runtime_error("CRITICAL SHIT! UNKNOWN OPCODE");
	}
}

void VirtualMachine::RaiseException(ObjectInstance* exceptionReg)
{

}

void VirtualMachine::Run()
{
	if (Program.EntryPoint == nullptr)
		throw std::runtime_error("entry point was null");

	CallStackFrame* entryFrame = PushFrame(Program.EntryPoint);
	InvokeMethod(entryFrame);
	PopFrame();
}
