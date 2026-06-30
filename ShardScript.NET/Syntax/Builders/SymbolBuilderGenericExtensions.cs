using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Scripting;
using ShardScript.Syntax.Symbols;
using System.Reflection;
using System.Runtime.InteropServices;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Generic extension methods for ScopedClassBuilder providing automatic type inference and marshalling.
/// </summary>
public static class SymbolBuilderGenericExtensions
{
    #region Func<TResult> - No parameters

    public static TBuilder WithMethod<TBuilder,TResult>(
        this TBuilder builder,
        string name,
        Func<TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,TResult>(
        this TBuilder builder,
        string name,
        Func<TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T, TResult> - One parameter

    public static TBuilder WithMethod<TBuilder,T, TResult>(
        this TBuilder builder,
        string name,
        Func<T, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T, TResult>(
        this TBuilder builder,
        string name,
        Func<T, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T1, T2, TResult> - Two parameters

    public static TBuilder WithMethod<TBuilder,T1, T2, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T1, T2, T3, TResult> - Three parameters

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T1, T2, T3, T4, TResult> - Four parameters

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T1, T2, T3, T4, T5, TResult> - Five parameters

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T1, T2, T3, T4, T5, T6, TResult> - Six parameters

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, T6, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, T6, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T1, T2, T3, T4, T5, T6, T7, TResult> - Seven parameters

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, T6, T7, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, T6, T7, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Func<T1, T2, T3, T4, T5, T6, T7, T8, TResult> - Eight parameters

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7, T8, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, T6, T7, T8, TResult> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7, T8, TResult>(
        this TBuilder builder,
        string name,
        Func<T1, T2, T3, T4, T5, T6, T7, T8, TResult> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Action overloads

    public static TBuilder WithMethod<TBuilder>(
        this TBuilder builder,
        string name,
        Action body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder>(
        this TBuilder builder,
        string name,
        Action body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T>(
        this TBuilder builder,
        string name,
        Action<T> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T>(
        this TBuilder builder,
        string name,
        Action<T> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2>(
        this TBuilder builder,
        string name,
        Action<T1, T2> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2>(
        this TBuilder builder,
        string name,
        Action<T1, T2> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5, T6> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5, T6> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5, T6, T7> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5, T6, T7> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7, T8>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5, T6, T7, T8> body,
        out MethodSymbol symbol,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out symbol);
        return builder;
    }

    public static TBuilder WithMethod<TBuilder,T1, T2, T3, T4, T5, T6, T7, T8>(
        this TBuilder builder,
        string name,
        Action<T1, T2, T3, T4, T5, T6, T7, T8> body,
        SymbolLinking linking = SymbolLinking.Static,
        SymbolAccessibility accessibility = SymbolAccessibility.Public)
        where TBuilder : IContainerBuilder
    {
        CreateMethodWithCallback(builder, name, linking, accessibility, body, out _);
        return builder;
    }

    #endregion

    #region Helper methods

    private static void CreateMethodWithCallback(
        IContainerBuilder builder,
        string name,
        SymbolLinking linking,
        SymbolAccessibility accessibility,
        Delegate body,
        out MethodSymbol symbol)
    {
        if (builder == null)
            throw new ArgumentNullException(nameof(builder));

        if (name == null)
            throw new ArgumentNullException(nameof(name));

        if (body == null)
            throw new ArgumentNullException(nameof(body));

        // Extract parameter types from delegate
        Type[] parameterTypes = GetParameterTypes(body);
        Type returnType = GetReturnType(body);

        // Map types to ShardScript primitives
        TypeSymbol returnTypeSymbol = MapType(builder.Context, returnType);
        TypeSymbol[] parameterTypeSymbols = new TypeSymbol[parameterTypes.Length];
        for (int i = 0; i < parameterTypes.Length; i++)
        {
            parameterTypeSymbols[i] = MapType(builder.Context, parameterTypes[i]);
        }

        // Create the method
        MethodBuilder methodBuilder = new MethodBuilder(
            builder.Context,
            builder.Symbol,
            name,
            returnTypeSymbol);

        // Apply linking and accessibility
        if (linking == SymbolLinking.Instance)
            methodBuilder.Instance();
        else
            methodBuilder.Static();

        if (accessibility == SymbolAccessibility.Private)
            methodBuilder.Private();
        else
            methodBuilder.Public();

        // Add parameters
        for (int i = 0; i < parameterTypes.Length; i++)
        {
            string paramName = $"arg{i}";
            methodBuilder.Parameter(paramName, parameterTypeSymbols[i]);
        }

        // Create the marshalled callback
        ShardManagedMethodCallback callback = CreateMarshalledCallback(body);
        methodBuilder.Callback(callback);

        symbol = methodBuilder.Symbol;
    }

    private static TypeSymbol MapType(CompilationContext context, Type type)
    {
        PrimitiveType primitive = ShardTypeMapper.MapPrimitiveType(type);
        return SymbolBuilder.Primitive(context, primitive);
    }

    private static ShardManagedMethodCallback CreateMarshalledCallback(Delegate body)
    {
        // Resolve the matching ShardMarshallingBridge.CreateCallback overload by
        // matching the closed delegate type exactly. This disambiguates overloads
        // that share a generic arity (e.g. Func<TResult> vs Action<T>) and handles
        // the non-generic parameterless Action overload.
        Type delegateType = body.GetType();
        Type[] genericArgs = delegateType.IsGenericType
            ? delegateType.GetGenericArguments()
            : Array.Empty<Type>();

        foreach (MethodInfo m in typeof(ShardMarshallingBridge).GetMethods())
        {
            if (m.Name != nameof(ShardMarshallingBridge.CreateCallback))
                continue;

            ParameterInfo[] parameters = m.GetParameters();
            if (parameters.Length != 1)
                continue;

            Type parameterType = parameters[0].ParameterType;

            if (m.IsGenericMethod)
            {
                if (!delegateType.IsGenericType)
                    continue;

                Type[] methodGenerics = m.GetGenericArguments();
                if (methodGenerics.Length != genericArgs.Length)
                    continue;

                Type closedParameter;
                try { closedParameter = parameterType.GetGenericTypeDefinition().MakeGenericType(genericArgs); }
                catch { continue; }

                if (closedParameter == delegateType)
                {
                    MethodInfo closedMethod = m.MakeGenericMethod(genericArgs);
                    return (ShardManagedMethodCallback)closedMethod.Invoke(null, new object[] { body })!;
                }
            }
            else if (parameterType == delegateType)
            {
                return (ShardManagedMethodCallback)m.Invoke(null, new object[] { body })!;
            }
        }

        throw new NotSupportedException($"Delegate type '{delegateType}' is not supported.");
    }

    private static Type[] GetParameterTypes(Delegate del)
    {
        if (del == null)
            return Array.Empty<Type>();

        MethodInfo method = del.Method;
        ParameterInfo[] parameters = method.GetParameters();
        Type[] paramTypes = new Type[parameters.Length];
        for (int i = 0; i < parameters.Length; i++)
        {
            paramTypes[i] = parameters[i].ParameterType;
        }
        return paramTypes;
    }

    private static Type GetReturnType(Delegate del)
    {
        if (del == null)
            return typeof(void);

        return del.Method.ReturnType;
    }

    #endregion
}
