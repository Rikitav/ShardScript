namespace ShardScript.NET.Scripting;

/// <summary>
/// Marks a C# class or struct as available to ShardScript scripts.
/// Static public methods and properties are automatically bound as ShardScript symbols.
/// </summary>
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false, AllowMultiple = false)]
public sealed class ShardScriptUserDataAttribute : Attribute
{
    public string? Namespace { get; set; }
    public string? TypeName { get; set; }
}
