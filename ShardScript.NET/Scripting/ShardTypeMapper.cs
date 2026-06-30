using ShardScript.Runtime;

namespace ShardScript.Scripting;

/// <summary>
/// Maps C# types to ShardScript primitive types.
/// </summary>
public static class ShardTypeMapper
{
    /// <summary>
    /// Maps a C# type to its corresponding ShardScript primitive type.
    /// </summary>
    public static PrimitiveType MapPrimitiveType(Type type)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        Type underlying = Nullable.GetUnderlyingType(type) ?? type;

        return underlying switch
        {
            _ when underlying == typeof(void) => PrimitiveType.Void,
            _ when underlying == typeof(bool) => PrimitiveType.Boolean,
            _ when underlying == typeof(byte) || underlying == typeof(sbyte)
                || underlying == typeof(short) || underlying == typeof(ushort)
                || underlying == typeof(int) || underlying == typeof(uint)
                || underlying == typeof(long) || underlying == typeof(ulong) => PrimitiveType.Integer,
            _ when underlying == typeof(float) || underlying == typeof(double) => PrimitiveType.Double,
            _ when underlying == typeof(char) => PrimitiveType.Char,
            _ when underlying == typeof(string) => PrimitiveType.String,
            _ when underlying == typeof(ObjectInstance) => PrimitiveType.Any,
            _ => throw new NotSupportedException($"Type '{type.Name}' cannot be mapped to a ShardScript primitive.")
        };
    }

    /// <summary>
    /// Gets the corresponding C# type for a ShardScript primitive type.
    /// </summary>
    public static Type GetClrType(PrimitiveType primitive)
    {
        return primitive switch
        {
            PrimitiveType.Void => typeof(void),
            PrimitiveType.Boolean => typeof(bool),
            PrimitiveType.Integer => typeof(long),
            PrimitiveType.Double => typeof(double),
            PrimitiveType.Char => typeof(char),
            PrimitiveType.String => typeof(string),
            PrimitiveType.Any => typeof(ObjectInstance),
            PrimitiveType.Null or PrimitiveType.Array => typeof(object),
            _ => throw new NotSupportedException($"Primitive type '{primitive}' is not supported.")
        };
    }

    /// <summary>
    /// Determines if the given type can be mapped to a ShardScript primitive.
    /// </summary>
    public static bool IsSupportedType(Type type)
    {
        if (type == null)
            return false;

        Type underlying = Nullable.GetUnderlyingType(type) ?? type;

        return underlying == typeof(void)
            || underlying == typeof(bool)
            || underlying == typeof(byte) || underlying == typeof(sbyte)
            || underlying == typeof(short) || underlying == typeof(ushort)
            || underlying == typeof(int) || underlying == typeof(uint)
            || underlying == typeof(long) || underlying == typeof(ulong)
            || underlying == typeof(float) || underlying == typeof(double)
            || underlying == typeof(char)
            || underlying == typeof(string)
            || underlying == typeof(ObjectInstance);
    }
}
