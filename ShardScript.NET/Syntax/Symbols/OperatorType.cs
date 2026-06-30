namespace ShardScript.Syntax.Symbols;

/// <summary>
/// Specifies the type of operator overload.
/// </summary>
public enum OperatorType
{
    // Unary operators
    Implicit = 1,
    Explicit = 2,
    UnaryPlus = 3,
    UnaryMinus = 4,
    LogicalNot = 5,
    BitwiseNot = 6,
    Increment = 7,
    Decrement = 8,

    // Binary operators
    Add = 10,
    Subtract = 11,
    Multiply = 12,
    Divide = 13,
    Modulo = 14,
    BitwiseAnd = 15,
    BitwiseOr = 16,
    ExclusiveOr = 17,
    LeftShift = 18,
    RightShift = 19,

    // Comparison operators
    Equals = 20,
    NotEquals = 21,
    GreaterThan = 22,
    LessThan = 23,
    GreaterThanOrEqual = 24,
    LessThanOrEqual = 25,

    // Compound assignment
    AddAssign = 30,
    SubtractAssign = 31,
    MultiplyAssign = 32,
    DivideAssign = 33,
    ModuloAssign = 34
}
