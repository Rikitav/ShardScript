using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Syntax;
using ShardScript.Syntax.Builders;
using ShardScript.Syntax.Symbols;
using System.Reflection;

namespace ShardScript.Scripting;

/// <summary>
/// Reflects over C# types and objects and binds them as ShardScript symbols.
/// </summary>
public sealed class ShardReflectionBinder
{
    private readonly CompilationContext _context;

    public ShardReflectionBinder(CompilationContext context)
    {
        _context = context ?? throw new ArgumentNullException(nameof(context));
    }

    /// <summary>
    /// Binds an anonymous object's properties as ShardScript globals.
    /// For simple primitives a single <c>Globals</c> class is emitted without field
    /// initializers (the compiler's emitter cannot generate bytecode for field
    /// initializers). The host sets the values after compilation via
    /// <see cref="GarbageCollector.SetStaticField"/>.
    /// </summary>
    /// <returns>
    /// The source code for the generated globals class, or <c>null</c> if no primitive
    /// globals were emitted.
    /// </returns>
    public string? BindGlobals(object globals, string? ns = null)
    {
        if (globals == null)
            return null;

        Type type = globals.GetType();
        PropertyInfo[] properties = type.GetProperties(BindingFlags.Public | BindingFlags.Instance);

        List<string> primitiveGlobals = new();
        foreach (PropertyInfo property in properties)
        {
            object? value = property.GetValue(globals);
            if (value == null)
                continue;

            Type valueType = value.GetType();
            if (IsPrimitiveShardType(valueType))
            {
                // Static string fields are not supported by the current runtime layout
                // generator, so we skip them. Other primitives are emitted without
                // initializers and populated by the host after compilation.
                if (valueType == typeof(string))
                    continue;

                primitiveGlobals.Add($"public static {property.Name}: {GetShardTypeName(valueType)};");
            }
            else
            {
                BindType(valueType, ns);
            }
        }

        if (primitiveGlobals.Count == 0)
            return null;

        string nsName = ns ?? "__globals";
        return $"namespace {nsName}; public static class Globals {{ {string.Join(" ", primitiveGlobals)} }}";
    }

    /// <summary>
    /// Binds a single C# type (marked with <see cref="ShardScriptUserDataAttribute"/> or not)
    /// into the given namespace.
    /// </summary>
    public void BindType(Type type, string? ns = null)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        ShardScriptUserDataAttribute? attr = type.GetCustomAttribute<ShardScriptUserDataAttribute>();
        string namespaceName = ns ?? attr?.Namespace ?? type.Namespace ?? "csharp";
        string typeName = attr?.TypeName ?? type.Name;

        NamespaceSymbol namespaceSymbol = SymbolBuilder.Namespace(_context, namespaceName).Build();
        TypeSymbol typeSymbol = SymbolBuilder.Class(_context, typeName, namespaceSymbol).Build();

        foreach (MethodInfo method in type.GetMethods(BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly))
        {
            if (method.IsSpecialName)
                continue;

            BindMethod(typeSymbol, method, isStatic: true);
        }
    }

    /// <summary>
    /// Binds types configured by <paramref name="config"/> under the given namespace path.
    /// </summary>
    public void RegisterFunctions(string path, Action<ShardReflectionBinderOptions> config)
    {
        if (path == null)
            throw new ArgumentNullException(nameof(path));

        if (config == null)
            throw new ArgumentNullException(nameof(config));

        ShardReflectionBinderOptions options = new ShardReflectionBinderOptions(path);
        config(options);

        foreach (Type type in options.Types)
            BindType(type, path);

        foreach (Assembly assembly in options.Assemblies)
        {
            foreach (Type type in assembly.GetTypes())
            {
                if (type.GetCustomAttribute<ShardScriptUserDataAttribute>() != null)
                    BindType(type, path);
            }
        }
    }

    private void BindMethod(TypeSymbol parentType, MethodInfo method, bool isStatic)
    {
        TypeSymbol returnType = ResolveTypeSymbol(method.ReturnType);
        MethodBuilder builder = SymbolBuilder.Method(_context, parentType, method.Name, returnType);

        if (isStatic)
            builder.Static();

        builder.Public();

        ParameterInfo[] parameters = method.GetParameters();
        foreach (ParameterInfo parameter in parameters)
        {
            TypeSymbol parameterType = ResolveTypeSymbol(parameter.ParameterType);
            builder.Parameter(parameter.Name ?? "arg", parameterType);
        }

        ReflectionMethodCallback callback = new ReflectionMethodCallback(method, parameters);

        // The Callback extension wraps the managed delegate in a native delegate and keeps it
        // alive on the builder, so the GC will not collect it while the symbol exists.
        builder.Callback(callback.Invoke);
    }

    private TypeSymbol ResolveTypeSymbol(Type type)
    {
        Type underlying = Nullable.GetUnderlyingType(type) ?? type;
        PrimitiveType primitive = underlying switch
        {
            _ when underlying == typeof(void) => PrimitiveType.Void,
            _ when underlying == typeof(bool) => PrimitiveType.Boolean,
            _ when underlying == typeof(byte) || underlying == typeof(sbyte)
                || underlying == typeof(short) || underlying == typeof(ushort)
                || underlying == typeof(int) || underlying == typeof(uint)
                || underlying == typeof(long) || underlying == typeof(ulong) => PrimitiveType.Integer,
            _ when underlying == typeof(float) || underlying == typeof(double) => PrimitiveType.Double,
            _ when underlying == typeof(char) => PrimitiveType.Char,
            _ when underlying == typeof(string) => PrimitiveType.String,
            _ => throw new NotSupportedException($"Type '{type.Name}' cannot be mapped to a ShardScript primitive.")
        };

        return SymbolBuilder.Primitive(_context, primitive);
    }

    private static bool IsPrimitiveShardType(Type type)
    {
        return type == typeof(bool) ||
               type == typeof(byte) || type == typeof(sbyte) ||
               type == typeof(short) || type == typeof(ushort) ||
               type == typeof(int) || type == typeof(uint) ||
               type == typeof(long) || type == typeof(ulong) ||
               type == typeof(float) || type == typeof(double) ||
               type == typeof(char) || type == typeof(string);
    }

    private static string GetShardTypeName(Type type)
    {
        return type switch
        {
            _ when type == typeof(void) => "void",
            _ when type == typeof(bool) => "bool",
            _ when type == typeof(float) || type == typeof(double) => "double",
            _ when type == typeof(string) => "string",
            _ when type == typeof(char) => "char",
            _ => "int"
        };
    }

    private static string Literal(object value)
    {
        return value switch
        {
            bool b => b ? "true" : "false",
            string s => "\"" + s.Replace("\\", "\\\\").Replace("\"", "\\\"") + "\"",
            char c => "'" + c + "'",
            float f => f.ToString(System.Globalization.CultureInfo.InvariantCulture),
            double d => d.ToString(System.Globalization.CultureInfo.InvariantCulture),
            _ => Convert.ToInt64(value).ToString(System.Globalization.CultureInfo.InvariantCulture)
        };
    }

    private sealed class ReflectionMethodCallback
    {
        private readonly MethodInfo _method;
        private readonly ParameterInfo[] _parameters;

        public ReflectionMethodCallback(MethodInfo method, ParameterInfo[] parameters)
        {
            _method = method;
            _parameters = parameters;
        }

        public IntPtr Invoke(IntPtr methodPtr, IntPtr[] args, int argsCount, IntPtr userDataPtr, IntPtr collectorPtr)
        {
            if (argsCount != _parameters.Length)
                throw new InvalidOperationException($"Method '{_method.Name}' expects {_parameters.Length} arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collectorPtr);
            object?[] csharpArgs = new object?[_parameters.Length];

            for (int i = 0; i < _parameters.Length; i++)
            {
                ObjectInstance arg = new(args[i]);
                csharpArgs[i] = ShardMarshaller.FromObjectInstance(arg, _parameters[i].ParameterType);
            }

            object? result = _method.Invoke(null, csharpArgs);

            if (_method.ReturnType == typeof(void))
                return IntPtr.Zero;

            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        }
    }
}
