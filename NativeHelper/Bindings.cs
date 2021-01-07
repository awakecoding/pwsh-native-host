using System;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;
using System.Runtime.InteropServices;

namespace NativeHelper
{
    public static class Bindings
    {
        [UnmanagedCallersOnly]
        public static void LoadAssemblyData(IntPtr ptr, int size)
        {
            byte[] bytes = new byte[size];
            Marshal.Copy(ptr, bytes, 0, size);
            Stream stream = new MemoryStream(bytes);
            AssemblyLoadContext.Default.LoadFromStream(stream);
        }

        [UnmanagedCallersOnly]
        public static void LoadAssemblyFile(IntPtr ptr)
        {
            string filename = Marshal.PtrToStringUTF8(ptr);
            AssemblyLoadContext.Default.LoadFromAssemblyPath(filename);
        }
    }
}
