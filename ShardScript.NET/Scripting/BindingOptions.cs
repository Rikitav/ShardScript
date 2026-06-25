using System.Reflection;

namespace ShardScript.NET.Scripting;

/// <summary>
/// Configuration for reflection-based binding of C# members into ShardScript.
/// </summary>
public sealed class BindingOptions
{
    internal string Namespace { get; }
    internal List<Type> Types { get; } = new();
    internal List<Assembly> Assemblies { get; } = new();

    internal bool IncludeStaticMethods { get; set; } = true;
    internal bool IncludeInstanceMethods { get; set; } = false;
    internal bool IncludeProperties { get; set; } = true;
    internal bool IncludeFields { get; set; } = false;

    internal BindingOptions(string ns)
    {
        Namespace = ns ?? throw new ArgumentNullException(nameof(ns));
    }

    public BindingOptions IncludeType<T>() => IncludeType(typeof(T));

    public BindingOptions IncludeType(Type type)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        Types.Add(type);
        return this;
    }

    public BindingOptions IncludeAssembly(Assembly assembly)
    {
        if (assembly == null)
            throw new ArgumentNullException(nameof(assembly));

        Assemblies.Add(assembly);
        return this;
    }

    public BindingOptions IncludeCurrentAssembly()
    {
        Assemblies.Add(Assembly.GetCallingAssembly());
        return this;
    }

    public BindingOptions WithStaticMethods(bool include = true)
    {
        IncludeStaticMethods = include;
        return this;
    }

    public BindingOptions WithInstanceMethods(bool include = true)
    {
        IncludeInstanceMethods = include;
        return this;
    }

    public BindingOptions WithProperties(bool include = true)
    {
        IncludeProperties = include;
        return this;
    }

    public BindingOptions WithFields(bool include = true)
    {
        IncludeFields = include;
        return this;
    }
}
