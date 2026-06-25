using System.Reflection;

namespace ShardScript.Scripting;

/// <summary>
/// Configuration for reflection-based binding of C# members into ShardScript.
/// </summary>
public sealed class ShardReflectionBinderOptions
{
    public string Namespace { get; }
    public List<Type> Types { get; } = [];
    public List<Assembly> Assemblies { get; } = [];

    public bool IncludeStaticMethods { get; set; } = true;
    public bool IncludeInstanceMethods { get; set; } = false;
    public bool IncludeProperties { get; set; } = true;
    public bool IncludeFields { get; set; } = false;

    public ShardReflectionBinderOptions(string ns)
    {
        Namespace = ns ?? throw new ArgumentNullException(nameof(ns));
    }

    public ShardReflectionBinderOptions IncludeType<T>() => IncludeType(typeof(T));

    public ShardReflectionBinderOptions IncludeType(Type type)
    {
        if (type == null)
            throw new ArgumentNullException(nameof(type));

        Types.Add(type);
        return this;
    }

    public ShardReflectionBinderOptions IncludeAssembly(Assembly assembly)
    {
        if (assembly == null)
            throw new ArgumentNullException(nameof(assembly));

        Assemblies.Add(assembly);
        return this;
    }

    public ShardReflectionBinderOptions IncludeCurrentAssembly()
    {
        Assemblies.Add(Assembly.GetCallingAssembly());
        return this;
    }

    public ShardReflectionBinderOptions WithStaticMethods(bool include = true)
    {
        IncludeStaticMethods = include;
        return this;
    }

    public ShardReflectionBinderOptions WithInstanceMethods(bool include = true)
    {
        IncludeInstanceMethods = include;
        return this;
    }

    public ShardReflectionBinderOptions WithProperties(bool include = true)
    {
        IncludeProperties = include;
        return this;
    }

    public ShardReflectionBinderOptions WithFields(bool include = true)
    {
        IncludeFields = include;
        return this;
    }
}
