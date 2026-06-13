namespace ShardScript.NET;

class Program
{
    public static int Main()
    {
        Console.WriteLine($"ShardScript version: {ShardInfo.Version}");

        using CompilationContext context = new CompilationContext();

        string stdLibPath = Path.Combine(AppContext.BaseDirectory, "ShardScript.Framework.dll");
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
            return 1;
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
            return 2;
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
                Console.WriteLine($"\nInvoked Program.Add(10, 32) -> {result.AsInteger()}");
            }
        }

        return 0;
    }
}
