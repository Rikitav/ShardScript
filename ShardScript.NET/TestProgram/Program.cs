using ShardScript.NET.Application;
using ShardScript.NET.Runtime;
using ShardScript.NET.Scripting;
using ShardScript.NET.Syntax.Builders;
using ShardScript.NET.Syntax.Nodes;
using ShardScript.NET.Syntax.Symbols;

namespace ShardScript.NET.TestProgram;

class Program
{
    public static int Main(string[] args)
    {
        Console.WriteLine($"ShardScript version: {ShardScriptAPI.Version}");

        if (args.Length > 0)
        {
            RunFile(args[0]);
            return 0;
        }

        RunFileBasedExample();
        RunSyntaxBuilderExample();
        RunSymbolBuilderCallbackExample();
        RunSdkExamples();

        return 0;
    }

    private static void RunFile(string filePath)
    {
        Console.WriteLine($"\n--- Running file: {filePath} ---");

        using ScriptEngine engine = new ScriptEngine();
        engine.LoadStandardLibraries();
        engine.AddSourceFile(filePath, CompilationUnitOrigin.SourceFile);

        if (!engine.Compile())
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Compilation failed with {engine.Context.ErrorCount} error(s):");
            Console.WriteLine(engine.Diagnostics);
            Console.ResetColor();
            return;
        }

        Console.WriteLine("\nRunning entry point...");
        engine.Run();
    }

    private static void RunFileBasedExample()
    {
        Console.WriteLine("\n--- File-based example ---");

        using ScriptEngine engine = new ScriptEngine();
        engine.LoadStandardLibraries();

        string testFilePath = Path.Combine(AppContext.BaseDirectory, "TestProgram", "Test.ss");
        engine.AddSourceFile(testFilePath, CompilationUnitOrigin.SourceFile);

        if (!engine.Compile())
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Compilation failed with {engine.Context.ErrorCount} error(s):");
            Console.WriteLine(engine.Diagnostics);
            Console.ResetColor();
            return;
        }

        Console.WriteLine("\nCompilation units:");
        foreach (CompilationUnitSyntax? unit in engine.GetSourceUnits())
        {
            if (unit.Namespace is { } ns)
                Console.WriteLine($"  Namespace: {ns.Name}");

            foreach (ClassDeclarationSyntax cls in unit.GetClasses())
            {
                Console.WriteLine($"  Class: {cls.Name}");
                foreach (MethodSymbol method in cls.GetMethods(engine.Context))
                {
                    Console.WriteLine($"    Method: {method.Name}({method.ParameterCount} params), static: {method.IsStatic}");
                }
            }
        }

        Console.WriteLine("\nRunning entry point...");
        engine.Run();

        MethodSymbol? addMethod = engine.FindMethod("Program", "Add", 2);
        if (addMethod != null)
        {
            ObjectInstance argA = engine.GarbageCollector.FromInteger(37);
            ObjectInstance argB = engine.GarbageCollector.FromInteger(32);

            ObjectInstance result = engine.Invoke(addMethod, argA, argB);
            Console.WriteLine($"\nInvoked Program.Add(37, 32) -> {result.AsInteger()}");
        }
    }

    private static void RunSyntaxBuilderExample()
    {
        Console.WriteLine("\n--- SyntaxBuilder example ---");

        using ScriptEngine engine = new ScriptEngine();
        engine.LoadStandardLibraries();

        CompilationContext context = engine.Context;

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

        if (!engine.Compile())
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"SyntaxBuilder compilation failed with {engine.Context.ErrorCount} error(s):");
            Console.WriteLine(engine.Diagnostics);
            Console.ResetColor();
            return;
        }

        engine.Run();

        MethodSymbol? addSymbol = engine.FindMethod("Program", "Add", 2);
        if (addSymbol != null)
        {
            ObjectInstance argA = engine.GarbageCollector.FromInteger(10);
            ObjectInstance argB = engine.GarbageCollector.FromInteger(32);

            ObjectInstance result = engine.Invoke(addSymbol, argA, argB);
            Console.WriteLine($"SyntaxBuilder Program.Add(10, 32) -> {result.AsInteger()}");
        }
    }

    private static void RunSymbolBuilderCallbackExample()
    {
        Console.WriteLine("\n--- SymbolBuilder callback example ---");

        using ScriptEngine engine = new ScriptEngine();
        engine.LoadStandardLibraries();

        CompilationContext context = engine.Context;

        // Programmatically create the namespace, type and method symbols, then
        // bind a C# callback to the method. The script below can see the type
        // because the compiler resolves namespace members through the shared
        // namespace tree.
        NamespaceSymbol ns = SymbolBuilder.Namespace(context, "callback_demo").Build();
        TypeSymbol externalMath = SymbolBuilder.Class(context, "ExternalMath", ns).Build();

        MethodSymbol addMethod = SymbolBuilder.Method(context, externalMath, "Add", SymbolBuilder.Primitive(context, PrimitiveType.Integer))
            .Public()
            .Static()
            .Parameter("a", SymbolBuilder.Primitive(context, PrimitiveType.Integer))
            .Parameter("b", SymbolBuilder.Primitive(context, PrimitiveType.Integer))
            .Callback((method, args, argsCount, userData, collector) =>
            {
                long a = new ObjectInstance(args[0]).AsInteger();
                long b = new ObjectInstance(args[1]).AsInteger();
                return new GarbageCollector(collector).FromInteger(a + b).Handle;
            }).Symbol;

        engine.AddSource("Callback.ss", """
            using stdio;
            namespace callback_demo;
            public static func Main() -> void
            {
                x := ExternalMath.Add(10, 32);
                println(x);
            }
            """, CompilationUnitOrigin.SourceFile);

        if (!engine.Compile())
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"SymbolBuilder callback compilation failed with {engine.Context.ErrorCount} error(s):");
            Console.WriteLine(engine.Diagnostics);
            Console.ResetColor();
            return;
        }

        Console.WriteLine("Running entry point that calls C# callback...");
        engine.Run();

        // Also invoke the callback directly from C#.
        ObjectInstance argA = engine.GarbageCollector.FromInteger(100);
        ObjectInstance argB = engine.GarbageCollector.FromInteger(23);
        ObjectInstance result = engine.Invoke(addMethod, argA, argB);
        Console.WriteLine($"Direct C# invoke ExternalMath.Add(100, 23) -> {result.AsInteger()}");
    }

    private static void RunSdkExamples()
    {
        RunEvaluateExample();
        RunGlobalsExample();
        RunCallAndIndexerExample();
        RunReflectionBinderExample();
        RunScopedAstBuilderExample();
    }

    private static void RunEvaluateExample()
    {
        Console.WriteLine("\n--- SDK Evaluate<T> example ---");

        int result = ShardScript.Evaluate<int>("1 + 2", ScriptOptions.Default.WithStandardLibraries());
        Console.WriteLine($"Evaluate<int>(\"1 + 2\") -> {result}");
    }

    private static void RunGlobalsExample()
    {
        Console.WriteLine("\n--- SDK globals example ---");

        var globals = new { PlayerHealth = 100 };
        using var state = ShardScript.Run("""
            namespace globals;
            public static class Program
            {
                public static func GetHealth() -> int { return Globals.PlayerHealth; }
            }
            """, ScriptOptions.Default.WithStandardLibraries().WithNamespace("globals").WithEntryPoint(false), globals);

        Console.WriteLine($"Globals.PlayerHealth = {state["Globals.PlayerHealth"]}");
        state["Globals.PlayerHealth"] = 200;
        Console.WriteLine($"After assignment: Globals.PlayerHealth = {state["Globals.PlayerHealth"]}");
        Console.WriteLine($"Call<int>(\"Program.GetHealth\") = {state.Call<int>("Program.GetHealth")}");
    }

    private static void RunCallAndIndexerExample()
    {
        Console.WriteLine("\n--- SDK Call<T> and indexer example ---");

        using var state = ShardScript.Run("""
            using stdio;
            namespace call_demo;
            public static class Program
            {
                public static func Add(a: int, b: int) -> int { return a + b; }
                public static Counter: int;
            }
            """, ScriptOptions.Default.WithStandardLibraries().WithEntryPoint(false));

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

    private static void RunReflectionBinderExample()
    {
        Console.WriteLine("\n--- SDK reflection binder example ---");

        using var state = new ScriptState(ScriptOptions.Default.WithStandardLibraries().WithEntryPoint(false));
        state.RegisterFunctions("reflection_demo", options => options.IncludeType<ExternalMathApi>());
        state.Compile("""
            namespace reflection_demo;
            public static class Program
            {
                public static func GetProduct() -> int { return Math.Multiply(6, 7); }
            }
            """);

        Console.WriteLine($"Call<int>(\"Program.GetProduct\") = {state.Call<int>("Program.GetProduct")}");
    }

    private static void RunScopedAstBuilderExample()
    {
        Console.WriteLine("\n--- SDK scoped AST builder example ---");

        using var state = new ScriptState(ScriptOptions.Default.WithStandardLibraries().WithEntryPoint(false));

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

        state.Compile();
        long result = state.Call<long>("Program.Add", 10, 32);
        Console.WriteLine($"Scoped AST builder Program.Add(10, 32) -> {result}");
    }
}
