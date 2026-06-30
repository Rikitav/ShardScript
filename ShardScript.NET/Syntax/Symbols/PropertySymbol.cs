namespace ShardScript.Syntax.Symbols;

/// <summary>
/// Represents a property symbol in ShardScript.
/// Note: Programmatic property creation is not currently supported via native API.
/// Properties can be emulated using getter/setter method pairs.
/// </summary>
public sealed class PropertySymbol : SyntaxSymbol
{
    /// <summary>
    /// Gets the name of this property.
    /// </summary>
    public string Name { get; }

    /// <summary>
    /// Gets the type of this property.
    /// </summary>
    public TypeSymbol PropertyType { get; }

    /// <summary>
    /// Gets whether this property has a getter.
    /// </summary>
    public bool HasGetter { get; }

    /// <summary>
    /// Gets whether this property has a setter.
    /// </summary>
    public bool HasSetter { get; }

    /// <summary>
    /// Gets whether this is a static property.
    /// </summary>
    public bool IsStatic { get; }

    internal PropertySymbol(IntPtr handle, string name, TypeSymbol propertyType, bool hasGetter, bool hasSetter, bool isStatic)
        : base(handle)
    {
        Name = name;
        PropertyType = propertyType;
        HasGetter = hasGetter;
        HasSetter = hasSetter;
        IsStatic = isStatic;
    }
}
