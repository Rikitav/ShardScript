# Библия синтаксиса: Современный C-Style скриптовый язык

## 0. Идиомы именования (Naming Conventions)

Язык диктует жесткие правила именования на уровне стандарта. Это обеспечивает визуальную консистентность кода и упрощает работу линтеров.

* **PascalCase:** Классы, Структуры, Интерфейсы (`NetworkClient`, `Vector2`, `IThrowable`). Для интерфейсов допускается префикс `I`.
* **snake_case:** Функции, методы, переменные, локальные поля и свойства (`calculate_hash`, `user_id`, `is_ready`).
* **SCREAMING_SNAKE_CASE:** Глобальные и локальные константы (`MAX_CONNECTIONS`, `PI`).

## 1. Архитектурная философия

1. **Чтение слева направо:** Все объявления следуют паттерну `имя: тип`. Это делает грамматику дружелюбной как для человека, так и для LL(1)-совместимого парсера.
2. **Строгость и Однозначность:** Круглые скобки в условиях запрещены, фигурные скобки обязательны. Модификаторы строго упорядочены. Никакого скрытого состояния (запрет на статические локальные переменные).
3. **Композиция вместо наследования:** Язык не поддерживает классическое наследование классов (`class A : B`). Полиморфизм строится исключительно на интерфейсах.
4. **Выразительность (Expressions > Statements):** Ветвления (`if`, `switch`) и выброс исключений (`throw`) работают как выражения, возвращающие значение, что минимизирует количество мусорных переменных.

---

## 2. Базовые объявления

Чтение кода всегда начинается с идентификатора, затем следует тип и значение.

**Явная типизация**

```rust
count: int = 0;
first_name: string = "Tamerlan";

```

**Вывод типов (Type Inference)**
Оператор `:=` заменяет избыточные ключевые слова `var` или `let`.

```rust
index := 0;
is_ready := true;

```

**Константы**
Значения, неизменяемые во время компиляции.

```rust
MAX_CONNECTIONS: const int = 100;
PI :: 3.14159; // Краткая форма константы

```

---

## 3. Пользовательские типы и Инкапсуляция

Язык строго разделяет ссылочные (Class) и значимые (Struct) типы.

**Структуры (Value Types)**
Передаются по значению, выделяются на стеке (или инлайнятся в куче). Гарантируют непрерывное расположение в памяти для взаимодействия с C/C++.

```rust
struct Vector2 {
    x: float;
    y: float;

    // Конструктор инициализации
    init(x: float, y: float) {
        this.x = x;
        this.y = y;
    }
}

```

**Классы (Reference Types)**
Передаются по ссылке, управляются сборщиком мусора/ARC.

```rust
class Node {
    parent: Node?; // '?' обозначает Nullable тип
    
    // Конструктор по умолчанию
    init() {
        this.parent = null;
    }
}

```

**Инстанцирование (Создание экземпляров)**
Синтаксис явно разделяет выделение памяти на стеке и в куче:

```rust
// Инстанцирование структуры (Стек)
position := Vector2(10.5, 20.0);

// Инстанцирование класса (Куча)
root_node := new Node();

```

**Полные и Вычисляемые свойства (Properties)**
Используются контекстные ключевые слова `field` (для обращения к неявно сгенерированной памяти) и `value` (для аргумента сеттера).

```rust
class Character {
    // Автоматическое свойство
    name: string { get; private set; }

    // Полное свойство с валидацией (память выделяется под field)
    health: int {
        get { return field; }
        set {
            field = if value < 0 { 0 } 
                    else if value > 100 { 100 } 
                    else { value };
        }
    }

    // Вычисляемое свойство без field (память не выделяется)
    is_alive: bool => this.health > 0;
}

```

---

## 4. Функции и Семантические зоны

Функции определяются ключевым словом `fn`. Возвращаемый тип указывается после стрелки `->`. Разрешено объявление вложенных функций для изоляции логики.

**1. Глобальная функция (Уровень модуля)**
Находится в корне файла, не привязана к типу.

```rust
fn calculate_hash(data: string) -> int {
    return data.get_hash();
}

```

**2. Статическая функция (Уровень типа)**
Привязана к классу, но не имеет доступа к `this`. Используется для паттерна Фабрики.

```rust
export static func create_default() -> Node {
    return new Node();
}

```

**3. Метод экземпляра (Уровень типа)**
Имеет скрытый параметр `this`.

```rust
export func resize_window(new_w: int, new_h: int) {
    this.width = new_w;
    this.height = new_h;
}

```

**4. Вложенная функция (Локальный уровень)**
Инкапсулирована внутри другой функции. Может захватывать переменные из внешнего контекста (замыкание).

```rust
fn process_items(items: List<int>) {
    multiplier := 2;

    func apply_multiplier(val: int) -> int {
        return val * multiplier;
    }

    items.map(apply_multiplier);
}

```

**Анонимные функции и Лямбды (`=>`)**

```rust
// Лямбда-выражение (Arrow function)
filter_logic := (x: int, y: int) => x > y;

```

---

## 5. Интерфейсы и Дженерики

**Интерфейсы**
Описывают контракт. Единственный способ достижения полиморфизма.

```rust
interface IRenderable {
    is_visible: bool { get; }
    func render(context: GraphicsContext);
}

```

**Реализация интерфейса**

```rust
class Sprite : IRenderable {
    is_visible: bool { get; private set; }

    init() { this.is_visible = true; }
    func render(context: GraphicsContext) { /* ... */ }
}

```

**Дженерики и Ограничения**
Ограничения задаются через оператор `where`.

```rust
class Repository<T> where T: IRenderable {
    items: List<T>;
}

```

---

## 6. Поток управления (Control Flow)

**Классическое ветвление (без круглых скобок)**

```rust
if count > 0 {
    process_data();
} else {
    revert_changes();
}

```

**Ветвление как выражение (Замена тернарного оператора)**

```rust
current_status := if is_active { "online" } else { "offline" };

```

**Постфиксный `if` (Guard Clauses)**
Синтаксический сахар для однострочных проверок.

```rust
return if buffer == null;
console.log("ready") if is_ready;

```

**Безопасное извлечение (`if-let`)**

```rust
if user := get_user() {
    console.log(user.name); // user гарантированно не null
}

```

**Сопоставление с образцом (`switch` как выражение)**
Безопасная замена классическому `switch/case`, возвращает значение, не требует `break`.

```rust
state_name := switch state_code {
    200 => "OK",
    404 => "Not Found",
    _   => "Unknown" // '_' используется как default
};

```

---

## 7. Атрибуты и Модификаторы

**Порядок модификаторов**
Строго фиксирован: Видимость -> Поведение -> Объявление.

```rust
export static active_connections: int = 0; 

```

**Атрибуты**
Добавляют метаданные к узлам AST. Указываются строго до объявления (через аккумулятор в парсере).

```rust
[inline]
[deprecated("Use v2.0 API")]
export func legacy_update() { }

```

---

## 8. Обработка ошибок

Язык использует механизм размотки стека (Exceptions) без иерархии наследования. Все объекты ошибок реализуют системный интерфейс `IThrowable`.

**Контракт IThrowable и Пользовательские ошибки**

```rust
interface IThrowable {
    message: string { get; }
    stack_trace: string { get; }
}

class NetworkException : IThrowable {
    message: string { get; private set; }
    stack_trace: string { get; private set; }
    
    init(msg: string) {
        this.message = msg;
        this.stack_trace = runtime.capture_stack_trace();
    }
}

```

**Выброс исключения (`throw` как выражение)**

```rust
user_id := get_id() ?? throw new ArgumentException("ID cannot be null");

```

**Перехват исключений (`try / catch`)**

```rust
try {
    fetch_server_data();
} catch e: NetworkException {
    console.log("Network error: " + e.message);
} catch e: IThrowable {
    // Полиморфный перехват
    console.log("Generic error: " + e.message);
} catch {
    // Анонимный fallback (ловит всё)
    throw; // Проброс оригинального исключения
}

```

---

## 9. FFI (Foreign Function Interface) и Нативная линковка

Скрипт является единым источником истины (Single Source of Truth). Взаимодействие с нативным кодом (C/C++) декларируется явно через модификатор `extern` и атрибут `[link]`.

**Импорт глобальных функций**

```rust
// Указываем библиотеку и экспортированный символ extern "C"
export extern func fast_inv_sqrt(number: float) -> float;

// Если имена совпадают, второй аргумент можно опустить
[link("physics_engine")]
extern func calculate_collision(a: Vector2, b: Vector2) -> bool;

```

**Нативные методы классов (Неявный this)**

```rust
export class GraphicsBuffer {
    buffer_id: int;

    // В C++: void ClearBuffer(GraphicsBuffer* this_ptr, int color)
    [link("renderer_vulkan", "ClearBuffer")]
    export extern func clear(color: int);
}

```
