using System.Reflection;
using System.Runtime.InteropServices;

namespace ShardScript;

internal static class ShardScriptAPI
{
    public const string LibraryName = "ShardScript";

    public static string Version
    {
        get
        {
            IntPtr ptr = NativeMethods.Shard_GetVersion();
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!;
        }
    }

    static ShardScriptAPI()
    {
        NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), DllImportResolver);
    }

    private static IntPtr DllImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
    {
        if (libraryName == LibraryName)
        {
            string actualName = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? "ShardScript.dll"
                : "libShardScript.so";

            if (NativeLibrary.TryLoad(actualName, assembly, searchPath, out IntPtr handle))
            {
                return handle;
            }
        }

        return IntPtr.Zero;
    }

    private static class NativeMethods
    {
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetVersion();
    }
}
