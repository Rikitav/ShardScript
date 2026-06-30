using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Syntax.Builders;
using ShardScript.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.Syntax;

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate IntPtr ShardManagedMethodCallback(
    IntPtr method,
    IntPtr[] args,
    int argsCount,
    IntPtr userData,
    IntPtr collector);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate IntPtr ShardManagedMethodCallbackNative(
    IntPtr method,
    IntPtr args,
    int argsCount,
    IntPtr userData,
    IntPtr collector);

internal static class SymbolFactory
{
    private static readonly Dictionary<IntPtr, ShardManagedMethodCallbackNative> Callbacks = new();

    public static TypeSymbol GetPrimitiveType(CompilationContext context, PrimitiveType primitive)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        IntPtr handle = NativeMethods.Shard_GetPrimitiveType(context.Handle, (int)primitive);
        return new TypeSymbol(handle);
    }

    public static MethodSymbol CreateMethod(
        CompilationContext context,
        TypeSymbol parentType,
        string name,
        TypeSymbol returnType,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (parentType == null)
            throw new ArgumentNullException(nameof(parentType));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (returnType == null)
            throw new ArgumentNullException(nameof(returnType));

        MethodBuilder method = new MethodBuilder(context, parentType, name, returnType);

        if (linking == SymbolLinking.Instance)
            method.Instance();
        if (accessibility == SymbolAccessibility.Private)
            method.Private();

        return method.Build();
    }

    public static ParameterSymbol CreateParameter(CompilationContext context, string name, TypeSymbol type)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (type == null)
            throw new ArgumentNullException(nameof(type));

        IntPtr handle = NativeMethods.Shard_CreateParameterSymbol(context.Handle, name, type.Handle);
        if (handle == IntPtr.Zero)
            throw new InvalidOperationException("Failed to create parameter symbol.");
        return new ParameterSymbol(handle);
    }

    public static void AddParameter(MethodSymbol method, ParameterSymbol parameter)
    {
        if (method == null)
            throw new ArgumentNullException(nameof(method));

        if (parameter == null)
            throw new ArgumentNullException(nameof(parameter));

        ShardEngineException.ThrowIfError(NativeMethods.Shard_AddMethodParameter(method.Handle, parameter.Handle));
    }

    public static FieldSymbol CreateField(
        CompilationContext context,
        TypeSymbol parentType,
        string name,
        TypeSymbol type,
        SymbolLinking linking = SymbolLinking.Static)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (parentType == null)
            throw new ArgumentNullException(nameof(parentType));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (type == null)
            throw new ArgumentNullException(nameof(type));

        FieldBuilder field = new FieldBuilder(context, parentType, name, type);

        if (linking == SymbolLinking.Instance)
            field.Instance();

        return field.Build();
    }

    public static void SetCallback(MethodSymbol method, ShardManagedMethodCallback callback, IntPtr userData = default)
    {
        if (method == null)
            throw new ArgumentNullException(nameof(method));

        if (callback == null)
            throw new ArgumentNullException(nameof(callback));

        ShardManagedMethodCallbackNative nativeCallback = (methodPtr, argsPtr, argsCount, userDataPtr, collectorPtr) =>
        {
            IntPtr[] args = new IntPtr[argsCount];
            for (int i = 0; i < argsCount; i++)
                args[i] = Marshal.ReadIntPtr(argsPtr, i * IntPtr.Size);

            return callback(methodPtr, args, argsCount, userDataPtr, collectorPtr);
        };

        Callbacks[method.Handle] = nativeCallback;
        method.KeepCallbackAlive(nativeCallback);
        ShardEngineException.ThrowIfError(NativeMethods.Shard_SetMethodManagedCallback(method.Handle, nativeCallback, userData));
    }

    public static void SetAccessibility(SyntaxSymbol symbol, SymbolAccessibility accessibility)
    {
        if (symbol == null)
            throw new ArgumentNullException(nameof(symbol));

        ShardEngineException.ThrowIfError(NativeMethods.Shard_SetSymbolAccesibility(symbol.Handle, (int)accessibility));
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetPrimitiveType(IntPtr ctx, int primitiveKind);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateParameterSymbol(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddMethodParameter(IntPtr method, IntPtr parameter);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetSymbolAccesibility(IntPtr symbol, int accessibility);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetMethodManagedCallback(IntPtr method, ShardManagedMethodCallbackNative callback, IntPtr userData);
    }
}

/// <summary>
/// Factory for building ShardScript symbols programmatically and binding managed callbacks to them.
/// </summary>
public static class SymbolBuilder
{
    /// <summary>
    /// Creates a namespace using the fluent lambda-scoped API.
    /// </summary>
    public static NamespaceBuilder CreateNamespace(CompilationContext context, string name, Action<NamespaceBuilder> configure)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (configure == null)
            throw new ArgumentNullException(nameof(configure));

        NamespaceBuilder builder = new NamespaceBuilder(context, name);
        configure(builder);
        return builder;
    }

    /// <summary>
    /// Creates a namespace using the classic chaining API.
    /// </summary>
    public static NamespaceBuilder Namespace(CompilationContext context, string name, NamespaceSymbol? parent = null)
    {
        return new NamespaceBuilder(context, name, parent);
    }

    public static ClassBuilder Class(CompilationContext context, string name, NamespaceSymbol? parentNamespace = null)
    {
        return new ClassBuilder(context, parentNamespace, name);
    }

    public static MethodBuilder Method(CompilationContext context, TypeSymbol parentType, string name, TypeSymbol returnType)
    {
        return new MethodBuilder(context, parentType, name, returnType);
    }

    public static TypeSymbol Primitive(CompilationContext context, PrimitiveType primitive)
    {
        return SymbolFactory.GetPrimitiveType(context, primitive);
    }

    public static MethodBuilder CallbackMethod(
        CompilationContext context,
        string typeName,
        string methodName,
        ShardManagedMethodCallback callback,
        TypeSymbol returnType,
        IEnumerable<(string Name, TypeSymbol Type)> parameters,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public,
        NamespaceSymbol? parentNamespace = null)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (typeName == null)
            throw new ArgumentNullException(nameof(typeName));

        if (methodName == null)
            throw new ArgumentNullException(nameof(methodName));

        if (callback == null)
            throw new ArgumentNullException(nameof(callback));

        if (returnType == null)
            throw new ArgumentNullException(nameof(returnType));

        if (parameters == null)
            throw new ArgumentNullException(nameof(parameters));

        ClassBuilder type = Class(context, typeName, parentNamespace);
        MethodBuilder method = (linking, accessibility) switch
        {
            (SymbolLinking.Static, SymbolAccessibility.Public) => Method(context, type.Symbol, methodName, returnType).Static().Public().Callback(callback),
            (SymbolLinking.Static, SymbolAccessibility.Private) => Method(context, type.Symbol, methodName, returnType).Static().Private().Callback(callback),
            (SymbolLinking.Instance, SymbolAccessibility.Public) => Method(context, type.Symbol, methodName, returnType).Instance().Public().Callback(callback),
            (SymbolLinking.Instance, SymbolAccessibility.Private) => Method(context, type.Symbol, methodName, returnType).Instance().Private().Callback(callback),
            _ => Method(context, type.Symbol, methodName, returnType).Static().Public().Callback(callback)
        };

        foreach ((string name, TypeSymbol type) parameter in parameters)
            method.Parameter(parameter.name, parameter.type);

        return method;
    }
}
