# ShardScript

**ShardScript** is an embeddable, scripting, compiled, programming language with strict static typing, developed in C++. The project features a custom lexer/parser, semantic analyzer, bytecode compiler, stack virtual machine, and a standard library framework.

## Check out my Website!
[ShardScript.ru](https://ShardScript.ru)

## 📋 Specifications

* **Implementation:** C++20 (MSVC), CMake/Ninja build system.
* **Typing:** Static, strong.
* **Paradigm:** Object-oriented (classes, structs, interfaces) with functional elements (delegates, lambdas).
* **Memory Management:** Automatic garbage collector based on reference counting and object tracking.
* **Target:** Custom stack-based virtual machine with compiled bytecode.

## ✅ Implemented Features

### Syntax & Language Constructs
* **Basic Types:** `int`, `double`, `bool`, `char`, `string`, `void`.
* **Control Flow:**
  * Conditionals: `if`, `else if`, `else`, `unless` (inverse `if`).
  * Loops: `for`, `while`, `until`, `foreach`.
  * Jump statements: `break`, `continue`, `return`, `throw`.
* **Functions:** Top-level and member functions with `func Name(args) -> ReturnType` syntax.
* **Functional Programming:**
  * Named delegates and inline delegate types.
  * Static lambda expressions.
* **OOP:**
  * Classes and structures.
  * Interfaces with implementation mapping.
  * Access modifiers: `public`, `private`, `protected`, `internal`.
  * Fields, properties with `get`/`set`, indexers, constructors.
  * Static fields, methods and properties.
  * Operator overloading (`operator +`, `-`, `==`, etc.).
  * Access operator overload (`operator .(name: string) -> T`) for dynamic member resolution.
* **Other:**
  * Namespaces and `using` directives.
  * `defer` statements for deterministic cleanup.
  * `try`/`catch` blocks.
  * `extern` methods for native C++ interop.
* **External modules:** write libs in C++ and load them at runtime.
  * See `library-guide.md` for more info and `ShardScript.Framework` for real examples.

### Runtime
* **Compiler:** Lexer/parser → semantic analysis → bytecode emission.
* **Interpreter:** Stack virtual machine with custom call-stack handling.
* **Error Handling:** Compile-time diagnostics and runtime exception system (`throw`/`try`/`catch`).

### Framework
* **Standard Library (WIP):**
  * `stdio`: Console input and output.
  * `collections`: `List<T>` `Dictionary<TKey, TValue>`, `Stack<T>`, `Queue<T>`.
  * `filesystem`: Directories and Files operations, Path resolving, Glob pattern matching.
  * `math`: Math functions, and random number generation.
  * `strings`: String manipulation and formatting functions.
  * `json`: JSON parsing and serialization.
  * `http`: HTTP client and server.
  * `socket`: TCP socket operations.

## 📝 Code Examples

### 1. Functions & Custom Loops
```
using stdio;

public static func Main() -> void
{
    i: int = 0;
    until (i == 5)
    {
        println(i);
        i++;
    }
}
```

### 2. Recursion (Fibonacci)
```
using stdio;

public static func Fibonacci(n: int) -> int
{
    if (n <= 1)
        return n;

    return Fibonacci(n - 1) + Fibonacci(n - 2);
}

public static func Main() -> void
{
    result: int = Fibonacci(10);
    println(result);
    // Output: 55
}
```

### 3. Delegates & Lambdas
```
using stdio;

delegate Operation(a: int, b: int) -> int;

public static func Main() -> void
{
    sum: Operation = lambda (x: int, y: int) -> int
    {
        return x + y;
    };

    res: int = sum(5, 7);
    println(res);
    // Output: 12
}
```

### 4. Operator Overloading
```
using stdio;

namespace Operators;

class Box
{
    public Value: int;

    public static operator +(a: Box, b: Box) -> int
    {
        return 42;
    }
}

public static func Main() -> void
{
    a: Box = new Box();
    b: Box = new Box();
    println(a + b);
    // Output: 42
}
```

### 5. Access Operator Overload
```
using stdio;

namespace DynamicAccess;

class Dynamic
{
    public operator .(name: string) -> int
    {
        return 42;
    }
}

public static func Main() -> void
{
    d: Dynamic = new Dynamic();
    println(d.foo);
    // Output: 42
}
```

### 6. Extension Method
```
using stdio;

namespace hello_world;

static func Double(x: int) -> int
{
    return x * 2;
}

public static func Main() -> void
{
    a: int = 5;
    println(a.Double());
    // Output: 10
}
```

### 7. Resource Cleanup with `defer`
```
using stdio;

namespace hello_world;

public class Resource : IDisposable
{
    public func Dispose() -> void
    {
        println("disposed");
    }
}

public static func Main() -> void
{
    defer r: Resource = new Resource();
    defer println("deferred expression");
    println("using resource");
    // Output:
    // using resource
    // deferred expression
    // disposed
}
```

### 8. Ranges and `for ... in`
```
using stdio;

namespace hello_world;

public static func Main() -> void
{
    for (i in 0..3)
    {
        defer println("Completed: " + i);
        println("Step: " + i);
    }
}
```

### 9. Generic Collections
```
using collections;

namespace hello_world;

public static func Main() -> void
{
    List<int> list = new List<int>();
    list.Add(10);
    list.Add(20);
}
```

### 10. Printing Built-in Types
```
using stdio;

namespace hello_world;

public static func Main() -> void
{
    println(42);
    println(true);
    println("hello");

    i: int = 3;
    s: string = "world";
    b: bool = true;

    println(i);
    println(s);
    println(b);
}
```

## 🚀 Roadmap

### Language Core
* [x] **IPrintable interface** — standard contract for string conversion; all primitives implement it; `print`/`println` accept `IPrintable` instead of `any`.
* [ ] **Branching as expression** — `current_status := if is_active "online" else "offline";`
* [ ] **Attributes** — metadata annotations on declarations (e.g. `[deprecated("...")]`).
* [x] **Exceptions** — `IThrowable` contract, custom exception classes, polymorphic `try`/`catch`, `throw` as expression.
* [ ] **Enums** — plain enums, flag enums (`: flags`), and record-style enums (`: struct(...)`).
* [x] **Implicit extension methods** — any static method taking a type as its first argument becomes callable as a member on that type.
* [x] **Operator overload by token identifier** — `operator AddOperator`, `operator EqualsOperator`, etc.

### Collections & Iteration
* [x] **Foreach and IEnumerable** — `IEnumerator<T>` / `IEnumerable<T>` contracts; arrays and collections implement them.
* [ ] **Iterators** — `yield return`, `yield break`, and `yield range` with compiler-generated state machines.
* [ ] **LINQ-style library (`shard.linq`)** — `Where`, `Select`, `ToList`, etc., built on extension methods and `IEnumerable`.

### Advanced Runtime Features
* [x] **Enhanced defer / IDisposable** — resource cleanup via `defer variable := expression` and `IDisposable` support.
* [ ] **Async / await** — non-blocking asynchronous methods without explicit `Task<T>` wrapping; compiler-generated state machines.
* [ ] **Yield + defer interaction** — iterator state machines guarantee deferred cleanup even when iteration is abandoned.

### Tooling & Ecosystem
* [ ] **Sharding** — continue expanding and improving "Basic Shards Collection" (BSC), by adding and refiining new shards (libraries) 
* [ ] **Diagnostics** — improved compile-time and runtime error reporting.
* [ ] **Lang-based build system** — declarative project builds via `shard.build` and a `build.ss` script.
* [ ] **Shell interpreter** — REPL mode and `shard.shell` module for system automation tasks.

## 🤝 Contributing

The project is under active development. If you find a bug, have ideas for syntax improvements, or want to suggest a new feature — please create an Issue! All feedback is welcome. In case of a sudden crash of the interpreter, please attach your code to the issue so I can debug it.
