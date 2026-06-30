#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/PrimitiveOperators.hpp>

#include <shard/compilation/ByteCodeEncoder.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <vector>
#include <ranges>
#include <new>
#include <utility>
#include <optional>
#include <sstream>
#include <memory>

using namespace std::ranges;
using namespace std::views;
using namespace shard;

TypeSymbol* SymbolTable::Global::Type = new TypeSymbol(GlobalTypeName, SyntaxKind::CompilationUnit);
SemanticScope* SymbolTable::Global::Scope = new SemanticScope(Type, nullptr);

static std::unique_ptr<SymbolTable> GlobalSymbolTable;
static bool CreatingGlobalSymbolTable = false;

inline static void declareGlobal(SyntaxSymbol* symbol)
{
	SymbolTable::Global::Scope->DeclareSymbol(symbol);
}

template<typename B, typename D>
static std::unique_ptr<D> unique_cast(std::unique_ptr<B> ptr)
{
	D* tmp = static_cast<D*>(ptr.get());
	std::unique_ptr<D> derivedPointer;

	if (tmp != nullptr)
	{
		ptr.release();
		derivedPointer.reset(tmp);
	}

	return derivedPointer;
}

static std::string WStringToString(const std::wstring& value)
{
	std::string result;
	result.reserve(value.size());
	for (wchar_t ch : value)
		result.push_back(static_cast<char>(ch));

	return result;
}

static std::wstring ObjectInstanceToString(const VirtualMachine* host, ObjectInstance* instance)
{
	TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
	MethodSymbol* implementation = type->FindInterfaceImplementation(TRAIT_PRINTABLE_ToString);

	/*
	if (implementation == nullptr)
		throw std::runtime_error("Type '" + WStringToString(type->FullName) + "' does not implement IPrintable");
	*/

	if (implementation == nullptr)
		return type->FullName;

	host->InvokeMethod(implementation, { instance });
	ObjectInstance* result = host->CurrentFrame()->PopStack();
	if (result == nullptr || result->getInfo() != SymbolTable::Primitives::String)
		throw std::runtime_error("ToString did not return a string");

	return result->AsString();
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

static ObjectInstance* primitive_string_to_string(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	//self->IncrementReference();
	return self;
}

static ObjectInstance* primitive_array_to_string(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->getInfo());
	size_t size = array->Length;

	if (size == 0)
		return context.Collector.FromValue(L"[]");

	std::wostringstream result;
	result << L"[";

	ObjectInstance* element = instance->GetElement(0);
	result << ObjectInstanceToString(&context.Runtimer, element);
	context.Collector.CollectInstance(element);

	for (size_t i = 1; i < size; i++)
	{
		element = instance->GetElement(i);
		result << L", " << ObjectInstanceToString(&context.Runtimer, element);
		context.Collector.CollectInstance(element);
	}

	result << L"]";
	return context.Collector.FromValue(result.str());
}

static ObjectInstance* primitive_nint_to_string(const CallState& context)
{
	std::size_t self = reinterpret_cast<std::size_t>(context.Args[0]->AsNint());
	return context.Collector.FromValue(std::to_wstring(self));
}

static ObjectInstance* runtime_capture_stack_trace(const CallState& context)
{
	std::wstring trace = context.Runtimer.GetStackTrace();
	return context.Collector.FromValue(trace);
}

static AccessorSymbol* AddInterfaceGetter(InterfaceSymbol* iface, const std::wstring& name, SymbolFactory& factory)
{
	PropertySymbol* property = factory.Property(name, SymbolTable::Primitives::String, LINK_INSTANCE);
	property->Accesibility = SymbolAccesibility::Public;
	property->Parent = iface;

	AccessorSymbol* getter = factory.Getter(property);
	getter->Parent = property;
	getter->HandleType = MethodHandleType::None;

	iface->Properties.push_back(property);
	iface->Methods.push_back(getter);

	return getter;
}

static void AddRuntimeExceptionProperty(ClassSymbol* cls, const std::wstring& name, FieldSymbol*& outField, SymbolFactory& factory)
{
	PropertySymbol* property = factory.Property(name, SymbolTable::Primitives::String, LINK_INSTANCE);
	property->Accesibility = SymbolAccesibility::Public;
	property->Parent = cls;

	FieldSymbol* backingField = factory.BackingField(property);
	backingField->Parent = cls;
	cls->Fields.push_back(backingField);

	AccessorSymbol* getter = factory.Getter(property);
	getter->Parent = property;

	ByteCodeEncoder encoder;
	encoder.EmitLoadVarible(getter->ExecutableByteCode, 0);
	encoder.EmitLoadField(getter->ExecutableByteCode, backingField);

	cls->Properties.push_back(property);
	cls->Methods.push_back(getter);

	outField = backingField;
}

static ClassSymbol* g_ArrayEnumeratorClass = nullptr;
static TypeParameterSymbol* g_ArrayEnumeratorT = nullptr;
static FieldSymbol* g_ArrayEnumeratorSource = nullptr;
static FieldSymbol* g_ArrayEnumeratorIndex = nullptr;
static FieldSymbol* g_ArrayEnumeratorLength = nullptr;

static ObjectInstance* array_enumerator_MoveNext(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(g_ArrayEnumeratorIndex, context.Frame)->AsInteger();
	std::int64_t length = self->GetField(g_ArrayEnumeratorLength, context.Frame)->AsInteger();
	
	index++;
	self->SetField(g_ArrayEnumeratorIndex, context.Collector.FromValue(index), context.Frame);
	return context.Collector.FromValue(index < length);
}

static ObjectInstance* array_enumerator_Current_get(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(g_ArrayEnumeratorIndex, context.Frame)->AsInteger();
	ObjectInstance* source = self->GetField(g_ArrayEnumeratorSource, context.Frame);
	return source->GetElement(static_cast<std::size_t>(index), context.Frame);
}

static ObjectInstance* primitive_array_get_enumerator(const CallState& context)
{
	ObjectInstance* array = context.Args[0];
	const ArrayTypeSymbol* arrayType = static_cast<const ArrayTypeSymbol*>(array->getInfo());
	TypeSymbol* concreteT = const_cast<TypeSymbol*>(arrayType->UnderlayingType);

	GenericTypeSymbol* enumeratorType = new GenericTypeSymbol(g_ArrayEnumeratorClass);
	enumeratorType->AddTypeParameter(g_ArrayEnumeratorT, concreteT);
	enumeratorType->Inlining = TypeInlining::ByReference;
	enumeratorType->MemoryBytesSize = g_ArrayEnumeratorClass->MemoryBytesSize;
	enumeratorType->State = TypeLayoutingState::Visited;

	ObjectInstance* enumerator = context.Collector.AllocateInstance(enumeratorType);
	enumerator->SetField(g_ArrayEnumeratorSource, array, context.Frame);
	enumerator->SetField(g_ArrayEnumeratorIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)), context.Frame);
	enumerator->SetField(g_ArrayEnumeratorLength, context.Collector.FromValue(static_cast<std::int64_t>(arrayType->Length)), context.Frame);

	return enumerator;
}

static void MakePrimitivePrintable(TypeSymbol* primitive, MethodSymbolDelegate toString, SymbolFactory& factory, MethodSymbol* TRAIT_PRINTABLE_ToString)
{
	MethodSymbol* method = factory.Method(SymbolAccesibility::Public, LINK_INSTANCE, SymbolTable::Primitives::String, L"ToString", toString);
	method->Parent = primitive;

	primitive->Methods.push_back(method);
	primitive->Interfaces.push_back(TRAIT_PRINTABLE);
	primitive->InterfaceMethodMap[TRAIT_PRINTABLE_ToString] = method;
}

static void ResolvePrimitives(SymbolTable* table)
{
	static bool resolved = false;
	if (resolved)
		return;

	SymbolFactory factory(table);

	// ============================================================================
	// Matters
	// ============================================================================
	SymbolTable::Primitives::Void = new StructSymbol(L"Void");
	SymbolTable::Primitives::Any = new StructSymbol(L"Any");
	SymbolTable::Primitives::Null = new StructSymbol(L"Null");
	SymbolTable::Primitives::NativeInteger = new ClassSymbol(L"IntPtr");

	SymbolTable::Primitives::Void->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Any->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Null->State = TypeLayoutingState::Visited;

	SymbolTable::Primitives::Void->MemoryBytesSize = 0;
	SymbolTable::Primitives::Any->MemoryBytesSize = 0;
	SymbolTable::Primitives::Null->MemoryBytesSize = 0;
	SymbolTable::Primitives::NativeInteger->MemoryBytesSize = sizeof(void*);

	// ============================================================================
	// Primitives
	// ============================================================================
	SymbolTable::Primitives::Boolean = new StructSymbol(L"Boolean");
	SymbolTable::Primitives::Integer = new StructSymbol(L"Integer");
	SymbolTable::Primitives::Double = new StructSymbol(L"Double");
	SymbolTable::Primitives::Char = new StructSymbol(L"Char");
	SymbolTable::Primitives::String = new ClassSymbol(L"String");
	SymbolTable::Primitives::Array = new ClassSymbol(L"Array");

	SymbolTable::Primitives::Boolean->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Integer->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Double->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Char->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::String->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Array->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::NativeInteger->State = TypeLayoutingState::Visited;

	SymbolTable::Primitives::Boolean->MemoryBytesSize = sizeof(bool);
	SymbolTable::Primitives::Integer->MemoryBytesSize = sizeof(std::int64_t);
	SymbolTable::Primitives::Double->MemoryBytesSize = sizeof(double);
	SymbolTable::Primitives::Char->MemoryBytesSize = sizeof(wchar_t);
	SymbolTable::Primitives::String->MemoryBytesSize = sizeof(std::int64_t) + sizeof(wchar_t*); // long _length + char[] _data
	SymbolTable::Primitives::Array->MemoryBytesSize = sizeof(std::int64_t);					    // long _length

	resolved = true;
}

static void ResolveInterfaces(SymbolTable* table)
{
	static bool resolved = false;
	if (resolved)
		return;

	SymbolFactory factory(table);

	// IThrowable
	{
		TRAIT_THROWABLE = factory.Interface(L"IThrowable", SymbolAccesibility::Public, SymbolTable::Global::Type);
		TRAIT_THROWABLE_getMessage = AddInterfaceGetter(TRAIT_THROWABLE, L"message", factory);
		TRAIT_THROWABLE_getStackTrace = AddInterfaceGetter(TRAIT_THROWABLE, L"stack_trace", factory);

		declareGlobal(TRAIT_THROWABLE);
	}

	// IDisposable
	{
		TRAIT_DISPOSABLE = factory.Interface(L"IDisposable", SymbolAccesibility::Public, SymbolTable::Global::Type);
		TRAIT_DISPOSABLE_Dispose = factory.Method(L"Dispose", SymbolTable::Primitives::Void, LINK_INSTANCE);

		TRAIT_DISPOSABLE_Dispose->Parent = TRAIT_DISPOSABLE;
		TRAIT_DISPOSABLE->Methods.push_back(TRAIT_DISPOSABLE_Dispose);

		declareGlobal(TRAIT_DISPOSABLE);
	}

	// IPrintable
	{
		TRAIT_PRINTABLE = factory.Interface(L"IPrintable", SymbolAccesibility::Public, SymbolTable::Global::Type);
		TRAIT_PRINTABLE_ToString = factory.Method(L"ToString", SymbolTable::Primitives::String, LINK_INSTANCE);
		
		TRAIT_PRINTABLE_ToString->Parent = TRAIT_PRINTABLE;
		TRAIT_PRINTABLE->Methods.push_back(TRAIT_PRINTABLE_ToString);

		declareGlobal(TRAIT_PRINTABLE);
	}

	// IEnumerator<T>
	{
		TRAIT_ENUMERATOR = factory.Interface(L"IEnumerator", SymbolAccesibility::Public, SymbolTable::Global::Type);

		TypeParameterSymbol* enumeratorTypeParam = factory.TypeParameter(L"T", TRAIT_ENUMERATOR);
		TRAIT_ENUMERATOR->TypeParameters.push_back(enumeratorTypeParam);

		TRAIT_ENUMERATOR_MOVENEXT = factory.Method(L"MoveNext", SymbolTable::Primitives::Boolean, LINK_INSTANCE);
		TRAIT_ENUMERATOR_MOVENEXT->Accesibility = SymbolAccesibility::Public;
		TRAIT_ENUMERATOR_MOVENEXT->Parent = TRAIT_ENUMERATOR;
		TRAIT_ENUMERATOR->Methods.push_back(TRAIT_ENUMERATOR_MOVENEXT);

		PropertySymbol* currentProperty = factory.Property(L"Current", enumeratorTypeParam, LINK_INSTANCE);
		currentProperty->Accesibility = SymbolAccesibility::Public;
		currentProperty->Parent = TRAIT_ENUMERATOR;
		AccessorSymbol* currentGetter = factory.Getter(currentProperty);
		currentGetter->Parent = currentProperty;
		currentGetter->HandleType = MethodHandleType::None;
		TRAIT_ENUMERATOR->Properties.push_back(currentProperty);
		TRAIT_ENUMERATOR->Methods.push_back(currentGetter);
		TRAIT_ENUMERATOR_CURRENT_GET = currentGetter;

		declareGlobal(TRAIT_ENUMERATOR);
	}

	// IEnumerable<T>
	{
		TRAIT_ENUMERABLE = factory.Interface(L"IEnumerable", SymbolAccesibility::Public, SymbolTable::Global::Type);

		TypeParameterSymbol* enumerableTypeParam = factory.TypeParameter(L"T", TRAIT_ENUMERABLE);
		TRAIT_ENUMERABLE->TypeParameters.push_back(enumerableTypeParam);

		GenericTypeSymbol* enumeratorReturnType = factory.GenericType(TRAIT_ENUMERATOR, { { L"T", enumerableTypeParam } });

		TRAIT_ENUMERABLE_GETENUMERATOR = factory.Method(L"GetEnumerator", enumeratorReturnType, LINK_INSTANCE);
		TRAIT_ENUMERABLE_GETENUMERATOR->Accesibility = SymbolAccesibility::Public;
		TRAIT_ENUMERABLE_GETENUMERATOR->Parent = TRAIT_ENUMERABLE;
		TRAIT_ENUMERABLE->Methods.push_back(TRAIT_ENUMERABLE_GETENUMERATOR);

		declareGlobal(TRAIT_ENUMERABLE);
	}

	resolved = true;
}

static void ResolveEnumerables(SymbolTable* table)
{
	static bool resolved = false;
	if (resolved)
		return;

	SymbolFactory factory(table);

	// ArrayEnumerator<T>
	{
		ClassSymbol* raw = factory.Class(L"ArrayEnumerator");
		raw->Accesibility = SymbolAccesibility::Public;
		raw->Inlining = TypeInlining::ByReference;
		raw->Parent = SymbolTable::Global::Type;
		raw->FullName = L"ArrayEnumerator";
		declareGlobal(raw);

		TypeParameterSymbol* typeParam = factory.TypeParameter(L"T", raw);
		raw->TypeParameters.push_back(typeParam);

		g_ArrayEnumeratorClass = raw;
		g_ArrayEnumeratorT = typeParam;

		g_ArrayEnumeratorSource = factory.Field(L"_source", factory.Array(typeParam), LINK_INSTANCE);
		g_ArrayEnumeratorSource->Accesibility = SymbolAccesibility::Private;
		g_ArrayEnumeratorSource->Parent = raw;
		raw->Fields.push_back(g_ArrayEnumeratorSource);

		g_ArrayEnumeratorIndex = factory.Field(L"_index", SymbolTable::Primitives::Integer, LINK_INSTANCE);
		g_ArrayEnumeratorIndex->Accesibility = SymbolAccesibility::Private;
		g_ArrayEnumeratorIndex->Parent = raw;
		raw->Fields.push_back(g_ArrayEnumeratorIndex);

		g_ArrayEnumeratorLength = factory.Field(L"_length", SymbolTable::Primitives::Integer, LINK_INSTANCE);
		g_ArrayEnumeratorLength->Accesibility = SymbolAccesibility::Private;
		g_ArrayEnumeratorLength->Parent = raw;
		raw->Fields.push_back(g_ArrayEnumeratorLength);

		std::size_t layoutOffset = 0;
		g_ArrayEnumeratorSource->MemoryBytesOffset = layoutOffset;
		layoutOffset += sizeof(void*);
		g_ArrayEnumeratorIndex->MemoryBytesOffset = layoutOffset;
		layoutOffset += sizeof(std::int64_t);
		g_ArrayEnumeratorLength->MemoryBytesOffset = layoutOffset;
		layoutOffset += sizeof(std::int64_t);
		raw->MemoryBytesSize = layoutOffset;
		raw->State = TypeLayoutingState::Visited;

		MethodSymbol* moveNext = factory.Method(
			SymbolAccesibility::Public,
			LINK_INSTANCE,
			SymbolTable::Primitives::Boolean,
			L"MoveNext",
			&array_enumerator_MoveNext);

		moveNext->Parent = raw;
		raw->Methods.push_back(moveNext);

		PropertySymbol* currentProperty = factory.Property(L"Current", typeParam, LINK_INSTANCE);
		currentProperty->Accesibility = SymbolAccesibility::Public;
		currentProperty->Parent = raw;
		AccessorSymbol* currentGetter = factory.Getter(currentProperty);
		currentGetter->Parent = currentProperty;
		currentGetter->FunctionPointer = &array_enumerator_Current_get;
		currentGetter->HandleType = MethodHandleType::External;
		raw->Properties.push_back(currentProperty);
		raw->Methods.push_back(currentGetter);

		GenericTypeSymbol* enumeratorInterface = factory.GenericType(TRAIT_ENUMERATOR, { { L"T", typeParam } });
		raw->Interfaces.push_back(enumeratorInterface);
		raw->InterfaceMethodMap[TRAIT_ENUMERATOR_MOVENEXT] = moveNext;
		raw->InterfaceMethodMap[TRAIT_ENUMERATOR_CURRENT_GET] = currentGetter;

		SymbolTable::StandardTypes::ArrayEnumerator = raw;
		SymbolTable::StandardTypes::ArrayEnumerator_T = typeParam;
		SymbolTable::StandardTypes::ArrayEnumerator_SourceField = g_ArrayEnumeratorSource;
		SymbolTable::StandardTypes::ArrayEnumerator_IndexField = g_ArrayEnumeratorIndex;
		SymbolTable::StandardTypes::ArrayEnumerator_LengthField = g_ArrayEnumeratorLength;
	}

	// Make the primitive array type implement IEnumerable<T> through a native GetEnumerator
	{
		GenericTypeSymbol* enumerableInterface = factory.GenericType(TRAIT_ENUMERABLE, { { L"T", TRAIT_ENUMERABLE->TypeParameters[0] } });
		MethodSymbol* getEnumerator = factory.Method(
			SymbolAccesibility::Public,
			LINK_INSTANCE,
			factory.GenericType(TRAIT_ENUMERATOR, { { L"T", TRAIT_ENUMERATOR->TypeParameters[0] } }),
			L"GetEnumerator",
			&primitive_array_get_enumerator);

		getEnumerator->Parent = SymbolTable::Primitives::Array;
		SymbolTable::Primitives::Array->Methods.push_back(getEnumerator);
		SymbolTable::Primitives::Array->Interfaces.push_back(enumerableInterface);
		SymbolTable::Primitives::Array->InterfaceMethodMap[TRAIT_ENUMERABLE_GETENUMERATOR] = getEnumerator;
	}

	resolved = true;
}

static void ResolveStandards(SymbolTable* table)
{
	static bool resolved = false;
	if (resolved)
		return;

	SymbolFactory factory(table);

	// Runtime class with capture_stack_trace
	{
		ClassSymbol* raw = factory.Class(L"runtime");
		raw->Accesibility = SymbolAccesibility::Public;
		raw->Parent = SymbolTable::Global::Type;
		raw->FullName = L"runtime";
		declareGlobal(raw);

		MethodSymbol* method = factory.Method(
			SymbolAccesibility::Public,
			LINK_STATIC,
			SymbolTable::Primitives::String,
			L"capture_stack_trace",
			&runtime_capture_stack_trace);

		method->Parent = raw;
		raw->Methods.push_back(method);

		SymbolTable::StandardTypes::Runtime = raw;
		SymbolTable::StandardTypes::RuntimeCaptureStackTrace = method;
	}

	// RuntimeException class
	{
		ClassSymbol* raw = factory.Class(L"RuntimeException");
		raw->Accesibility = SymbolAccesibility::Public;
		raw->Parent = SymbolTable::Global::Type;
		raw->FullName = L"RuntimeException";
		raw->Interfaces.push_back(TRAIT_THROWABLE);
		declareGlobal(raw);

		ConstructorSymbol* ctor = factory.Constructor(raw, SymbolAccesibility::Public);
		ctor->Parent = raw;
		raw->Constructors.push_back(ctor);

		FieldSymbol* messageField = nullptr;
		FieldSymbol* stackTraceField = nullptr;
		AddRuntimeExceptionProperty(raw, L"message", messageField, factory);
		AddRuntimeExceptionProperty(raw, L"stack_trace", stackTraceField, factory);

		SymbolTable::StandardTypes::RuntimeExceptionMessageField = messageField;
		SymbolTable::StandardTypes::RuntimeExceptionStackTraceField = stackTraceField;

		// Map IThrowable getters to RuntimeException getters
		for (PropertySymbol* ifaceProp : TRAIT_THROWABLE->Properties)
		{
			if (ifaceProp->Getter == nullptr)
				continue;

			for (PropertySymbol* classProp : raw->Properties)
			{
				if (classProp->Getter == nullptr)
					continue;

				if (classProp->Name == ifaceProp->Name &&
					TypeSymbol::Equals(classProp->ReturnType, ifaceProp->ReturnType))
				{
					raw->InterfaceMethodMap[ifaceProp->Getter] = classProp->Getter;
					break;
				}
			}
		}

		SymbolTable::StandardTypes::RuntimeException = raw;
	}

	resolved = true;
}

static void ResolveGlobalComponents(SymbolTable* table)
{
	static bool resolved = false;
	if (resolved)
		return;

	// Standard components are owned by a process-wide table so that their
	// pointers remain valid after any individual CompilationContext is destroyed.
	CreatingGlobalSymbolTable = true;
	GlobalSymbolTable = std::make_unique<SymbolTable>();
	CreatingGlobalSymbolTable = false;

	SymbolTable* global = GlobalSymbolTable.get();

	ResolvePrimitives(global);
	ResolveInterfaces(global); // Resolve standard interface traits before they are used below.
	ResolveEnumerables(global);
	ResolveStandards(global);

	// Make standard primitives implement IPrintable
	SymbolFactory factory(global);

	MakePrimitivePrintable(SymbolTable::Primitives::Boolean, &primitive_boolean_to_string, factory, TRAIT_PRINTABLE_ToString);
	MakePrimitivePrintable(SymbolTable::Primitives::Integer, &primitive_integer_to_string, factory, TRAIT_PRINTABLE_ToString);
	MakePrimitivePrintable(SymbolTable::Primitives::Double, &primitive_double_to_string, factory, TRAIT_PRINTABLE_ToString);
	MakePrimitivePrintable(SymbolTable::Primitives::Char, &primitive_char_to_string, factory, TRAIT_PRINTABLE_ToString);
	MakePrimitivePrintable(SymbolTable::Primitives::String, &primitive_string_to_string, factory, TRAIT_PRINTABLE_ToString);
	MakePrimitivePrintable(SymbolTable::Primitives::Array, &primitive_array_to_string, factory, TRAIT_PRINTABLE_ToString);
	MakePrimitivePrintable(SymbolTable::Primitives::NativeInteger, &primitive_nint_to_string, factory, TRAIT_PRINTABLE_ToString);

	RegisterPrimitiveOperators(factory);

	resolved = true;
}

SymbolTable::SymbolTable()
{
	if (CreatingGlobalSymbolTable)
		return;

	ResolveGlobalComponents(this);
}

SymbolTable::~SymbolTable()
{
	ClearSymbols();
}

void SymbolTable::ClearSymbols()
{
	symbolToNodeMap.clear();
	nodeToSymbolMap.clear();

	namespacesList.clear();
	typesList.clear();
	membersList.clear();
	triviasList.clear();
}

std::optional<SyntaxSymbol*> SymbolTable::LookupSymbol(SyntaxNode* node)
{
	auto choise = nodeToSymbolMap.find(node);
	return choise == nodeToSymbolMap.end() ? std::nullopt : std::optional<SyntaxSymbol*>(choise->second);
}

std::optional<SyntaxNode*> SymbolTable::LookupNode(SyntaxSymbol * symbol)
{
	auto choise = symbolToNodeMap.find(symbol);
	return choise == symbolToNodeMap.end() ? std::nullopt : std::optional<SyntaxNode*>(choise->second);
}

SyntaxSymbol* SymbolTable::BindSymbol(SyntaxNode* node, std::unique_ptr<SyntaxSymbol> symbol)
{
	SyntaxSymbol* raw = symbol.get();
	nodeToSymbolMap[node] = raw;
	symbolToNodeMap[raw] = node;

	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
	{
		namespacesList.push_back(unique_cast<SyntaxSymbol, NamespaceSymbol>(std::move(symbol)));
	}
	else if (symbol->IsType())
	{
		typesList.push_back(unique_cast<SyntaxSymbol, TypeSymbol>(std::move(symbol)));
	}
	else if (symbol->IsMember())
	{
		membersList.push_back(unique_cast<SyntaxSymbol, MemberSymbol>(std::move(symbol)));
	}
	else
	{
		triviasList.push_back(std::move(symbol));
	}

	return raw;
}

SyntaxSymbol* SymbolTable::ImplicitSymbol(std::unique_ptr<SyntaxSymbol> symbol)
{
	SyntaxSymbol* raw = symbol.get();
	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
	{
		namespacesList.push_back(unique_cast<SyntaxSymbol, NamespaceSymbol>(std::move(symbol)));
	}
	else if (symbol->IsType())
	{
		typesList.push_back(unique_cast<SyntaxSymbol, TypeSymbol>(std::move(symbol)));
	}
	else if (symbol->IsMember())
	{
		membersList.push_back(unique_cast<SyntaxSymbol, MemberSymbol>(std::move(symbol)));
	}
	else
	{
		triviasList.push_back(std::move(symbol));
	}

	return raw;
}

const std::vector<NamespaceSymbol*> SymbolTable::GetNamespaceSymbols()
{
	std::vector<NamespaceSymbol*> result;
	result.reserve(namespacesList.size());
	for (const auto& symbol : namespacesList)
		result.push_back(symbol.get());

	return result;
}

const std::vector<TypeSymbol*> SymbolTable::GetTypeSymbols()
{
	std::vector<TypeSymbol*> result;
	result.reserve(typesList.size());
	for (const auto& symbol : typesList)
		result.push_back(symbol.get());

	return result;
}

const std::vector<MethodSymbol*> shard::SymbolTable::GetMethodSymbols()
{
	std::vector<MethodSymbol*> methods;
	for (const auto& member : membersList)
	{
		if (member->Kind == SyntaxKind::MethodDeclaration)
			methods.push_back(static_cast<MethodSymbol*>(member.get()));
	}

	return methods;
}
