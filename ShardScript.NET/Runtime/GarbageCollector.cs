using ShardScript.NET.Application;
using ShardScript.NET.Syntax.Symbols;
using System.Runtime.InteropServices;

namespace ShardScript.NET.Runtime;

/// <summary>
/// Allocates and manages ShardScript object instances for the current domain.
/// </summary>
public sealed class GarbageCollector
{
    private readonly IntPtr _handle;

    public IntPtr Handle => _handle;

    public GarbageCollector(IntPtr handle)
    {
        _handle = handle;
    }

    public ObjectInstance FromInteger(long value)
    {
        return new ObjectInstance(NativeMethods.Shard_GCFromInteger(_handle, value));
    }

    public ObjectInstance FromDouble(double value)
    {
        return new ObjectInstance(NativeMethods.Shard_GCFromDouble(_handle, value));
    }

    public ObjectInstance FromBool(bool value)
    {
        return new ObjectInstance(NativeMethods.Shard_GCFromBool(_handle, value ? 1 : 0));
    }

    public ObjectInstance FromString(string value)
    {
        return new ObjectInstance(NativeMethods.Shard_GCFromString(_handle, value));
    }

    public ObjectInstance GetStaticField(FieldSymbol field)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        IntPtr value = NativeMethods.Shard_GCGetStaticField(_handle, field.Handle);
        return new ObjectInstance(value);
    }

    public void SetStaticField(FieldSymbol field, ObjectInstance value)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        int result = NativeMethods.Shard_GCSetStaticField(_handle, field.Handle, value.Handle);
        if (result != 0)
            throw new InvalidOperationException($"Failed to set static field '{field.Name}': {ShardEngineException.GetLastErrorMessage()}");
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GCFromInteger(IntPtr gc, long value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GCFromDouble(IntPtr gc, double value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GCFromBool(IntPtr gc, int value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GCFromString(IntPtr gc, [MarshalAs(UnmanagedType.LPWStr)] string value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GCGetStaticField(IntPtr gc, IntPtr field);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GCSetStaticField(IntPtr gc, IntPtr field, IntPtr value);
    }
}
