using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Scripting;
using ShardScript.Syntax.Symbols;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Builder for creating indexer symbols with getter/setter callbacks.
/// Indexers are implemented as methods named "get_Item" and "set_Item".
/// </summary>
public sealed class IndexerBuilder
{
    /// <summary>
    /// Gets the compilation context.
    /// </summary>
    public CompilationContext Context { get; }

    /// <summary>
    /// Gets the index (key) type.
    /// </summary>
    public TypeSymbol IndexType { get; }

    /// <summary>
    /// Gets the value type.
    /// </summary>
    public TypeSymbol ValueType { get; }

    /// <summary>
    /// Gets the parent type containing this indexer.
    /// </summary>
    public TypeSymbol ParentType { get; }

    private Func<ObjectInstance, ObjectInstance, object?>? _getter;
    private Action<ObjectInstance, ObjectInstance, ObjectInstance>? _setter;

    internal IndexerBuilder(CompilationContext context, TypeSymbol parentType, TypeSymbol indexType, TypeSymbol valueType)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        ParentType = parentType ?? throw new ArgumentNullException(nameof(parentType));
        IndexType = indexType ?? throw new ArgumentNullException(nameof(indexType));
        ValueType = valueType ?? throw new ArgumentNullException(nameof(valueType));
    }

    /// <summary>
    /// Sets the getter callback.
    /// </summary>
    public IndexerBuilder WithGetter(Func<ObjectInstance, ObjectInstance, object?> getter)
    {
        if (getter == null)
            throw new ArgumentNullException(nameof(getter));

        _getter = getter;
        return this;
    }

    /// <summary>
    /// Sets the setter callback.
    /// </summary>
    public IndexerBuilder WithSetter(Action<ObjectInstance, ObjectInstance, ObjectInstance> setter)
    {
        if (setter == null)
            throw new ArgumentNullException(nameof(setter));

        _setter = setter;
        return this;
    }

    /// <summary>
    /// Builds the indexer by creating get_Item and set_Item methods.
    /// </summary>
    public (MethodSymbol? Getter, MethodSymbol? Setter) Build()
    {
        MethodSymbol? getterMethod = null;
        MethodSymbol? setterMethod = null;

        // Create get_Item method if specified
        if (_getter != null)
        {
            MethodBuilder getterBuilder = new MethodBuilder(Context, ParentType, "get_Item", ValueType);
            getterBuilder.Instance().Public();

            // Add index parameter
            getterBuilder.Parameter("index", IndexType);

            ShardManagedMethodCallback callback = (method, args, argsCount, userData, collector) =>
            {
                ObjectInstance instance = new ObjectInstance(args[0]);
                ObjectInstance index = new ObjectInstance(args[1]);
                object? result = _getter!(instance, index);
                return ShardMarshaller.ToObjectInstance(result!, new GarbageCollector(collector)).Handle;
            };

            getterBuilder.Callback(callback);
            getterMethod = getterBuilder.Symbol;
        }

        // Create set_Item method if specified
        if (_setter != null)
        {
            MethodBuilder setterBuilder = new MethodBuilder(Context, ParentType, "set_Item", SymbolBuilder.Primitive(Context, PrimitiveType.Void));
            setterBuilder.Instance().Public();

            // Add index and value parameters
            setterBuilder.Parameter("index", IndexType);
            setterBuilder.Parameter("value", ValueType);

            ShardManagedMethodCallback callback = (method, args, argsCount, userData, collector) =>
            {
                ObjectInstance instance = new ObjectInstance(args[0]);
                ObjectInstance index = new ObjectInstance(args[1]);
                ObjectInstance value = new ObjectInstance(args[2]);
                _setter!(instance, index, value);
                return IntPtr.Zero;
            };

            setterBuilder.Callback(callback);
            setterMethod = setterBuilder.Symbol;
        }

        return (getterMethod, setterMethod);
    }
}
