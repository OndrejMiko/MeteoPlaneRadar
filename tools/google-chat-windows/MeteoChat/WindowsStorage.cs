using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using MeteoChat.Core;
using Microsoft.Win32;

namespace MeteoChat.Windows;

// DPAPI defaults to the current Windows user; no machine-wide access or UI.
internal sealed class UserProtector : IProtector
{
    [StructLayout(LayoutKind.Sequential)] private struct Blob { public int Size; public IntPtr Data; }
    [DllImport("crypt32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CryptProtectData(ref Blob input, string? description, IntPtr entropy,
        IntPtr reserved, IntPtr prompt, int flags, out Blob output);
    [DllImport("crypt32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CryptUnprotectData(ref Blob input, IntPtr description, IntPtr entropy,
        IntPtr reserved, IntPtr prompt, int flags, out Blob output);
    [DllImport("kernel32.dll")] private static extern IntPtr LocalFree(IntPtr memory);
    public byte[] Protect(byte[] bytes) => Transform(bytes, true);
    public byte[] Unprotect(byte[] bytes) => Transform(bytes, false);
    private static byte[] Transform(byte[] bytes, bool encrypt)
    {
        var input = new Blob { Size = bytes.Length, Data = Marshal.AllocHGlobal(bytes.Length) };
        Blob output = default;
        try
        {
            Marshal.Copy(bytes, 0, input.Data, bytes.Length);
            bool ok = encrypt ? CryptProtectData(ref input, "Meteo Chat", IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, 1, out output)
                : CryptUnprotectData(ref input, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, 1, out output);
            if (!ok) throw new Win32Exception(Marshal.GetLastWin32Error());
            var result = new byte[output.Size]; Marshal.Copy(output.Data, result, 0, result.Length); return result;
        }
        finally
        {
            // Wipe unmanaged plaintext before freeing it.
            for (var i = 0; i < input.Size; ++i) Marshal.WriteByte(input.Data, i, 0);
            Marshal.FreeHGlobal(input.Data);
            if (output.Data != IntPtr.Zero)
            {
                for (var i = 0; i < output.Size; ++i) Marshal.WriteByte(output.Data, i, 0);
                LocalFree(output.Data);
            }
        }
    }
}
internal static class Startup
{
    private const string Key = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private const string Name = "MeteoChat";
    private static string Command => "\"" + Application.ExecutablePath + "\" --background";
    public static bool Enabled { get { using var key = Registry.CurrentUser.OpenSubKey(Key); return key?.GetValue(Name) as string == Command; } }
    public static void Set(bool enabled)
    {
        using var key = Registry.CurrentUser.CreateSubKey(Key);
        if (enabled) key.SetValue(Name, Command, RegistryValueKind.String);
        else key.DeleteValue(Name, false);
    }
}
