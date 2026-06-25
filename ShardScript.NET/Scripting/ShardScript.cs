using ShardScript.NET.Scripting;

namespace ShardScript.NET;

/// <summary>
/// Static entry points for compiling and running ShardScript code.
/// </summary>
public static class ShardScript
{
    /// <summary>
    /// Compiles and runs the given code, returning the result of the entry point expression.
    /// The code is automatically wrapped in a <c>Main</c> function whose return type matches <typeparamref name="T"/>.
    /// </summary>
    public static T Evaluate<T>(string code, ScriptOptions? options = null)
    {
        if (code == null)
            throw new ArgumentNullException(nameof(code));

        options ??= ScriptOptions.Default;
        string returnType = GetShardTypeName(typeof(T));
        string wrappedCode = $@"
namespace __eval;
public static class __EvalProgram
{{
    public static func Main() -> void {{ }}
    public static func GetResult() -> {returnType}
    {{
        return ({code});
    }}
}}";

        using var state = new ScriptState(options);
        try
        {
            state.Compile(wrappedCode);
        }
        catch (InvalidOperationException)
        {
            throw new InvalidOperationException($"Failed to compile expression. Diagnostics:\n{state.Diagnostics}");
        }
        return state.Call<T>("__EvalProgram.GetResult");
    }

    /// <summary>
    /// Compiles and runs the given code, returning a <see cref="ScriptState"/> that can be
    /// used for further interaction (calling methods, accessing globals, continuing with more code).
    /// </summary>
    public static ScriptState Run(string code, ScriptOptions? options = null, object? globals = null)
    {
        if (code == null)
            throw new ArgumentNullException(nameof(code));

        options ??= ScriptOptions.Default;
        var state = new ScriptState(options, globals);

        try
        {
            state.Compile(code).Run();
        }
        catch (InvalidOperationException ex)
        {
            throw new InvalidOperationException($"Failed to run script. {ex.Message}\nDiagnostics:\n{state.Diagnostics}");
        }

        return state;
    }

    private static string GetShardTypeName(Type type)
    {
        Type underlying = Nullable.GetUnderlyingType(type) ?? type;
        return underlying switch
        {
            _ when underlying == typeof(void) => "void",
            _ when underlying == typeof(bool) => "bool",
            _ when underlying == typeof(float) || underlying == typeof(double) => "double",
            _ when underlying == typeof(string) => "string",
            _ when underlying == typeof(char) => "char",
            _ => "int"
        };
    }
}
