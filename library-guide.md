# Native Library Authoring Guide for ShardScript

This guide describes how to write dynamic libraries (`.dll` / `.so`) that extend ShardScript with new types, methods, and callbacks implemented in C++. It is aimed at library authors who want to work with the ShardScript API directly, without the C# wrapper.

---

## 1. What Happens When a Library Is Loaded

When the ShardScript engine loads a library (for example via `context.AddLibrary("MyLib.dll")` from C# or a similar mechanism in the native interpreter), it performs the following steps:

1. Loads the dynamic library.
2. Looks for two exported symbols:
   - `ShardLib_GetMetadata` — fills basic metadata (name, version, description).
   - `ShardLib_EntryPoint` — the entry point where the library registers all symbols.
3. Calls `ShardLib_EntryPoint`, passing a `CompilationContext&`.
4. Inside the entry point you create symbols (namespace, class, method, field, parameter) and bind C++ functions to methods.

---

## 2. Key Concepts

### 2.1. Symbols (`SyntaxSymbol`)

A symbol is an object in the ShardScript semantic model that represents a language entity. The main symbol classes are:

| Class | Represents |
|-------|------------|
| `NamespaceSymbol` | Namespace (`namespace foo;`) |
| `ClassSymbol` | Class (`class Foo { }`) |
| `StructSymbol` | Structure (`struct Bar { }`) |
| `MethodSymbol` | Method / function (`func Add(a: int, b: int) -> int`) |
| `OperatorSymbol` | Operator overload (`operator +`, `operator .`) |
| `ConstructorSymbol` | Constructor (`init() { }`) |
| `FieldSymbol` | Field (`x: int`) |
| `PropertySymbol` | Property (`Value: int { get; set; }`) |
| `ParameterSymbol` | Method parameter |
| `TypeParameterSymbol` | Generic type parameter (`T`) |
| `GenericTypeSymbol` | Concrete generic instantiation (`Box<int>`) |

Every symbol has:

- `Name` — short name.
- `FullName` — fully qualified name (`foo.Bar.Baz`).
- `Parent` — parent symbol.
- `Accesibility` — `Public` or `Private`.
- `Kind` — node kind (`SyntaxKind::ClassDeclaration`, `SyntaxKind::MethodDeclaration`, etc.).

Symbols do not own AST nodes (`SyntaxNode`). They live in the `SymbolTable`, which is owned by the semantic model.

### 2.2. `SymbolTable` and `NamespaceTree`

- `SymbolTable` — the table of all symbols. Symbols are registered via `BindSymbol(node, symbol)` (when an AST node exists) or `ImplicitSymbol(symbol)` (when there is no node, as in native libraries).
- `NamespaceTree` — the namespace tree. Each `NamespaceNode` contains:
  - `Owners` — a list of `NamespaceSymbol*` associated with this node.
  - `Members` — all types/members declared in this namespace.

When you create a `ClassSymbol` inside a `NamespaceSymbol` and call `ns->OnSymbolDeclared(cls)`, the class is added to both `ns->Members` and `ns->Node->Members`. This lets scripts see the type through `using` or through the namespace.

### 2.3. `MethodHandleType`

`MethodSymbol` has a `HandleType` field that determines how the method is invoked:

- `MethodHandleType::Body` — ordinary method with VM bytecode body.
- `MethodHandleType::External` — native method invoked through `FunctionPointer`.
- `MethodHandleType::Lambda` — anonymous function.

For native libraries, `External` is the relevant value.

### 2.4. `CallState`

When a native method is called, the engine passes a `CallState` structure to the callback:

```cpp
struct CallState
{
    ApplicationDomain& Domain;
    ProgramVirtualImage& Program;
    VirtualMachine& Runtimer;
    GarbageCollector& Collector;

    CallStackFrame* Frame;
    MethodSymbol* Method;
    ArgumentsSpan& Args;  // std::span<ObjectInstance*>
};
```

Fields:

- `Domain` — the application domain.
- `Collector` — the garbage collector; used to create return values.
- `Frame` — the current call-stack frame.
- `Method` — the symbol of the called method.
- `Args` — argument array; `Args[0]` is the first argument.

Callback function type:

```cpp
using MethodSymbolDelegate = shard::ObjectInstance* (*)(const shard::CallState& context);
```

### 2.5. `ObjectInstance` and Reading Values

`ObjectInstance` is a wrapper around a value on the ShardScript heap.

Main methods:

```cpp
std::int64_t& AsInteger() const;
double& AsDouble() const;
bool& AsBoolean() const;
wchar_t& AsCharacter() const;
const wchar_t* AsString() const;

void WriteInteger(const std::int64_t& value) const;
void WriteDouble(const double& value) const;
void WriteBoolean(const bool& value) const;
void WriteCharacter(const wchar_t& value) const;
void WriteString(const std::wstring& value) const;

ObjectInstance* GetField(FieldSymbol* field, CallStackFrame* frame = nullptr);
void SetField(FieldSymbol* field, ObjectInstance* instance, CallStackFrame* frame = nullptr);

ObjectInstance* GetElement(std::size_t index, CallStackFrame* frame = nullptr);
void SetElement(std::size_t index, ObjectInstance* instance, CallStackFrame* frame = nullptr);

std::size_t GetArrayLength() const;
```

To create new values, use the `GarbageCollector`:

```cpp
ObjectInstance* FromValue(std::int64_t value);
ObjectInstance* FromValue(double value);
ObjectInstance* FromValue(bool value);
ObjectInstance* FromValue(wchar_t value);
ObjectInstance* FromValue(const std::wstring& value);
ObjectInstance* FromValue(const wchar_t* value, bool isTransient);

ObjectInstance* AllocateInstance(const TypeSymbol* objectInfo, bool isTransient = false);
ObjectInstance* AllocateArray(TypeSymbol* elementType, std::size_t length, bool isTransient = false);
```

---

## 3. Minimal Native Library

### 3.1. Step 1 — Metadata and Entry Point

Every library must export two functions. The macros in `<shard/ShardScriptLIB.hpp>` make this easy:

```cpp
#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>

SHARDLIB_GETMETADATA
{
    lib.Name        = L"MyMath";
    lib.Description = L"Example native math library for ShardScript";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    // Symbol registration happens here.
}
```

`SHARDLIB_GETMETADATA` and `SHARDLIB_ENTRYPOINT` already include the correct `extern "C"` and visibility attributes for Windows (`__declspec(dllexport)`) and GCC/Clang (`visibility("default")`).

### 3.2. Step 2 — Creating Namespace and Class via `SymbolBuilder`

Instead of manually calling `SymbolFactory`, use the fluent `SymbolBuilder<T>` builder. It automatically sets `Parent`, `FullName`, `Accesibility`, registers the symbol in the `NamespaceTree`, and calls `OnSymbolDeclared`.

Available specializations:

- `SymbolBuilder<NamespaceSymbol>(ctx, name[, parent])`
  - `AddClass(name[, access])` → `SymbolBuilder<ClassSymbol>`
  - `AddNamespace(name[, access])` → `SymbolBuilder<NamespaceSymbol>`
- `SymbolBuilder<ClassSymbol>(ctx, name[, parent])`
  - `AddMethod(name, returnType[, isStatic][, access])` → `SymbolBuilder<MethodSymbol>`
  - `AddField(name, type[, isStatic][, access])` → `SymbolBuilder<FieldSymbol>`
- `SymbolBuilder<MethodSymbol>`
  - `AddParameter(name, type)` → reference to itself
  - `SetCallback(fn)` / `SetCallback(managedFn, userData)` → reference to itself

`SymbolBuilder` is non-copyable and moveable. You can get a pointer to the symbol via `Get()` or implicit conversion:

```cpp
#include <shard/syntax/SymbolBuilder.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>

using namespace shard;

SymbolBuilder<NamespaceSymbol> ns(ctx, L"mymath");
ClassSymbol* math = ns.AddClass(L"Math");   // implicit conversion to ClassSymbol*
```

Nested namespaces:

```cpp
SymbolBuilder<NamespaceSymbol> root(ctx, L"company");
SymbolBuilder<NamespaceSymbol> math = root.AddNamespace(L"math");
math.AddClass(L"Algorithms");
```

### 3.3. Step 3 — Creating a Method and Binding a C++ Function

```cpp
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

using namespace shard;

static ObjectInstance* MyAdd(const CallState& ctx)
{
    std::int64_t a = ctx.Args[0]->AsInteger();
    std::int64_t b = ctx.Args[1]->AsInteger();
    return ctx.Collector.FromValue(a + b);
}

// inside SHARDLIB_ENTRYPOINT
ns.AddClass(L"Math")
  .AddMethod(L"Add", SymbolTable::Primitives::Integer)
      .AddParameter(L"a", SymbolTable::Primitives::Integer)
      .AddParameter(L"b", SymbolTable::Primitives::Integer)
      .SetCallback(&MyAdd);
```

`AddMethod` creates a `MethodSymbol`, registers it in the `SymbolTable`, and makes it static by default (`Linking = LINK_STATIC`). `SetCallback` sets `HandleType = External`, `FunctionPointer = fn`, and `IsExtern = true`.

### 3.4. Step 4 — Parameters

`AddParameter` adds a `ParameterSymbol` to `MethodSymbol::Parameters`. The order of calls must match the order of arguments when calling from ShardScript.

```cpp
ns.AddClass(L"Math")
    .AddMethod(L"Pow", SymbolTable::Primitives::Double)
        .AddParameter(L"base", SymbolTable::Primitives::Double)
        .AddParameter(L"exp",  SymbolTable::Primitives::Double)
        .SetCallback(&MyPow);
```

### 3.5. Complete `MyMath.cpp` File

```cpp
#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/syntax/SymbolBuilder.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <cmath>

using namespace shard;

static ObjectInstance* MyAdd(const CallState& ctx)
{
    std::int64_t a = ctx.Args[0]->AsInteger();
    std::int64_t b = ctx.Args[1]->AsInteger();
    return ctx.Collector.FromValue(a + b);
}

static ObjectInstance* MyPow(const CallState& ctx)
{
    double base = ctx.Args[0]->AsDouble();
    double exp  = ctx.Args[1]->AsDouble();
    return ctx.Collector.FromValue(std::pow(base, exp));
}

SHARDLIB_GETMETADATA
{
    lib.Name        = L"MyMath";
    lib.Description = L"Native math library for ShardScript";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    auto math = SymbolBuilder<NamespaceSymbol>(context, L"mymath").AddClass(L"Math");

    math.AddMethod(L"Add", SymbolTable::Primitives::Integer)
        .AddParameter(L"a", SymbolTable::Primitives::Integer)
        .AddParameter(L"b", SymbolTable::Primitives::Integer)
        .SetCallback(&MyAdd);

    math.AddMethod(L"Pow", SymbolTable::Primitives::Double)
        .AddParameter(L"base", SymbolTable::Primitives::Double)
        .AddParameter(L"exp",  SymbolTable::Primitives::Double)
        .SetCallback(&MyPow);
}
```

---

## 4. Reading Parameters and Returning Values

### 4.1. Primitive Types

```cpp
static ObjectInstance* CloneInteger(const CallState& ctx)
{
    std::int64_t value = ctx.Args[0]->AsInteger();
    return ctx.Collector.FromValue(value);
}

static ObjectInstance* CloneDouble(const CallState& ctx)
{
    double value = ctx.Args[0]->AsDouble();
    return ctx.Collector.FromValue(value);
}

static ObjectInstance* InvertBool(const CallState& ctx)
{
    bool value = ctx.Args[0]->AsBoolean();
    return ctx.Collector.FromValue(!value);
}

static ObjectInstance* CloneString(const CallState& ctx)
{
    const wchar_t* str = ctx.Args[0]->AsString();
    std::wstring result(str);
    return ctx.Collector.FromValue(result);
}
```

### 4.2. Checking Argument Count

```cpp
static ObjectInstance* SafeAdd(const CallState& ctx)
{
    if (ctx.Args.size() != 2)
    {
        // You may return null or throw an exception.
        // In the current version of ShardScript, exceptions from native code
        // are caught by the VM as a fatal error.
        throw std::runtime_error("Add expects 2 arguments");
    }

    std::int64_t a = ctx.Args[0]->AsInteger();
    std::int64_t b = ctx.Args[1]->AsInteger();
    return ctx.Collector.FromValue(a + b);
}
```

### 4.3. ShardScript ↔ C++ Type Correspondence Table

| ShardScript | C++ Type | Read | Write |
|-------------|----------|------|-------|
| `int` | `std::int64_t` | `AsInteger()` | `WriteInteger(v)` / `FromValue(v)` |
| `double` | `double` | `AsDouble()` | `WriteDouble(v)` / `FromValue(v)` |
| `bool` | `bool` | `AsBoolean()` | `WriteBoolean(v)` / `FromValue(v)` |
| `char` | `wchar_t` | `AsCharacter()` | `WriteCharacter(v)` / `FromValue(v)` |
| `string` | `const wchar_t*` / `std::wstring` | `AsString()` | `WriteString(v)` / `FromValue(v)` |
| `T[]` | `ObjectInstance*` | `GetArrayLength()`, `GetElement(i)` | `AllocateArray(elemType, len)` |
| class/struct | `ObjectInstance*` | `GetField()`, `SetField()` | `AllocateInstance(typeInfo)` |
| `null` | — | `GarbageCollector::NullInstance` | — |

### 4.4. Working with `null`

```cpp
static ObjectInstance* MaybeNull(const CallState& ctx)
{
    ObjectInstance* arg = ctx.Args[0];
    if (arg == GarbageCollector::NullInstance)
    {
        return GarbageCollector::NullInstance;
    }

    return ctx.Collector.FromValue(arg->AsInteger());
}
```

---

## 5. Static and Instance Methods

### 5.1. Static Method

For a static method, `Linking = LINK_STATIC`, and `ctx.Args[0]` is the first parameter.

```cpp
math.AddMethod(L"Add", SymbolTable::Primitives::Integer, true)
    .AddParameter(L"a", SymbolTable::Primitives::Integer)
    .AddParameter(L"b", SymbolTable::Primitives::Integer)
    .SetCallback(&MathAdd);
```

### 5.2. Instance Method

For a non-static method, `Linking = LINK_INSTANCE`, and `ctx.Args[0]` is `this`.

```cpp
static ObjectInstance* CounterNext(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    std::int64_t value = self->AsInteger();
    self->WriteInteger(value + 1);
    return self;
}
```

If `this` is an object with fields:

```cpp
static FieldSymbol* g_valueField = nullptr;

static ObjectInstance* CounterNext(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    ObjectInstance* valueObj = self->GetField(g_valueField, ctx.Frame);
    std::int64_t value = valueObj->AsInteger();
    valueObj->WriteInteger(value + 1);
    return valueObj;
}
```

### 5.3. Constructors

A constructor is a special `MethodSymbolDelegate` that receives an already allocated `this`:

```cpp
static ObjectInstance* CounterCtor(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    std::int64_t initial = ctx.Args[1]->AsInteger();

    ObjectInstance* valueField = self->GetField(g_valueField, ctx.Frame);
    valueField->WriteInteger(initial);

    return self; // or GarbageCollector::NullInstance
}
```

Registration:

```cpp
SymbolFactory factory(ctx.GetSemanticModel().Table.get());

ConstructorSymbol* ctor = factory.Constructor(L"Counter");
ctor->Parent = counterClass;
ctor->FunctionPointer = &CounterCtor;
ctor->HandleType = MethodHandleType::External;
ctor->Accesibility = SymbolAccesibility::Public;

counterClass->Constructors.push_back(ctor);
counterClass->OnSymbolDeclared(ctor);

ParameterSymbol* initial = factory.Parameter(L"initial", SymbolTable::Primitives::Integer);
initial->Parent = ctor;
ctor->Parameters.push_back(initial);
```

### 5.4. Operator Overloads

You can register operator overloads on classes and structs through `AddOperator`. The builder takes the operator token, return type, and linking. The internal name (e.g. `op_AddOperator`, `op_DotOperator`) is generated automatically.

#### 5.4.1. Static Operator

All overloadable operators except the access operator (`.`) must be static.

```cpp
static ObjectInstance* VectorAdd(const CallState& ctx)
{
    ObjectInstance* a = ctx.Args[0];
    ObjectInstance* b = ctx.Args[1];
    // custom logic...
    return ctx.Collector.FromValue(0);
}

auto vecClass = ns.AddClass(L"Vector2");

vecClass.AddOperator(shard::TokenType::AddOperator, SymbolTable::Primitives::Integer, LINK_STATIC)
    .AddParameter(L"a", vecClass)
    .AddParameter(L"b", vecClass)
    .SetCallback(&VectorAdd);
```

#### 5.4.2. Access Operator

The access operator (`.`) takes a single `string` parameter and returns the dynamic member value. It can be declared either as an instance member or as a static member:

- **Instance** — invoked on an object (`obj.member`). The callback receives `this` as `Args[0]` and the member name as `Args[1]`.
- **Static** — invoked on the type itself (`TypeName.member`). The callback receives only the member name as `Args[0]`.

A type that declares an access operator cannot have public fields.

**Instance example:**

```cpp
static ObjectInstance* DynamicAccess(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    const wchar_t* name = ctx.Args[1]->AsString();
    // Resolve by name and return value...
    return ctx.Collector.FromValue(42);
}

auto dynamicClass = ns.AddClass(L"Dynamic");

dynamicClass.AddOperator(shard::TokenType::Delimeter, SymbolTable::Primitives::Integer, LINK_INSTANCE)
    .AddParameter(L"name", SymbolTable::Primitives::String)
    .SetCallback(&DynamicAccess);
```

```shard
using mylib;

d := Dynamic();
println(d.foo);   // calls DynamicAccess(d, "foo")
```

**Static example:**

```cpp
#include <cstdlib>

static ObjectInstance* EnvironmentAccess(const CallState& ctx)
{
    const wchar_t* name = ctx.Args[0]->AsString();
    const wchar_t* value = _wgetenv(name);
    if (value == nullptr)
        value = L"";

    return ctx.Collector.FromValue(std::wstring(value));
}

auto envClass = ns.AddClass(L"Environment");
envClass.Get()->Linking = LINK_STATIC;

envClass.AddOperator(shard::TokenType::Delimeter, SymbolTable::Primitives::String, LINK_STATIC)
    .AddParameter(L"name", SymbolTable::Primitives::String)
    .SetCallback(&EnvironmentAccess);
```

```shard
using environment;

println(Environment.PROCESSOR_ARCHITECTURE);
println(Environment.COMPUTERNAME);
```

> **Note:** when the access operator is static, the callback receives the member name as `Args[0]`. There is no implicit `this`.

---

## 6. Fields and Properties

### 6.1. Registering a Field

```cpp
ns.AddClass(L"Math")
    .AddField(L"Pi", SymbolTable::Primitives::Double, true);   // static

auto counter = ns.AddClass(L"Counter");
counter.AddField(L"Value", SymbolTable::Primitives::Integer); // instance
```

### 6.2. Static Field

For static fields, use `GarbageCollector::SetStaticField` / `GetStaticField`:

```cpp
static FieldSymbol* g_piField = nullptr;

static ObjectInstance* GetPi(const CallState& ctx)
{
    return ctx.Collector.GetStaticField(g_piField);
}
```

Initializing a static field when the library loads:

```cpp
ObjectInstance* piValue = ctx.Collector.FromValue(3.14159265358979);
ctx.Collector.SetStaticField(g_piField, piValue);
```

### 6.3. Properties

A property is registered through `PropertySymbol`, which has `Getter` and `Setter` of type `AccessorSymbol*`. For native properties it is easier to create accessor methods and bind them:

```cpp
PropertySymbol* prop = factory.Property(L"Value", SymbolTable::Primitives::Integer);
prop->Accesibility = SymbolAccesibility::Public;
prop->Parent = cls;
prop->Linking = LINK_INSTANCE;
cls->OnSymbolDeclared(prop);

AccessorSymbol* getter = factory.Getter(L"Value", prop);
getter->FunctionPointer = &GetValue;
getter->HandleType = MethodHandleType::External;

AccessorSymbol* setter = factory.Setter(L"Value", prop);
setter->FunctionPointer = &SetValue;
setter->HandleType = MethodHandleType::External;
```

---

## 7. Arrays

### 7.1. Reading an Array

```cpp
static ObjectInstance* SumArray(const CallState& ctx)
{
    ObjectInstance* arr = ctx.Args[0];
    std::size_t len = arr->GetArrayLength();

    std::int64_t sum = 0;
    for (std::size_t i = 0; i < len; ++i)
    {
        ObjectInstance* elem = arr->GetElement(i, ctx.Frame);
        sum += elem->AsInteger();
    }

    return ctx.Collector.FromValue(sum);
}
```

### 7.2. Creating an Array

```cpp
static ObjectInstance* Range(const CallState& ctx)
{
    std::int64_t from = ctx.Args[0]->AsInteger();
    std::int64_t to   = ctx.Args[1]->AsInteger();
    std::size_t count = static_cast<std::size_t>(to - from + 1);

    ObjectInstance* arr = ctx.Collector.AllocateArray(
        SymbolTable::Primitives::Integer,
        count);

    for (std::size_t i = 0; i < count; ++i)
    {
        ObjectInstance* elem = ctx.Collector.FromValue(from + static_cast<std::int64_t>(i));
        arr->SetElement(i, elem, ctx.Frame);
    }

    return arr;
}
```

---

## 8. Generics in the Raw API

Generics in ShardScript work through two symbol kinds:

- `TypeParameterSymbol` — type parameter (`T`).
- `GenericTypeSymbol` — concrete instantiation (`Box<int>`), storing `UnderlayingType` and a substitution map.

### 8.1. Example: `Identity<T>`

Let us declare a `Container<T>` class with a single static method `Identity` that simply returns its argument.

```cpp
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>

static ObjectInstance* Identity(const CallState& ctx)
{
    // The method simply returns the first argument.
    return ctx.Args[0];
}

static void RegisterGenericContainer(CompilationContext& ctx, NamespaceSymbol* ns)
{
    SymbolFactory factory(ctx.GetSemanticModel().Table.get());

    // 1. Create the open generic type Container<T>.
    ClassSymbol* container = factory.Class(L"Container");
    container->Accesibility = SymbolAccesibility::Public;
    container->IsReferenceType = true;
    container->Parent = ns;
    container->FullName = ns->FullName + L".Container";

    // 2. Type parameter T.
    TypeParameterSymbol* T = factory.TypeParameter(L"T");
    T->Parent = container;
    container->TypeParameters.push_back(T);

    ns->OnSymbolDeclared(container);

    // 3. Static method Container<T>.Identity(T value) -> T.
    MethodSymbol* identity = factory.Method(
        SymbolAccesibility::Public,
        true,
        T,                       // return type is T
        L"Identity",
        &Identity);

    identity->Parent = container;
    identity->FullName = container->FullName + L".Identity";
    container->OnSymbolDeclared(identity);

    ParameterSymbol* param = factory.Parameter(L"value", T);
    param->Parent = identity;
    identity->Parameters.push_back(param);
}
```

Now from ShardScript you can write:

```shard
using mymath;
x := Container<int>.Identity(42);
```

The engine will create a `GenericTypeSymbol` for `Container<int>`, substitute `T = int` during type checking, and call your native callback.

### 8.2. Example: `Box<T>` with a Field

```cpp
static FieldSymbol* g_boxValueField = nullptr;

static ObjectInstance* BoxGetValue(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    return self->GetField(g_boxValueField, ctx.Frame);
}

static ObjectInstance* BoxSetValue(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    ObjectInstance* value = ctx.Args[1];
    self->SetField(g_boxValueField, value, ctx.Frame);
    return GarbageCollector::NullInstance;
}

static void RegisterGenericBox(CompilationContext& ctx, NamespaceSymbol* ns)
{
    SymbolFactory factory(ctx.GetSemanticModel().Table.get());

    ClassSymbol* box = factory.Class(L"Box");
    box->Accesibility = SymbolAccesibility::Public;
    box->IsReferenceType = true;
    box->Parent = ns;
    box->FullName = ns->FullName + L".Box";

    TypeParameterSymbol* T = factory.TypeParameter(L"T");
    T->Parent = box;
    box->TypeParameters.push_back(T);

    ns->OnSymbolDeclared(box);

    g_boxValueField = factory.Field(L"value", T, false);
    g_boxValueField->Accesibility = SymbolAccesibility::Public;
    g_boxValueField->Parent = box;
    g_boxValueField->FullName = box->FullName + L".value";
    box->OnSymbolDeclared(g_boxValueField);

    MethodSymbol* get = factory.Method(
        SymbolAccesibility::Public,
        false,
        T,
        L"GetValue",
        &BoxGetValue);
    get->Parent = box;
    get->FullName = box->FullName + L".GetValue";
    box->OnSymbolDeclared(get);

    MethodSymbol* set = factory.Method(
        SymbolAccesibility::Public,
        false,
        SymbolTable::Primitives::Void,
        L"SetValue",
        &BoxSetValue);
    set->Parent = box;
    set->FullName = box->FullName + L".SetValue";
    box->OnSymbolDeclared(set);

    ParameterSymbol* valueParam = factory.Parameter(L"value", T);
    valueParam->Parent = set;
    set->Parameters.push_back(valueParam);
}
```

Usage:

```shard
using mymath;
b := Box<int>();
b.SetValue(42);
println(b.GetValue());  // 42
```

> **Important:** in the current version of ShardScript, a native constructor for a generic type requires the same registration as for an ordinary class. The engine will allocate a `GenericTypeSymbol` instance and pass it as `this`.

### 8.3. How to Get the Concrete Type Inside a Callback

If a method is called on a concrete instantiation, `ctx.Method->Parent` may be a `GenericTypeSymbol`:

```cpp
static ObjectInstance* DumpType(const CallState& ctx)
{
    TypeSymbol* parent = ctx.Method->Parent;
    if (parent != nullptr && parent->Kind == SyntaxKind::GenericType)
    {
        GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(parent);
        // generic->UnderlayingType is Box
        // Parameter substitutions live inside generic.
    }

    return GarbageCollector::NullInstance;
}
```

---

## 9. Building a Library

All examples below assume ShardScript is already built and located next to your project.

### 9.1. Directory Structure

```text
ShardScript/
├── include/            # ShardScript headers
├── build/bin/          # libShardScript.dll, libShardScript.dll.a, libShardScript.so
└── MyLib/
    ├── MyLib.cpp
    └── CMakeLists.txt
```

### 9.2. Windows + MinGW (GCC)

```bash
cd MyLib
x86_64-w64-mingw32-g++ -shared -O2 -std=c++20 \
    -I../ShardScript/include \
    -L../ShardScript/build/bin \
    -lShardScript \
    -o MyLib.dll \
    MyLib.cpp
```

The `-lShardScript` flag tells the linker to look for:

- `libShardScript.dll.a` (import library) — preferred.
- `libShardScript.a`.

> If you only have a `.dll` without an import library, generate one via `gendef` + `dlltool`, or use `LoadLibrary`/`GetProcAddress` for dynamic loading.

### 9.3. Windows + MSVC

```cmd
cd MyLib
cl /std:c++20 /EHsc /MD /I..\ShardScript\include /LD MyLib.cpp \
   /link ..\ShardScript\build\bin\libShardScript.lib /OUT:MyLib.dll
```

> The `.lib` and `.dll` names depend on the CMake configuration. If the CMake target is named `ShardScript`, MSVC usually produces `ShardScript.dll` + `ShardScript.lib`. Check the contents of `build/bin`.

### 9.4. Linux + GCC / Clang

```bash
cd MyLib
g++ -shared -fPIC -O2 -std=c++20 \
    -I../ShardScript/include \
    -L../ShardScript/build/bin \
    -lShardScript \
    -o libMyLib.so \
    MyLib.cpp
```

At runtime, make sure both `libShardScript.so` and `libMyLib.so` are visible to the loader:

```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/ShardScript/build/bin:/path/to/MyLib
```

### 9.5. CMake Template

```cmake
project(MyLib)

add_library(MyLib SHARED MyLib.cpp)

target_include_directories(MyLib PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../ShardScript/include"
)

target_link_libraries(MyLib PRIVATE
    ShardScript
)

set_target_properties(MyLib PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
```

If you add ShardScript as a subproject, `target_link_libraries(MyLib PRIVATE ShardScript)` will automatically pick up the required include paths and import libraries.

### 9.6. What to Export

You only need to export `ShardLib_GetMetadata` and `ShardLib_EntryPoint`. The `SHARDLIB_GETMETADATA` / `SHARDLIB_ENTRYPOINT` macros do this automatically. Your helper functions (`MyAdd`, `MyPow`, etc.) do not need to be exported.

---

## 10. Shipping Source Code from a Library

A native library can not only register symbols directly, but also feed ShardScript source code into the compiler. This is convenient when part of the API is easier to express in the language itself, while low-level functions are implemented in C++.

### 10.1. `CompilationContext::ProvideSource`

```cpp
#include <shard/CompilationContext.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>

static void ProvideStringSource(CompilationContext& ctx,
                                const wchar_t* fileName,
                                const std::wstring& code)
{
    // StringStreamReader must stay alive until the context is done with it.
    // The simplest approach is to store it statically.
    static std::vector<std::unique_ptr<StringStreamReader>> readers;

    auto reader = std::make_unique<StringStreamReader>(fileName, code);
    StringStreamReader* raw = reader.get();
    readers.push_back(std::move(reader));

    ctx.ProvideSource(raw);
}
```

Usage inside `ShardLib_EntryPoint`:

```cpp
SHARDLIB_ENTRYPOINT
{
    std::wstring code = LR"(
        using stdio;
        namespace mylib;

        public static func Hello() -> void
        {
            println("Hello from embedded source!");
        }
    )";

    ProvideStringSource(context, L"MyLibEmbedded.ss", code);
}
```

After this, `mylib.Hello()` becomes available to any script that loads this library.

### 10.2. Embedded Resources on Windows

To avoid shipping separate `.ss` files, source code can be embedded into the `.dll` as a Win32 resource of type `SOURCE_CODE` and extracted directly from the library image. This is how `ShardScript.Framework` works.

```cpp
#include <Windows.h>
#include <stdexcept>
#include <string>

static void GetEmbeddedSource(const wchar_t* resourceName, std::wstring& out)
{
    HMODULE hModule = nullptr;

    // Get the HMODULE of the current DLL from a function address.
    using CurrentModuleQuery = HMODULE(*)();
    CurrentModuleQuery query = []() -> HMODULE
    {
        HMODULE hm = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(&GetEmbeddedSource),
            &hm);
        return hm;
    };

    hModule = query();
    if (!hModule)
        throw std::runtime_error("cannot get current module handle");

    HRSRC hResource = FindResourceW(hModule, resourceName, L"SOURCE_CODE");
    if (!hResource)
        throw std::runtime_error("resource not found");

    HGLOBAL hData = LoadResource(hModule, hResource);
    if (!hData)
        throw std::runtime_error("cannot load resource");

    std::size_t size = SizeofResource(hModule, hResource);
    const wchar_t* data = static_cast<const wchar_t*>(LockResource(hData));

    out.assign(data, size / sizeof(wchar_t));
}
```

> **Linux:** there are no Win32 resources there. Source code is usually embedded as a string literal via `constexpr const wchar_t*`, or read from a file next to the `.so`.

### 10.3. Example with Embedded Source

```cpp
SHARDLIB_ENTRYPOINT
{
    std::wstring source;
    GetEmbeddedSource(L"MYLIB_CORE", source);

    ProvideStringSource(context, L"MyLibCore.ss", source);
}
```

The `MYLIB_CORE` resource must be described in the project's `.rc` file:

```rc
MYLIB_CORE SOURCE_CODE "MyLibCore.ss"
```

> A real-world example of this approach can be found in `ShardScript.Framework/dllmain.cpp` in this repository.

---

## 11. The `[link]` Attribute

The `[link]` attribute lets you declare a method in ShardScript source as `extern`, and bind its implementation to a C++ function in the same native library that provided the source.

### 11.1. Syntax

```shard
[link("symbol_name")]
public static extern func Add(a: int, b: int) -> int;
```

Two arguments are also parsed:

```shard
[link("library_name", "symbol_name")]
public static extern func Foo() -> void;
```

but currently linking always happens against the library that provided the source file.

### 11.2. How It Works

1. The engine loads the library (`AddLibrary`).
2. The engine calls `ShardLib_EntryPoint`.
3. Inside the entry point, the library passes source code via `ProvideSource`.
4. During analysis, `DeclarationCollector` sees the `[link]` attribute and records the symbol name in `MethodSymbol::LinkSymbol`.
5. `CompilationContext::LinkExternSymbols` looks up this symbol in the loaded library (analogous to `GetProcAddress`) and writes the address into `MethodSymbol::FunctionPointer`.
6. When the method is called, the VM invokes that C++ function through `FunctionPointer`.

### 11.3. Requirements for the C++ Function

The function referenced by `[link]` must:

- Have the signature `shard::ObjectInstance* (*)(const shard::CallState&)`.
- Be exported from the DLL (`extern "C"` + `__declspec(dllexport)` / `visibility("default")`).
- Have a name that exactly matches the string in the attribute.

Example:

```cpp
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

extern "C" __declspec(dllexport) // Windows
shard::ObjectInstance* MyAdd(const shard::CallState& ctx)
{
    std::int64_t a = ctx.Args[0]->AsInteger();
    std::int64_t b = ctx.Args[1]->AsInteger();
    return ctx.Collector.FromValue(a + b);
}
```

> On Linux use `__attribute__((visibility("default")))` instead of `__declspec(dllexport)`.

### 11.4. Full Example: Library + Embedded Source + `[link]`

**MathLibNative.cpp**

```cpp
#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <vector>
#include <memory>
#include <cmath>

using namespace shard;

extern "C" __declspec(dllexport)
ObjectInstance* NativeAdd(const CallState& ctx)
{
    std::int64_t a = ctx.Args[0]->AsInteger();
    std::int64_t b = ctx.Args[1]->AsInteger();
    return ctx.Collector.FromValue(a + b);
}

extern "C" __declspec(dllexport)
ObjectInstance* NativeSqrt(const CallState& ctx)
{
    double value = ctx.Args[0]->AsDouble();
    return ctx.Collector.FromValue(std::sqrt(value));
}

static std::vector<std::unique_ptr<StringStreamReader>> g_readers;

static void ProvideEmbeddedSource(CompilationContext& ctx)
{
    std::wstring code = LR"(
        namespace mathlib;

        [link("NativeAdd")]
        public static extern func Add(a: int, b: int) -> int;

        [link("NativeSqrt")]
        public static extern func Sqrt(value: double) -> double;
    )";

    auto reader = std::make_unique<StringStreamReader>(L"MathLib.ss", code);
    StringStreamReader* raw = reader.get();
    g_readers.push_back(std::move(reader));

    ctx.ProvideSource(raw);
}

SHARDLIB_GETMETADATA
{
    lib.Name        = L"MathLibLink";
    lib.Description = L"[link] example";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    ProvideEmbeddedSource(context);
}
```

Usage from ShardScript:

```shard
using mathlib;

x := mathlib.Add(10, 32);
y := mathlib.Sqrt(16.0);
println(x); // 42
println(y); // 4
```

### 11.5. Global Methods via `SemanticAnalyzer::AddSymbol`

If a method should not belong to any namespace or class, it can be added directly to the global scope:

```cpp
#include <ctime>

static ObjectInstance* GlobalNow(const CallState& ctx)
{
    return ctx.Collector.FromValue(static_cast<std::int64_t>(time(nullptr)));
}

SHARDLIB_ENTRYPOINT
{
    SymbolFactory factory(context.GetSemanticModel().Table.get());

    MethodSymbol* nowMethod = factory.Method(
        SymbolAccesibility::Public,
        true,
        SymbolTable::Primitives::Integer,
        L"now",
        &GlobalNow);

    context.GetSemanticAnalyzer().AddSymbol(nowMethod);
}
```

Now from ShardScript you can simply call:

```shard
t := now();
println(t);
```

> **Important:** `SemanticAnalyzer::AddSymbol` adds the symbol to the global scope. If you add a method with parameters, do not forget to fill `method->Parameters`.

---

## 12. Loading and Using from ShardScript

### 12.1. From the C# Wrapper

```csharp
using ShardScript.NET;

using CompilationContext context = new CompilationContext();
context.AddLibrary(@"C:\path\to\MyLib.dll");

context.AddSource("Test.ss", """
    using mymath;
    x := Math.Add(10, 32);
    println(x);          // 42
    y := Math.Pow(2.0, 10.0);
    println(y);          // 1024
    """, CompilationUnitOrigin.SourceFile);

context.Analyze();
if (context.HasErrors)
{
    Console.WriteLine(context.GetDiagnostics());
    return;
}

using ApplicationDomain domain = context.Compile();
domain.Run();
```

### 12.2. From the Native Interpreter

If the interpreter supports command-line arguments:

```bash
ShardScript.Interpreter.exe --library MyLib.dll --source script.ss
```

The exact syntax depends on the CLI implementation.

### 12.3. Library Lookup

The engine searches for the library:

1. By absolute path.
2. In the executable's directory.
3. In directories listed in `PATH` (Windows) or `LD_LIBRARY_PATH` (Linux).

---

## 13. Debugging and Common Errors

### 13.1. `Symbol 'Foo' not found in current scope`

- Make sure the symbol is added to `NamespaceNode->Members` via `ns->OnSymbolDeclared(cls)`.
- Make sure the script has `using mynamespace;` or that you use the fully qualified name.
- Ensure `NamespaceSymbol::Node` is not `nullptr`.

### 13.2. `No method "Add" found that accepts arguments (...)`

- Make sure parameters are added to `method->Parameters`.
- Check the order and types of parameters.
- Make sure the method is registered in the class via `cls->OnSymbolDeclared(method)`.

### 13.3. Segfault Inside a Callback

- Check `ctx.Args.size()` before accessing `ctx.Args[i]`.
- Make sure you do not write to an `ObjectInstance` that is `NullInstance`.
- For instance methods, remember that `ctx.Args[0]` is `this`.

### 13.4. Callback Is Not Called

- Check that `FunctionPointer` is not `nullptr`.
- Check that `HandleType == MethodHandleType::External`.
- `factory.Method(...)` with a delegate already sets `HandleType = External`.

### 13.5. Library Does Not Load

- Check that `ShardLib_EntryPoint` and `ShardLib_GetMetadata` are exported (use `dumpbin /exports MyLib.dll` or `nm -D libMyLib.so`).
- Make sure you use `extern "C"`.
- Check dependencies via `ldd libMyLib.so` (Linux) or Dependencies (Windows).

---

## 14. Complete Working Example: `StringLib`

```cpp
#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/syntax/SymbolBuilder.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <algorithm>
#include <cwctype>

using namespace shard;

static ObjectInstance* ToUpper(const CallState& ctx)
{
    const wchar_t* src = ctx.Args[0]->AsString();
    std::wstring result(src);
    std::transform(result.begin(), result.end(), result.begin(), std::towupper);
    return ctx.Collector.FromValue(result);
}

static ObjectInstance* Concat(const CallState& ctx)
{
    const wchar_t* a = ctx.Args[0]->AsString();
    const wchar_t* b = ctx.Args[1]->AsString();
    std::wstring result = std::wstring(a) + std::wstring(b);
    return ctx.Collector.FromValue(result);
}

SHARDLIB_GETMETADATA
{
    lib.Name        = L"StringLib";
    lib.Description = L"Example string manipulation library";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    auto strings = SymbolBuilder<NamespaceSymbol>(context, L"stringlib").AddClass(L"Strings");

    strings.AddMethod(L"ToUpper", SymbolTable::Primitives::String)
        .AddParameter(L"value", SymbolTable::Primitives::String)
        .SetCallback(&ToUpper);

    strings.AddMethod(L"Concat", SymbolTable::Primitives::String)
        .AddParameter(L"a", SymbolTable::Primitives::String)
        .AddParameter(L"b", SymbolTable::Primitives::String)
        .SetCallback(&Concat);
}
```

MinGW build:

```bash
x86_64-w64-mingw32-g++ -shared -O2 -std=c++20 \
    -I../ShardScript/include \
    -L../ShardScript/build/bin \
    -lShardScript \
    -o StringLib.dll StringLib.cpp
```

Usage:

```shard
using stringlib;
hello := Strings.ToUpper("hello");   // HELLO
full  := Strings.Concat("Shard", "Script");
println(hello);
println(full);
```

---

## 15. Recommendations

1. **Use `SymbolBuilder<T>` for namespace/class/method/field.** It automatically sets `Parent`, `FullName`, `Accesibility`, registers the symbol in the `NamespaceTree`, and calls `OnSymbolDeclared`.
2. **When working manually with `SymbolFactory`, do not forget `Parent`, `FullName`, and `OnSymbolDeclared`.** Without them, name resolution and member access break.
3. **Do not store `ObjectInstance*` across calls without `IncrementReference`.** The GC may collect the object.
4. **Use `ctx.Collector` to create return values.** Do not allocate memory manually.
5. **For instance methods, remember `this`.** `ctx.Args[0]` is the object, the rest are parameters.
6. **Check `ctx.Args.size()`.** This protects against unexpected calls.
7. **Do not modify `CallState`.** The structure is part of the ABI — any changes will break compatibility.

---

## 16. Conclusion

Creating a native library for ShardScript boils down to a few steps:

1. Export `ShardLib_GetMetadata` and `ShardLib_EntryPoint`.
2. In `ShardLib_EntryPoint`, create symbols via `SymbolBuilder<T>`.
3. Bind a `MethodSymbol` to a C++ function via `SetCallback`.
4. Read arguments from `CallState::Args` and return new values through the `GarbageCollector`.
5. Build a shared library and load it into the `CompilationContext`.

For advanced scenarios — generics, properties, arrays — use the corresponding symbols (`TypeParameterSymbol`, `GenericTypeSymbol`, `AccessorSymbol`, `ArrayTypeSymbol`) and the `ObjectInstance` / `GarbageCollector` methods.
