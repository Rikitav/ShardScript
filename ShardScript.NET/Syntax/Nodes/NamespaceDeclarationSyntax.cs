using System.Runtime.InteropServices;

namespace ShardScript.Syntax.Nodes;

public sealed class NamespaceDeclarationSyntax
{
    public IntPtr Handle { get; }

    internal NamespaceDeclarationSyntax(IntPtr handle)
    {
        Handle = handle;
    }

    public string Name
    {
        get
        {
            int count = NativeMethods.Shard_GetNamespaceIdentifierCount(Handle);
            if (count == 0)
                return string.Empty;

            List<string> parts = new List<string>(count);
            for (int i = 0; i < count; i++)
            {
                IntPtr ptr = NativeMethods.Shard_GetNamespaceIdentifier(Handle, i);
                parts.Add(ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(ptr)!);
            }

            return string.Join(".", parts);
        }
    }

    private static class NativeMethods
    {
        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int Shard_GetNamespaceIdentifierCount(IntPtr ns);

        [DllImport(ShardScriptAPI.LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr Shard_GetNamespaceIdentifier(IntPtr ns, int index);
    }
}
