using ShardScript.NET.Runtime;

namespace ShardScript.NET.Scripting;

/// <summary>
/// Converts between C# values and ShardScript runtime object instances.
/// </summary>
public static class Marshaller
{
    public static ObjectInstance ToObjectInstance(object? value, GarbageCollector gc)
    {
        if (value == null)
            return default;

        return value switch
        {
            bool b => gc.FromBool(b),
            byte u8 => gc.FromInteger(u8),
            sbyte i8 => gc.FromInteger(i8),
            short i16 => gc.FromInteger(i16),
            ushort u16 => gc.FromInteger(u16),
            int i32 => gc.FromInteger(i32),
            uint u32 => gc.FromInteger((long)u32),
            long i64 => gc.FromInteger(i64),
            ulong u64 => gc.FromInteger((long)u64),
            float f => gc.FromDouble(f),
            double d => gc.FromDouble(d),
            string s => gc.FromString(s),
            ObjectInstance o => o,
            _ => throw new NotSupportedException($"Marshalling of type '{value.GetType().Name}' is not supported yet.")
        };
    }

    public static T FromObjectInstance<T>(ObjectInstance instance)
    {
        return (T?)FromObjectInstance(instance, typeof(T))
            ?? throw new InvalidOperationException($"Cannot marshal null to value type '{typeof(T).Name}'.");
    }

    public static object? FromObjectInstance(ObjectInstance instance, Type targetType)
    {
        if (instance.IsNull)
        {
            if (targetType.IsValueType && Nullable.GetUnderlyingType(targetType) == null)
                throw new InvalidOperationException($"Cannot marshal null to non-nullable value type '{targetType.Name}'.");

            return null;
        }

        Type underlyingType = Nullable.GetUnderlyingType(targetType) ?? targetType;

        if (underlyingType == typeof(bool))
            return instance.AsBool();

        if (underlyingType == typeof(byte))
            return (byte)instance.AsInteger();

        if (underlyingType == typeof(sbyte))
            return (sbyte)instance.AsInteger();

        if (underlyingType == typeof(short))
            return (short)instance.AsInteger();

        if (underlyingType == typeof(ushort))
            return (ushort)instance.AsInteger();

        if (underlyingType == typeof(int))
            return (int)instance.AsInteger();

        if (underlyingType == typeof(uint))
            return (uint)instance.AsInteger();

        if (underlyingType == typeof(long))
            return instance.AsInteger();

        if (underlyingType == typeof(ulong))
            return (ulong)instance.AsInteger();

        if (underlyingType == typeof(float))
            return (float)instance.AsDouble();

        if (underlyingType == typeof(double))
            return instance.AsDouble();

        if (underlyingType == typeof(string))
            return instance.AsString();

        if (underlyingType == typeof(ObjectInstance))
            return instance;

        throw new NotSupportedException($"Marshalling to type '{targetType.Name}' is not supported yet.");
    }
}
