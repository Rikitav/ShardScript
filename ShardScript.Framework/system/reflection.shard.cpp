#include <ShardScript.hpp>

#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <cstdint>
#include <string>
#include <vector>

using namespace shard;

// =========================================================================
// Internal state
// =========================================================================

static SymbolTable* g_symbolTable = nullptr;

static ClassSymbol* typeClass_raw = nullptr;
static ClassSymbol* methodInfoClass_raw = nullptr;
static ClassSymbol* fieldInfoClass_raw = nullptr;
static ClassSymbol* propertyInfoClass_raw = nullptr;
static ClassSymbol* parameterInfoClass_raw = nullptr;

static FieldSymbol* type_handleField = nullptr;
static FieldSymbol* methodInfo_handleField = nullptr;
static FieldSymbol* fieldInfo_handleField = nullptr;
static FieldSymbol* propertyInfo_handleField = nullptr;
static FieldSymbol* parameterInfo_handleField = nullptr;

// =========================================================================
// Helpers
// =========================================================================

template <typename T>
static T* HandleTo(ObjectInstance* wrapper, FieldSymbol* handleField, CallStackFrame* frame)
{
	std::int64_t handle = wrapper->GetField(handleField->SlotIndex, frame)->AsInteger();
	if (handle == 0)
		return nullptr;

	return reinterpret_cast<T*>(static_cast<std::uintptr_t>(handle));
}

static std::int64_t PointerToHandle(void* pointer)
{
	return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

static ObjectInstance* MakeArray(TypeSymbol* elementType,
	const std::vector<ObjectInstance*>& items,
	const CallState& context)
{
	ObjectInstance* array = context.Collector.AllocateArray(elementType, items.size());
	for (std::size_t i = 0; i < items.size(); ++i)
		array->SetElement(i, items[i], context.Frame);

	return array;
}

// =========================================================================
// Type wrapper
// =========================================================================

static ObjectInstance* MakeType(TypeSymbol* type, const CallState& context)
{
	if (type == nullptr)
		return GarbageCollector::NullInstance;

	ObjectInstance* obj = context.Collector.AllocateInstance(typeClass_raw);
	obj->SetField(type_handleField->SlotIndex,
		context.Collector.FromValue(PointerToHandle(type)),
		context.Frame);

	return obj;
}

static ObjectInstance* shard_type_Of(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	if (instance == GarbageCollector::NullInstance)
		return GarbageCollector::NullInstance;

	return MakeType(const_cast<TypeSymbol*>(instance->getInfo()), context);
}

static TypeSymbol* FindTypeByName(const std::wstring& name)
{
	auto matches = [&](TypeSymbol* type) -> bool
	{
		return type != nullptr && (type->Name == name || type->FullName == name);
	};

	if (matches(SymbolTable::Primitives::Void)) return SymbolTable::Primitives::Void;
	if (matches(SymbolTable::Primitives::Null)) return SymbolTable::Primitives::Null;
	if (matches(SymbolTable::Primitives::Any)) return SymbolTable::Primitives::Any;
	if (matches(SymbolTable::Primitives::Boolean)) return SymbolTable::Primitives::Boolean;
	if (matches(SymbolTable::Primitives::Integer)) return SymbolTable::Primitives::Integer;
	if (matches(SymbolTable::Primitives::Double)) return SymbolTable::Primitives::Double;
	if (matches(SymbolTable::Primitives::Char)) return SymbolTable::Primitives::Char;
	if (matches(SymbolTable::Primitives::String)) return SymbolTable::Primitives::String;
	if (matches(SymbolTable::Primitives::Array)) return SymbolTable::Primitives::Array;
	if (matches(SymbolTable::Primitives::NativeInteger)) return SymbolTable::Primitives::NativeInteger;

	if (matches(SymbolTable::StandardTypes::IThrowable)) return SymbolTable::StandardTypes::IThrowable;
	if (matches(SymbolTable::StandardTypes::IDisposable)) return SymbolTable::StandardTypes::IDisposable;
	if (matches(SymbolTable::StandardTypes::IPrintable)) return SymbolTable::StandardTypes::IPrintable;
	if (matches(SymbolTable::StandardTypes::IEnumerable)) return SymbolTable::StandardTypes::IEnumerable;
	if (matches(SymbolTable::StandardTypes::IEnumerator)) return SymbolTable::StandardTypes::IEnumerator;
	if (matches(SymbolTable::StandardTypes::ArrayEnumerator)) return SymbolTable::StandardTypes::ArrayEnumerator;
	if (matches(SymbolTable::StandardTypes::Runtime)) return SymbolTable::StandardTypes::Runtime;
	if (matches(SymbolTable::StandardTypes::RuntimeException)) return SymbolTable::StandardTypes::RuntimeException;

	if (g_symbolTable != nullptr)
	{
		for (TypeSymbol* type : g_symbolTable->GetTypeSymbols())
		{
			if (type->Name == name || type->FullName == name)
				return type;
		}
	}

	return nullptr;
}

static ObjectInstance* shard_type_GetType_name(const CallState& context) noexcept(false)
{
	const wchar_t* name = context.Args[0]->AsString();
	if (name == nullptr)
		return GarbageCollector::NullInstance;

	TypeSymbol* type = FindTypeByName(name);
	if (type == nullptr)
		throw std::runtime_error("type not found by name");

	return MakeType(type, context);
}

static ObjectInstance* shard_type_Name_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(type->Name);
}

static ObjectInstance* shard_type_FullName_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(type->FullName);
}

static ObjectInstance* shard_type_Namespace_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr)
		return context.Collector.FromValue(std::wstring());

	const std::wstring& fullName = type->FullName;
	std::size_t pos = fullName.find_last_of(L'.');
	if (pos == std::wstring::npos)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(fullName.substr(0, pos));
}

static ObjectInstance* shard_type_IsArray_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	return context.Collector.FromValue(type != nullptr && type->Kind == SyntaxKind::ArrayType);
}

static ObjectInstance* shard_type_IsClass_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	return context.Collector.FromValue(type != nullptr && type->Kind == SyntaxKind::ClassDeclaration);
}

static ObjectInstance* shard_type_IsStruct_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	return context.Collector.FromValue(type != nullptr && type->Kind == SyntaxKind::StructDeclaration);
}

static ObjectInstance* shard_type_IsInterface_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	return context.Collector.FromValue(type != nullptr && type->Kind == SyntaxKind::InterfaceDeclaration);
}

static ObjectInstance* shard_type_IsEnum_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	return context.Collector.FromValue(type != nullptr && type->Kind == SyntaxKind::EnumDeclaration);
}

static ObjectInstance* shard_type_IsGeneric_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	return context.Collector.FromValue(type != nullptr && type->Kind == SyntaxKind::GenericType);
}

static ObjectInstance* shard_type_IsPrimitive_get(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	return context.Collector.FromValue(type != nullptr && SemanticModel::IsPrimitiveType(type));
}

static ObjectInstance* shard_type_GetElementType(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr || type->Kind != SyntaxKind::ArrayType)
		return GarbageCollector::NullInstance;

	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
	return MakeType(arrayType->UnderlayingType, context);
}

static ObjectInstance* shard_type_GetInterfaces(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr)
		return MakeArray(typeClass_raw, {}, context);

	std::vector<ObjectInstance*> items;
	items.reserve(type->Interfaces.size());
	for (TypeSymbol* iface : type->Interfaces)
		items.push_back(MakeType(iface, context));

	return MakeArray(typeClass_raw, items, context);
}

static ObjectInstance* shard_type_IsAssignableFrom(const CallState& context) noexcept(false)
{
	TypeSymbol* target = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	TypeSymbol* source = HandleTo<TypeSymbol>(context.Args[1], type_handleField, context.Frame);

	if (target == nullptr || source == nullptr)
		return context.Collector.FromValue(false);

	return context.Collector.FromValue(SemanticModel::IsAssignableTo(target, source));
}

// =========================================================================
// ParameterInfo wrapper
// =========================================================================

static ObjectInstance* MakeParameterInfo(ParameterSymbol* param, const CallState& context)
{
	if (param == nullptr)
		return GarbageCollector::NullInstance;

	ObjectInstance* obj = context.Collector.AllocateInstance(parameterInfoClass_raw);
	obj->SetField(parameterInfo_handleField->SlotIndex,
		context.Collector.FromValue(PointerToHandle(param)),
		context.Frame);

	return obj;
}

static ObjectInstance* shard_parameterinfo_Name_get(const CallState& context) noexcept(false)
{
	ParameterSymbol* param = HandleTo<ParameterSymbol>(context.Args[0], parameterInfo_handleField, context.Frame);
	if (param == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(param->Name);
}

static ObjectInstance* shard_parameterinfo_ParameterType_get(const CallState& context) noexcept(false)
{
	ParameterSymbol* param = HandleTo<ParameterSymbol>(context.Args[0], parameterInfo_handleField, context.Frame);
	if (param == nullptr)
		return GarbageCollector::NullInstance;

	return MakeType(param->Type, context);
}

// =========================================================================
// MethodInfo wrapper
// =========================================================================

static ObjectInstance* MakeMethodInfo(MethodSymbol* method, const CallState& context)
{
	if (method == nullptr)
		return GarbageCollector::NullInstance;

	ObjectInstance* obj = context.Collector.AllocateInstance(methodInfoClass_raw);
	obj->SetField(methodInfo_handleField->SlotIndex,
		context.Collector.FromValue(PointerToHandle(method)),
		context.Frame);

	return obj;
}

static ObjectInstance* shard_methodinfo_Name_get(const CallState& context) noexcept(false)
{
	MethodSymbol* method = HandleTo<MethodSymbol>(context.Args[0], methodInfo_handleField, context.Frame);
	if (method == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(method->Name);
}

static ObjectInstance* shard_methodinfo_ReturnType_get(const CallState& context) noexcept(false)
{
	MethodSymbol* method = HandleTo<MethodSymbol>(context.Args[0], methodInfo_handleField, context.Frame);
	if (method == nullptr)
		return GarbageCollector::NullInstance;

	return MakeType(method->ReturnType, context);
}

static ObjectInstance* shard_methodinfo_IsStatic_get(const CallState& context) noexcept(false)
{
	MethodSymbol* method = HandleTo<MethodSymbol>(context.Args[0], methodInfo_handleField, context.Frame);
	return context.Collector.FromValue(method != nullptr && method->Linking == SymbolLinking::Static);
}

static ObjectInstance* shard_methodinfo_GetParameters(const CallState& context) noexcept(false)
{
	MethodSymbol* method = HandleTo<MethodSymbol>(context.Args[0], methodInfo_handleField, context.Frame);
	if (method == nullptr)
		return MakeArray(parameterInfoClass_raw, {}, context);

	std::vector<ObjectInstance*> items;
	items.reserve(method->Parameters.size());
	for (ParameterSymbol* param : method->Parameters)
		items.push_back(MakeParameterInfo(param, context));

	return MakeArray(parameterInfoClass_raw, items, context);
}

// =========================================================================
// FieldInfo wrapper
// =========================================================================

static ObjectInstance* MakeFieldInfo(FieldSymbol* field, const CallState& context)
{
	if (field == nullptr)
		return GarbageCollector::NullInstance;

	ObjectInstance* obj = context.Collector.AllocateInstance(fieldInfoClass_raw);
	obj->SetField(fieldInfo_handleField->SlotIndex,
		context.Collector.FromValue(PointerToHandle(field)),
		context.Frame);

	return obj;
}

static ObjectInstance* shard_fieldinfo_Name_get(const CallState& context) noexcept(false)
{
	FieldSymbol* field = HandleTo<FieldSymbol>(context.Args[0], fieldInfo_handleField, context.Frame);
	if (field == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(field->Name);
}

static ObjectInstance* shard_fieldinfo_FieldType_get(const CallState& context) noexcept(false)
{
	FieldSymbol* field = HandleTo<FieldSymbol>(context.Args[0], fieldInfo_handleField, context.Frame);
	if (field == nullptr)
		return GarbageCollector::NullInstance;

	return MakeType(field->ReturnType, context);
}

static ObjectInstance* shard_fieldinfo_IsStatic_get(const CallState& context) noexcept(false)
{
	FieldSymbol* field = HandleTo<FieldSymbol>(context.Args[0], fieldInfo_handleField, context.Frame);
	return context.Collector.FromValue(field != nullptr && field->Linking == SymbolLinking::Static);
}

// =========================================================================
// PropertyInfo wrapper
// =========================================================================

static ObjectInstance* MakePropertyInfo(PropertySymbol* property, const CallState& context)
{
	if (property == nullptr)
		return GarbageCollector::NullInstance;

	ObjectInstance* obj = context.Collector.AllocateInstance(propertyInfoClass_raw);
	obj->SetField(propertyInfo_handleField->SlotIndex,
		context.Collector.FromValue(PointerToHandle(property)),
		context.Frame);

	return obj;
}

static ObjectInstance* shard_propertyinfo_Name_get(const CallState& context) noexcept(false)
{
	PropertySymbol* property = HandleTo<PropertySymbol>(context.Args[0], propertyInfo_handleField, context.Frame);
	if (property == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(property->Name);
}

static ObjectInstance* shard_propertyinfo_PropertyType_get(const CallState& context) noexcept(false)
{
	PropertySymbol* property = HandleTo<PropertySymbol>(context.Args[0], propertyInfo_handleField, context.Frame);
	if (property == nullptr)
		return GarbageCollector::NullInstance;

	return MakeType(property->ReturnType, context);
}

static ObjectInstance* shard_propertyinfo_IsStatic_get(const CallState& context) noexcept(false)
{
	PropertySymbol* property = HandleTo<PropertySymbol>(context.Args[0], propertyInfo_handleField, context.Frame);
	return context.Collector.FromValue(property != nullptr && property->Linking == SymbolLinking::Static);
}

// =========================================================================
// Type enumeration helpers (added after helper classes are defined)
// =========================================================================

static ObjectInstance* shard_type_GetMethods(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr)
		return MakeArray(methodInfoClass_raw, {}, context);

	TypeSymbol* effective = type;
	if (effective->Kind == SyntaxKind::GenericType)
		effective = static_cast<GenericTypeSymbol*>(effective)->UnderlayingType;

	std::vector<ObjectInstance*> items;
	items.reserve(effective->Methods.size());
	for (MethodSymbol* method : effective->Methods)
		items.push_back(MakeMethodInfo(method, context));

	return MakeArray(methodInfoClass_raw, items, context);
}

static ObjectInstance* shard_type_GetFields(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr)
		return MakeArray(fieldInfoClass_raw, {}, context);

	TypeSymbol* effective = type;
	if (effective->Kind == SyntaxKind::GenericType)
		effective = static_cast<GenericTypeSymbol*>(effective)->UnderlayingType;

	std::vector<ObjectInstance*> items;
	items.reserve(effective->Fields.size());
	for (FieldSymbol* field : effective->Fields)
		items.push_back(MakeFieldInfo(field, context));

	return MakeArray(fieldInfoClass_raw, items, context);
}

static ObjectInstance* shard_type_GetProperties(const CallState& context) noexcept(false)
{
	TypeSymbol* type = HandleTo<TypeSymbol>(context.Args[0], type_handleField, context.Frame);
	if (type == nullptr)
		return MakeArray(propertyInfoClass_raw, {}, context);

	TypeSymbol* effective = type;
	if (effective->Kind == SyntaxKind::GenericType)
		effective = static_cast<GenericTypeSymbol*>(effective)->UnderlayingType;

	std::vector<ObjectInstance*> items;
	items.reserve(effective->Properties.size());
	for (PropertySymbol* property : effective->Properties)
		items.push_back(MakePropertyInfo(property, context));

	return MakeArray(propertyInfoClass_raw, items, context);
}

// =========================================================================
// Library metadata and entry point
// =========================================================================

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.reflection";
	lib.Description = L"Lightweight runtime reflection helpers";
	lib.Version = L"0.1.0";
}

SHARDLIB_ENTRYPOINT
{
	g_symbolTable = context.GetSemanticModel().Table.get();
	SymbolFactory factory(context.GetSemanticModel().Table.get());

	SymbolBuilder<NamespaceSymbol> reflectionNs(context, L"reflection");

	// --- class Type ---
	SymbolBuilder<ClassSymbol> typeClass = reflectionNs.AddClass(L"Type");
	typeClass_raw = typeClass.Get();

	type_handleField = typeClass
		.AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE).Get();

	typeClass.AddMethod(L"Of", typeClass.Get(), LINK_STATIC)
		.AddParameter(L"object", TYPE_ANY)
		.SetCallback(&shard_type_Of);

	typeClass.AddMethod(L"GetType", typeClass.Get(), LINK_STATIC)
		.AddParameter(L"name", TYPE_STRING)
		.SetCallback(&shard_type_GetType_name);

	typeClass.AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_Name_get);

	typeClass.AddProperty(L"FullName", TYPE_STRING, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_FullName_get);

	typeClass.AddProperty(L"Namespace", TYPE_STRING, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_Namespace_get);

	typeClass.AddProperty(L"IsArray", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_IsArray_get);

	typeClass.AddProperty(L"IsClass", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_IsClass_get);

	typeClass.AddProperty(L"IsStruct", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_IsStruct_get);

	typeClass.AddProperty(L"IsInterface", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_IsInterface_get);

	typeClass.AddProperty(L"IsEnum", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_IsEnum_get);

	typeClass.AddProperty(L"IsGeneric", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_IsGeneric_get);

	typeClass.AddProperty(L"IsPrimitive", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_type_IsPrimitive_get);

	typeClass.AddMethod(L"GetElementType", typeClass.Get(), LINK_INSTANCE)
		.SetCallback(&shard_type_GetElementType);

	typeClass.AddMethod(L"GetInterfaces", factory.Array(typeClass.Get()), LINK_INSTANCE)
		.SetCallback(&shard_type_GetInterfaces);

	typeClass.AddMethod(L"IsAssignableFrom", TYPE_BOOL, LINK_INSTANCE)
		.AddParameter(L"other", typeClass.Get())
		.SetCallback(&shard_type_IsAssignableFrom);

	// --- class ParameterInfo ---
	SymbolBuilder<ClassSymbol> parameterInfoClass = reflectionNs.AddClass(L"ParameterInfo");
	parameterInfoClass_raw = parameterInfoClass.Get();

	parameterInfo_handleField = parameterInfoClass
		.AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE).Get();

	parameterInfoClass.AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_parameterinfo_Name_get);

	parameterInfoClass.AddProperty(L"ParameterType", typeClass.Get(), LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_parameterinfo_ParameterType_get);

	// --- class MethodInfo ---
	SymbolBuilder<ClassSymbol> methodInfoClass = reflectionNs.AddClass(L"MethodInfo");
	methodInfoClass_raw = methodInfoClass.Get();

	methodInfo_handleField = methodInfoClass
		.AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE).Get();

	methodInfoClass.AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_methodinfo_Name_get);

	methodInfoClass.AddProperty(L"ReturnType", typeClass.Get(), LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_methodinfo_ReturnType_get);

	methodInfoClass.AddProperty(L"IsStatic", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_methodinfo_IsStatic_get);

	methodInfoClass.AddMethod(L"GetParameters", factory.Array(parameterInfoClass.Get()), LINK_INSTANCE)
		.SetCallback(&shard_methodinfo_GetParameters);

	// --- class FieldInfo ---
	SymbolBuilder<ClassSymbol> fieldInfoClass = reflectionNs.AddClass(L"FieldInfo");
	fieldInfoClass_raw = fieldInfoClass.Get();

	fieldInfo_handleField = fieldInfoClass
		.AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE).Get();

	fieldInfoClass.AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_fieldinfo_Name_get);

	fieldInfoClass.AddProperty(L"FieldType", typeClass.Get(), LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_fieldinfo_FieldType_get);

	fieldInfoClass.AddProperty(L"IsStatic", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_fieldinfo_IsStatic_get);

	// --- class PropertyInfo ---
	SymbolBuilder<ClassSymbol> propertyInfoClass = reflectionNs.AddClass(L"PropertyInfo");
	propertyInfoClass_raw = propertyInfoClass.Get();

	propertyInfo_handleField = propertyInfoClass
		.AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE).Get();

	propertyInfoClass.AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_propertyinfo_Name_get);

	propertyInfoClass.AddProperty(L"PropertyType", typeClass.Get(), LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_propertyinfo_PropertyType_get);

	propertyInfoClass.AddProperty(L"IsStatic", TYPE_BOOL, LINK_INSTANCE)
		.AddGetter().SetCallback(&shard_propertyinfo_IsStatic_get);

	// --- Type enumeration methods (need helper classes already defined) ---
	typeClass.AddMethod(L"GetMethods", factory.Array(methodInfoClass.Get()), LINK_INSTANCE)
		.SetCallback(&shard_type_GetMethods);

	typeClass.AddMethod(L"GetFields", factory.Array(fieldInfoClass.Get()), LINK_INSTANCE)
		.SetCallback(&shard_type_GetFields);

	typeClass.AddMethod(L"GetProperties", factory.Array(propertyInfoClass.Get()), LINK_INSTANCE)
		.SetCallback(&shard_type_GetProperties);
}
