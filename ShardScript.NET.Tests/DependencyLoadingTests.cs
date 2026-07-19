using ShardScript.Application;

namespace ShardScript.NET.Tests;

/// <summary>
/// Regression tests for deterministic library loading and dependency resolution.
/// </summary>
public sealed class DependencyLoadingTests
{
    public static void RunAll()
    {
        TestReversedDependencyOrder();
        TestMissingDependencyDiagnostic();
        TestCircularDependencyDiagnostic();
    }

    private static string BinDirectory =>
        Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "out", "build", "x64-debug", "bin"));

    private static string SystemLibraryPath(string name) =>
        Path.Combine(BinDirectory, "system", $"{name}.dll");

    private static string TestLibraryPath(string name) =>
        Path.Combine(BinDirectory, "test_libs", $"{name}.dll");

    /// <summary>
    /// Loads libraries in reversed path order where one depends on another.
    /// The resolver must still produce a valid compilation because it sorts them
    /// topologically.
    /// </summary>
    public static void TestReversedDependencyOrder()
    {
        Console.WriteLine("\n--- Dependency loading: reversed order ---");

        string collectionsPath = SystemLibraryPath("collections.shard");
        string subprocessPath = SystemLibraryPath("subprocess.shard");

        if (!File.Exists(collectionsPath) || !File.Exists(subprocessPath))
        {
            Console.WriteLine("Skipping: framework libraries not built yet.");
            return;
        }

        using CompilationContext context = new();

        // subprocess depends on collections, so the resolver must load collections first.
        context.AddLibraries(new[] { subprocessPath, collectionsPath });

        context.AddSource("reversed_order.ss", """
            using process;

            namespace reversed_order_test;

            public class Program
            {
                public static func Main() -> void
                {
                    p := Process.Start("cmd", "/c echo dependency_ok");
                }
            }
            """);

        if (context.HasErrors)
        {
            Console.WriteLine("Reversed-order load produced unexpected errors:");
            Console.WriteLine(context.GetDiagnostics());
            return;
        }

        // Compilation exercises the resolved dependency order: subprocess needs
        // collections' Dictionary layout to be registered before its entry point runs.
        using ApplicationDomain domain = context.Compile();
        Console.WriteLine("Reversed-order load and compile succeeded.");
    }

    /// <summary>
    /// Loading a library without its dependency must produce a clear diagnostic
    /// instead of a cryptic namespace-not-found error.
    /// </summary>
    public static void TestMissingDependencyDiagnostic()
    {
        Console.WriteLine("\n--- Dependency loading: missing dependency ---");

        string subprocessPath = SystemLibraryPath("subprocess.shard");
        if (!File.Exists(subprocessPath))
        {
            Console.WriteLine("Skipping: framework libraries not built yet.");
            return;
        }

        using CompilationContext context = new();

        // Only load subprocess; its dependency on collections is intentionally omitted.
        context.AddLibraries(new[] { subprocessPath });

        if (!context.HasErrors)
        {
            Console.WriteLine("FAILED: expected an error for missing dependency.");
            return;
        }

        string diagnostics = context.GetDiagnostics();
        if (!diagnostics.Contains("shard.collections", StringComparison.Ordinal))
        {
            Console.WriteLine("FAILED: diagnostic does not mention missing dependency.");
            Console.WriteLine(diagnostics);
            return;
        }

        Console.WriteLine("Missing-dependency diagnostic reported correctly.");
    }

    /// <summary>
    /// Two libraries that declare circular dependencies must be rejected with a
    /// clear circular-dependency diagnostic.
    /// </summary>
    public static void TestCircularDependencyDiagnostic()
    {
        Console.WriteLine("\n--- Dependency loading: circular dependency ---");

        string libA = TestLibraryPath("circular_lib_a.shard");
        string libB = TestLibraryPath("circular_lib_b.shard");

        if (!File.Exists(libA) || !File.Exists(libB))
        {
            Console.WriteLine("Skipping: circular test libraries not built yet.");
            return;
        }

        using CompilationContext context = new();
        context.AddLibraries(new[] { libA, libB });

        if (!context.HasErrors)
        {
            Console.WriteLine("FAILED: expected an error for circular dependency.");
            return;
        }

        string diagnostics = context.GetDiagnostics();
        if (!diagnostics.Contains("Circular", StringComparison.OrdinalIgnoreCase))
        {
            Console.WriteLine("FAILED: diagnostic does not mention circular dependency.");
            Console.WriteLine(diagnostics);
            return;
        }

        Console.WriteLine("Circular-dependency diagnostic reported correctly.");
    }
}
