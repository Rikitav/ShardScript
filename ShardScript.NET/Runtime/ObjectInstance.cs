using System.Runtime.InteropServices;

namespace ShardScript.Runtime;

public enum PrimitiveType
{
    Void = 0,
    Null = 1,
    Any = 2,
    Boolean = 3,
    Integer = 4,
    Double = 5,
    Char = 6,
    String = 7,
    Array = 8
}

/// <summary>
/// A handle to a ShardScript runtime object instance.
/// </summary>
public readonly struct ObjectInstance : IEquatable<ObjectInstance>
{
    public IntPtr Handle { get; }

    public ObjectInstance(IntPtr handle)
    {
        Handle = handle;
    }

    public bool IsValid => Handle != IntPtr.Zero;

    public bool IsNull => Handle == IntPtr.Zero;

    public bool Equals(ObjectInstance other) => Handle == other.Handle;

    public override bool Equals(object? obj) => obj is ObjectInstance other && Equals(other);

    public override int GetHashCode() => Handle.GetHashCode();

    public static bool operator ==(ObjectInstance left, ObjectInstance right) => left.Equals(right);

    public static bool operator !=(ObjectInstance left, ObjectInstance right) => !left.Equals(right);

    public override string ToString() => $"ObjectInstance(0x{Handle:X})";

    public long AsInteger()
    {
        return NativeMethods.Shard_ReadInteger(Handle);
    }

    public double AsDouble()
    {
        return NativeMethods.Shard_ReadDouble(Handle);
    }

    public bool AsBool()
    {
        return NativeMethods.Shard_ReadBool(Handle) != 0;
    }

    public string AsString()
    {
        IntPtr ptr = NativeMethods.Shard_ReadString(Handle);
        return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern long Shard_ReadInteger(IntPtr instance);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern double Shard_ReadDouble(IntPtr instance);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_ReadBool(IntPtr instance);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_ReadString(IntPtr instance);
    }
}
