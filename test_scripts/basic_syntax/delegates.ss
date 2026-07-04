using stdio;
using debug;

namespace hello_world;

public delegate GetIntegerDelegate(a: int) -> int;

public static func Main() -> void
{
    typedDelegateFromLambda: GetIntegerDelegate = lambda (a: int) -> int
    {
        return a + 10;
    };

    fabricatedDelegateFromLambda: delegate int(int) = lambda (a: int) -> int
    {
        return a + 10;
    };

    typedDelegateFromMethod: GetIntegerDelegate = TestMethod;

    fabricatedDelegateFromMethod: delegate int(int) = TestMethod;

    println(typedDelegateFromLambda(1));
    println(fabricatedDelegateFromLambda(2));
    println(typedDelegateFromMethod(3));
    println(fabricatedDelegateFromMethod(4));

    PrintGcInfo();
}

static func TestTypedDelegateParam(dlg: GetIntegerDelegate) -> void
{
    println(dlg(4));
}

static func TestTypedDelegateParam(dlg: delegate int(int)) -> void
{
    println(dlg(4));
}

static func TestMethod(a: int) -> int
{
    return a + 10;
}
