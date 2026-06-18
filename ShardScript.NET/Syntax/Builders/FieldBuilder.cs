using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Builders;

public sealed class FieldBuilder : ISymbolBuilder<FieldSymbol>, IAccessible<FieldBuilder>, ILinkable<FieldBuilder>
{
    private readonly CompilationContext _context;
    private readonly TypeSymbol _parentType;
    private readonly string _name;
    private readonly TypeSymbol _type;
    private SymbolAccessibility _accessibility = SymbolAccessibility.Public;
    private SymbolLinking _linking = SymbolLinking.Static;

    private FieldSymbol? _symbol;

    public IntPtr Handle => Symbol.Handle;
    public FieldSymbol Symbol => _symbol ?? throw new InvalidOperationException("Field symbol has not been built yet. Call Build() first.");

    public FieldBuilder(CompilationContext context, TypeSymbol parentType, string name, TypeSymbol type)
    {
        _context = context ?? throw new ArgumentNullException(nameof(context));
        _parentType = parentType ?? throw new ArgumentNullException(nameof(parentType));
        _name = name ?? throw new ArgumentNullException(nameof(name));
        _type = type ?? throw new ArgumentNullException(nameof(type));
    }

    public FieldSymbol Build()
    {
        if (_symbol != null)
            return _symbol;

        IntPtr handle = NativeMethods.Shard_CreateFieldSymbol(
            _context.Handle,
            _parentType.Handle,
            _name,
            _type.Handle,
            _linking == SymbolLinking.Static ? 1 : 0);

        if (handle == IntPtr.Zero)
            throw new InvalidOperationException("Failed to create field symbol.");
        ShardEngineException.ThrowIfError(NativeMethods.Shard_SetSymbolAccesibility(handle, (int)_accessibility));

        _symbol = new FieldSymbol(handle);
        return _symbol;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateFieldSymbol(IntPtr ctx, IntPtr parentType, [MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr type, int isStatic);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetSymbolAccesibility(IntPtr symbol, int accessibility);
    }
}
