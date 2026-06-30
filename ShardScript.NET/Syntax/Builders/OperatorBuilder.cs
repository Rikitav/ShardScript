using ShardScript.Application;
using ShardScript.Runtime;
using ShardScript.Scripting;
using ShardScript.Syntax.Symbols;

namespace ShardScript.Syntax.Builders;

/// <summary>
/// Builder for creating operator overload symbols.
/// Operators are implemented as specially named methods.
/// </summary>
public sealed class OperatorBuilder
{
    /// <summary>
    /// Gets the compilation context.
    /// </summary>
    public CompilationContext Context { get; }

    /// <summary>
    /// Gets the operator type.
    /// </summary>
    public OperatorType Operator { get; }

    /// <summary>
    /// Gets the return type.
    /// </summary>
    public TypeSymbol ReturnType { get; }

    /// <summary>
    /// Gets the parent type.
    /// </summary>
    public TypeSymbol ParentType { get; }

    private readonly Func<ObjectInstance, ObjectInstance, ObjectInstance>? _binaryCallback;
    private readonly Func<ObjectInstance, ObjectInstance>? _unaryCallback;

    internal OperatorBuilder(
        CompilationContext context,
        TypeSymbol parentType,
        OperatorType op,
        TypeSymbol returnType,
        Func<ObjectInstance, ObjectInstance, ObjectInstance>? binaryCallback = null,
        Func<ObjectInstance, ObjectInstance>? unaryCallback = null)
    {
        Context = context ?? throw new ArgumentNullException(nameof(context));
        ParentType = parentType ?? throw new ArgumentNullException(nameof(parentType));
        Operator = op;
        ReturnType = returnType ?? throw new ArgumentNullException(nameof(returnType));
        _binaryCallback = binaryCallback;
        _unaryCallback = unaryCallback;
    }

    /// <summary>
    /// Builds the operator method.
    /// </summary>
    public MethodSymbol Build()
    {
        string methodName = GetOperatorMethodName(Operator);
        bool isUnary = IsUnaryOperator(Operator);

        MethodBuilder builder = new MethodBuilder(Context, ParentType, methodName, ReturnType);

        if (IsStaticOperator(Operator))
            builder.Static();
        else
            builder.Instance();

        builder.Public();

        // Add parameters based on operator type
        if (isUnary)
        {
            // Unary operators take one parameter
            builder.Parameter("operand", SymbolBuilder.Primitive(Context, PrimitiveType.Any));
        }
        else
        {
            // Binary operators take two parameters
            builder.Parameter("left", SymbolBuilder.Primitive(Context, PrimitiveType.Any));
            builder.Parameter("right", SymbolBuilder.Primitive(Context, PrimitiveType.Any));
        }

        // Create the callback
        ShardManagedMethodCallback callback = isUnary
            ? CreateUnaryCallback()
            : CreateBinaryCallback();

        builder.Callback(callback);
        return builder.Symbol;
    }

    private ShardManagedMethodCallback CreateBinaryCallback()
    {
        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 2)
                throw new InvalidOperationException($"Binary operator expects 2 arguments but received {argsCount}.");

            ObjectInstance left = new ObjectInstance(args[0]);
            ObjectInstance right = new ObjectInstance(args[1]);
            ObjectInstance result = _binaryCallback!(left, right);
            return result.Handle;
        };
    }

    private ShardManagedMethodCallback CreateUnaryCallback()
    {
        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 1)
                throw new InvalidOperationException($"Unary operator expects 1 argument but received {argsCount}.");

            ObjectInstance operand = new ObjectInstance(args[0]);
            ObjectInstance result = _unaryCallback!(operand);
            return result.Handle;
        };
    }

    private static string GetOperatorMethodName(OperatorType op)
    {
        return op switch
        {
            OperatorType.Add => "op_Add",
            OperatorType.Subtract => "op_Subtract",
            OperatorType.Multiply => "op_Multiply",
            OperatorType.Divide => "op_Divide",
            OperatorType.Modulo => "op_Modulus",
            OperatorType.BitwiseAnd => "op_BitwiseAnd",
            OperatorType.BitwiseOr => "op_BitwiseOr",
            OperatorType.ExclusiveOr => "op_ExclusiveOr",
            OperatorType.LeftShift => "op_LeftShift",
            OperatorType.RightShift => "op_RightShift",
            OperatorType.Equals => "op_Equality",
            OperatorType.NotEquals => "op_Inequality",
            OperatorType.GreaterThan => "op_GreaterThan",
            OperatorType.LessThan => "op_LessThan",
            OperatorType.GreaterThanOrEqual => "op_GreaterThanOrEqual",
            OperatorType.LessThanOrEqual => "op_LessThanOrEqual",
            OperatorType.Implicit => "op_Implicit",
            OperatorType.Explicit => "op_Explicit",
            OperatorType.UnaryPlus => "op_UnaryPlus",
            OperatorType.UnaryMinus => "op_UnaryNegation",
            OperatorType.LogicalNot => "op_LogicalNot",
            OperatorType.BitwiseNot => "op_OnesComplement",
            OperatorType.Increment => "op_Increment",
            OperatorType.Decrement => "op_Decrement",
            _ => throw new NotSupportedException($"Operator type '{op}' is not supported.")
        };
    }

    private static bool IsUnaryOperator(OperatorType op)
    {
        return op is OperatorType.Implicit or OperatorType.Explicit
            or OperatorType.UnaryPlus or OperatorType.UnaryMinus
            or OperatorType.LogicalNot or OperatorType.BitwiseNot
            or OperatorType.Increment or OperatorType.Decrement;
    }

    private static bool IsStaticOperator(OperatorType op)
    {
        // Most operators are static, except for a few special cases
        return true;
    }
}
