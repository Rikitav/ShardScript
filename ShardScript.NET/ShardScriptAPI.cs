using System.Runtime.InteropServices;

namespace ShardScript;

internal static class ShardScriptAPI
{
    public const string LibraryName = "ShardScript.dll";

    public static string Version
    {
        get
        {
            IntPtr ptr = NativeMethods.Shard_GetVersion();
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    private static class NativeMethods
    {
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetVersion();
    }
}
