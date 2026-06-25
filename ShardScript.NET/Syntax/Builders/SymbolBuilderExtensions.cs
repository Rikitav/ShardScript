using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Builders;

/// <summary>
/// Fluent extensions for configuring symbols created with <see cref="SymbolBuilder"/>.
/// </summary>
public static class SymbolBuilderExtensions
{
    public static TSymbol Build<TSymbol>(this ISymbolBuilder<TSymbol> builder)
        where TSymbol : SyntaxSymbol
    {
        return builder.Symbol;
    }

    public static T Public<T>(this T builder) where T : IAccessible
    {
        int result = NativeMethods.Shard_SetSymbolAccesibility(builder.Handle, (int)SymbolAccessibility.Public);
        ShardEngineException.ThrowIfError(result);
        return builder;
    }

    public static T Private<T>(this T builder) where T : IAccessible
    {
        int result = NativeMethods.Shard_SetSymbolAccesibility(builder.Handle, (int)SymbolAccessibility.Private);
        ShardEngineException.ThrowIfError(result);
        return builder;
    }

    public static T Static<T>(this T builder) where T : ILinkable
    {
        int result = NativeMethods.Shard_SetSymbolLinking(builder.Handle, (int)SymbolLinking.Static);
        ShardEngineException.ThrowIfError(result);
        return builder;
    }

    public static T Instance<T>(this T builder) where T : ILinkable
    {
        int result = NativeMethods.Shard_SetSymbolLinking(builder.Handle, (int)SymbolLinking.Instance);
        ShardEngineException.ThrowIfError(result);
        return builder;
    }

    public static T Parameter<T>(this T builder, string name, TypeSymbol type) where T : IParameterizable
    {
        if (name is null)
            throw new ArgumentNullException(nameof(name));

        if (type is null)
            throw new ArgumentNullException(nameof(type));

        IntPtr parameterHandle = NativeMethods.Shard_CreateParameterSymbol(builder.Context.Handle, name, type.Handle);
        if (parameterHandle == IntPtr.Zero)
            throw new InvalidOperationException("Failed to create parameter symbol.");

        ShardEngineException.ThrowIfError(NativeMethods.Shard_AddMethodParameter(builder.Handle, parameterHandle));
        return builder;
    }

    public static T Callback<T>(this T builder, ShardManagedMethodCallback callback, IntPtr userData = default) where T : ICallback
    {
        IntPtr nativeCallbackImpl(nint methodPtr, nint argsPtr, int argsCount, nint userDataPtr, nint collectorPtr)
        {
            IntPtr[] args = new IntPtr[argsCount];
            for (int i = 0; i < argsCount; i++)
                args[i] = Marshal.ReadIntPtr(argsPtr, i * IntPtr.Size);

            return callback.Invoke(methodPtr, args, argsCount, userDataPtr, collectorPtr);
        }

        ShardManagedMethodCallbackNative nativeCallback = nativeCallbackImpl;
        builder.KeepCallbackAlive(nativeCallback);

        int result = NativeMethods.Shard_SetMethodManagedCallback(builder.Handle, nativeCallback, userData);
        ShardEngineException.ThrowIfError(result);
        return builder;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetSymbolAccesibility(IntPtr symbol, int accessibility);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetSymbolLinking(IntPtr symbol, int accessibility);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetMethodManagedCallback(IntPtr method, ShardManagedMethodCallbackNative callback, IntPtr userData);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateParameterSymbol(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddMethodParameter(IntPtr method, IntPtr parameter);
    }
}
