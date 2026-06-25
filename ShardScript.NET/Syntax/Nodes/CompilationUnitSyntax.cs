using System.Runtime.InteropServices;

namespace ShardScript.NET.Syntax.Nodes;

public enum CompilationUnitOrigin
{
    Unknown = 0,
    SourceFile = 1,
    DynamicLib = 2
}

public sealed class CompilationUnitSyntax
{
    public IntPtr Handle { get; }

    public CompilationUnitOrigin Origin => (CompilationUnitOrigin)NativeMethods.Shard_GetCompilationUnitOrigin(Handle);

    public NamespaceDeclarationSyntax? Namespace
    {
        get
        {
            var ns = NativeMethods.Shard_GetUnitNamespace(Handle);
            return ns == IntPtr.Zero ? null : new NamespaceDeclarationSyntax(ns);
        }
    }

    public CompilationUnitSyntax(IntPtr handle)
    {
        Handle = handle;
    }

    public IReadOnlyList<ClassDeclarationSyntax> GetClasses()
    {
        int count = NativeMethods.Shard_GetUnitClassCount(Handle);
        ClassDeclarationSyntax[] classes = new ClassDeclarationSyntax[count];
        for (int i = 0; i < count; i++)
            classes[i] = new ClassDeclarationSyntax(NativeMethods.Shard_GetUnitClass(Handle, i));

        return classes;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetCompilationUnitOrigin(IntPtr unit);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetUnitNamespace(IntPtr unit);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetUnitClassCount(IntPtr unit);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetUnitClass(IntPtr unit, int index);
    }
}
