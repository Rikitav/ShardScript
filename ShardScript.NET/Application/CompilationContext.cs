using ShardScript.Syntax;
using ShardScript.Syntax.Nodes;
using ShardScript.Syntax.Symbols;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace ShardScript.Application;

/// <summary>
/// Represents a single compilation session. Owns the syntax tree, semantic model and diagnostics.
/// </summary>
public sealed class CompilationContext : IDisposable
{
    private IntPtr _handle;

    public IntPtr Handle => _handle;

    public bool EntryPointEnabled
    {
        set => ShardEngineException.ThrowIfError(NativeMethods.Shard_SetEntryPoint(_handle, value ? 1 : 0));
        get => NativeMethods.Shard_GetEntryPoint(_handle) != 0;
    }

    public bool HasErrors => NativeMethods.Shard_HasErrors(_handle) != 0;

    public int ErrorCount => NativeMethods.Shard_GetErrorCount(_handle);

    public CompilationContext()
    {
        _handle = NativeMethods.Shard_CreateCompilationContext();
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to create compilation context: {ShardEngineException.GetLastErrorMessage()}");
    }

    public void AddLibrary(string path)
    {
        int result = NativeMethods.Shard_AddLibrary(_handle, path);
        ShardEngineException.ThrowIfError(result);
    }

    public void AddLibraries(IEnumerable<string> paths)
    {
        string[] array = paths.ToArray();
        IntPtr[] nativePaths = new IntPtr[array.Length];
        IntPtr arrayPointer = IntPtr.Zero;

        try
        {
            for (int i = 0; i < array.Length; i++)
                nativePaths[i] = Marshal.StringToHGlobalUni(array[i]);

            arrayPointer = Marshal.AllocHGlobal(IntPtr.Size * nativePaths.Length);
            Marshal.Copy(nativePaths, 0, arrayPointer, nativePaths.Length);

            int result = NativeMethods.Shard_AddLibraries(_handle, arrayPointer, (UIntPtr)array.Length);
            ShardEngineException.ThrowIfError(result);
        }
        finally
        {
            if (arrayPointer != IntPtr.Zero)
                Marshal.FreeHGlobal(arrayPointer);

            for (int i = 0; i < nativePaths.Length; i++)
            {
                if (nativePaths[i] != IntPtr.Zero)
                    Marshal.FreeHGlobal(nativePaths[i]);
            }
        }
    }

    public void AddSource(string name, string code, CompilationUnitOrigin origin = CompilationUnitOrigin.SourceFile)
    {
        int result = NativeMethods.Shard_AddSource(_handle, name, code, (int)origin);
        ShardEngineException.ThrowIfError(result);
    }

    public void AddSourceFile(string filePath, CompilationUnitOrigin origin = CompilationUnitOrigin.SourceFile)
    {
        int result = NativeMethods.Shard_AddSourceFile(_handle, filePath, (int)origin);
        ShardEngineException.ThrowIfError(result);
    }

    public void Analyze()
    {
        int result = NativeMethods.Shard_Analyze(_handle);
        ShardEngineException.ThrowIfError(result);
    }

    public ApplicationDomain Compile()
    {
        IntPtr domain = NativeMethods.Shard_Compile(_handle);
        if (domain == IntPtr.Zero)
            throw new InvalidOperationException($"Compilation failed: {ShardEngineException.GetLastErrorMessage()}");

        return new ApplicationDomain(domain);
    }

    public ApplicationDomain CompileAndRun()
    {
        IntPtr domain = NativeMethods.Shard_CompileAndRun(_handle);
        if (domain == IntPtr.Zero)
            throw new InvalidOperationException($"Compilation/run failed: {ShardEngineException.GetLastErrorMessage()}");

        return new ApplicationDomain(domain);
    }
    public void ResetDiagnostics()
    {
        int result = NativeMethods.Shard_ResetDiagnostics(_handle);
        ShardEngineException.ThrowIfError(result);
    }

    public string GetDiagnostics()
    {
        int length = NativeMethods.Shard_GetDiagnostics(_handle, null, 0);
        if (length <= 0)
            return string.Empty;

        StringBuilder sb = new StringBuilder(length + 1);
        NativeMethods.Shard_GetDiagnostics(_handle, sb, sb.Capacity);
        return sb.ToString();
    }

    public IReadOnlyList<CompilationUnitSyntax> GetCompilationUnits()
    {
        int count = NativeMethods.Shard_GetCompilationUnitCount(_handle);
        CompilationUnitSyntax[] units = new CompilationUnitSyntax[count];

        for (int i = 0; i < count; i++)
            units[i] = new CompilationUnitSyntax(NativeMethods.Shard_GetCompilationUnit(_handle, i));

        return units;
    }

    public TypeSymbol? FindType(string name)
    {
        IntPtr type = NativeMethods.Shard_FindType(_handle, name);
        return type == IntPtr.Zero ? null : new TypeSymbol(type);
    }

    public void AddCompilationUnit(CompilationUnitSyntax unit)
    {
        if (unit == null)
            throw new ArgumentNullException(nameof(unit));

        int result = NativeMethods.Shard_AddCompilationUnit(_handle, unit.Handle);
        ShardEngineException.ThrowIfError(result);
    }

    public void AddCompilationUnit(SyntaxCompilationUnit unit)
    {
        if (unit == null)
            throw new ArgumentNullException(nameof(unit));

        int result = NativeMethods.Shard_AddCompilationUnit(_handle, unit.Handle);
        ShardEngineException.ThrowIfError(result);
    }

    public void MarkForReAnalyze()
    {
        ShardEngineException.ThrowIfError(NativeMethods.Shard_MarkForReAnalyze(_handle));
    }

    public void Dispose()
    {
        if (_handle != IntPtr.Zero)
        {
            NativeMethods.Shard_DestroyCompilationContext(_handle);
            _handle = IntPtr.Zero;
        }
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CreateCompilationContext();

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_DestroyCompilationContext(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddLibrary(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string path);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddLibraries(IntPtr ctx,
            IntPtr paths,
            UIntPtr count);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddSource(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPWStr)] string sourceName,
            [MarshalAs(UnmanagedType.LPWStr)] string code,
            int origin);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddSourceFile(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPWStr)] string filePath,
            int origin);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_Analyze(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_Compile(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_CompileAndRun(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetEntryPoint(IntPtr ctx, int value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetEntryPoint(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_HasErrors(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetErrorCount(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_ResetDiagnostics(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetDiagnostics(IntPtr ctx, [Out] StringBuilder? buffer, int bufferLen);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetCompilationUnitCount(IntPtr ctx);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetCompilationUnit(IntPtr ctx, int index);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_FindType(IntPtr ctx, [MarshalAs(UnmanagedType.LPWStr)] string name);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_AddCompilationUnit(IntPtr ctx, IntPtr unit);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_MarkForReAnalyze(IntPtr ctx);
    }
}
