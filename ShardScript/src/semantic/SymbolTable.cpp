#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/PrimitiveOperators.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

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
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/compilation/ByteCodeEncoder.hpp>

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

NamespaceSymbol* SymbolTable::Global::Namespace = new NamespaceSymbol(GlobalTypeName);
SemanticScope* SymbolTable::Global::Scope = new SemanticScope(Namespace, nullptr);

static std::unique_ptr<SymbolTable> GlobalSymbolTable;
static bool CreatingGlobalSymbolTable = false;

namespace
{
	inline static void declareGlobal(SyntaxSymbol* symbol)
	{
		SymbolTable::Global::Scope->DeclareSymbol(symbol);
	}

	template<typename B, typename D>
	static inline std::unique_ptr<D> unique_cast(std::unique_ptr<B> ptr)
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

	static inline std::string WStringToString(const std::wstring& value)
	{
		std::string result;
		result.reserve(value.size());
		for (wchar_t ch : value)
			result.push_back(static_cast<char>(ch));

		return result;
	}

	template<typename T>
	static inline void make_primitive(TypeSymbol*& symbol, const wchar_t* name, const size_t memoryBytesSize)
	{
		symbol = new T(name);
		symbol->MemoryBytesSize = memoryBytesSize;
		symbol->LayoutingState = TypeLayoutingState::Visited;
		symbol->AnalysisState = SymbolAnalysisState::Ready;
	}

	template<typename T>
	static inline void make_printable(TypeSymbol*& symbol, SymbolTable* table, MethodSymbolDelegate toString)
	{
		SymbolBuilder<T> builder(table, static_cast<T*>(symbol));
		builder.Implements(TRAIT_PRINTABLE);

		builder.AddMethod(L"ToString", SymbolTable::Primitives::String, LINK_INSTANCE)
			.IsImplementationOf(TRAIT_PRINTABLE_ToString)
			.SetCallback(toString);
	}

	static inline void inherit_size(FieldSymbol* field)
	{
		TypeSymbol* parent = static_cast<TypeSymbol*>(field->Parent);
		if (field->SlotIndex == std::numeric_limits<std::uint32_t>::max())
			field->SlotIndex = parent->NextSlotIndex++;

		field->MemoryBytesOffset = parent->MemoryBytesSize;
		parent->MemoryBytesSize += field->ReturnType->GetInlineSize();
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

static ObjectInstance* runtime_capture_stack_trace(const CallState& context)
{
	std::wstring trace = context.Runtimer.GetStackTrace();
	return context.Collector.FromValue(trace);
}

static ObjectInstance* array_enumerator_MoveNext(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex)->AsInteger();
	std::int64_t length = self->GetField(CLASS_ARRAYENUMERATOR_LengthField->SlotIndex)->AsInteger();
	
	index++;
	self->SetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex, context.Collector.FromValue(index));
	return context.Collector.FromValue(index < length);
}

static ObjectInstance* array_enumerator_Current_get(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex)->AsInteger();
	ObjectInstance* source = self->GetField(CLASS_ARRAYENUMERATOR_SourceField->SlotIndex);
	return source->GetElement(static_cast<std::size_t>(index));
}

static ObjectInstance* primitive_array_get_enumerator(const CallState& context)
{
	ObjectInstance* array = context.Args[0];
	const ArrayTypeSymbol* arrayType = static_cast<const ArrayTypeSymbol*>(array->getInfo());
	TypeSymbol* concreteT = const_cast<TypeSymbol*>(arrayType->UnderlayingType);

	ObjectInstance* enumerator = context.Collector.AllocateGeneric(CLASS_ARRAYENUMERATOR, std::vector<TypeSymbol*>{ concreteT });
	enumerator->SetField(CLASS_ARRAYENUMERATOR_SourceField->SlotIndex, array);
	enumerator->SetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)));
	enumerator->SetField(CLASS_ARRAYENUMERATOR_LengthField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(arrayType->Length)));

	return enumerator;
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
	make_primitive<StructSymbol>(SymbolTable::Primitives::Void, L"Void", 0);
	make_primitive<StructSymbol>(SymbolTable::Primitives::Any, L"Any", 0);
	make_primitive<StructSymbol>(SymbolTable::Primitives::Null, L"Null", 0);
	make_primitive<ClassSymbol>(SymbolTable::Primitives::NativeInteger, L"IntPtr", sizeof(void*));

	// ============================================================================
	// Primitives
	// ============================================================================
	make_primitive<StructSymbol>(SymbolTable::Primitives::Boolean, L"Boolean", sizeof(bool));
	make_primitive<StructSymbol>(SymbolTable::Primitives::Integer, L"Integer", sizeof(std::int64_t));
	make_primitive<StructSymbol>(SymbolTable::Primitives::Double, L"Double", sizeof(double));
	make_primitive<StructSymbol>(SymbolTable::Primitives::Char, L"Char", sizeof(wchar_t));
	make_primitive<ClassSymbol>(SymbolTable::Primitives::String, L"String", sizeof(std::int64_t) + sizeof(wchar_t*));	// long _length + char[] _data
	make_primitive<ClassSymbol>(SymbolTable::Primitives::Array, L"Array", sizeof(std::int64_t));						// long _length

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
		SymbolBuilder<InterfaceSymbol> builder(table, L"IThrowable", SymbolTable::Global::Namespace);
		TRAIT_THROWABLE = builder
			.DeclareGlobal();

		TRAIT_THROWABLE_getMessage = builder.AddProperty(L"message", TYPE_STRING, LINK_INSTANCE).AddGetter();
		TRAIT_THROWABLE_getStackTrace = builder.AddProperty(L"stack_trace", TYPE_STRING, LINK_INSTANCE).AddGetter();

		TRAIT_THROWABLE_getMessage->IsAbstract = true;
		TRAIT_THROWABLE_getStackTrace->IsAbstract = true;
	}

	// IDisposable
	{
		SymbolBuilder<InterfaceSymbol> builder(table, L"IDisposable", SymbolTable::Global::Namespace);
		TRAIT_DISPOSABLE = builder
			.DeclareGlobal();

		TRAIT_DISPOSABLE_Dispose = builder.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE);
		TRAIT_DISPOSABLE_Dispose->IsAbstract = true;
	}

	// IPrintable
	{
		SymbolBuilder<InterfaceSymbol> builder(table, L"IPrintable", SymbolTable::Global::Namespace);
		TRAIT_PRINTABLE = builder
			.DeclareGlobal();

		TRAIT_PRINTABLE_ToString = builder.AddMethod(L"ToString", TYPE_STRING, LINK_INSTANCE);
		TRAIT_PRINTABLE_ToString->IsAbstract = true;
	}

	// IEnumerator<T>
	{
		SymbolBuilder<InterfaceSymbol> builder(table, L"IEnumerator", SymbolTable::Global::Namespace);
		TRAIT_ENUMERATOR = builder
			.DeclareGlobal();

		TypeParameterSymbol* enumeratorTypeParam = builder
			.AddTypeParameter(L"T");

		TRAIT_ENUMERATOR_MOVENEXT = builder.AddMethod(L"MoveNext", TYPE_BOOL, LINK_INSTANCE);
		TRAIT_ENUMERATOR_CURRENT_GET = builder.AddProperty(L"Current", enumeratorTypeParam, LINK_INSTANCE).AddGetter();

		TRAIT_ENUMERATOR_MOVENEXT->IsAbstract = true;
		TRAIT_ENUMERATOR_CURRENT_GET->IsAbstract = true;
	}

	// IEnumerable<T>
	{
		SymbolBuilder<InterfaceSymbol> builder(table, L"IEnumerable", SymbolTable::Global::Namespace);
		TRAIT_ENUMERABLE = builder
			.DeclareGlobal();

		TypeParameterSymbol* enumerableTypeParam = builder
			.AddTypeParameter(L"T");

		GenericTypeSymbol* enumeratorReturnType = factory.GenericType(TRAIT_ENUMERATOR, { { L"T", enumerableTypeParam } });
		TRAIT_ENUMERABLE_GETENUMERATOR = builder.AddMethod(L"GetEnumerator", enumeratorReturnType, LINK_INSTANCE);
		TRAIT_ENUMERABLE_GETENUMERATOR->IsAbstract = true;
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
		SymbolBuilder<ClassSymbol> builder(table, L"ArrayEnumerator", SymbolTable::Global::Namespace);
		CLASS_ARRAYENUMERATOR = builder
			.Implements(TRAIT_ENUMERATOR)
			.DeclareGlobal();

		CLASS_ARRAYENUMERATOR_T = builder
			.AddTypeParameter(L"T");

		CLASS_ARRAYENUMERATOR_SourceField = builder
			.AddField(L"_source", factory.Array(CLASS_ARRAYENUMERATOR_T), LINK_INSTANCE, ACS_PRIVATE);

		CLASS_ARRAYENUMERATOR_IndexField = builder
			.AddField(L"_index", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

		CLASS_ARRAYENUMERATOR_LengthField = builder
			.AddField(L"_length", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

		inherit_size(CLASS_ARRAYENUMERATOR_SourceField);
		inherit_size(CLASS_ARRAYENUMERATOR_IndexField);
		inherit_size(CLASS_ARRAYENUMERATOR_LengthField);

		builder.AddMethod(L"MoveNext", TYPE_BOOL, LINK_INSTANCE)
			.IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
			.SetCallback(&array_enumerator_MoveNext);

		builder.AddProperty(L"Current", CLASS_ARRAYENUMERATOR_T, LINK_INSTANCE)
			.AddGetter()
			.IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET)
			.SetCallback(&array_enumerator_Current_get);
	}

	// Make the primitive array type implement IEnumerable<T> through a native GetEnumerator
	{
		SymbolBuilder<ClassSymbol> builder(table, static_cast<ClassSymbol*>(TYPE_ARRAY));
		builder.Implements(TRAIT_ENUMERABLE);

		GenericTypeSymbol* enumeratorReturnType = factory.GenericType(TRAIT_ENUMERATOR, { { L"T", TRAIT_ENUMERATOR->TypeParameters[0] } });
		builder.AddMethod(L"GetEnumerator", enumeratorReturnType, LINK_INSTANCE)
			.IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
			.SetCallback(&primitive_array_get_enumerator);
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
		SymbolBuilder<ClassSymbol> builder = SymbolBuilder<ClassSymbol>(table, L"runtime", SymbolTable::Global::Namespace);
		SymbolTable::StandardTypes::Runtime = builder
			.DeclareGlobal();
		
		SymbolTable::StandardTypes::RuntimeCaptureStackTrace = builder.AddMethod(L"capture_stack_trace", SymbolTable::Primitives::String, LINK_STATIC)
			.SetCallback(&runtime_capture_stack_trace);
	}

	// RuntimeException class
	{
		SymbolBuilder<ClassSymbol> builder = SymbolBuilder<ClassSymbol>(table, L"RuntimeException", SymbolTable::Global::Namespace);
		SymbolTable::StandardTypes::RuntimeException = builder
			.Implements(TRAIT_THROWABLE)
			.DeclareGlobal();

		builder
			.AddInit();

		SymbolBuilder<PropertySymbol> messageProp = builder.AddProperty(L"message", SymbolTable::Primitives::String, LINK_INSTANCE);
		SymbolTable::StandardTypes::RuntimeExceptionMessageField = messageProp
			.AddBackingField();

		messageProp.AddGetter()
			.IsImplementationOf(TRAIT_THROWABLE_getMessage)
			.SetCallback([](const CallState& context) { return context.Args[0]->GetField(SymbolTable::StandardTypes::RuntimeExceptionMessageField->SlotIndex); });
			
		SymbolBuilder<PropertySymbol> stackTraceProp = builder.AddProperty(L"message", SymbolTable::Primitives::String, LINK_INSTANCE);
		SymbolTable::StandardTypes::RuntimeExceptionStackTraceField = stackTraceProp
			.AddBackingField();

		stackTraceProp.AddGetter()
			.IsImplementationOf(TRAIT_THROWABLE_getStackTrace)
			.SetCallback([](const CallState& context) { return context.Args[0]->GetField(SymbolTable::StandardTypes::RuntimeExceptionStackTraceField->SlotIndex); });

		// RuntimeException is created implicitly and does not go through normal
		// layout generation; lay it out manually so the runtime can instantiate it.
		inherit_size(SymbolTable::StandardTypes::RuntimeExceptionMessageField);
		inherit_size(SymbolTable::StandardTypes::RuntimeExceptionStackTraceField);

		SymbolTable::StandardTypes::RuntimeException->AnalysisState = SymbolAnalysisState::Ready;
		SymbolTable::StandardTypes::RuntimeException->LayoutingState = TypeLayoutingState::Visited;
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
	SymbolTable::Global::Namespace->Node = new NamespaceNode();

	ResolvePrimitives(global);
	ResolveInterfaces(global); // Resolve standard interface traits before they are used below.
	ResolveEnumerables(global);
	ResolveStandards(global);

	// Make standard primitives implement IPrintable
	make_printable<StructSymbol>(SymbolTable::Primitives::Boolean, global, &primitive_boolean_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::Integer, global, &primitive_integer_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::Double, global, &primitive_double_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::Char, global, &primitive_char_to_string);
	make_printable<ClassSymbol>(SymbolTable::Primitives::String, global, &primitive_string_to_string);
	make_printable<ClassSymbol>(SymbolTable::Primitives::Array, global, &primitive_array_to_string);
	make_printable<StructSymbol>(SymbolTable::Primitives::NativeInteger, global, &primitive_nint_to_string);

	SymbolFactory factory(table);
	RegisterPrimitiveOperators(factory);

	global->MarkAllSymbolsReady();

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

void SymbolTable::MarkAllSymbolsReady()
{
	for (const auto& symbol : namespacesList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);

	for (const auto& symbol : typesList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);

	for (const auto& symbol : membersList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);

	for (const auto& symbol : triviasList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
}

void SymbolTable::MarkJustCreatedSymbolsReady()
{
	for (const auto& symbol : namespacesList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}

	for (const auto& symbol : typesList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}

	for (const auto& symbol : membersList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}

	for (const auto& symbol : triviasList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}
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
