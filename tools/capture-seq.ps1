# Captura uma sequencia de frames do cliente MU (para comparar cenas).
# uso: powershell -File capture-seq.ps1 -Renderer modern -Times 10,14,18,22,30 -Prefix M
param(
    [string]$Renderer = "modern",
    [string]$Times = "10,14,18,22,30",
    [string]$Prefix = "seq",
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class CapS {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
  public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@

$exe = Join-Path $PSScriptRoot "..\Release\Main.exe"
$argsList = @("--renderer=$Renderer")
if ($Verbose) { $argsList += "--gfx-verbose" }
$p = Start-Process -FilePath $exe -ArgumentList $argsList -WorkingDirectory (Join-Path $PSScriptRoot "..\Release") -PassThru -WindowStyle Normal
Write-Output "PID=$($p.Id) renderer=$Renderer"

$hwnd = [IntPtr]::Zero
$targetPid = $p.Id
$schedule = $Times -split "," | ForEach-Object { [int]$_.Trim() } | Sort-Object
$start = Get-Date

function Save-Frame([IntPtr]$h, [string]$file) {
    $r = New-Object CapS+RECT
    [CapS]::GetWindowRect($h, [ref]$r) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap(($r.R - $r.L), ($r.B - $r.T))
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
    $bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Output "  salvo $file"
}

foreach ($t in $schedule) {
    # espera ate o instante t
    while (((Get-Date) - $start).TotalSeconds -lt $t) {
        Start-Sleep -Milliseconds 250
        if (-not (Get-Process -Id $p.Id -ErrorAction SilentlyContinue)) { Write-Output "processo encerrou"; break }
    }
    if (-not (Get-Process -Id $p.Id -ErrorAction SilentlyContinue)) { break }

    if ($hwnd -eq [IntPtr]::Zero) {
        $cb = [CapS+EnumWindowsProc]{ param($h,$l)
            $procId = 0; [CapS]::GetWindowThreadProcessId($h, [ref]$procId) | Out-Null
            if ($procId -eq $script:targetPid) {
                $sb = New-Object System.Text.StringBuilder 256
                [CapS]::GetWindowText($h,$sb,256) | Out-Null
                if ($sb.ToString() -like "*MU*") { $script:hwnd = $h }
            }
            return $true
        }
        $script:targetPid = $targetPid
        [CapS]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
    }
    if ($hwnd -ne [IntPtr]::Zero) {
        [CapS]::ShowWindow($hwnd, 9) | Out-Null
        [CapS]::SetForegroundWindow($hwnd) | Out-Null
        Save-Frame $hwnd (Join-Path $env:TEMP ("{0}_{1}_{2}s.png" -f $Prefix, $Renderer, $t))
    } else {
        Write-Output "  janela nao encontrada em ${t}s"
    }
}

Get-Process -Id $p.Id -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Write-Output "fim"
