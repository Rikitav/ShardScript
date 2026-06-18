using System.Runtime.InteropServices;

namespace ShardScript.NET.Runtime;

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
    }
}
