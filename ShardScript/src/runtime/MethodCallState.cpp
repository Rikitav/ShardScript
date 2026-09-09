#include <shard/runtime/MethodCallState.hpp>

#include <mimalloc.h>
#include <cstdint>
#include <span>

using namespace shard;

ObjectInstance CallState::ReturnView() const
{
	return Frame->ReturnView();
}

void CallState::PlaceReturned(ObjectInstance value) const
{
	Frame->PlaceReturned(value);
}

void CallState::ReturnInteger(std::int64_t value) const
{
	ObjectInstance retReg(Collector.ResolveShape(TYPE_INT), reinterpret_cast<std::byte*>(&value));
	PlaceReturned(retReg);
}

void CallState::ReturnByte(std::uint8_t value) const
{
	ObjectInstance retReg(Collector.ResolveShape(TYPE_BYTE), reinterpret_cast<std::byte*>(&value));
	PlaceReturned(retReg);
}

void CallState::ReturnDouble(double value) const
{
	ObjectInstance retReg(Collector.ResolveShape(TYPE_DOUBLE), reinterpret_cast<std::byte*>(&value));
	PlaceReturned(retReg);
}

void CallState::ReturnBoolean(bool value) const
{
	ObjectInstance retReg(Collector.ResolveShape(TYPE_BOOL), reinterpret_cast<std::byte*>(&value));
	PlaceReturned(retReg);
}

void CallState::ReturnChar(wchar_t value) const
{
	ObjectInstance retReg(Collector.ResolveShape(TYPE_CHAR), reinterpret_cast<std::byte*>(&value));
	PlaceReturned(retReg);
}


int CallState::TryInvokeMethodImpl(const MethodSymbol* method, const ObjectInstance* argv, const std::size_t argc, const TypeSymbol* const* typev, const std::size_t typec, std::byte* returnBuffer, TypeShape* returnType) const
{
	if (method == nullptr)
		throw undefined_behaviour("TryInvokeMethod: method is null");

	const MethodSymbol* targetMethod = method;
	if (method->IsAbstract)
	{
		if (argc == 0)
			throw undefined_behaviour(L"Tried to call abstract method without 'this' argument");

		if (argv[0].IsNullInstance())
			throw undefined_behaviour(L"Tried to call abstract method on a null instance");

		TypeSymbol* receiverType = const_cast<TypeSymbol*>(argv[0].getInfo());
		if (receiverType != nullptr)
		{
			targetMethod = receiverType->FindInterfaceImplementation(method);
			if (targetMethod == nullptr)
				throw undefined_behaviour(L"Failed to resolve abstract method");
		}
	}

	CallStackFrame* callingFrame = Runtimer.CurrentFrame();

	if (targetMethod->TypeParameters.size() != typec)
		throw undefined_behaviour(L"Method " + targetMethod->Name + L" expected " + std::to_wstring(targetMethod->TypeParameters.size()) + L" generic type arguments, but got " + std::to_wstring(typec));

	if (typec != 0)
		Runtimer.SetPendingTypeArguments(std::span<TypeSymbol*>(const_cast<TypeSymbol**>(typev), typec));

	try
	{
		CallStackFrame* frame = Runtimer.PushFrame(targetMethod);
		if (frame->ReturnShape() != returnType)
			throw undefined_behaviour(L"return TypeShape mismatch");

		for (std::size_t i = 0; i < argc; i++)
			frame->SetLocal(i, argv[i]);

		frame->ReturnSlot = returnBuffer;
		Runtimer.InvokeMethodInternal(targetMethod, frame);

		if (callingFrame->InterruptionReason == FrameInterruptionReason::None)
		{
			if (targetMethod->ReturnType != SymbolTable::Primitives::Void)
			{
				ObjectInstance retReg = frame->ReturnView();
				std::byte* memory = retReg.getMemory();
				const TypeShape* returnShape = frame->ReturnShape();

				//std::byte* entry = returnBuffer;
				std::byte* payload = returnBuffer; //entry + CallStackFrame::SlotHeaderBytes;

				if (returnShape->IsReferenceType())
				{
					//std::memcpy(entry, &returnShape, sizeof(TypeShape*));
					std::memcpy(payload, &memory, sizeof(std::byte*));
				}
				else
				{
					//std::memcpy(entry, &returnShape, sizeof(TypeShape*));
					std::memcpy(payload, memory, returnShape->Size);
				}
			}

			/*
			std::byte* returnSlot = frame->ReturnSlotMemory();
			std::size_t returnSize = frame->ReturnShape()->Size;

			std::memcpy(returnBuffer, returnSlot, returnSize);

			const TypeShape* returnShape = frame->ReturnShape();
			std::byte* entry = frame->ReturnSlotMemory();
			std::byte* payload = entry + CallStackFrame::SlotHeaderBytes;

			if (returnShape->IsReferenceType())
			{
				std::memcpy(entry, &returnShape, sizeof(TypeShape*));
				std::memcpy(payload, &stored, sizeof(std::byte*));
			}
			else
			{
				std::memcpy(entry, &returnShape, sizeof(TypeShape*));
				std::memcpy(payload, stored, returnShape->Size);
			}
			*/
		}
	}
	catch (...)
	{
		Runtimer.PopFrame();
		throw;
	}

	if (callingFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
	{
		ObjectInstance exception = callingFrame->CurrentException;
		if (!exception.IsNullInstance())
		{
			exception.IncrementReference();
			callingFrame->CurrentException.DecrementReference();
			Collector.CollectInstance(callingFrame->CurrentException);
		}

		callingFrame->InterruptionReason = FrameInterruptionReason::None;
		callingFrame->CurrentException = ObjectInstance();
	}

	Runtimer.PopFrame();
	return 0;
}

InvokeResult CallState::TryInvokeMethod(MethodSymbol* method) const
{
	TypeShape* returnShape = Collector.ResolveShape(method->ReturnType, {});
	InvokeResult result(this, method, returnShape);

	if (TryInvokeMethodImpl(method, nullptr, 0, nullptr, 0, result.m_returned, returnShape) != 0)
	{
		// TODO: IMPLEMENT
	}

	return result;
}

InvokeResult CallState::TryInvokeMethod(MethodSymbol* method, const std::span<ObjectInstance> args) const
{
	TypeShape* returnShape = Collector.ResolveShape(method->ReturnType, {});
	InvokeResult result(this, method, returnShape);

	if (TryInvokeMethodImpl(method, args.data(), args.size(), nullptr, 0, result.m_returned, returnShape) != 0)
	{
		// TODO: IMPLEMENT
	}

	return result;
}

InvokeResult CallState::TryInvokeMethod(MethodSymbol* method, const std::initializer_list<ObjectInstance> args) const
{
	TypeShape* returnShape = Collector.ResolveShape(method->ReturnType, {});
	InvokeResult result(this, method, returnShape);

	if (TryInvokeMethodImpl(method, args.data(), args.size(), nullptr, 0, result.m_returned, returnShape) != 0)
	{
		// TODO: IMPLEMENT
	}

	return result;
}

InvokeResult CallState::TryInvokeMethod(MethodSymbol* method, const std::span<ObjectInstance> args, const std::span<TypeSymbol*> typeArguments) const
{
	TypeShape* returnShape = Collector.ResolveShape(method->ReturnType, typeArguments);
	InvokeResult result(this, method, returnShape);

	if (TryInvokeMethodImpl(method, args.data(), args.size(), typeArguments.data(), typeArguments.size(), result.m_returned, returnShape) != 0)
	{
		// TODO: IMPLEMENT
	}

	return result;
}

InvokeResult CallState::TryInvokeMethod(MethodSymbol* method, const std::initializer_list<ObjectInstance> args, const std::initializer_list<TypeSymbol*> typeArguments) const
{
	TypeShape* returnShape = Collector.ResolveShape(method->ReturnType, typeArguments);
	InvokeResult result(this, method, returnShape);

	if (TryInvokeMethodImpl(method, args.data(), args.size(), typeArguments.data(), typeArguments.size(), result.m_returned, returnShape) != 0)
	{
		// TODO: IMPLEMENT
	}

	return result;
}

void CallState::Propagate(ObjectInstance exception) const
{
	if (Frame == nullptr)
		throw undefined_behaviour("Propagate: callback has no frame");

	if (exception.IsNullInstance())
		throw undefined_behaviour("Propagate: exception is null");

	exception.IncrementReference();
	Frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
	Frame->CurrentException = exception;
}

InvokeResult CallState::NewObject(TypeSymbol* type) const
{
	ConstructorSymbol* ctor = FindParameterlessConstructor(type);
	if (ctor == nullptr)
		throw undefined_behaviour("Type has no parameterless constructor");

	ObjectInstance instance = Collector.AllocateInstance(type);
	instance.IncrementReference();

	return TryInvokeMethod(ctor);
}

InvokeResult CallState::NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::span<ObjectInstance> args) const
{
	if (type == nullptr)
		throw undefined_behaviour("NewObject: type is null");

	if (ctor == nullptr)
		throw undefined_behaviour("NewObject: constructor is null");

	ObjectInstance instance = Collector.AllocateInstance(type);
	instance.IncrementReference();

	return TryInvokeMethod(ctor, args);
}

InvokeResult CallState::NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::initializer_list<ObjectInstance> args) const
{
	if (type == nullptr)
		throw undefined_behaviour("NewObject: type is null");

	if (ctor == nullptr)
		throw undefined_behaviour("NewObject: constructor is null");

	ObjectInstance instance = Collector.AllocateInstance(type);
	instance.IncrementReference();

	return TryInvokeMethod(ctor, args);
}

InvokeResult CallState::NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::span<ObjectInstance> args, const std::span<TypeSymbol*> typeArgs) const
{
	if (type == nullptr)
		throw undefined_behaviour("NewObject: class is null");

	if (ctor == nullptr)
		throw undefined_behaviour("NewObject: no constructor with matching parameter count");

	ObjectInstance instance = Collector.AllocateGeneric(type, typeArgs);
	instance.IncrementReference();

	return TryInvokeMethod(ctor, args, typeArgs);
}

InvokeResult CallState::NewObject(TypeSymbol* type, ConstructorSymbol* ctor, const std::initializer_list<ObjectInstance> args, const std::initializer_list<TypeSymbol*> typeArgs) const
{
	if (type == nullptr)
		throw undefined_behaviour("NewObject: class is null");

	if (ctor == nullptr)
		throw undefined_behaviour("NewObject: no constructor with matching parameter count");

	ObjectInstance instance = Collector.AllocateGeneric(type, typeArgs);
	instance.IncrementReference();

	return TryInvokeMethod(ctor, args, typeArgs);
}

InvokeResult CallState::GetProperty(ObjectInstance obj, PropertySymbol* prop) const
{
	if (obj.IsNullInstance())
		throw undefined_behaviour("GetProperty: object is null");

	if (prop == nullptr)
		throw undefined_behaviour("GetProperty: property is null");

	if (prop->Getter == nullptr)
		throw undefined_behaviour("GetProperty: property has no getter");

	return TryInvokeMethod(prop->Getter, { obj });
}

InvokeResult CallState::SetProperty(ObjectInstance obj, PropertySymbol* prop, ObjectInstance value) const
{
	if (obj.IsNullInstance())
		throw undefined_behaviour("SetProperty: object is null");

	if (prop == nullptr)
		throw undefined_behaviour("SetProperty: property is null");

	if (prop->Setter == nullptr)
		throw undefined_behaviour("SetProperty: property has no setter");

	return TryInvokeMethod(prop->Setter, { obj, value });
}

/*
/// <summary>
/// Reads a field value.
/// </summary>
ObjectInstance CallState::GetField(ObjectInstance obj, FieldSymbol* field)
{
	if (obj.IsNullInstance())
		throw undefined_behaviour("GetField: object is null");

	if (field == nullptr)
		throw undefined_behaviour("GetField: field is null");

	return obj.GetField(field);
}

/// <summary>
/// Writes a field value.
/// </summary>
void CallState::SetField(ObjectInstance obj, FieldSymbol* field, ObjectInstance value)
{
	if (obj.IsNullInstance())
		throw undefined_behaviour("SetField: object is null");

	if (field == nullptr)
		throw undefined_behaviour("SetField: field is null");

	obj.SetField(field, value);
}
*/

/// <summary>
/// Wraps an ObjectInstance of delegate type into a callable RAII DelegateRef.
/// </summary>
DelegateRef CallState::WrapDelegate(ObjectInstance delegate) const
{
	if (delegate.IsNullInstance())
		throw undefined_behaviour("WrapDelegate: delegate is null");

	if (delegate.getInfo()->Kind != SyntaxKind::DelegateType)
		throw undefined_behaviour("WrapDelegate: object is not a delegate");

	return DelegateRef(Runtimer, delegate);
}

InvokeResult::InvokeResult(const CallState* callState, const MethodSymbol* callingMethod, const TypeShape* returnType)
	: m_callState(callState), m_callingMethod(callingMethod), m_returnType(returnType)
{
	if (m_returnType != nullptr)
	{
		m_retSize = returnType->IsReferenceType() ? sizeof(std::byte*) : returnType->Size;
		//m_retSize += CallStackFrame::SlotHeaderBytes;

		if (m_retSize != 0)
			// PlaceReturned writes a SlotHeaderBytes shape header before the payload,
			// so reserve room for it to avoid a heap overflow.
			m_returned = reinterpret_cast<std::byte*>(mi_malloc(m_retSize + CallStackFrame::SlotHeaderBytes));
	}
}

InvokeResult::~InvokeResult()
{
	if (m_returned != nullptr)
		mi_free(m_returned);
}

InvokeResult::operator bool() const
{
	return IsOk();
}

bool InvokeResult::IsOk() const
{
	return m_callState->Frame->InterruptionReason == FrameInterruptionReason::None;
}

ObjectInstance InvokeResult::Value() const
{
	if (!IsOk())
		throw undefined_behaviour("InvokeResult: Value() on a failed invocation");

	return ObjectInstance(m_returnType, m_returnType->IsReferenceType() ? *reinterpret_cast<std::byte**>(m_returned) : m_returned);
}

ObjectInstance InvokeResult::Exception() const
{
	if (IsOk())
		throw undefined_behaviour("InvokeResult: Exception() on a successful invocation");

	return m_callState->Frame->CurrentException;
}
