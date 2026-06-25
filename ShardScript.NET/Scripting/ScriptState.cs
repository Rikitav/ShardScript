using ShardScript.NET.Application;
using ShardScript.NET.Runtime;
using ShardScript.NET.Syntax.Symbols;
using System.Reflection;

namespace ShardScript.NET.Scripting;

/// <summary>
/// Represents a compiled ShardScript session. Supports incremental (REPL-style)
/// submissions via <see cref="ContinueWith"/> and provides access to globals and methods.
/// </summary>
public sealed class ScriptState : IDisposable
{
    private readonly ScriptOptions _options;
    private readonly CompilationContext _context;
    private readonly ReflectionBinder _binder;
    private readonly List<string> _submissions = [];
    private int _submissionsAddedCount;
    private object? _globals;
    private string? _globalsNamespace;
    private string? _globalsSource;
    private ApplicationDomain? _domain;
    private bool _disposed;

    public IntPtr ContextHandle => _context.Handle;
    public IntPtr DomainHandle => _domain?.Handle ?? IntPtr.Zero;

    public bool HasErrors => _context.HasErrors;
    public string Diagnostics => _context.GetDiagnostics();

    internal CompilationContext Context => _context;
    internal ApplicationDomain? Domain => _domain;

    public ScriptState(ScriptOptions options, object? globals = null)
    {
        _options = options ?? throw new ArgumentNullException(nameof(options));
        _context = new CompilationContext();
        _context.EntryPointEnabled = options.EntryPointEnabled;
        _binder = new ReflectionBinder(_context);

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
    /// Compiles all currently added sources and syntax trees.
    /// </summary>
    public ScriptState Compile()
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
    public ScriptState Compile(string code)
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
    public ScriptState Run()
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
    public ScriptState ContinueWith(string code)
    {
        ThrowIfDisposed();
        Compile(code);
        Run();
        return this;
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

        (TypeSymbol type, MethodSymbol method) = ResolveMethod(fullName, args.Length);

        ObjectInstance[] marshalledArgs = new ObjectInstance[args.Length];
        for (int i = 0; i < args.Length; i++)
            marshalledArgs[i] = Marshaller.ToObjectInstance(args[i], _domain.GarbageCollector);

        ObjectInstance result = _domain.VirtualMachine.InvokeMethod(method, marshalledArgs);
        return Marshaller.FromObjectInstance<T>(result);
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

            (TypeSymbol type, FieldSymbol field) = ResolveField(fullName);
            ObjectInstance value = _domain.GarbageCollector.GetStaticField(field);
            return Marshaller.FromObjectInstance(value, ResolveClrType(field));
        }
        set
        {
            ThrowIfDisposed();
            if (fullName == null)
                throw new ArgumentNullException(nameof(fullName));
            if (_domain == null)
                throw new InvalidOperationException("State has not been compiled yet. Call Compile first.");

            (TypeSymbol type, FieldSymbol field) = ResolveField(fullName);
            ObjectInstance marshalled = Marshaller.ToObjectInstance(value, _domain.GarbageCollector);
            _domain.GarbageCollector.SetStaticField(field, marshalled);
        }
    }

    /// <summary>
    /// Binds C# functions/types configured by <paramref name="config"/> into the given namespace.
    /// </summary>
    public ScriptState RegisterFunctions(string path, Action<BindingOptions> config)
    {
        ThrowIfDisposed();
        _binder.RegisterFunctions(path, config);
        return this;
    }

    /// <summary>
    /// Builds a syntax tree using the scoped fluent AST builder and adds it to the compilation.
    /// </summary>
    public ScriptState BuildAst(Action<Syntax.Ast.CompilationUnitBuilder> configure)
    {
        ThrowIfDisposed();
        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        SyntaxCompilationUnit unit = Syntax.Ast.AstBuilder.Unit(_context, configure);
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
            Path.Combine(AppContext.BaseDirectory, "framework"),
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "out", "build", "x64-debug", "bin", "framework"))
        };

        foreach (string frameworkDir in frameworkDirs)
        {
            if (!Directory.Exists(frameworkDir))
                continue;

            foreach (string libraryPath in Directory.EnumerateFiles(frameworkDir, "*.dll"))
            {
                try
                {
                    _context.AddLibrary(libraryPath);
                }
                catch
                {
                    // Standard libraries are best-effort.
                }
            }
        }
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

            ObjectInstance marshalled = Marshaller.ToObjectInstance(value, gc);
            gc.SetStaticField(field, marshalled);
        }
    }

    private (TypeSymbol Type, MethodSymbol Method) ResolveMethod(string fullName, int parameterCount)
    {
        (string typeName, string memberName) = SplitFullName(fullName);

        TypeSymbol? type = _context.FindType(typeName);
        if (type == null)
            throw new InvalidOperationException($"Type '{typeName}' was not found in the compiled script.");

        MethodSymbol? method = type.FindMethod(memberName, parameterCount);
        if (method == null)
            throw new InvalidOperationException($"Method '{memberName}' with {parameterCount} parameter(s) was not found on type '{typeName}'.");

        return (type, method);
    }

    private (TypeSymbol Type, FieldSymbol Field) ResolveField(string fullName)
    {
        (string typeName, string memberName) = SplitFullName(fullName);

        TypeSymbol? type = _context.FindType(typeName);
        if (type == null)
            throw new InvalidOperationException($"Type '{typeName}' was not found in the compiled script.");

        FieldSymbol? field = type.FindField(memberName);
        if (field == null)
            throw new InvalidOperationException($"Field '{memberName}' was not found on type '{typeName}'.");

        return (type, field);
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
            throw new ObjectDisposedException(nameof(ScriptState));
    }
}
