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

    public CompilationUnitOrigin Origin => (CompilationUnitOrigin)ShardScriptAPI.Shard_GetCompilationUnitOrigin(Handle);

    public NamespaceDeclarationSyntax? Namespace
    {
        get
        {
            var ns = ShardScriptAPI.Shard_GetUnitNamespace(Handle);
            return ns == IntPtr.Zero ? null : new NamespaceDeclarationSyntax(ns);
        }
    }

    public CompilationUnitSyntax(IntPtr handle)
    {
        Handle = handle;
    }

    public IReadOnlyList<ClassDeclarationSyntax> GetClasses()
    {
        int count = ShardScriptAPI.Shard_GetUnitClassCount(Handle);
        ClassDeclarationSyntax[] classes = new ClassDeclarationSyntax[count];
        for (int i = 0; i < count; i++)
            classes[i] = new ClassDeclarationSyntax(ShardScriptAPI.Shard_GetUnitClass(Handle, i));

        return classes;
    }
}
