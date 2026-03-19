# ShardScript

**ShardScript** is an compiled, object-oriented programming language with strict static typing, developed in C++. The project features a custom lexer/parser, semantic analyzer, a stack virtual machine, and a standard library framework.

## Check out my Website!
[ShardScript.ru](https://ShardScript.ru)

## 📋 Specifications

* **Implementation:** C++ (MS Visual C++).
* **Typing:** Static, Strong.
* **Paradigm:** Object-Oriented (Classes, Structs) & Functional elements.
* **Memory Management:** Automatic (Garbage Collector based on reference counting and object tracking).
* **Interop:** `extern` methods mechanism for C++ implementations.

## ✅ Implemented Features

### Syntax & Language Constructs
* **Basic Types:** `int`, `double`, `bool`, `char`, `string`, `void`, `var` (type inference).
* **Control Flow:**
  * Conditionals: `if`, `else if`, `else`, `unless` (inverse if).
  * Loops: `for`, `while`, `until` (loop until condition).
* **Functional Programming:**
  * typedef Delegates.
  * Static Lambda expressions.
* **OOP (Basic):**
  * Classes and Structures.
  * Access Modifiers (`public`, `private`, `protected`, `internal`).
  * Properties with `get`/`set`.
  * Static fields and methods.
* **External modules:** write libs in C++ and add them to run-time
  * see `modules.md` for more info

### Runtime & Framework
* **Interpreter:** Tree-walking interpreter with custom call stack handling.
* **Error Handling:** Runtime exception system (`throw`).
* **Standard Library (WIP):**
  * `System.Collections`: `List<T>` (Dynamic array implementation). DO NOT USE, generic type parameter are under rewriting!
  * `System.Filesystem`: Basic file and directory operations.
  * `System`: Primitives, Math, Random.

## 📝 Code Examples

### 1. Generics & Custom Loops
```csharp
using System.Collections;

namespace MyApp
{
    class Program
    {
        public static void Main()
        {
            // Using generics (basic usage)
            List<int> numbers = new List<int>();
            numbers.Add(10);
            numbers.Add(20);

            // Unique 'until' loop syntax
            int i = 0;
            until (i == numbers.Length)
            {
                println(numbers.ElementAt(i));
                i++;
            }
        }
    }
}
```

### 2. Recursion (Fibonacci)
```csharp
namespace Math
{
    class Algorithms
    {
        public static int Fibonacci(int n)
        {
            if (n <= 1)
                return n;
            
            return Fibonacci(n - 1) + Fibonacci(n - 2);
        }

        public static void Main()
        {
            int result = Fibonacci(10);
            // Output: 55
        }
    }
}
```

### 3. Delegates & Lambdas
```csharp
namespace Functional
{
    private delegate int Operation(int a, int b);

    class Program
    {
        public static void Main()
        {
            // Lambda expression assignment
            Operation sum = lambda (int x, int y) => return x + y;

            int res = sum(5, 7); 
            // res is 12
        }
    }
}
```

## 🚀 Roadmap
​[ ] OOP Complete: Interfaces, extension methods and pipe operator.  
​[ ] Meta-Programming: Attributes, Decorators, and AOP (Aspect-Oriented Programming).  
​[ ] Error Handling: Full try-catch blocks.  
​[ ] Generics: Advanced constraints and full generalization support.  
​[ ] Diagnostics: Improved compile-time and runtime error reporting.  
[ ] Generics: Basic support for simple structures, fields, and variables (primarily used for `extern` backing; constraints and generalization not yet available).

## ​🤝 Contributing
​The project is under active development. If you find a bug, have ideas for syntax improvements, or want to suggest a new feature — please create an Issue! All feedback is welcome. In case of a sudden crash of the interpreter, please attach your code to issue so I can debug it.
