using System.Runtime.InteropServices;
using System.Text;

namespace ShardScript.NET.Application;

// TODO: Error codes
/*
public enum ShardErrorCode : int
{
    Success = 0,
}
*/

public class ShardEngineException(string? message, Exception? innerException) : Exception(message, innerException)
{
    public static string GetLastErrorMessage()
    {
        int length = NativeMethods.Shard_GetLastError(null, 0);
        if (length <= 0)
            return string.Empty;

        StringBuilder sb = new StringBuilder(length + 1);
        NativeMethods.Shard_GetLastError(sb, sb.Capacity);
        return sb.ToString();
    }

    public static void ThrowIfError(int result)
    {
        if (result != 0)
            throw new InvalidOperationException($"ShardScript API error: {GetLastErrorMessage()}");
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetLastError([Out] StringBuilder? buffer, int bufferLen);
    }
}
