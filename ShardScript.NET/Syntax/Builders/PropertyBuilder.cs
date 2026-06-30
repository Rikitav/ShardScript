using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Scripting;
using ShardScript.Syntax.Symbols;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Builder for creating property symbols with getter/setter callbacks.
/// Note: Programmatic property creation is emulated using getter/setter method pairs.
/// </summary>
public sealed class PropertyBuilder
{
    /// <summary>
    /// Gets the compilation context.
    /// </summary>
    public CompilationContext Context { get; }

    /// <summary>
    /// Gets the property type.
    /// </summary>
    public TypeSymbol PropertyType { get; }

    /// <summary>
    /// Gets the name of the property.
    /// </summary>
    public string Name { get; }

    /// <summary>
    /// Gets whether this is a static property.
    /// </summary>
    public bool IsStatic { get; }

    private readonly TypeSymbol? _parentType;
    private Func<ObjectInstance, object?>? _getter;
    private Action<ObjectInstance, object?>? _setter;

    internal PropertyBuilder(CompilationContext context, TypeSymbol? parentType, string name, TypeSymbol propertyType, bool isStatic = false)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        Name = name ?? throw new ArgumentNullException(nameof(name));
        PropertyType = propertyType ?? throw new ArgumentNullException(nameof(propertyType));
        IsStatic = isStatic;
        _parentType = parentType;
    }

    /// <summary>
    /// Sets the getter callback.
    /// </summary>
    public PropertyBuilder WithGetter<T>(Func<ObjectInstance, T> getter)
    {
        if (getter == null)
            throw new ArgumentNullException(nameof(getter));

        _getter = instance => getter(instance);
        return this;
    }

    /// <summary>
    /// Sets the setter callback.
    /// </summary>
    public PropertyBuilder WithSetter<T>(Action<ObjectInstance, T> setter)
    {
        if (setter == null)
            throw new ArgumentNullException(nameof(setter));

        _setter = (instance, value) => setter(instance, (T)value!);
        return this;
    }

    /// <summary>
    /// Builds the property by creating getter/setter methods.
    /// </summary>
    /// <returns>
    /// A tuple containing the getter method symbol (if any) and setter method symbol (if any).
    /// </returns>
    public (MethodSymbol? Getter, MethodSymbol? Setter) Build()
    {
        MethodSymbol? getterMethod = null;
        MethodSymbol? setterMethod = null;

        // Create getter method if specified
        if (_getter != null)
        {
            MethodBuilder getterBuilder = new MethodBuilder(Context, _parentType!, $"get_{Name}", PropertyType);
            if (!IsStatic)
                getterBuilder.Instance();
            else
                getterBuilder.Static();

            getterBuilder.Public();

            ShardManagedMethodCallback callback = (method, args, argsCount, userData, collector) =>
            {
                ObjectInstance instance = IsStatic ? new ObjectInstance(IntPtr.Zero) : new ObjectInstance(args[0]);
                object? result = _getter!(instance);
                return ShardMarshaller.ToObjectInstance(result!, new GarbageCollector(collector)).Handle;
            };

            getterBuilder.Callback(callback);
            getterMethod = getterBuilder.Symbol;
        }

        // Create setter method if specified
        if (_setter != null)
        {
            MethodBuilder setterBuilder = new MethodBuilder(Context, _parentType!, $"set_{Name}", SymbolBuilder.Primitive(Context, PrimitiveType.Void));
            if (!IsStatic)
                setterBuilder.Instance();
            else
                setterBuilder.Static();

            setterBuilder.Public();

            // Add value parameter
            var valueType = SymbolBuilder.Primitive(Context, PrimitiveType.Any);
            setterBuilder.Parameter("value", valueType);

            ShardManagedMethodCallback callback = (method, args, argsCount, userData, collector) =>
            {
                ObjectInstance instance = IsStatic ? new ObjectInstance(IntPtr.Zero) : new ObjectInstance(args[0]);
                ObjectInstance value = IsStatic ? new ObjectInstance(args[0]) : new ObjectInstance(args[1]);
                _setter!(instance, ShardMarshaller.FromObjectInstance(value, typeof(object)));
                return IntPtr.Zero;
            };

            setterBuilder.Callback(callback);
            setterMethod = setterBuilder.Symbol;
        }

        return (getterMethod, setterMethod);
    }
}
