namespace ShardScript.NET;

class Program
{
    public static int Main()
    {
        Console.WriteLine($"ShardScript version: {ShardInfo.Version}");

        RunFileBasedExample();
        RunSyntaxBuilderExample();
        RunSymbolBuilderCallbackExample();

        return 0;
    }

    private static void RunFileBasedExample()
    {
        Console.WriteLine("\n--- File-based example ---");

        using CompilationContext context = new CompilationContext();

        string stdLibPath = Path.Combine(AppContext.BaseDirectory, "libShardScript.Framework.dll");
        if (File.Exists(stdLibPath))
        {
            context.AddLibrary(stdLibPath);
        }
        else
        {
            Console.WriteLine("Warning: standard library not found next to the executable.");
        }

        context.AddSource("Test.ss", File.ReadAllText("Test.ss"), CompilationUnitOrigin.SourceFile);
        context.Analyze();

        if (context.HasErrors)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Compilation failed with {context.ErrorCount} error(s):");
            Console.WriteLine(context.GetDiagnostics());
            Console.ResetColor();
            return;
        }

        Console.WriteLine("\nCompilation units:");
        foreach (CompilationUnit? unit in context.GetCompilationUnits().Where(u => u.Origin == CompilationUnitOrigin.SourceFile))
        {
            if (unit.Namespace is { } ns)
                Console.WriteLine($"  Namespace: {ns.Name}");

            foreach (ClassDeclaration cls in unit.GetClasses())
            {
                Console.WriteLine($"  Class: {cls.Name}");
                foreach (MethodSymbol method in cls.GetMethods(context))
                {
                    Console.WriteLine($"    Method: {method.Name}({method.ParameterCount} params), static: {method.IsStatic}");
                }
            }
        }

        using ApplicationDomain? domain = context.Compile();
        if (domain is null)
        {
            Console.WriteLine("Compilation returned null domain.");
            return;
        }

        Console.WriteLine("\nRunning entry point...");
        domain.Run();

        TypeSymbol? programType = context.FindType("Program");
        if (programType != null)
        {
            MethodSymbol? addMethod = programType.FindMethod("Add", 2);
            if (addMethod != null)
            {
                VirtualMachine vm = domain.GetVirtualMachine();
                GarbageCollector gc = domain.GetGarbageCollector();

                ObjectInstance argA = gc.FromInteger(37);
                ObjectInstance argB = gc.FromInteger(32);

                ObjectInstance result = vm.InvokeMethod(addMethod, argA, argB);
                Console.WriteLine($"\nInvoked Program.Add(37, 32) -> {result.AsInteger()}");
            }
        }
    }

    private static void RunSyntaxBuilderExample()
    {
        Console.WriteLine("\n--- SyntaxBuilder example ---");

        using CompilationContext context = new CompilationContext();

        string stdLibPath = Path.Combine(AppContext.BaseDirectory, "libShardScript.Framework.dll");
        if (File.Exists(stdLibPath))
            context.AddLibrary(stdLibPath);

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

        context.Analyze();

        if (context.HasErrors)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"SyntaxBuilder compilation failed with {context.ErrorCount} error(s):");
            Console.WriteLine(context.GetDiagnostics());
            Console.ResetColor();
            return;
        }

        ApplicationDomain? domain;
        try
        {
            domain = context.Compile();
        }
        catch (InvalidOperationException)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"SyntaxBuilder compile failed with diagnostics:\n{context.GetDiagnostics()}");
            Console.ResetColor();
            return;
        }

        if (domain is null)
        {
            Console.WriteLine("SyntaxBuilder compilation returned null domain.");
            return;
        }

        domain.Run();

        TypeSymbol? programType = context.FindType("Program");
        if (programType != null)
        {
            MethodSymbol? addSymbol = programType.FindMethod("Add", 2);
            if (addSymbol != null)
            {
                VirtualMachine vm = domain.GetVirtualMachine();
                GarbageCollector gc = domain.GetGarbageCollector();

                ObjectInstance argA = gc.FromInteger(10);
                ObjectInstance argB = gc.FromInteger(32);

                ObjectInstance result = vm.InvokeMethod(addSymbol, argA, argB);
                Console.WriteLine($"SyntaxBuilder Program.Add(10, 32) -> {result.AsInteger()}");
            }
        }
    }

    private static void RunSymbolBuilderCallbackExample()
    {
        Console.WriteLine("\n--- SymbolBuilder callback example ---");

        using CompilationContext context = new CompilationContext();

        string stdLibPath = Path.Combine(AppContext.BaseDirectory, "libShardScript.Framework.dll");
        if (File.Exists(stdLibPath))
            context.AddLibrary(stdLibPath);

        // Programmatically create the namespace, type and method symbols, then
        // bind a C# callback to the method. The script below can see the type
        // because the compiler resolves namespace members through the shared
        // namespace tree.
        SymbolNamespace ns = SymbolFactory.CreateNamespace(context, "callback_demo");
        SymbolType externalMath = SymbolFactory.CreateClass(context, "ExternalMath", ns);

        SymbolMethod addMethod = SymbolFactory.CreateMethod(
            context,
            externalMath,
            "Add",
            SymbolFactory.GetPrimitiveType(context, PrimitiveType.Integer),
            true,
            SymbolAccessibility.Public);

        SymbolFactory.AddParameter(
            addMethod,
            SymbolFactory.CreateParameter(context, "a", SymbolFactory.GetPrimitiveType(context, PrimitiveType.Integer)));
        SymbolFactory.AddParameter(
            addMethod,
            SymbolFactory.CreateParameter(context, "b", SymbolFactory.GetPrimitiveType(context, PrimitiveType.Integer)));

        SymbolFactory.SetCallback(
            addMethod,
            (method, args, argsCount, userData, collector) =>
            {
                long a = new ObjectInstance(args[0]).AsInteger();
                long b = new ObjectInstance(args[1]).AsInteger();
                return new GarbageCollector(collector).FromInteger(a + b).Handle;
            });

        context.AddSource("Callback.ss", """
            using stdio;
            namespace callback_demo;
            public static func Main() -> void
            {
                x := ExternalMath.Add(10, 32);
                println(x);
            }
            """, CompilationUnitOrigin.SourceFile);

        context.Analyze();

        if (context.HasErrors)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"SymbolBuilder callback compilation failed with {context.ErrorCount} error(s):");
            Console.WriteLine(context.GetDiagnostics());
            Console.ResetColor();
            return;
        }

        using ApplicationDomain? domain = context.Compile();
        if (domain is null)
        {
            Console.WriteLine("SymbolBuilder callback compilation returned null domain.");
            return;
        }

        Console.WriteLine("Running entry point that calls C# callback...");
        domain.Run();

        // Also invoke the callback directly from C#.
        VirtualMachine vm = domain.GetVirtualMachine();
        GarbageCollector gc = domain.GetGarbageCollector();

        ObjectInstance argA = gc.FromInteger(100);
        ObjectInstance argB = gc.FromInteger(23);
        ObjectInstance result = vm.InvokeMethod(new MethodSymbol(addMethod.Handle), argA, argB);
        Console.WriteLine($"Direct C# invoke ExternalMath.Add(100, 23) -> {result.AsInteger()}");
    }
}
