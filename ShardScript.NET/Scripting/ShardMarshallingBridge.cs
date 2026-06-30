namespace ShardScript.Scripting;

using ShardScript.Runtime;
using ShardScript.Syntax;
using ShardScript.Syntax.Builders;
using ShardScript.Syntax.Symbols;
using System.Runtime.InteropServices;

// Forward declare the native callback delegate
delegate IntPtr ShardManagedMethodCallbackNative(IntPtr method, IntPtr args, int argsCount, IntPtr userData, IntPtr collector);

/// <summary>
/// Bridges C# delegates to native ShardScript callbacks with automatic marshalling.
/// </summary>
public static class ShardMarshallingBridge
{
    /// <summary>
    /// Creates a marshalled callback for a parameterless function returning a value.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<TResult>(Func<TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 0)
                throw new InvalidOperationException($"Callback expects 0 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            TResult result = function();
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with one parameter.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T, TResult>(Func<T, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 1)
                throw new InvalidOperationException($"Callback expects 1 argument but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            ObjectInstance arg0 = new ObjectInstance(args[0]);
            T typedArg = ShardMarshaller.FromObjectInstance<T>(arg0);
            TResult result = function(typedArg);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with two parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, TResult>(Func<T1, T2, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 2)
                throw new InvalidOperationException($"Callback expects 2 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            ObjectInstance arg0 = new ObjectInstance(args[0]);
            ObjectInstance arg1 = new ObjectInstance(args[1]);
            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(arg0);
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(arg1);
            TResult result = function(typedArg0, typedArg1);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with three parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, TResult>(Func<T1, T2, T3, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 3)
                throw new InvalidOperationException($"Callback expects 3 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            TResult result = function(typedArg0, typedArg1, typedArg2);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with four parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, TResult>(Func<T1, T2, T3, T4, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 4)
                throw new InvalidOperationException($"Callback expects 4 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            TResult result = function(typedArg0, typedArg1, typedArg2, typedArg3);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with five parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5, TResult>(Func<T1, T2, T3, T4, T5, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 5)
                throw new InvalidOperationException($"Callback expects 5 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            TResult result = function(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with six parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5, T6, TResult>(Func<T1, T2, T3, T4, T5, T6, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 6)
                throw new InvalidOperationException($"Callback expects 6 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            T6 typedArg5 = ShardMarshaller.FromObjectInstance<T6>(new ObjectInstance(args[5]));
            TResult result = function(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4, typedArg5);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with seven parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5, T6, T7, TResult>(Func<T1, T2, T3, T4, T5, T6, T7, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 7)
                throw new InvalidOperationException($"Callback expects 7 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            T6 typedArg5 = ShardMarshaller.FromObjectInstance<T6>(new ObjectInstance(args[5]));
            T7 typedArg6 = ShardMarshaller.FromObjectInstance<T7>(new ObjectInstance(args[6]));
            TResult result = function(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4, typedArg5, typedArg6);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function with eight parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5, T6, T7, T8, TResult>(Func<T1, T2, T3, T4, T5, T6, T7, T8, TResult> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 8)
                throw new InvalidOperationException($"Callback expects 8 arguments but received {argsCount}.");

            GarbageCollector gc = new GarbageCollector(collector);
            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            T6 typedArg5 = ShardMarshaller.FromObjectInstance<T6>(new ObjectInstance(args[5]));
            T7 typedArg6 = ShardMarshaller.FromObjectInstance<T7>(new ObjectInstance(args[6]));
            T8 typedArg7 = ShardMarshaller.FromObjectInstance<T8>(new ObjectInstance(args[7]));
            TResult result = function(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4, typedArg5, typedArg6, typedArg7);
            return ShardMarshaller.ToObjectInstance(result, gc).Handle;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action (void return) with no parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback(Action action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 0)
                throw new InvalidOperationException($"Callback expects 0 arguments but received {argsCount}.");

            action();
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with one parameter.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T>(Action<T> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 1)
                throw new InvalidOperationException($"Callback expects 1 argument but received {argsCount}.");

            T typedArg = ShardMarshaller.FromObjectInstance<T>(new ObjectInstance(args[0]));
            action(typedArg);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with two parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2>(Action<T1, T2> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 2)
                throw new InvalidOperationException($"Callback expects 2 arguments but received {argsCount}.");

            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            action(typedArg0, typedArg1);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with three parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3>(Action<T1, T2, T3> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 3)
                throw new InvalidOperationException($"Callback expects 3 arguments but received {argsCount}.");

            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            action(typedArg0, typedArg1, typedArg2);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with four parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4>(Action<T1, T2, T3, T4> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 4)
                throw new InvalidOperationException($"Callback expects 4 arguments but received {argsCount}.");

            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            action(typedArg0, typedArg1, typedArg2, typedArg3);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with five parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5>(Action<T1, T2, T3, T4, T5> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 5)
                throw new InvalidOperationException($"Callback expects 5 arguments but received {argsCount}.");

            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            action(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with six parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5, T6>(Action<T1, T2, T3, T4, T5, T6> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 6)
                throw new InvalidOperationException($"Callback expects 6 arguments but received {argsCount}.");

            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            T6 typedArg5 = ShardMarshaller.FromObjectInstance<T6>(new ObjectInstance(args[5]));
            action(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4, typedArg5);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with seven parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5, T6, T7>(Action<T1, T2, T3, T4, T5, T6, T7> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 7)
                throw new InvalidOperationException($"Callback expects 7 arguments but received {argsCount}.");

            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            T6 typedArg5 = ShardMarshaller.FromObjectInstance<T6>(new ObjectInstance(args[5]));
            T7 typedArg6 = ShardMarshaller.FromObjectInstance<T7>(new ObjectInstance(args[6]));
            action(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4, typedArg5, typedArg6);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for an action with eight parameters.
    /// </summary>
    public static ShardManagedMethodCallback CreateCallback<T1, T2, T3, T4, T5, T6, T7, T8>(Action<T1, T2, T3, T4, T5, T6, T7, T8> action)
    {
        if (action == null)
            throw new ArgumentNullException(nameof(action));

        return (method, args, argsCount, userData, collector) =>
        {
            if (argsCount != 8)
                throw new InvalidOperationException($"Callback expects 8 arguments but received {argsCount}.");

            T1 typedArg0 = ShardMarshaller.FromObjectInstance<T1>(new ObjectInstance(args[0]));
            T2 typedArg1 = ShardMarshaller.FromObjectInstance<T2>(new ObjectInstance(args[1]));
            T3 typedArg2 = ShardMarshaller.FromObjectInstance<T3>(new ObjectInstance(args[2]));
            T4 typedArg3 = ShardMarshaller.FromObjectInstance<T4>(new ObjectInstance(args[3]));
            T5 typedArg4 = ShardMarshaller.FromObjectInstance<T5>(new ObjectInstance(args[4]));
            T6 typedArg5 = ShardMarshaller.FromObjectInstance<T6>(new ObjectInstance(args[5]));
            T7 typedArg6 = ShardMarshaller.FromObjectInstance<T7>(new ObjectInstance(args[6]));
            T8 typedArg7 = ShardMarshaller.FromObjectInstance<T8>(new ObjectInstance(args[7]));
            action(typedArg0, typedArg1, typedArg2, typedArg3, typedArg4, typedArg5, typedArg6, typedArg7);
            return IntPtr.Zero;
        };
    }

    /// <summary>
    /// Creates a marshalled callback for a function that works with ObjectInstance directly (low-level control).
    /// </summary>
    public static ShardManagedMethodCallback CreateRawCallback(Func<ObjectInstance[], ObjectInstance> function)
    {
        if (function == null)
            throw new ArgumentNullException(nameof(function));

        return (method, args, argsCount, userData, collector) =>
        {
            ObjectInstance[] instances = new ObjectInstance[argsCount];
            for (int i = 0; i < argsCount; i++)
                instances[i] = new ObjectInstance(args[i]);

            ObjectInstance result = function(instances);
            return result.Handle;
        };
    }
}
