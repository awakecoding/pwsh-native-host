using System;
using System.Reflection;
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
            Assembly.Load(bytes);
        }

        [UnmanagedCallersOnly]
        public static void LoadAssemblyFile(IntPtr ptr)
        {
            string filename = Marshal.PtrToStringUTF8(ptr);
            Assembly.LoadFile(filename);
        }
    }
}
