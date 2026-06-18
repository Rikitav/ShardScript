using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Builders;

public sealed class MethodBuilder : ISymbolBuilder<MethodSymbol>,
    IAccessible<MethodBuilder>,
    ILinkable<MethodBuilder>,
    IParameterizable<MethodBuilder>,
    ICallback<MethodBuilder>
{
    private readonly CompilationContext _context;
    private readonly TypeSymbol _parentType;
    private readonly string _name;
    private readonly TypeSymbol _returnType;
    private SymbolAccessibility _accessibility = SymbolAccessibility.Public;
    private SymbolLinking _linking = SymbolLinking.Static;
    private readonly List<(string Name, TypeSymbol Type)> _parameters = new();
    private ShardManagedMethodCallback? _callback;
    private IntPtr _userData;

    private MethodSymbol? _symbol;

    public IntPtr Handle => Symbol.Handle;
    public MethodSymbol Symbol => _symbol ?? throw new InvalidOperationException("Method symbol has not been built yet. Call Build() first.");

    public MethodBuilder(CompilationContext context, TypeSymbol parentType, string name, TypeSymbol returnType)
    {
        _context = context ?? throw new ArgumentNullException(nameof(context));
        _parentType = parentType ?? throw new ArgumentNullException(nameof(parentType));
        _name = name ?? throw new ArgumentNullException(nameof(name));
        _returnType = returnType ?? throw new ArgumentNullException(nameof(returnType));
    }

    public MethodBuilder Public()
    {
        _accessibility = SymbolAccessibility.Public;
        return this;
    }

    public MethodBuilder Private()
    {
        _accessibility = SymbolAccessibility.Private;
        return this;
    }

    public MethodBuilder Static()
    {
        _linking = SymbolLinking.Static;
        return this;
    }

    public MethodBuilder Instance()
    {
        _linking = SymbolLinking.Instance;
        return this;
    }

    public MethodBuilder Parameter(string name, TypeSymbol type)
    {
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (type == null)
            throw new ArgumentNullException(nameof(type));

        _parameters.Add((name, type));
        return this;
    }

    public MethodBuilder Callback(ShardManagedMethodCallback callback, IntPtr userData = default)
    {
        _callback = callback ?? throw new ArgumentNullException(nameof(callback));
        _userData = userData;
        return this;
    }

    public MethodSymbol Build()
    {
        if (_symbol != null)
            return _symbol;

        IntPtr handle = NativeMethods.Shard_CreateMethodSymbol(
            _context.Handle,
            _parentType.Handle,
            _name,
            _returnType.Handle,
            _linking == SymbolLinking.Static ? 1 : 0,
            (int)_accessibility);

        if (handle == IntPtr.Zero)
            throw new InvalidOperationException("Failed to create method symbol.");
        _symbol = new MethodSymbol(handle);

        foreach ((string name, TypeSymbol type) in _parameters)
        {
            IntPtr parameterHandle = NativeMethods.Shard_CreateParameterSymbol(_context.Handle, name, type.Handle);
            if (parameterHandle == IntPtr.Zero)
                throw new InvalidOperationException("Failed to create parameter symbol.");
            ShardEngineException.ThrowIfError(NativeMethods.Shard_AddMethodParameter(handle, parameterHandle));
        }

        if (_callback != null)
        {
            ShardManagedMethodCallbackNative nativeCallback = (methodPtr, argsPtr, argsCount, userDataPtr, collectorPtr) =>
            {
                IntPtr[] args = new IntPtr[argsCount];
                for (int i = 0; i < argsCount; i++)
                    args[i] = Marshal.ReadIntPtr(argsPtr, i * IntPtr.Size);

                return _callback!(methodPtr, args, argsCount, userDataPtr, collectorPtr);
            };

            _symbol.KeepCallbackAlive(nativeCallback);
            ShardEngineException.ThrowIfError(NativeMethods.Shard_SetMethodManagedCallback(handle, nativeCallback, _userData));
        }

        return _symbol;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateMethodSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr returnType, int isStatic, int accessibility);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateParameterSymbol(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddMethodParameter(IntPtr method, IntPtr parameter);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetMethodManagedCallback(IntPtr method, ShardManagedMethodCallbackNative callback, IntPtr userData);
    }
}
