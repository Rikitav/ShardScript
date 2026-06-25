using ShardScript.NET.Application;
using ShardScript.NET.Runtime;
using ShardScript.NET.Syntax.Nodes;
using ShardScript.NET.Syntax.Symbols;

namespace ShardScript.NET;

/// <summary>
/// High-level facade for compiling and running ShardScript code.
/// </summary>
public sealed class ScriptEngine : IDisposable
{
    private readonly CompilationContext _context;
    private ApplicationDomain? _domain;
    private bool _disposed;

    /// <summary>
    /// Creates a new script engine.
    /// </summary>
    public ScriptEngine()
    {
        _context = new CompilationContext();
    }

    /// <summary>
    /// Creates a new script engine and loads the given libraries.
    /// </summary>
    public ScriptEngine(IEnumerable<string> libraryPaths)
        : this()
    {
        if (libraryPaths == null)
            throw new ArgumentNullException(nameof(libraryPaths));

        foreach (string path in libraryPaths)
            LoadLibrary(path);
    }

    /// <summary>
    /// The underlying compilation context.
    /// </summary>
    public CompilationContext Context => _context;

    /// <summary>
    /// The compiled application domain, if <see cref="Compile"/> or <see cref="CompileAndRun"/> has succeeded.
    /// </summary>
    public ApplicationDomain? Domain => _domain;

    /// <summary>
    /// Whether the last compilation produced errors.
    /// </summary>
    public bool HasErrors => _context.HasErrors;

    /// <summary>
    /// Returns the diagnostic messages from the last compilation.
    /// </summary>
    public string Diagnostics => _context.GetDiagnostics();

    /// <summary>
    /// Loads all *.shard.dll libraries from the default "framework" directory next to the application.
    /// </summary>
    public void LoadStandardLibraries()
    {
        string frameworkDir = Path.Combine(AppContext.BaseDirectory, "framework");
        if (!Directory.Exists(frameworkDir))
            return;

        foreach (string libraryPath in Directory.EnumerateFiles(frameworkDir, "*.dll"))
        {
            try
            {
                LoadLibrary(libraryPath);
            }
            catch (Exception ex)
            {
                // Standard libraries are best-effort; individual failures should not stop loading.
                Console.WriteLine($"Warning: failed to load library '{libraryPath}': {ex.Message}");
            }
        }
    }

    /// <summary>
    /// Loads a native ShardScript library.
    /// </summary>
    public void LoadLibrary(string path)
    {
        if (path == null)
            throw new ArgumentNullException(nameof(path));

        _context.AddLibrary(path);
    }

    /// <summary>
    /// Adds source code directly.
    /// </summary>
    public void AddSource(string name, string code, CompilationUnitOrigin origin = CompilationUnitOrigin.SourceFile)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));
        if (code == null)
            throw new ArgumentNullException(nameof(code));

        _context.AddSource(name, code, origin);
    }

    /// <summary>
    /// Adds a source file.
    /// </summary>
    public void AddSourceFile(string path, CompilationUnitOrigin origin = CompilationUnitOrigin.SourceFile)
    {
        if (path == null)
            throw new ArgumentNullException(nameof(path));

        _context.AddSourceFile(path, origin);
    }

    /// <summary>
    /// Adds a programmatically built compilation unit.
    /// </summary>
    public void AddCompilationUnit(SyntaxCompilationUnit unit)
    {
        if (unit == null)
            throw new ArgumentNullException(nameof(unit));

        _context.AddCompilationUnit(unit);
    }

    /// <summary>
    /// Analyzes all added sources and libraries.
    /// </summary>
    public void Analyze()
    {
        _context.Analyze();
    }

    /// <summary>
    /// Compiles the loaded sources. Returns <c>true</c> on success.
    /// </summary>
    public bool Compile()
    {
        ThrowIfDisposed();
        _domain?.Dispose();
        _domain = _context.Compile();
        return _domain != null;
    }

    /// <summary>
    /// Compiles and immediately runs the entry point. Returns <c>true</c> on success.
    /// </summary>
    public bool CompileAndRun()
    {
        ThrowIfDisposed();
        _domain?.Dispose();
        _domain = _context.CompileAndRun();
        return _domain != null;
    }

    /// <summary>
    /// Runs the already compiled entry point.
    /// </summary>
    public void Run()
    {
        ThrowIfDisposed();
        if (_domain == null)
            throw new InvalidOperationException("Engine has not been compiled yet. Call Compile() first.");

        _domain.Run();
    }

    /// <summary>
    /// Finds a type by its full name in the compiled symbol table.
    /// </summary>
    public TypeSymbol? FindType(string name)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return _context.FindType(name);
    }

    /// <summary>
    /// Finds a method by type name, method name and parameter count.
    /// </summary>
    public MethodSymbol? FindMethod(string typeName, string methodName, int parameterCount)
    {
        if (typeName == null)
            throw new ArgumentNullException(nameof(typeName));
        if (methodName == null)
            throw new ArgumentNullException(nameof(methodName));

        return FindType(typeName)?.FindMethod(methodName, parameterCount);
    }

    /// <summary>
    /// Invokes a compiled method.
    /// </summary>
    public ObjectInstance Invoke(MethodSymbol method, params ObjectInstance[] args)
    {
        ThrowIfDisposed();
        if (method == null)
            throw new ArgumentNullException(nameof(method));
        if (_domain == null)
            throw new InvalidOperationException("Engine has not been compiled yet. Call Compile() first.");

        return _domain.VirtualMachine.InvokeMethod(method, args);
    }

    /// <summary>
    /// Invokes a method looked up by type and method name.
    /// </summary>
    public ObjectInstance Invoke(string typeName, string methodName, int parameterCount, params ObjectInstance[] args)
    {
        MethodSymbol? method = FindMethod(typeName, methodName, parameterCount)
            ?? throw new InvalidOperationException($"Method '{typeName}.{methodName}({parameterCount} params)' not found.");

        return Invoke(method, args);
    }

    /// <summary>
    /// Gets the garbage collector associated with the compiled domain.
    /// </summary>
    public GarbageCollector GarbageCollector
    {
        get
        {
            if (_domain == null)
                throw new InvalidOperationException("Engine has not been compiled yet. Call Compile() first.");

            return _domain.GarbageCollector;
        }
    }

    /// <summary>
    /// Gets the virtual machine associated with the compiled domain.
    /// </summary>
    public VirtualMachine VirtualMachine
    {
        get
        {
            if (_domain == null)
                throw new InvalidOperationException("Engine has not been compiled yet. Call Compile() first.");

            return _domain.VirtualMachine;
        }
    }

    /// <summary>
    /// Enumerates source-file compilation units.
    /// </summary>
    public IEnumerable<CompilationUnitSyntax> GetSourceUnits()
    {
        return _context.GetCompilationUnits().Where(u => u.Origin == CompilationUnitOrigin.SourceFile);
    }

    public void Dispose()
    {
        if (_disposed)
            return;

        _domain?.Dispose();
        _context.Dispose();
        _disposed = true;
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(ScriptEngine));
    }
}
