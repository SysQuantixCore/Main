# Captura e entrada sintetica em um cliente MU JA em execucao.
#
# Diferente de capture.ps1/capture-seq.ps1, este script nao inicia nem encerra
# o processo. Isso evita dois problemas recorrentes na automacao:
#   * o UAC intermitente do Start-Process;
#   * a janela do cliente ficar atras de outras janelas (ela e elevada e um
#     processo comum nao consegue traze-la para frente).
#
# Para capturar mesmo com a janela ocluida, usa PrintWindow com
# PW_RENDERFULLCONTENT (0x2), que no Windows 10/11 le o conteudo composto pelo
# DWM. Se o driver/janela nao suportar, cai para CopyFromScreen.
#
# uso:
#   powershell -File shot-now.ps1 -OutFile shot.png
#   powershell -File shot-now.ps1 -Times "30,60,90" -Prefix noria
#   powershell -File shot-now.ps1 -ClickXY "850,470" -DblClick -Enter -Wait 10
#
# ATENCAO com -AltTab: em um jogo fullscreen, o Alt+Tab e recebido como teclas
# pelo cliente (abre o menu dentro do jogo). Use uma vez, nunca em loop -
# repeti-lo fecha o cliente. Prefira iniciar o cliente e capturar no mesmo
# comando do shell (mantem o processo vivo).
param(
    [string]$OutFile = "$env:TEMP\shot_now.png",
    [switch]$Enter,
    [int]$Wait = 6,
    [string]$Times = "",
    [string]$Prefix = "now",
    [string]$ClickXY = "",
    [switch]$DblClick,
    [switch]$ScreenCopy,
    [switch]$ClearDesktop,
    [switch]$AltTab,
    [string]$Keys = ""
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class Shot {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
  public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@

$procs = @(Get-Process Main -ErrorAction SilentlyContinue)
if ($procs.Count -eq 0) { Write-Output "Main.exe nao esta rodando"; exit 1 }
$pids = @($procs | ForEach-Object { $_.Id })
Write-Output ("Main.exe pids: {0}" -f ($pids -join ","))

$script:hwnd = [IntPtr]::Zero
$script:pids = $pids
$cb = [Shot+EnumWindowsProc]{ param($h,$l)
    $procId = 0
    [Shot]::GetWindowThreadProcessId($h, [ref]$procId) | Out-Null
    if ($script:pids -contains [int]$procId) {
        $sb = New-Object System.Text.StringBuilder 256
        [Shot]::GetWindowText($h,$sb,256) | Out-Null
        if ($sb.ToString() -like "*MU*") { $script:hwnd = $h }
    }
    return $true
}
[Shot]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null

if ($script:hwnd -eq [IntPtr]::Zero) { Write-Output "janela nao encontrada"; exit 1 }
$dbgPid = 0
[Shot]::GetWindowThreadProcessId($script:hwnd, [ref]$dbgPid) | Out-Null
$dbgTitle = New-Object System.Text.StringBuilder 256
[Shot]::GetWindowText($script:hwnd, $dbgTitle, 256) | Out-Null
Write-Output ("janela encontrada: hwnd={0} pid={1} titulo='{2}'" -f $script:hwnd, $dbgPid, $dbgTitle.ToString())

function TakeShot([string]$path) {
    $r = New-Object Shot+RECT
    [Shot]::GetWindowRect($script:hwnd, [ref]$r) | Out-Null
    $w = $r.R - $r.L
    $h = $r.B - $r.T
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)

    $ok = $false
    if (-not $ScreenCopy) {
        $hdc = $g.GetHdc()
        # PW_RENDERFULLCONTENT = 0x2 (conteudo composto pelo DWM)
        $ok = [Shot]::PrintWindow($script:hwnd, $hdc, 2)
        if (-not $ok) { $ok = [Shot]::PrintWindow($script:hwnd, $hdc, 0) }
        $g.ReleaseHdc($hdc)

        if ($ok) {
            # PrintWindow pode devolver uma imagem preta quando o driver nao
            # expoe o conteudo; nesse caso volta para a copia de tela.
            $sample = 0
            for ($y = 40; $y -lt $h; $y += 120) {
                for ($x = 40; $x -lt $w; $x += 160) {
                    $c = $bmp.GetPixel($x, $y)
                    $sample += $c.R + $c.G + $c.B
                }
            }
            if ($sample -eq 0) { $ok = $false }
        }
    }

    if (-not $ok) {
        $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
    }

    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Output ("salvo {0} (fonte={1})" -f $path, $(if ($ok) { "PrintWindow" } else { "CopyFromScreen" }))
}

if ($AltTab) {
    [Shot]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero)       # ALT down
    Start-Sleep -Milliseconds 60
    [Shot]::keybd_event(0x09, 0, 0, [UIntPtr]::Zero)       # TAB down
    Start-Sleep -Milliseconds 60
    [Shot]::keybd_event(0x09, 0, 2, [UIntPtr]::Zero)       # TAB up
    Start-Sleep -Milliseconds 60
    [Shot]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)       # ALT up
    Write-Output "ALT+TAB enviado"
    Start-Sleep -Seconds 2
}
if ($Keys -ne "") {
    foreach ($k in ($Keys -split ",")) {
        $vk = switch ($k.Trim().ToUpper()) {
            "I"      { 0x49 }
            "C"      { 0x43 }
            "M"      { 0x4D }
            "ESC"    { 0x1B }
            "F12"    { 0x7B }
            default  { 0 }
        }
        if ($vk -ne 0) {
            [Shot]::keybd_event([byte]$vk, 0, 0, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 80
            [Shot]::keybd_event([byte]$vk, 0, 2, [UIntPtr]::Zero)
            Write-Output ("tecla enviada: " + $k)
            Start-Sleep -Seconds 2
        }
    }
}
[Shot]::ShowWindow($script:hwnd, 9) | Out-Null
[Shot]::SetForegroundWindow($script:hwnd) | Out-Null
Start-Sleep -Seconds 1

if ($Times -ne "") {
    $prev = 0
    foreach ($t in ($Times -split ",")) {
        $target = [int]$t
        $delta = $target - $prev
        if ($delta -gt 0) { Start-Sleep -Seconds $delta }
        $prev = $target
        TakeShot ("$env:TEMP\{0}_{1}s.png" -f $Prefix, $target)
    }
    exit 0
}

TakeShot $OutFile

if ($ClickXY -ne "") {
    $r2 = New-Object Shot+RECT
    [Shot]::GetWindowRect($script:hwnd, [ref]$r2) | Out-Null
    $parts = $ClickXY -split "[,;]"
    $cx = $r2.L + [int]$parts[0]
    $cy = $r2.T + [int]$parts[1]
    [Shot]::SetCursorPos($cx, $cy) | Out-Null
    Start-Sleep -Milliseconds 300
    $times = 1
    if ($DblClick) { $times = 2 }
    foreach ($k in 1..$times) {
        [Shot]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
        [Shot]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 120
    }
    Write-Output "clique enviado em $cx,$cy"
    Start-Sleep -Seconds 2
}

if ($Enter) {
    [Shot]::keybd_event(0x0D, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [Shot]::keybd_event(0x0D, 0, 2, [UIntPtr]::Zero)
    Write-Output "ENTER enviado"
    Start-Sleep -Seconds $Wait
    $after = [System.IO.Path]::ChangeExtension($OutFile, $null)
    TakeShot ("{0}_after.png" -f $after.TrimEnd('.'))
}
