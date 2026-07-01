using ShardScript.Application;
using ShardScript.Syntax.Symbols;
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

    // =====================================================================
    // Instance field access
    // =====================================================================

    /// <summary>
    /// Reads an instance field by its field symbol.
    /// </summary>
    public ObjectInstance GetField(FieldSymbol field)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        IntPtr value = NativeMethods.Shard_GetInstanceField(Handle, field.Handle);
        if (value == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to read instance field '{field.Name}': {ShardEngineException.GetLastErrorMessage()}");

        return new ObjectInstance(value);
    }

    /// <summary>
    /// Writes an instance field by its field symbol, assigning an existing object instance.
    /// </summary>
    public void SetField(FieldSymbol field, ObjectInstance value)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        int result = NativeMethods.Shard_SetInstanceField(Handle, field.Handle, value.Handle);
        if (result != 0)
            throw new InvalidOperationException($"Failed to write instance field '{field.Name}': {ShardEngineException.GetLastErrorMessage()}");
    }

    /// <summary>Writes a primitive value-type field directly (no GC required).</summary>
    public void SetField(FieldSymbol field, long value)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        int result = NativeMethods.Shard_SetInstanceFieldInteger(Handle, field.Handle, value);
        if (result != 0)
            throw new InvalidOperationException($"Failed to write instance field '{field.Name}': {ShardEngineException.GetLastErrorMessage()}");
    }

    /// <summary>Writes a primitive value-type field directly (no GC required).</summary>
    public void SetField(FieldSymbol field, double value)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        int result = NativeMethods.Shard_SetInstanceFieldDouble(Handle, field.Handle, value);
        if (result != 0)
            throw new InvalidOperationException($"Failed to write instance field '{field.Name}': {ShardEngineException.GetLastErrorMessage()}");
    }

    /// <summary>Writes a primitive value-type field directly (no GC required).</summary>
    public void SetField(FieldSymbol field, bool value)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        int result = NativeMethods.Shard_SetInstanceFieldBool(Handle, field.Handle, value ? 1 : 0);
        if (result != 0)
            throw new InvalidOperationException($"Failed to write instance field '{field.Name}': {ShardEngineException.GetLastErrorMessage()}");
    }

    /// <summary>Writes a primitive value-type field directly (no GC required).</summary>
    public void SetField(FieldSymbol field, char value)
    {
        if (field == null)
            throw new ArgumentNullException(nameof(field));

        int result = NativeMethods.Shard_SetInstanceFieldChar(Handle, field.Handle, value);
        if (result != 0)
            throw new InvalidOperationException($"Failed to write instance field '{field.Name}': {ShardEngineException.GetLastErrorMessage()}");
    }

    /// <summary>
    /// Writes a string (reference type) field. Strings require a GC-owned value;
    /// use <see cref="GarbageCollector.FromString"/> to create one and pass it to
    /// <see cref="SetField(FieldSymbol, ObjectInstance)"/>.
    /// </summary>
    public void SetField(FieldSymbol field, string value, GarbageCollector gc)
    {
        if (value == null)
            value = string.Empty;

        SetField(field, gc.FromString(value));
    }

    // =====================================================================
    // Array element access
    // =====================================================================

    /// <summary>Reads an array element by index.</summary>
    public ObjectInstance GetElement(long index)
    {
        if (index < 0)
            throw new ArgumentOutOfRangeException(nameof(index));

        IntPtr value = NativeMethods.Shard_GetArrayElement(Handle, (nuint)index);
        if (value == IntPtr.Zero)
            throw new InvalidOperationException($"Failed to read array element {index}: {ShardEngineException.GetLastErrorMessage()}");

        return new ObjectInstance(value);
    }

    /// <summary>Writes an array element by index.</summary>
    public void SetElement(long index, ObjectInstance value)
    {
        if (index < 0)
            throw new ArgumentOutOfRangeException(nameof(index));

        int result = NativeMethods.Shard_SetArrayElement(Handle, (nuint)index, value.Handle);
        if (result != 0)
            throw new InvalidOperationException($"Failed to write array element {index}: {ShardEngineException.GetLastErrorMessage()}");
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

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetInstanceField(IntPtr instance, IntPtr field);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetInstanceField(IntPtr instance, IntPtr field, IntPtr value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetInstanceFieldInteger(IntPtr instance, IntPtr field, long value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetInstanceFieldDouble(IntPtr instance, IntPtr field, double value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetInstanceFieldBool(IntPtr instance, IntPtr field, int value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetInstanceFieldChar(IntPtr instance, IntPtr field, char value);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetArrayElement(IntPtr array, nuint index);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_SetArrayElement(IntPtr array, nuint index, IntPtr value);
    }
}
