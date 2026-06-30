using ShardScript.Runtime;
using ShardScript.Scripting;
using ShardScript.Syntax.Symbols;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Extension methods for ScopedClassBuilder providing property and indexer support.
/// </summary>
public static class ClassBuilderExtensions
{
    /// <summary>
    /// Adds a property with automatic type inference.
    /// </summary>
    public static ClassBuilder WithProperty<T>(
        this ClassBuilder builder,
        string name,
        Func<ObjectInstance, T>? getter = null,
        Action<ObjectInstance, T>? setter = null)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        PrimitiveType primitive = ShardTypeMapper.MapPrimitiveType(typeof(T));
        TypeSymbol propertyType = SymbolBuilder.Primitive(builder.Context, primitive);

        PropertyBuilder propertyBuilder = new PropertyBuilder(builder.Context, builder.Symbol, name, propertyType, isStatic: false);

        if (getter != null)
            propertyBuilder.WithGetter(getter);

        if (setter != null)
            propertyBuilder.WithSetter(setter);

        propertyBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds a property with automatic type inference and extracts the property methods.
    /// </summary>
    public static ClassBuilder WithProperty<T>(
        this ClassBuilder builder,
        string name,
        Func<ObjectInstance, T>? getter,
        out MethodSymbol? getterMethod,
        Action<ObjectInstance, T>? setter,
        out MethodSymbol? setterMethod)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        PrimitiveType primitive = ShardTypeMapper.MapPrimitiveType(typeof(T));
        TypeSymbol propertyType = SymbolBuilder.Primitive(builder.Context, primitive);

        PropertyBuilder propertyBuilder = new PropertyBuilder(builder.Context, builder.Symbol, name, propertyType, isStatic: false);

        if (getter != null)
            propertyBuilder.WithGetter(getter);

        if (setter != null)
            propertyBuilder.WithSetter(setter);

        (getterMethod, setterMethod) = propertyBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds a static property with automatic type inference.
    /// </summary>
    public static ClassBuilder WithStaticProperty<T>(
        this ClassBuilder builder,
        string name,
        Func<T>? getter = null,
        Action<T>? setter = null)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        PrimitiveType primitive = ShardTypeMapper.MapPrimitiveType(typeof(T));
        TypeSymbol propertyType = SymbolBuilder.Primitive(builder.Context, primitive);

        PropertyBuilder propertyBuilder = new PropertyBuilder(builder.Context, builder.Symbol, name, propertyType, isStatic: true);

        if (getter != null)
        {
            propertyBuilder.WithGetter((instance) => getter());
        }

        if (setter != null)
        {
            propertyBuilder.WithSetter<T>((instance, value) => setter(value));
        }

        propertyBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds an indexer with automatic type inference.
    /// </summary>
    public static ClassBuilder WithIndexer<TKey, TValue>(
        this ClassBuilder builder,
        Func<ObjectInstance, ObjectInstance, object?>? getter = null,
        Action<ObjectInstance, ObjectInstance, ObjectInstance>? setter = null)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        PrimitiveType keyPrimitive = ShardTypeMapper.MapPrimitiveType(typeof(TKey));
        PrimitiveType valuePrimitive = ShardTypeMapper.MapPrimitiveType(typeof(TValue));

        TypeSymbol keyType = SymbolBuilder.Primitive(builder.Context, keyPrimitive);
        TypeSymbol valueType = SymbolBuilder.Primitive(builder.Context, valuePrimitive);

        IndexerBuilder indexerBuilder = new IndexerBuilder(builder.Context, builder.Symbol, keyType, valueType);

        if (getter != null)
            indexerBuilder.WithGetter(getter);

        if (setter != null)
            indexerBuilder.WithSetter(setter);

        indexerBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds an indexer with automatic type inference and extracts the methods.
    /// </summary>
    public static ClassBuilder WithIndexer<TKey, TValue>(
        this ClassBuilder builder,
        Func<ObjectInstance, ObjectInstance, object?>? getter,
        out MethodSymbol? getterMethod,
        Action<ObjectInstance, ObjectInstance, ObjectInstance>? setter,
        out MethodSymbol? setterMethod)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        PrimitiveType keyPrimitive = ShardTypeMapper.MapPrimitiveType(typeof(TKey));
        PrimitiveType valuePrimitive = ShardTypeMapper.MapPrimitiveType(typeof(TValue));

        TypeSymbol keyType = SymbolBuilder.Primitive(builder.Context, keyPrimitive);
        TypeSymbol valueType = SymbolBuilder.Primitive(builder.Context, valuePrimitive);

        IndexerBuilder indexerBuilder = new IndexerBuilder(builder.Context, builder.Symbol, keyType, valueType);

        if (getter != null)
            indexerBuilder.WithGetter(getter);

        if (setter != null)
            indexerBuilder.WithSetter(setter);

        (getterMethod, setterMethod) = indexerBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds an operator overload.
    /// </summary>
    public static ClassBuilder WithOperator(
        this ClassBuilder builder,
        OperatorType op,
        Func<ObjectInstance, ObjectInstance, ObjectInstance> binaryCallback)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        var returnType = SymbolBuilder.Primitive(builder.Context, PrimitiveType.Any);
        OperatorBuilder operatorBuilder = new OperatorBuilder(builder.Context, builder.Symbol, op, returnType, binaryCallback: binaryCallback);
        operatorBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds an operator overload and extracts the method symbol.
    /// </summary>
    public static ClassBuilder WithOperator(
        this ClassBuilder builder,
        OperatorType op,
        Func<ObjectInstance, ObjectInstance, ObjectInstance> binaryCallback,
        out MethodSymbol operatorMethod)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        var returnType = SymbolBuilder.Primitive(builder.Context, PrimitiveType.Any);
        OperatorBuilder operatorBuilder = new OperatorBuilder(builder.Context, builder.Symbol, op, returnType, binaryCallback: binaryCallback);
        operatorMethod = operatorBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds a unary operator overload.
    /// </summary>
    public static ClassBuilder WithUnaryOperator(
        this ClassBuilder builder,
        OperatorType op,
        Func<ObjectInstance, ObjectInstance> unaryCallback)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        var returnType = SymbolBuilder.Primitive(builder.Context, PrimitiveType.Any);
        OperatorBuilder operatorBuilder = new OperatorBuilder(builder.Context, builder.Symbol, op, returnType, unaryCallback: unaryCallback);
        operatorBuilder.Build();
        return builder;
    }

    /// <summary>
    /// Adds a unary operator overload and extracts the method symbol.
    /// </summary>
    public static ClassBuilder WithUnaryOperator(
        this ClassBuilder builder,
        OperatorType op,
        Func<ObjectInstance, ObjectInstance> unaryCallback,
        out MethodSymbol operatorMethod)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        var returnType = SymbolBuilder.Primitive(builder.Context, PrimitiveType.Any);
        OperatorBuilder operatorBuilder = new OperatorBuilder(builder.Context, builder.Symbol, op, returnType, unaryCallback: unaryCallback);
        operatorMethod = operatorBuilder.Build();
        return builder;
    }
}
