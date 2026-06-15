# Руководство по созданию нативных библиотек для ShardScript

Это руководство описывает, как писать динамические библиотеки (`.dll` / `.so`), которые могут расширять ShardScript новыми типами, методами и callback-функциями, написанными на C++. Оно ориентировано на автора библиотек, который хочет работать с API ShardScript "в сыром виде" — без C#-обёртки.

---

## 1. Что происходит при загрузке библиотеки

Когда движок ShardScript загружает библиотеку (например, через `context.AddLibrary("MyLib.dll")` в C# или аналогичный механизм в интерпретаторе), он:

1. Загружает динамическую библиотеку.
2. Ищет два экспортируемых символа:
   - `ShardLib_GetMetadata` — заполняет базовые метаданные (название, версия, описание).
   - `ShardLib_EntryPoint` — точка входа, в которой библиотека регистрирует все символы.
3. Вызывает `ShardLib_EntryPoint`, передавая `CompilationContext&`.
4. Внутри точки входа вы создаёте символы (namespace, class, method, field, parameter) и привязываете C++-функции к методам.

---

## 2. Ключевые понятия

### 2.1. Символы (`SyntaxSymbol`)

Символ — это объект семантической модели ShardScript, представляющий сущность языка. Основные классы символов:

| Класс | Что представляет |
|-------|------------------|
| `NamespaceSymbol` | Пространство имён (`namespace foo;`) |
| `ClassSymbol` | Класс (`class Foo { }`) |
| `StructSymbol` | Структура (`struct Bar { }`) |
| `MethodSymbol` | Метод/функция (`func Add(a: int, b: int) -> int`) |
| `ConstructorSymbol` | Конструктор (`init() { }`) |
| `FieldSymbol` | Поле (`field x: int`) |
| `PropertySymbol` | Свойство (`prop Value: int`) |
| `ParameterSymbol` | Параметр метода |
| `TypeParameterSymbol` | Параметр дженерика (`T`) |
| `GenericTypeSymbol` | Конкретная инстанциация дженерика (`Box<int>`) |

Каждый символ имеет:

- `Name` — имя.
- `FullName` — полное имя (`foo.Bar.Baz`).
- `Parent` — родительский символ.
- `Accesibility` — `Public` или `Private`.
- `Kind` — вид узла (`SyntaxKind::ClassDeclaration`, `SyntaxKind::MethodDeclaration` и т.д.).

Символы не владеют AST-узлами (`SyntaxNode`). Они живут в `SymbolTable`, которой владеет семантическая модель.

### 2.2. `SymbolTable` и `NamespaceTree`

- `SymbolTable` — таблица всех символов. Символы регистрируются через `BindSymbol(node, symbol)` (когда есть AST-узел) или `ImplicitSymbol(symbol)` (когда узла нет, как в нативных библиотеках).
- `NamespaceTree` — дерево пространств имён. Каждый `NamespaceNode` содержит:
  - `Owners` — список `NamespaceSymbol*`, связанных с этим узлом.
  - `Members` — все типы/члены, объявленные в этом пространстве имён.

Когда вы создаёте `ClassSymbol` внутри `NamespaceSymbol` и вызываете `ns->OnSymbolDeclared(cls)`, класс добавляется и в `ns->Members`, и в `ns->Node->Members`. Это позволяет скриптам видеть тип через `using` или через `namespace`.

### 2.3. `MethodHandleType`

`MethodSymbol` имеет поле `HandleType`, определяющее, как вызывается метод:

- `MethodHandleType::Body` — обычный метод с телом из байткода VM.
- `MethodHandleType::External` — нативный метод, вызываемый через `FunctionPointer`.
- `MethodHandleType::Lambda` — анонимная функция.

Для нативных библиотек важны `External`.

### 2.4. `CallState`

Когда нативный метод вызывается, движок передаёт в callback структуру `CallState`:

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

Поля:

- `Domain` — домен приложения.
- `Collector` — сборщик мусора; нужен для создания возвращаемых значений.
- `Frame` — текущий фрейм стека вызовов.
- `Method` — символ вызываемого метода.
- `Args` — массив аргументов; `Args[0]` — первый аргумент.

Тип callback-функции:

```cpp
using MethodSymbolDelegate = shard::ObjectInstance* (*)(const shard::CallState& context);
```

### 2.5. `ObjectInstance` и чтение значений

`ObjectInstance` — это обёртка над значением в куче ShardScript.

Основные методы:

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

Для создания новых значений используется `GarbageCollector`:

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

## 3. Минимальная нативная библиотека

### 3.1. Шаг 1 — метаданные и точка входа

Каждая библиотека должна экспортировать две функции. Для этого удобно использовать макросы из `<shard/ShardScriptLIB.hpp>`:

```cpp
#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>

SHARDLIB_GETMETADATA
{
    lib.Name        = L"MyMath";
    lib.Description = L"Пример нативной математической библиотеки";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    // Регистрация символов происходит здесь.
}
```

`SHARDLIB_GETMETADATA` и `SHARDLIB_ENTRYPOINT` уже содержат правильные `extern "C"` и атрибуты видимости для Windows (`__declspec(dllexport)`) и GCC/Clang (`visibility("default")`).

### 3.2. Шаг 2 — создание namespace и class через `SymbolBuilder`

Вместо ручного вызова `SymbolFactory` используйте шаблонный fluent-билдер `SymbolBuilder<T>`. Он автоматически устанавливает `Parent`, `FullName`, `Accesibility`, регистрирует символ в `NamespaceTree` и вызывает `OnSymbolDeclared`.

Доступные специализации:

- `SymbolBuilder<NamespaceSymbol>(ctx, name[, parent])`
  - `AddClass(name[, access])` → `SymbolBuilder<ClassSymbol>`
  - `AddNamespace(name[, access])` → `SymbolBuilder<NamespaceSymbol>`
- `SymbolBuilder<ClassSymbol>(ctx, name[, parent])`
  - `AddMethod(name, returnType[, isStatic][, access])` → `SymbolBuilder<MethodSymbol>`
  - `AddField(name, type[, isStatic][, access])` → `SymbolBuilder<FieldSymbol>`
- `SymbolBuilder<MethodSymbol>`
  - `AddParameter(name, type)` → ссылка на себя
  - `SetCallback(fn)` / `SetCallback(managedFn, userData)` → ссылка на себя

`SymbolBuilder` некопируемый, поддерживает move. Получить указатель на символ можно через `Get()` или неявное приведение:

```cpp
#include <shard/syntax/SymbolBuilder.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>

using namespace shard;

SymbolBuilder<NamespaceSymbol> ns(ctx, L"mymath");
ClassSymbol* math = ns.AddClass(L"Math");   // неявное приведение к ClassSymbol*
```

Вложенные namespace:

```cpp
SymbolBuilder<NamespaceSymbol> root(ctx, L"company");
SymbolBuilder<NamespaceSymbol> math = root.AddNamespace(L"math");
math.AddClass(L"Algorithms");
```

### 3.3. Шаг 3 — создание метода и привязка C++-функции

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

// внутри SHARDLIB_ENTRYPOINT
ns.AddClass(L"Math")
  .AddMethod(L"Add", SymbolTable::Primitives::Integer)
      .AddParameter(L"a", SymbolTable::Primitives::Integer)
      .AddParameter(L"b", SymbolTable::Primitives::Integer)
      .SetCallback(&MyAdd);
```

`AddMethod` создаёт `MethodSymbol`, регистрирует его в `SymbolTable` и по умолчанию делает его статическим (`IsStatic = true`). `SetCallback` устанавливает `HandleType = External`, `FunctionPointer = fn` и `IsExtern = true`.

### 3.4. Шаг 4 — параметры

`AddParameter` добавляет `ParameterSymbol` в `MethodSymbol::Parameters`. Порядок вызовов должен совпадать с порядком аргументов при вызове из ShardScript.

```cpp
ns.AddClass(L"Math")
    .AddMethod(L"Pow", SymbolTable::Primitives::Double)
        .AddParameter(L"base", SymbolTable::Primitives::Double)
        .AddParameter(L"exp",  SymbolTable::Primitives::Double)
        .SetCallback(&MyPow);
```

### 3.5. Полный файл `MyMath.cpp`

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
    lib.Description = L"Нативная математическая библиотека для ShardScript";
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

## 4. Чтение параметров и возврат значений

### 4.1. Примитивные типы

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

### 4.2. Проверка количества аргументов

```cpp
static ObjectInstance* SafeAdd(const CallState& ctx)
{
    if (ctx.Args.size() != 2)
    {
        // Можно вернуть null или бросить исключение.
        // В текущей версии ShardScript исключения из нативного кода
        // будут перехвачены VM как фатальная ошибка.
        throw std::runtime_error("Add expects 2 arguments");
    }

    std::int64_t a = ctx.Args[0]->AsInteger();
    std::int64_t b = ctx.Args[1]->AsInteger();
    return ctx.Collector.FromValue(a + b);
}
```

### 4.3. Таблица соответствия типов ShardScript ↔ C++

| ShardScript | C++ тип | Чтение | Запись |
|-------------|---------|--------|--------|
| `int` | `std::int64_t` | `AsInteger()` | `WriteInteger(v)` / `FromValue(v)` |
| `double` | `double` | `AsDouble()` | `WriteDouble(v)` / `FromValue(v)` |
| `bool` | `bool` | `AsBoolean()` | `WriteBoolean(v)` / `FromValue(v)` |
| `char` | `wchar_t` | `AsCharacter()` | `WriteCharacter(v)` / `FromValue(v)` |
| `string` | `const wchar_t*` / `std::wstring` | `AsString()` | `WriteString(v)` / `FromValue(v)` |
| `T[]` | `ObjectInstance*` | `GetArrayLength()`, `GetElement(i)` | `AllocateArray(elemType, len)` |
| class/struct | `ObjectInstance*` | `GetField()`, `SetField()` | `AllocateInstance(typeInfo)` |
| `null` | — | `GarbageCollector::NullInstance` | — |

### 4.4. Работа с `null`

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

## 5. Статические и instance-методы

### 5.1. Статический метод

Для статического метода `IsStatic = true`, и в `ctx.Args[0]` лежит первый параметр.

```cpp
math.AddMethod(L"Add", SymbolTable::Primitives::Integer, true)
    .AddParameter(L"a", SymbolTable::Primitives::Integer)
    .AddParameter(L"b", SymbolTable::Primitives::Integer)
    .SetCallback(&MathAdd);
```

### 5.2. Instance-метод

Для нестатического метода `IsStatic = false`, и `ctx.Args[0]` — это `this`.

```cpp
static ObjectInstance* CounterNext(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    std::int64_t value = self->AsInteger();
    self->WriteInteger(value + 1);
    return self;
}
```

Если `this` — объект с полями:

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

### 5.3. Конструкторы

Конструктор — это особый `MethodSymbolDelegate`, который получает уже аллоцированный `this`:

```cpp
static ObjectInstance* CounterCtor(const CallState& ctx)
{
    ObjectInstance* self = ctx.Args[0];
    std::int64_t initial = ctx.Args[1]->AsInteger();

    ObjectInstance* valueField = self->GetField(g_valueField, ctx.Frame);
    valueField->WriteInteger(initial);

    return self; // или GarbageCollector::NullInstance
}
```

Регистрация:

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

---

## 6. Поля и свойства

### 6.1. Регистрация поля

```cpp
ns.AddClass(L"Math")
    .AddField(L"Pi", SymbolTable::Primitives::Double, true);   // static

auto counter = ns.AddClass(L"Counter");
counter.AddField(L"Value", SymbolTable::Primitives::Integer); // instance
```

### 6.2. Статическое поле

Для статического поля используется `GarbageCollector::SetStaticField` / `GetStaticField`:

```cpp
static FieldSymbol* g_piField = nullptr;

static ObjectInstance* GetPi(const CallState& ctx)
{
    return ctx.Collector.GetStaticField(g_piField);
}
```

Инициализация статического поля при загрузке библиотеки:

```cpp
ObjectInstance* piValue = ctx.Collector.FromValue(3.14159265358979);
ctx.Collector.SetStaticField(g_piField, piValue);
```

### 6.3. Свойства

Свойство регистрируется через `PropertySymbol`, у которого есть `Getter` и `Setter` типа `AccessorSymbol*`. Для нативного свойства проще создать методы-аксессоры и привязать их:

```cpp
PropertySymbol* prop = factory.Property(L"Value", SymbolTable::Primitives::Integer);
prop->Accesibility = SymbolAccesibility::Public;
prop->Parent = cls;
prop->IsStatic = false;
cls->OnSymbolDeclared(prop);

AccessorSymbol* getter = factory.Getter(L"Value", prop);
getter->FunctionPointer = &GetValue;
getter->HandleType = MethodHandleType::External;

AccessorSymbol* setter = factory.Setter(L"Value", prop);
setter->FunctionPointer = &SetValue;
setter->HandleType = MethodHandleType::External;
```

---

## 7. Массивы

### 7.1. Чтение массива

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

### 7.2. Создание массива

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

## 8. Дженерики в сырой среде

Дженерики в ShardScript работают через два вида символов:

- `TypeParameterSymbol` — параметр типа (`T`).
- `GenericTypeSymbol` — конкретная инстанциация (`Box<int>`), которая хранит `UnderlayingType` и карту замен.

### 8.1. Пример: `Identity<T>`

Объявим класс `Container<T>` с одним статическим методом `Identity`, который просто возвращает свой аргумент.

```cpp
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>

static ObjectInstance* Identity(const CallState& ctx)
{
    // Метод просто возвращает первый аргумент.
    return ctx.Args[0];
}

static void RegisterGenericContainer(CompilationContext& ctx, NamespaceSymbol* ns)
{
    SymbolFactory factory(context.GetSemanticModel().Table.get());

    // 1. Создаём открытый generic-тип Container<T>.
    ClassSymbol* container = factory.Class(L"Container");
    container->Accesibility = SymbolAccesibility::Public;
    container->IsReferenceType = true;
    container->Parent = ns;
    container->FullName = ns->FullName + L".Container";

    // 2. Параметр типа T.
    TypeParameterSymbol* T = factory.TypeParameter(L"T");
    T->Parent = container;
    container->TypeParameters.push_back(T);

    ns->OnSymbolDeclared(container);

    // 3. Статический метод Container<T>.Identity(T value) -> T.
    MethodSymbol* identity = factory.Method(
        SymbolAccesibility::Public,
        true,
        T,                       // возвращаемый тип — T
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

Теперь из ShardScript можно писать:

```shard
using mymath;
x := Container<int>.Identity(42);
```

Движок сам создаст `GenericTypeSymbol` для `Container<int>`, подставит `T = int` при проверке типов и вызовет ваш нативный callback.

### 8.2. Пример: `Box<T>` с полем

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

Использование:

```shard
using mymath;
b := Box<int>();
b.SetValue(42);
println(b.GetValue());  // 42
```

> **Важно:** в текущей версии ShardScript нативный конструктор для generic-типа требует такой же регистрации, как и для обычного класса. Движок выделит экземпляр `GenericTypeSymbol` и передаст его как `this`.

### 8.3. Как получить конкретный тип внутри callback

Если метод вызван на конкретной инстанциации, `ctx.Method->Parent` может быть `GenericTypeSymbol`:

```cpp
static ObjectInstance* DumpType(const CallState& ctx)
{
    TypeSymbol* parent = ctx.Method->Parent;
    if (parent != nullptr && parent->Kind == SyntaxKind::GenericType)
    {
        GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(parent);
        // generic->UnderlayingType — Box
        // Замены параметров находятся внутри generic.
    }

    return GarbageCollector::NullInstance;
}
```

---

## 9. Сборка библиотеки

Все примеры ниже предполагают, что ShardScript уже собран и лежит рядом с вашим проектом.

### 9.1. Структура директорий

```text
ShardScript/
├── include/            # заголовки ShardScript
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

Флаг `-lShardScript` заставит линковщик искать:

- `libShardScript.dll.a` (импортная библиотека) — предпочтительно.
- `libShardScript.a`.

> Если у вас есть только `.dll` без импортной библиотеки, сгенерируйте её через `gendef` + `dlltool`, либо используйте `LoadLibrary`/`GetProcAddress` для динамической загрузки.

### 9.3. Windows + MSVC

```cmd
cd MyLib
cl /std:c++20 /EHsc /MD /I..\ShardScript\include /LD MyLib.cpp \
   /link ..\ShardScript\build\bin\libShardScript.lib /OUT:MyLib.dll
```

> Имена `.lib` и `.dll` зависят от конфигурации CMake. Если CMake-таргет называется `ShardScript`, MSVC обычно создаёт `ShardScript.dll` + `ShardScript.lib`. Проверьте содержимое `build/bin`.

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

При запуске убедитесь, что `libShardScript.so` и `libMyLib.so` видны линковщику:

```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/ShardScript/build/bin:/path/to/MyLib
```

### 9.5. CMake-шаблон

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

Если вы добавляете `ShardScript` как подпроект, `target_link_libraries(MyLib PRIVATE ShardScript)` автоматически подхватит нужные пути и импортные библиотеки.

### 9.6. Что экспортировать

Вам нужно экспортировать только `ShardLib_GetMetadata` и `ShardLib_EntryPoint`. Макросы `SHARDLIB_GETMETADATA` / `SHARDLIB_ENTRYPOINT` делают это автоматически. Ваши вспомогательные функции (`MyAdd`, `MyPow` и т.д.) экспортировать не нужно.

---

## 10. Поставка исходного кода из библиотеки

Нативная библиотека может не только регистрировать символы напрямую, но и передавать в компилятор исходный код на ShardScript. Это удобно, когда часть API удобнее выразить на самом языке, а низкоуровневые функции реализованы на C++.

### 10.1. `CompilationContext::ProvideSource`

```cpp
#include <shard/CompilationContext.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>

static void ProvideStringSource(CompilationContext& ctx,
                                const wchar_t* fileName,
                                const std::wstring& code)
{
    // StringStreamReader должен оставаться живым до конца работы с контекстом.
    // Проще всего сохранить его статически.
    static std::vector<std::unique_ptr<StringStreamReader>> readers;

    auto reader = std::make_unique<StringStreamReader>(fileName, code);
    StringStreamReader* raw = reader.get();
    readers.push_back(std::move(reader));

    ctx.ProvideSource(raw);
}
```

Использование в `ShardLib_EntryPoint`:

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

После этого код `mylib.Hello()` становится доступен любому скрипту, загружающему эту библиотеку.

### 10.2. Встроенные ресурсы Windows

Чтобы не таскать `.ss`-файлы отдельно, исходный код можно встроить в `.dll` как Win32-ресурс типа `SOURCE_CODE` и вытаскивать прямо из образа библиотеки. Так делает `ShardScript.Framework`.

```cpp
#include <Windows.h>
#include <stdexcept>
#include <string>

static void GetEmbeddedSource(const wchar_t* resourceName, std::wstring& out)
{
    HMODULE hModule = nullptr;

    // Получаем HMODULE текущей DLL по адресу функции.
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

> **Linux:** там нет Win32-ресурсов. Исходный код обычно встраивают как строковый литерал через `constexpr const wchar_t*`, либо читают из файла рядом с `.so`.

### 10.3. Пример с embedded source

```cpp
SHARDLIB_ENTRYPOINT
{
    std::wstring source;
    GetEmbeddedSource(L"MYLIB_CORE", source);

    ProvideStringSource(context, L"MyLibCore.ss", source);
}
```

Ресурс `MYLIB_CORE` должен быть описан в `.rc`-файле проекта:

```rc
MYLIB_CORE SOURCE_CODE "MyLibCore.ss"
```

> Реальный пример такого подхода можно посмотреть в файле `ShardScript.Framework/dllmain.cpp` этого репозитория.

---

## 11. Атрибут `[link]`

Атрибут `[link]` позволяет объявить метод в исходном коде ShardScript как `extern`, а реализацию подцепить из той же нативной библиотеки, которая предоставила этот исходный код.

### 11.1. Синтаксис

```shard
[link("symbol_name")]
public static extern func Add(a: int, b: int) -> int;
```

Два аргумента тоже разбираются:

```shard
[link("library_name", "symbol_name")]
public static extern func Foo() -> void;
```

но на текущий момент связывание всегда происходит с библиотекой, которая предоставила данный исходный файл.

### 11.2. Как работает

1. Библиотека загружается движком (`AddLibrary`).
2. Движок вызывает `ShardLib_EntryPoint`.
3. Внутри точки входа библиотека передаёт исходник через `ProvideSource`.
4. При анализе `DeclarationCollector` видит атрибут `[link]` и записывает имя символа в `MethodSymbol::LinkSymbol`.
5. `CompilationContext::LinkExternSymbols` ищет этот символ в загруженной библиотеке (аналог `GetProcAddress`) и записывает адрес в `MethodSymbol::FunctionPointer`.
6. При вызове метода VM вызывает эту C++-функцию через `FunctionPointer`.

### 11.3. Требования к C++-функции

Функция, на которую ссылается `[link]`, должна:

- Иметь сигнатуру `shard::ObjectInstance* (*)(const shard::CallState&)`.
- Быть экспортирована из DLL (`extern "C"` + `__declspec(dllexport)` / `visibility("default")`).
- Иметь имя, точно совпадающее со строкой в атрибуте.

Пример:

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

> На Linux используйте `__attribute__((visibility("default")))` вместо `__declspec(dllexport)`.

### 11.4. Полный пример: библиотека + embedded source + [link]

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
    lib.Description = L"Пример [link]";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    ProvideEmbeddedSource(context);
}
```

Использование из ShardScript:

```shard
using mathlib;

x := mathlib.Add(10, 32);
y := mathlib.Sqrt(16.0);
println(x); // 42
println(y); // 4
```

### 11.5. Глобальные методы через `SemanticAnalyzer::AddSymbol`

Если метод не должен принадлежать никакому namespace или классу, его можно добавить прямо в глобальную область видимости:

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

Теперь из ShardScript можно вызвать просто:

```shard
t := now();
println(t);
```

> **Важно:** `SemanticAnalyzer::AddSymbol` добавляет символ в глобальный скоуп. Если вы добавляете метод с параметрами, не забудьте заполнить `method->Parameters`.

---

## 12. Загрузка и использование из ShardScript

### 12.1. Из C#-обёртки

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

### 12.2. Из нативного интерпретатора

Если интерпретатор поддерживает аргументы командной строки:

```bash
ShardScript.Interpreter.exe --library MyLib.dll --source script.ss
```

Точный синтаксис зависит от реализации CLI.

### 12.3. Поиск библиотеки

Движок ищет библиотеку:

1. По абсолютному пути.
2. В директории исполняемого файла.
3. В директориях, перечисленных в `PATH` (Windows) или `LD_LIBRARY_PATH` (Linux).

---

## 13. Отладка и типичные ошибки

### 11.1. `Symbol 'Foo' not found in current scope`

- Проверьте, что символ добавлен в `NamespaceNode->Members` через `ns->OnSymbolDeclared(cls)`.
- Проверьте, что в скрипте есть `using mynamespace;` или что вы обращаетесь по полному имени.
- Убедитесь, что `NamespaceSymbol::Node` не `nullptr`.

### 11.2. `No method "Add" found that accepts arguments (...)`

- Проверьте, что параметры добавлены в `method->Parameters`.
- Проверьте порядок и типы параметров.
- Проверьте, что метод зарегистрирован в классе через `cls->OnSymbolDeclared(method)`.

### 11.3. Сегфолт внутри callback

- Проверьте `ctx.Args.size()` перед доступом к `ctx.Args[i]`.
- Убедитесь, что вы не пишете в `ObjectInstance`, который является `NullInstance`.
- Для instance-методов не забудьте, что `ctx.Args[0]` — `this`.

### 11.4. Callback не вызывается

- Проверьте, что `FunctionPointer` не `nullptr`.
- Проверьте, что `HandleType == MethodHandleType::External`.
- `factory.Method(...)` с делегатом уже устанавливает `HandleType = External`.

### 11.5. Библиотека не загружается

- Проверьте, что `ShardLib_EntryPoint` и `ShardLib_GetMetadata` экспортированы (используйте `dumpbin /exports MyLib.dll` или `nm -D libMyLib.so`).
- Убедитесь, что используете `extern "C"`.
- Проверьте зависимости через `ldd libMyLib.so` (Linux) или Dependencies (Windows).

---

## 14. Полный рабочий пример: `StringLib`

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
    lib.Description = L"Пример библиотеки строковых функций";
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

Сборка MinGW:

```bash
x86_64-w64-mingw32-g++ -shared -O2 -std=c++20 \
    -I../ShardScript/include \
    -L../ShardScript/build/bin \
    -lShardScript \
    -o StringLib.dll StringLib.cpp
```

Использование:

```shard
using stringlib;
hello := Strings.ToUpper("hello");   // HELLO
full  := Strings.Concat("Shard", "Script");
println(hello);
println(full);
```

---

## 15. Рекомендации

1. **Используйте `SymbolBuilder<T>` для namespace/class/method/field.** Он автоматически устанавливает `Parent`, `FullName`, `Accesibility`, регистрирует символ в `NamespaceTree` и вызывает `OnSymbolDeclared`.
2. **При ручной работе со `SymbolFactory` не забывайте `Parent`, `FullName` и `OnSymbolDeclared`.** Без них нарушается разрешение имён и доступ к членам.
3. **Не храните `ObjectInstance*` между вызовами без `IncrementReference`.** GC может собрать объект.
4. **Используйте `ctx.Collector` для создания возвращаемых значений.** Не выделяйте память вручную.
5. **Для instance-методов помните про `this`.** `ctx.Args[0]` — объект, остальные — параметры.
6. **Проверяйте `ctx.Args.size()`.** Это защитит от неожиданных вызовов.
7. **Не меняйте `CallState`.** Структура является частью ABI — любые изменения сломают совместимость.

---

## 16. Заключение

Создание нативной библиотеки для ShardScript сводится к нескольким шагам:

1. Экспортировать `ShardLib_GetMetadata` и `ShardLib_EntryPoint`.
2. В `ShardLib_EntryPoint` создать символы через `SymbolBuilder<T>`.
3. Связать `MethodSymbol` с C++-функцией через `SetCallback`.
4. Читать аргументы из `CallState::Args` и возвращать новые значения через `GarbageCollector`.
5. Собрать shared library и загрузить её в `CompilationContext`.

Для сложных сценариев — дженериков, свойств, массивов — используйте соответствующие символы (`TypeParameterSymbol`, `GenericTypeSymbol`, `AccessorSymbol`, `ArrayTypeSymbol`) и методы `ObjectInstance`/`GarbageCollector`.
