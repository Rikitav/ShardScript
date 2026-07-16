#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <shard/runtime/MethodCallState.hpp>

#include <sstream>

using namespace shard;

namespace
{
	template<typename T>
	static inline void make_printable(TypeSymbol*& symbol, SymbolTable* table, MethodSymbolDelegate toString)
	{
		SymbolBuilder<T> builder(table, static_cast<T*>(symbol));
		builder.Implements(TRAIT_PRINTABLE);

		builder.AddMethod(L"ToString", SymbolTable::Primitives::String, LINK_INSTANCE)
			.IsImplementationOf(TRAIT_PRINTABLE_ToString)
			.SetCallback(toString);
	}
}

static ObjectInstance* primitive_boolean_to_string(const CallState& context)
{
	bool value = context.Args[0]->AsBoolean();
	return context.Collector.FromValue(std::wstring(value ? L"true" : L"false"));
}

static ObjectInstance* primitive_integer_to_string(const CallState& context)
{
	std::int64_t value = context.Args[0]->AsInteger();
	return context.Collector.FromValue(std::to_wstring(value));
}

static ObjectInstance* primitive_double_to_string(const CallState& context)
{
	double value = context.Args[0]->AsDouble();
	return context.Collector.FromValue(std::to_wstring(value));
}

static ObjectInstance* primitive_char_to_string(const CallState& context)
{
	wchar_t value = context.Args[0]->AsCharacter();
	return context.Collector.FromValue(std::wstring(1, value));
}

static ObjectInstance* primitive_byte_to_string(const CallState& context)
{
	std::uint8_t value = context.Args[0]->AsByte();
	return context.Collector.FromValue(std::to_wstring(static_cast<int>(value)));
}

static ObjectInstance* primitive_string_to_string(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	return self;
}

static ObjectInstance* primitive_array_to_string(const CallState& context)
{
	auto InvokeToString = [&](ObjectInstance* instance) -> ObjectInstance*
	{
		TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
		MethodSymbol* implementation = type->FindInterfaceImplementation(TRAIT_PRINTABLE_ToString);

		if (implementation == nullptr)
			return nullptr;

		context.Runtimer.InvokeMethod(implementation, { instance });
		ObjectInstance* result = context.Runtimer.CurrentFrame()->PopStack();
		if (result == nullptr || result->getInfo() != SymbolTable::Primitives::String)
			throw std::runtime_error("ToString did not return a string");

		return result;
	};

	auto AppendAsString = [&](std::wostringstream& result, ObjectInstance* instance) -> void
	{
		ObjectInstance* asString = InvokeToString(instance);
		if (asString == nullptr)
		{
			result << instance->getInfo()->FullName;
			return;
		}

		result << asString->AsString();
		context.Collector.CollectInstance(asString);
	};

	ObjectInstance* instance = context.Args[0]; // this
	const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->getInfo());
	size_t size = array->Length;

	if (size == 0)
		return context.Collector.FromValue(L"[]");

	std::wostringstream result;
	result << L"[";

	ObjectInstance* element = instance->GetElement(0);
	AppendAsString(result, element);
	// context.Collector.CollectInstance(element);  // GetElement returns array-owned element, do not collect

	for (size_t i = 1; i < size; ++i)
	{
		result << L", ";
		ObjectInstance* element = instance->GetElement(i);
		AppendAsString(result, element);
		// context.Collector.CollectInstance(element);  // GetElement returns array-owned element, do not collect
	}

	result << L"]";
	return context.Collector.FromValue(result.str());
}

static ObjectInstance* primitive_nint_to_string(const CallState& context)
{
	std::size_t self = reinterpret_cast<std::size_t>(context.Args[0]->AsNint());
	return context.Collector.FromValue(std::to_wstring(self));
}

void SymbolTable::ResolvePrimitivePrintables(SymbolTable* globalTable)
{
	// Make standard primitives implement IPrintable
	make_printable<StructSymbol>(SymbolTable::Primitives::Boolean, globalTable, &primitive_boolean_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::Integer, globalTable, &primitive_integer_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::Double, globalTable, &primitive_double_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::Char, globalTable, &primitive_char_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::Byte, globalTable, &primitive_byte_to_string);
	make_printable<ClassSymbol>(SymbolTable::Primitives::String, globalTable, &primitive_string_to_string);
	make_printable<ClassSymbol>(SymbolTable::Primitives::Array, globalTable, &primitive_array_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::NativeInteger, globalTable, &primitive_nint_to_string);
}
