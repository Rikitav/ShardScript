using System.Runtime.InteropServices;

namespace ShardScript.NET;

public enum SymbolAccessibility
{
    Private = 0,
    Public = 1,
    Protected = 2
}

public enum PrimitiveType
{
    Void = 0,
    Null = 1,
    Any = 2,
    Boolean = 3,
    Integer = 4,
    Double = 5,
    Char = 6,
    String = 7,
    Array = 8
}

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate IntPtr ShardManagedMethodCallback(
    IntPtr method,
    IntPtr[] args,
    int argsCount,
    IntPtr userData,
    IntPtr collector);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate IntPtr ShardManagedMethodCallbackNative(
    IntPtr method,
    IntPtr args,
    int argsCount,
    IntPtr userData,
    IntPtr collector);

public abstract class Symbol
{
    public IntPtr Handle { get; }

    internal Symbol(IntPtr handle)
    {
        Handle = handle;
    }

    internal static IntPtr CreateHandle(Func<IntPtr> factory)
    {
        IntPtr handle = factory();
        if (handle == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to create symbol: {ShardException.GetLastError()}");

        return handle;
    }

    internal static void ThrowIfError(int result)
    {
        if (result != 0)
            throw new InvalidOperationException($"Failed to modify symbol: {ShardException.GetLastError()}");
    }
}

public sealed class SymbolNamespace : Symbol
{
    internal SymbolNamespace(IntPtr handle) : base(handle) { }
}

public sealed class SymbolType : Symbol
{
    internal SymbolType(IntPtr handle) : base(handle) { }
}

public sealed class SymbolMethod : Symbol
{
    internal SymbolMethod(IntPtr handle) : base(handle) { }
}

public sealed class SymbolParameter : Symbol
{
    internal SymbolParameter(IntPtr handle) : base(handle) { }
}

public sealed class SymbolField : Symbol
{
    internal SymbolField(IntPtr handle) : base(handle) { }
}

public static class SymbolFactory
{
    private static readonly Dictionary<IntPtr, ShardManagedMethodCallbackNative> Callbacks = new();

    public static SymbolType GetPrimitiveType(CompilationContext context, PrimitiveType primitive)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        return new SymbolType(Symbol.CreateHandle(() => Native.Shard_GetPrimitiveType(context.Handle, (int)primitive)));
    }

    public static SymbolNamespace CreateNamespace(CompilationContext context, string name, SymbolNamespace? parent = null)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SymbolNamespace(Symbol.CreateHandle(() => Native.Shard_CreateNamespaceSymbol(context.Handle, parent?.Handle ?? IntPtr.Zero, name)));
    }

    public static SymbolType CreateClass(CompilationContext context, string name, SymbolNamespace? parent = null)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        return new SymbolType(Symbol.CreateHandle(() => Native.Shard_CreateClassSymbol(context.Handle, parent?.Handle ?? IntPtr.Zero, name)));
    }

    public static SymbolMethod CreateMethod(
        CompilationContext context,
        SymbolType parentType,
        string name,
        SymbolType returnType,
        bool isStatic = true,
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

        return new SymbolMethod(Symbol.CreateHandle(() =>
            Native.Shard_CreateMethodSymbol(context.Handle, parentType.Handle, name, returnType.Handle, isStatic ? 1 : 0, (int)accessibility)));
    }

    public static SymbolParameter CreateParameter(CompilationContext context, string name, SymbolType type)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (type == null)
            throw new ArgumentNullException(nameof(type));

        return new SymbolParameter(Symbol.CreateHandle(() => Native.Shard_CreateParameterSymbol(context.Handle, name, type.Handle)));
    }

    public static void AddParameter(SymbolMethod method, SymbolParameter parameter)
    {
        if (method == null)
            throw new ArgumentNullException(nameof(method));

        if (parameter == null)
            throw new ArgumentNullException(nameof(parameter));

        Symbol.ThrowIfError(Native.Shard_AddMethodParameter(method.Handle, parameter.Handle));
    }

    public static SymbolField CreateField(
        CompilationContext context,
        SymbolType parentType,
        string name,
        SymbolType type,
        bool isStatic = false)
    {
        if (context == null)
            throw new ArgumentNullException(nameof(context));

        if (parentType == null)
            throw new ArgumentNullException(nameof(parentType));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (type == null)
            throw new ArgumentNullException(nameof(type));

        return new SymbolField(Symbol.CreateHandle(() =>
            Native.Shard_CreateFieldSymbol(context.Handle, parentType.Handle, name, type.Handle, isStatic ? 1 : 0)));
    }

    public static void SetCallback(SymbolMethod method, ShardManagedMethodCallback callback, IntPtr userData = default)
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
        Symbol.ThrowIfError(Native.Shard_SetMethodManagedCallback(method.Handle, nativeCallback, userData));
    }
}

public static class SymbolBuilder
{
    public static SymbolMethod CallbackMethod(
        CompilationContext context,
        string typeName,
        string methodName,
        ShardManagedMethodCallback callback,
        SymbolType returnType,
        IEnumerable<(string Name, SymbolType Type)> parameters,
        bool isStatic = true,
        SymbolAccessibility accessibility = SymbolAccessibility.Public,
        SymbolNamespace? parentNamespace = null)
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

        SymbolType type = SymbolFactory.CreateClass(context, typeName, parentNamespace);
        SymbolMethod method = SymbolFactory.CreateMethod(context, type, methodName, returnType, isStatic, accessibility);

        foreach ((string name, SymbolType type) parameter in parameters)
        {
            SymbolParameter param = SymbolFactory.CreateParameter(context, parameter.name, parameter.type);
            SymbolFactory.AddParameter(method, param);
        }

        SymbolFactory.SetCallback(method, callback);
        return method;
    }
}
