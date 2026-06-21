using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Builders;

public sealed class MethodBuilder : ISymbolBuilder<MethodSymbol>,
    IAccessible, ILinkable, IParameterizable, ICallback
{
    public CompilationContext Context { get; }
    public MethodSymbol Symbol { get; }
    public IntPtr Handle => Symbol.Handle;

    public MethodBuilder(CompilationContext context, TypeSymbol parentType, string name, TypeSymbol returnType)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));

        IntPtr handle = NativeMethods.Shard_CreateMethodSymbol(
            Context.Handle, parentType.Handle, name, returnType.Handle,
            (int)SymbolLinking.Static, (int)SymbolAccessibility.Public);

        if (handle == IntPtr.Zero)
            throw new InvalidOperationException("Failed to create method symbol.");

        Symbol = new MethodSymbol(handle);
    }

    public void KeepCallbackAlive(ShardManagedMethodCallbackNative callback)
    {
        Symbol.KeepCallbackAlive(callback);
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateMethodSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr returnType, int isStatic, int accessibility);
    }
}
