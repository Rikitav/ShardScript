using stdio;

namespace hello_world;

public class Program
{
    public static func Add(a: int, b: int) -> int
    {
        return a + b;
    }
}

public static func Main() -> void
{
    x := 21;
    y := Program.Add(x + 46);
    println(y); // 67
}
