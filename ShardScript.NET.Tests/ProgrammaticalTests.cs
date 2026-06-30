using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Scripting;
using ShardScript.Syntax;
using ShardScript.Syntax.Builders;
using ShardScript.Syntax.Nodes;
using ShardScript.Syntax.Symbols;

namespace ShardScript.NET.Tests;

// [TestClass]
public sealed class ProgrammaticalTests
{
    public static void Main()
    {
        ProgrammaticalTests tests = new ProgrammaticalTests();

        tests.RunFileBasedExample();
        tests.RunSyntaxBuilderExample();
        tests.RunSymbolBuilderCallbackExample();
        tests.RunNamespaceMethodExample();
        tests.RunFluentApiExample();
        tests.RunEvaluateExample();
        tests.RunGlobalsExample();
        tests.RunCallAndIndexerExample();
        tests.RunReflectionBinderExample();
        tests.RunScopedAstBuilderExample();
        tests.RunEnumExample();
    }

    // [TestMethod]
    public void RunScopedAstBuilderExample()
    {
        Console.WriteLine("\n--- SDK scoped AST builder example ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(false));

        state.BuildAst(unit => unit
            .InNamespace("fluent_demo", ns => ns
                .AddClass("Program", cls => cls
                    .Public()
                    .Static()
                    .AddMethod("Add", m => m
                        .Public()
                        .Static()
                        .Parameter("a", PrimitiveType.Integer)
                        .Parameter("b", PrimitiveType.Integer)
                        .Returns(PrimitiveType.Integer)
                        .Body(body => body
                            .Return(ctx => ctx.Identifier("a") + ctx.Identifier("b")))))));

        if (!state.TryCompile())
        {
            Console.WriteLine($"Scoped AST compilation failed with {state.Context.ErrorCount} error(s):");
            Console.WriteLine(state.Diagnostics);
            return;
        }

        long result = state.Call<long>("Program.Add", 10, 32);
        Console.WriteLine($"Scoped AST builder Program.Add(10, 32) -> {result}");
    }

    // [TestMethod]
    public void RunFile(string filePath)
    {
        Console.WriteLine($"\n--- Running file: {filePath} ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(true));

        state.AddSourceFile(filePath, CompilationUnitOrigin.SourceFile);

        if (!state.TryCompile())
        {
            Console.WriteLine($"Compilation failed with {state.Context.ErrorCount} error(s):");
            Console.WriteLine(state.Diagnostics);
            return;
        }

        Console.WriteLine("\nRunning entry point...");
        state.Run();
    }

    // [TestMethod]
    public void RunFileBasedExample()
    {
        Console.WriteLine("\n--- File-based example ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(true));

        string testFilePath = Path.Combine(AppContext.BaseDirectory, "Test.ss");
        state.AddSourceFile(testFilePath, CompilationUnitOrigin.SourceFile);

        if (!state.TryCompile())
        {
            Console.WriteLine($"Compilation failed with {state.Context.ErrorCount} error(s):");
            Console.WriteLine(state.Diagnostics);
            return;
        }

        Console.WriteLine("\nCompilation units:");
        foreach (CompilationUnitSyntax? unit in state.GetSourceUnits())
        {
            if (unit.Namespace is { } ns)
                Console.WriteLine($"  Namespace: {ns.Name}");

            foreach (ClassDeclarationSyntax cls in unit.GetClasses())
            {
                Console.WriteLine($"  Class: {cls.Name}");
                foreach (MethodSymbol method in cls.GetMethods(state.Context))
                {
                    Console.WriteLine($"    Method: {method.Name}({method.ParameterCount} params), static: {method.IsStatic}");
                }
            }
        }

        Console.WriteLine("\nRunning entry point...");
        state.Run();

        MethodSymbol? addMethod = state.FindMethod("Program", "Add", 2);
        if (addMethod != null)
        {
            ObjectInstance argA = state.GarbageCollector.FromInteger(37);
            ObjectInstance argB = state.GarbageCollector.FromInteger(32);

            ObjectInstance result = state.Call(addMethod, argA, argB);
            Console.WriteLine($"\nInvoked Program.Add(37, 32) -> {result.AsInteger()}");
        }
    }

    // [TestMethod]
    public void RunSyntaxBuilderExample()
    {
        Console.WriteLine("\n--- SyntaxBuilder example ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(true));

        CompilationContext context = state.Context;

        // Build a compilation unit programmatically:
        // namespace syntax_builder_demo;
        // public class Program
        // {
        //     public static func Add(a: int, b: int) -> int { return a + b; }
        //     public static func Main() -> void { }
        // }
        SyntaxCompilationUnit unit = SyntaxBuilder.Unit(context).SetOrigin(CompilationUnitOrigin.SourceFile);
        SyntaxNamespaceDeclaration ns = SyntaxBuilder.Namespace().AddIdentifier("syntax_builder_demo");
        unit.SetNamespace(ns);

        SyntaxClassDeclaration programClass = SyntaxBuilder.Class(ns, "Program");
        unit.AddMember(programClass);

        SyntaxParametersList addParams = SyntaxBuilder.Parameters()
            .AddParameter("a", SyntaxBuilder.IntType())
            .AddParameter("b", SyntaxBuilder.IntType());

        SyntaxMethodDeclaration addMethod = SyntaxBuilder.Method(programClass, "Add", SyntaxBuilder.IntType(programClass))
            .AddModifier(TokenType.PublicKeyword)
            .AddModifier(TokenType.StaticKeyword)
            .SetParameters(addParams)
            .SetBody(
                SyntaxBuilder.Block()
                    .AddStatement(
                        SyntaxBuilder.Return(null,
                            SyntaxBuilder.Binary(
                                SyntaxBuilder.Identifier("a"),
                                SyntaxBuilder.Identifier("b"),
                                TokenType.AddOperator))));

        programClass.AddMember(addMethod);

        SyntaxMethodDeclaration mainMethod = SyntaxBuilder.Method(unit, "Main", SyntaxBuilder.VoidType(unit))
            .AddModifier(TokenType.PublicKeyword)
            .AddModifier(TokenType.StaticKeyword)
            .SetParameters(SyntaxBuilder.Parameters())
            .SetBody(SyntaxBuilder.Block());

        unit.AddMember(mainMethod);
        context.AddCompilationUnit(unit);

        if (!state.TryCompile())
        {
            Console.WriteLine($"SyntaxBuilder compilation failed with {state.Context.ErrorCount} error(s):");
            Console.WriteLine(state.Diagnostics);
            return;
        }

        state.Run();

        MethodSymbol? addSymbol = state.FindMethod("Program", "Add", 2);
        if (addSymbol != null)
        {
            ObjectInstance argA = state.GarbageCollector.FromInteger(10);
            ObjectInstance argB = state.GarbageCollector.FromInteger(32);

            ObjectInstance result = state.Call(addSymbol, argA, argB);
            Console.WriteLine($"SyntaxBuilder Program.Add(10, 32) -> {result.AsInteger()}");
        }
    }

    // [TestMethod]
    public void RunSymbolBuilderCallbackExample()
    {
        Console.WriteLine("\n--- SymbolBuilder callback example (NEW FLUENT API) ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(true));

        CompilationContext context = state.Context;

        // NEW: Using fluent lambda-scoped API with automatic type inference and marshalling
        SymbolBuilder.CreateNamespace(context, "callback_demo", ns =>
        {
            ns.WithClass("ExternalMath", cls =>
            {
                cls.WithMethod("Add", (long a, long b) => a + b, out MethodSymbol addMethod);
            });
        });

        // The script can see ExternalMath because it was added to the shared namespace tree
        state.AddSource("Callback.ss", """
            using stdio;
            namespace callback_demo;
            public static func Main() -> void
            {
                x := ExternalMath.Add(10, 32);
                println(x);
            }
            """, CompilationUnitOrigin.SourceFile);

        if (!state.TryCompile())
        {
            Console.WriteLine($"SymbolBuilder callback compilation failed with {state.Context.ErrorCount} error(s):");
            Console.WriteLine(state.Diagnostics);
            return;
        }

        Console.WriteLine("Running entry point that calls C# callback...");
        state.Run();

        // Also invoke the callback directly from C# using the new fluent API
        MethodSymbol? addMethod = state.FindMethod("ExternalMath", "Add", 2);
        if (addMethod != null)
        {
            ObjectInstance argA = state.GarbageCollector.FromInteger(100);
            ObjectInstance argB = state.GarbageCollector.FromInteger(23);
            ObjectInstance result = state.Call(addMethod, argA, argB);
            Console.WriteLine($"Direct C# invoke ExternalMath.Add(100, 23) -> {result.AsInteger()}");
        }
    }

    // [TestMethod]
    public void RunSymbolBuilderCallbackExampleOLD()
    {
        Console.WriteLine("\n--- SymbolBuilder callback example (OLD API for comparison) ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(true));

        CompilationContext context = state.Context;

        // OLD: Verbose low-level API with manual marshalling
        NamespaceSymbol ns = SymbolBuilder.Namespace(context, "callback_demo").Symbol;
        TypeSymbol externalMath = SymbolBuilder.Class(context, "ExternalMath", ns).Symbol;

        MethodSymbol addMethod = SymbolBuilder.Method(context, externalMath, "Add", SymbolBuilder.Primitive(context, PrimitiveType.Integer))
            .Public().Static()
            .Parameter("a", SymbolBuilder.Primitive(context, PrimitiveType.Integer))
            .Parameter("b", SymbolBuilder.Primitive(context, PrimitiveType.Integer))
            .Callback((method, args, argsCount, userData, collector) =>
            {
                long a = new ObjectInstance(args[0]).AsInteger();
                long b = new ObjectInstance(args[1]).AsInteger();
                return new GarbageCollector(collector).FromInteger(a + b).Handle;
            }).Symbol;

        state.AddSource("Callback.ss", """
            using stdio;
            namespace callback_demo;
            public static func Main() -> void
            {
                x := ExternalMath.Add(10, 32);
                println(x);
            }
            """, CompilationUnitOrigin.SourceFile);

        if (!state.TryCompile())
        {
            Console.WriteLine($"SymbolBuilder callback compilation failed with {state.Context.ErrorCount} error(s):");
            Console.WriteLine(state.Diagnostics);
            return;
        }

        Console.WriteLine("Running entry point that calls C# callback...");
        state.Run();

        // Also invoke the callback directly from C#.
        ObjectInstance argA = state.GarbageCollector.FromInteger(100);
        ObjectInstance argB = state.GarbageCollector.FromInteger(23);
        ObjectInstance result = state.Call(addMethod, argA, argB);
        Console.WriteLine($"Direct C# invoke ExternalMath.Add(100, 23) -> {result.AsInteger()}");
    }

    // [TestMethod]
    public void RunFluentApiExample()
    {
        Console.WriteLine("\n--- Fluent API comprehensive example ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(false));

        CompilationContext context = state.Context;

        // NEW: Comprehensive fluent API demonstrating:
        // - Lambda scoping
        // - Automatic type inference
        // - Multiple method overloads
        // - Field creation with type inference
        // - Symbol extraction (out parameter)
        SymbolBuilder.CreateNamespace(context, "math_utils", ns =>
        {
            ns.WithClass("Calculator", cls =>
            {
                cls.Public();

                // Methods with automatic type inference
                cls.WithMethod("Add", (long a, long b) => a + b);
                cls.WithMethod("Add", (double a, double b) => a + b);
                cls.WithMethod("Subtract", (long a, long b) => a - b);
                cls.WithMethod("Multiply", (long a, long b) => a * b);
                cls.WithMethod("Divide", (double a, double b) =>
                {
                    if (b == 0.0)
                        throw new InvalidOperationException("Division by zero");

                    return a / b;
                });

                // Method with symbol extraction for direct invocation
                cls.WithMethod("Power", (double baseValue, double exponent) => Math.Pow(baseValue, exponent), out MethodSymbol powerMethod);

                // Method with multiple parameters
                cls.WithMethod("Sum", (long a, long b, long c, long d) => a + b + c + d);

                // Method with string parameter and return
                cls.WithMethod("Greet", (string name) => $"Hello, {name}!");

                // Void method
                cls.WithMethod("PrintMessage", (string message) =>
                {
                    Console.WriteLine($"[From C#] {message}");
                });

                // Fields with type inference
                cls.WithField<long>("PiApproximation", Access.Public);
                cls.WithField<double>("EApproximation", Access.Public);
            });
        });

        // Set static field values after compilation
        state.Compile("""
            namespace math_utils;

            public static class Program
            {
                public static func TestInt() -> int
                {
                    return Calculator.Add(10, 32);
                }

                public static func TestDouble() -> double
                {
                    return Calculator.Add(10.5, 20.3);
                }

                public static func TestPower() -> double
                {
                    return Calculator.Power(2.0, 10.0);
                }

                public static func TestSum() -> int
                {
                    return Calculator.Sum(1, 2, 3, 4);
                }

                public static func TestGreet() -> string
                {
                    return Calculator.Greet("World");
                }
            }
            """);

        // Test the methods from C#
        long intResult = state.Call<long>("Program.TestInt");
        Console.WriteLine($"Calculator.Add(10, 32) [int] -> {intResult}");

        double doubleResult = state.Call<double>("Program.TestDouble");
        Console.WriteLine($"Calculator.Add(10.5, 20.3) [double] -> {doubleResult:F2}");

        double powerResult = state.Call<double>("Program.TestPower");
        Console.WriteLine($"Calculator.Power(2.0, 10.0) -> {powerResult:F2}");

        long sumResult = state.Call<long>("Program.TestSum");
        Console.WriteLine($"Calculator.Sum(1, 2, 3, 4) -> {sumResult}");

        string greetResult = state.Call<string>("Program.TestGreet");
        Console.WriteLine($"Calculator.Greet(\"World\") -> {greetResult}");

        // Test direct invocation using FindMethod
        MethodSymbol? powerMethod = state.FindMethod("Calculator", "Power", 2);
        if (powerMethod != null)
        {
            ObjectInstance baseArg = state.GarbageCollector.FromDouble(3.0);
            ObjectInstance expArg = state.GarbageCollector.FromDouble(4.0);
            ObjectInstance directResult = state.Call(powerMethod, baseArg, expArg);
            Console.WriteLine($"Direct Calculator.Power(3.0, 4.0) -> {directResult.AsDouble():F2}");
        }
    }

    // [TestMethod]
    public void RunNamespaceMethodExample()
    {
        Console.WriteLine("\n--- Namespace-level method (free function) example ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(false));

        CompilationContext context = state.Context;

        // Methods attached directly to a namespace become free functions.
        MethodSymbol? addMethod = null;
        SymbolBuilder.CreateNamespace(context, "free_funcs", ns =>
        {
            ns.WithMethod("Add", (long a, long b) => a + b);
            ns.WithMethod("Greet", (string name) => $"Hello, {name}!");
            ns.WithMethod("Nothing", () => Console.WriteLine("void"), out addMethod);
        });

        state.AddSource("FreeFuncs.ss", """
            namespace free_funcs;

            public static class Program
            {
                public static func RunInt() -> int { return Add(40, 2); }
                public static func RunGreet() -> string { return Greet("Shard"); }
            }
            """, CompilationUnitOrigin.SourceFile);

        if (!state.TryCompile())
        {
            Console.WriteLine($"free_funcs compilation failed with {state.Context.ErrorCount} error(s):");
            Console.WriteLine(state.Diagnostics);
        }

        // Invoke the free function directly via its extracted symbol.
        if (addMethod != null)
        {
            //ObjectInstance a = state.GarbageCollector.FromInteger(100);
            //ObjectInstance b = state.GarbageCollector.FromInteger(23);
            state.Call(addMethod, []);
            //Console.WriteLine($"Direct free_funcs.Add(100, 23) -> {result.AsInteger()}");
        }

        MethodSymbol? runInt = state.FindMethod("Program", "RunInt", 0);
        if (runInt != null)
        {
            long intResult = state.Call<long>("Program.RunInt");
            Console.WriteLine($"free_funcs.Add(40, 2) via script -> {intResult}");
        }
        else
        {
            Console.WriteLine("Program.RunInt was not found in the compiled script.");
        }
    }

    // [TestMethod]
    public void RunEnumExample()
    {
        Console.WriteLine("\n--- SDK enum example ---");

        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(false));

        state.Compile("""
            using stdio;
            namespace enum_demo;

            public enum Color
            {
                Red,
                Green,
                Blue
            }

            public enum Permissions : flags
            {
                Read,
                Write,
                Execute
            }

            public static class Program
            {
                public static func GetColorName() -> string { return Color.Red.ToString(); }
                public static func GetPermissions() -> string { return (Permissions.Read | Permissions.Write).ToString(); }
                public static func HasExecute(p: Permissions) -> bool { return p.HasFlag(Permissions.Execute); }
                public static func CheckReadHasExecute() -> bool { return HasExecute(Permissions.Read); }
            }
            """);

        string colorName = state.Call<string>("Program.GetColorName");
        string permissions = state.Call<string>("Program.GetPermissions");
        bool hasExecute = state.Call<bool>("Program.CheckReadHasExecute");
        Console.WriteLine($"Program.GetColorName() -> {colorName}");
        Console.WriteLine($"Program.GetPermissions() -> {permissions}");
        Console.WriteLine($"Program.CheckReadHasExecute() -> {hasExecute}");
    }

    // [TestMethod]
    public void RunEvaluateExample()
    {
        Console.WriteLine("\n--- SDK Evaluate<T> example ---");

        int result = ShardScriptEngine.Evaluate<int>("1 + 2", ShardScriptOptions.Default.WithStandardLibraries());
        Console.WriteLine($"Evaluate<int>(\"1 + 2\") -> {result}");
    }

    // [TestMethod]
    public void RunGlobalsExample()
    {
        Console.WriteLine("\n--- SDK globals example ---");

        var globals = new { PlayerHealth = 100 };
        using var state = ShardScriptEngine.DoString("""
            namespace globals;
            public static class Program
            {
                public static func GetHealth() -> int
                {
                    return Globals.PlayerHealth;
                }
            }
            """, ShardScriptOptions.Default.WithStandardLibraries().WithNamespace("globals").WithEntryPoint(false), globals);

        Console.WriteLine($"Globals.PlayerHealth = {state["Globals.PlayerHealth"]}");
        state["Globals.PlayerHealth"] = 200;

        Console.WriteLine($"After assignment: Globals.PlayerHealth = {state["Globals.PlayerHealth"]}");
        Console.WriteLine($"Call<int>(\"Program.GetHealth\") = {state.Call<int>("Program.GetHealth")}");
    }

    // [TestMethod]
    public void RunCallAndIndexerExample()
    {
        Console.WriteLine("\n--- SDK Call<T> and indexer example ---");

        using var state = ShardScriptEngine.DoString("""
            using stdio;
            
            namespace call_demo;

            public static class Program
            {
                public static Counter: int;

                public static func Add(a: int, b: int) -> int
                {
                    return a + b;
                }
            }
            """, ShardScriptOptions.Default.WithStandardLibraries().WithEntryPoint(false));

        long sum = state.Call<long>("Program.Add", 12, 30);
        Console.WriteLine($"Call<long>(\"Program.Add\", 12, 30) -> {sum}");

        Console.WriteLine($"state[\"Program.Counter\"] = {state["Program.Counter"]}");
        state["Program.Counter"] = 99;
        Console.WriteLine($"After assignment: state[\"Program.Counter\"] = {state["Program.Counter"]}");
    }

    [ShardScriptUserData(Namespace = "reflection_demo", TypeName = "Math")]
    public class ExternalMathApi
    {
        public static int Multiply(int a, int b) => a * b;
    }

    // [TestMethod]
    public void RunReflectionBinderExample()
    {
        using ShardScriptState state = new ShardScriptState(ShardScriptOptions.Default
            .WithStandardLibraries()
            .WithEntryPoint(false));

        state.RegisterFunctions("reflection_demo", options => options.IncludeType<ExternalMathApi>());
        state.Compile("""
            namespace reflection_demo;

            public static class Program
            {
                public static func GetProduct() -> int
                {
                    return Math.Multiply(6, 7);
                }
            }
            """);

        int retValue = state.Call<int>("Program.GetProduct");
        Console.WriteLine($"Call<int>(\"Program.GetProduct\") = {retValue}");
    }
}
