using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Builders;

public sealed class FieldBuilder : ISymbolBuilder<FieldSymbol>, IAccessible, ILinkable
{
    public CompilationContext Context { get; }
    public FieldSymbol Symbol { get; }
    public IntPtr Handle => Symbol.Handle;

    public FieldBuilder(CompilationContext context, TypeSymbol parentType, string name, TypeSymbol returnType)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));

        IntPtr handle = NativeMethods.Shard_CreateFieldSymbol(
            Context.Handle, parentType.Handle, name, returnType.Handle,
            (int)SymbolLinking.Static, (int)SymbolAccessibility.Public);

        if (handle == IntPtr.Zero)
            throw new InvalidOperationException("Failed to create field symbol.");

        Symbol = new FieldSymbol(handle);
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateFieldSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type, int isStatic, int accesibilty);
    }
}
