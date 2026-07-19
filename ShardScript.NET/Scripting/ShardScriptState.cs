using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Syntax;
using ShardScript.Syntax.Nodes;
using ShardScript.Syntax.Symbols;
using System.Reflection;

namespace ShardScript.Scripting;

/// <summary>
/// Represents a compiled ShardScript session. Supports incremental (REPL-style)
/// submissions via <see cref="ContinueWith"/> and provides access to globals and methods.
/// </summary>
public sealed class ShardScriptState : IDisposable
{
    private readonly ShardScriptOptions _options;
    private readonly CompilationContext _context;
    private readonly ShardReflectionBinder _binder;
    private readonly List<string> _submissions = [];

    private readonly object? _globals;
    private readonly string? _globalsNamespace;
    private readonly string? _globalsSource;

    private ApplicationDomain? _domain;
    private int _submissionsAddedCount;
    private bool _disposed;

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
    /// Provides read/write access to public static fields by their full name (e.g. "Program.Counter").
    /// </summary>
    public object? this[string fullName]
    {
        get
        {
            ThrowIfDisposed();
            if (fullName == null)
                throw new ArgumentNullException(nameof(fullName));

            if (_domain == null)
                throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

            FieldSymbol field = ResolveField(fullName, out _);
            ObjectInstance value = _domain.GarbageCollector.GetStaticField(field);
            return ShardMarshaller.FromObjectInstance(value, ResolveClrType(field));
        }

        set
        {
            ThrowIfDisposed();
            if (fullName == null)
                throw new ArgumentNullException(nameof(fullName));

            if (_domain == null)
                throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

            FieldSymbol field = ResolveField(fullName, out _);
            ObjectInstance marshalled = ShardMarshaller.ToObjectInstance(value, _domain.GarbageCollector);
            _domain.GarbageCollector.SetStaticField(field, marshalled);
        }
    }

    public ShardScriptState(ShardScriptOptions options, object? globals = null)
    {
        _options = options ?? throw new ArgumentNullException(nameof(options));
        _context = new CompilationContext();
        _binder = new ShardReflectionBinder(_context);

        _context.EntryPointEnabled = options.EntryPointEnabled;

        ApplyOptions();

        if (globals != null)
        {
            _globals = globals;
            _globalsNamespace = options.DefaultNamespace ?? "globals_ns";
            _globalsSource = _binder.BindGlobals(globals, _globalsNamespace);
            if (!string.IsNullOrEmpty(_globalsSource))
                _context.AddSource("globals.ss", _globalsSource);
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
    /// Compiles all currently added sources and syntax trees.
    /// </summary>
    public bool TryCompile()
    {
        try
        {
            ThrowIfDisposed();
            _context.ResetDiagnostics();

            for (int i = _submissionsAddedCount; i < _submissions.Count; i++)
                _context.AddSource($"submission_{i}.ss", _submissions[i]);

            _submissionsAddedCount = _submissions.Count;

            _domain?.Dispose();
            _domain = _context.Compile();
            InitializeGlobals();
            return true;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Compiles all currently added sources and syntax trees.
    /// </summary>
    public ShardScriptState Compile()
    {
        ThrowIfDisposed();
        _context.ResetDiagnostics();

        for (int i = _submissionsAddedCount; i < _submissions.Count; i++)
            _context.AddSource($"submission_{i}.ss", _submissions[i]);

        _submissionsAddedCount = _submissions.Count;

        _domain?.Dispose();
        _domain = _context.Compile();
        InitializeGlobals();

        return this;
    }

    /// <summary>
    /// Adds the provided code as a submission and compiles the accumulated script.
    /// </summary>
    public ShardScriptState Compile(string code)
    {
        ThrowIfDisposed();
        if (code == null)
            throw new ArgumentNullException(nameof(code));

        _submissions.Add(code);
        return Compile();
    }

    /// <summary>
    /// Runs the compiled entry point.
    /// </summary>
    public ShardScriptState Run()
    {
        ThrowIfDisposed();
        if (_domain == null)
            throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

        if (_domain.EntryPointMethod != null)
            _domain.Run();

        return this;
    }

    /// <summary>
    /// Adds another submission and recompiles the accumulated script.
    /// </summary>
    public ShardScriptState ContinueWith(string code)
    {
        ThrowIfDisposed();
        Compile(code);
        Run();
        return this;
    }

    /// <summary>
    /// Enumerates source-file compilation units.
    /// </summary>
    public IEnumerable<CompilationUnitSyntax> GetSourceUnits()
    {
        return _context.GetCompilationUnits().Where(u => u.Origin == CompilationUnitOrigin.SourceFile);
    }

    /// <summary>
    /// Invokes a compiled method by its symbol.
    /// </summary>
    public ObjectInstance Call(MethodSymbol method, params ObjectInstance[] nativeArgs)
    {
        ThrowIfDisposed();
        if (_domain == null)
            throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

        ObjectInstance result = _domain.VirtualMachine.InvokeMethod(method, nativeArgs);
        return result;
    }

    /*
    /// <summary>
    /// Invokes a compiled method by its symbol.
    /// </summary>
    public void Call(MethodSymbol method, params object[] args)
    {
        ThrowIfDisposed();
        if (_domain == null)
            throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

        ObjectInstance[] marshalledArgs = new ObjectInstance[args.Length];
        for (int i = 0; i < args.Length; i++)
            marshalledArgs[i] = ShardMarshaller.ToObjectInstance(args[i], _domain.GarbageCollector);

        _domain.VirtualMachine.InvokeMethod(method, marshalledArgs);
    }
    */

    /// <summary>
    /// Invokes a compiled method by its symbol.
    /// </summary>
    public T Call<T>(MethodSymbol method, params object[] args)
    {
        ThrowIfDisposed();
        if (_domain == null)
            throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

        ObjectInstance[] marshalledArgs = new ObjectInstance[args.Length];
        for (int i = 0; i < args.Length; i++)
            marshalledArgs[i] = ShardMarshaller.ToObjectInstance(args[i], _domain.GarbageCollector);

        ObjectInstance result = _domain.VirtualMachine.InvokeMethod(method, marshalledArgs);
        return ShardMarshaller.FromObjectInstance<T>(result);
    }

    /// <summary>
    /// Invokes a compiled method by its full name (e.g. "Program.Add").
    /// </summary>
    public T Call<T>(string fullName, params object[] args)
    {
        ThrowIfDisposed();
        if (fullName == null)
            throw new ArgumentNullException(nameof(fullName));

        if (_domain == null)
            throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

        MethodSymbol method = ResolveMethod(fullName, args.Length, out _);

        ObjectInstance[] marshalledArgs = new ObjectInstance[args.Length];
        for (int i = 0; i < args.Length; i++)
            marshalledArgs[i] = ShardMarshaller.ToObjectInstance(args[i], _domain.GarbageCollector);

        ObjectInstance result = _domain.VirtualMachine.InvokeMethod(method, marshalledArgs);
        return ShardMarshaller.FromObjectInstance<T>(result);
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
    /// Binds C# functions/types configured by <paramref name="config"/> into the given namespace.
    /// </summary>
    public ShardScriptState RegisterFunctions(string path, Action<ShardReflectionBinderOptions> config)
    {
        ThrowIfDisposed();
        _binder.RegisterFunctions(path, config);
        return this;
    }

    /// <summary>
    /// Builds a syntax tree using the scoped fluent AST builder and adds it to the compilation.
    /// </summary>
    public ShardScriptState BuildAst(Action<Syntax.Ast.CompilationUnitSyntaxBuilder> configure)
    {
        ThrowIfDisposed();
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxCompilationUnit unit = Syntax.Ast.SyntaxBuilder.Unit(_context, configure);
        _context.AddCompilationUnit(unit);
        return this;
    }

    public void Dispose()
    {
        if (_disposed)
            return;

        _domain?.Dispose();
        _context.Dispose();
        _disposed = true;
    }

    private void ApplyOptions()
    {
        if (_options.LoadStandardLibraries)
            LoadStandardLibraries();

        foreach (string libraryPath in _options.LibraryPaths)
            _context.AddLibrary(libraryPath);

        foreach (string sourcePath in _options.SourcePaths)
            _context.AddSourceFile(sourcePath);
    }

    private void LoadStandardLibraries()
    {
        string[] frameworkDirs =
        {
            Path.Combine(AppContext.BaseDirectory, "system"),
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "out", "build", "x64-debug", "bin", "system"))
        };

        List<string> libraryPaths = new();
        foreach (string frameworkDir in frameworkDirs)
        {
            if (!Directory.Exists(frameworkDir))
                continue;

            foreach (string libraryPath in Directory.EnumerateFiles(frameworkDir, "*.dll"))
                libraryPaths.Add(libraryPath);
        }

        _context.AddLibraries(libraryPaths);
    }

    private void InitializeGlobals()
    {
        if (_globals == null || _domain == null)
            return;

        TypeSymbol? globalsType = _context.FindType("Globals");
        if (globalsType == null)
            return;

        GarbageCollector gc = _domain.GarbageCollector;
        foreach (PropertyInfo property in _globals.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance))
        {
            object? value = property.GetValue(_globals);
            if (value == null)
                continue;

            FieldSymbol? field = globalsType.FindField(property.Name);
            if (field == null)
                continue;

            ObjectInstance marshalled = ShardMarshaller.ToObjectInstance(value, gc);
            gc.SetStaticField(field, marshalled);
        }
    }

    private MethodSymbol ResolveMethod(string fullName, int parameterCount, out TypeSymbol? type)
    {
        (string typeName, string memberName) = SplitFullName(fullName);

        type = _context.FindType(typeName);
        if (type == null)
            throw new InvalidOperationException($"Type '{typeName}' was not found in the compiled script.");

        MethodSymbol? method = type.FindMethod(memberName, parameterCount);
        if (method == null)
            throw new InvalidOperationException($"Method '{memberName}' with {parameterCount} parameter(s) was not found on type '{typeName}'.");

        return method;
    }

    private FieldSymbol ResolveField(string fullName, out TypeSymbol? type)
    {
        (string typeName, string memberName) = SplitFullName(fullName);

        type = _context.FindType(typeName);
        if (type == null)
            throw new InvalidOperationException($"Type '{typeName}' was not found in the compiled script.");

        FieldSymbol? field = type.FindField(memberName);
        if (field == null)
            throw new InvalidOperationException($"Field '{memberName}' was not found on type '{typeName}'.");

        return field;
    }

    private static (string TypeName, string MemberName) SplitFullName(string fullName)
    {
        int lastDot = fullName.LastIndexOf('.');
        if (lastDot < 0)
            return (fullName, "Main");

        string typeName = fullName.Substring(0, lastDot);
        string memberName = fullName.Substring(lastDot + 1);
        return (typeName, memberName);
    }

    private static Type ResolveClrType(FieldSymbol field)
    {
        string typeName = field.FieldType.Name;
        return typeName.ToLowerInvariant() switch
        {
            "integer" or "int" => typeof(long),
            "double" => typeof(double),
            "boolean" or "bool" => typeof(bool),
            "string" => typeof(string),
            "char" => typeof(char),
            _ => typeof(object)
        };
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(ShardScriptState));
    }
}
