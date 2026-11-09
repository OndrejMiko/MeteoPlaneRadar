using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;

namespace MeteoChat.Windows;

internal static class TrayIcon
{
    [DllImport("user32.dll")] [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DestroyIcon(IntPtr icon);
    internal static Icon Create()
    {
        using var image = new Bitmap(64,64); using var graphics = Graphics.FromImage(image);
        graphics.SmoothingMode = SmoothingMode.AntiAlias;
        graphics.Clear(Color.Transparent);
        using var background = new SolidBrush(Color.FromArgb(16,25,35));
        using var ring = new Pen(Color.FromArgb(0,190,255),5);
        using var hands = new Pen(Color.White,5) { StartCap=LineCap.Round,EndCap=LineCap.Round };
        graphics.FillEllipse(background,3,3,58,58); graphics.DrawEllipse(ring,6,6,52,52);
        graphics.DrawLine(hands,32,32,32,17); graphics.DrawLine(hands,32,32,45,37);
        var handle = image.GetHicon();
        try { using var icon = Icon.FromHandle(handle); return (Icon)icon.Clone(); }
        finally { DestroyIcon(handle); }
    }
}
