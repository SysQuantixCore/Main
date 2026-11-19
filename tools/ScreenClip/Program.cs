using System;
using System.Diagnostics;

namespace ScreenClip
{
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            try
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = "explorer.exe",
                    Arguments = "ms-screenclip:",
                    UseShellExecute = true
                });
            }
            catch
            {
                // Não exibe uma janela de console nem interrompe o Windows.
            }
        }
    }
}
