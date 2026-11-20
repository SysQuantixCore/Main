# Captura de tela do cliente MU para comparacao legado/moderno.
#
# uso:
#   powershell -File capture.ps1 [-Seconds 70] [-Renderer modern|legacy]
#                             [-OutFile shot.png] [-EnterAfter 35] [-ClickXY "x,y"]
#
# Observacoes:
#   * -EnterAfter  envia a tecla ENTER ao cliente (o jogo so entra no
#                  mundo quando o jogador confirma a selecao de
#                  personagem). Sem isso a captura para na tela de
#                  selecao de personagem.
#   * -ClickXY     envia um duplo clique nas coordenadas de tela dadas.
param(
    [int]$Seconds = 70,
    [string]$Renderer = "modern",
    [string]$OutFile = "$env:TEMP\shot.png",
    [int]$EnterAfter = -1,
    [int]$EnterEvery = 0,
    [string]$ClickXY = ""
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class Cap {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
  public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@

$exe = Join-Path $PSScriptRoot "..\Release\Main.exe"
$p = Start-Process -FilePath $exe -ArgumentList "--renderer=$Renderer" -WorkingDirectory (Join-Path $PSScriptRoot "..\Release") -PassThru -WindowStyle Normal
Write-Output "PID=$($p.Id) renderer=$Renderer aguardando $Seconds s..."

$hwnd = [IntPtr]::Zero
$targetPid = $p.Id
for ($i = 0; $i -lt $Seconds; $i += 5) {
    Start-Sleep -Seconds 5
    if (-not (Get-Process -Id $p.Id -ErrorAction SilentlyContinue)) { Write-Output "processo encerrou"; break }
    if ($hwnd -eq [IntPtr]::Zero) {
        $cb = [Cap+EnumWindowsProc]{ param($h,$l)
            $procId = 0; [Cap]::GetWindowThreadProcessId($h, [ref]$procId) | Out-Null
            if ($procId -eq $script:targetPid) {
                $sb = New-Object System.Text.StringBuilder 256
                [Cap]::GetWindowText($h,$sb,256) | Out-Null
                if ($sb.ToString() -like "*MU*") { $script:hwnd = $h }
            }
            return $true
        }
        $script:targetPid = $targetPid
        [Cap]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
    }
}

if ($hwnd -ne [IntPtr]::Zero) {
    [Cap]::ShowWindow($hwnd, 9) | Out-Null
    [Cap]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Seconds 2

    # --- entrada sintetica (para tornar o teste deterministico) ---
    if ($EnterAfter -ge 0 -or $ClickXY -ne "") {
        $r0 = New-Object Cap+RECT
        [Cap]::GetWindowRect($hwnd, [ref]$r0) | Out-Null
        Write-Output ("janela em {0},{1} {2}x{3}" -f $r0.L, $r0.T, ($r0.R-$r0.L), ($r0.B-$r0.T))

        if ($ClickXY -ne "") {
            $parts = $ClickXY -split "[,;]"
            $cx = [int]$parts[0]
            $cy = [int]$parts[1]
            [Cap]::SetCursorPos($cx, $cy) | Out-Null
            Start-Sleep -Milliseconds 400
            # duplo clique
            foreach ($k in 1..2) {
                [Cap]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)  # LEFTDOWN
                [Cap]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)  # LEFTUP
                Start-Sleep -Milliseconds 60
            }
            Write-Output "duplo clique enviado em $cx,$cy"
            Start-Sleep -Seconds 3
        }

        if ($EnterAfter -ge 0) {
            [Cap]::keybd_event(0x0D, 0, 0, [UIntPtr]::Zero)       # VK_RETURN down
            Start-Sleep -Milliseconds 80
            [Cap]::keybd_event(0x0D, 0, 2, [UIntPtr]::Zero)       # VK_RETURN up
            Write-Output "ENTER enviado"
            Start-Sleep -Seconds 4
        }

        # ENTER repetido: cobre o caso em que o cliente so chega na tela de
        # selecao de personagem depois do tempo de carregar.
        if ($EnterEvery -gt 0) {
            for ($k = 0; $k -lt $EnterEvery; $k++) {
                [Cap]::keybd_event(0x0D, 0, 0, [UIntPtr]::Zero)
                Start-Sleep -Milliseconds 80
                [Cap]::keybd_event(0x0D, 0, 2, [UIntPtr]::Zero)
                Start-Sleep -Seconds 5
            }
            Write-Output "ENTER enviado $EnterEvery vezes"
            Start-Sleep -Seconds 3
        }
    }

    $r = New-Object Cap+RECT
    [Cap]::GetWindowRect($hwnd, [ref]$r) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap(($r.R - $r.L), ($r.B - $r.T))
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
    $bmp.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Output "captura salva: $OutFile"
} else {
    Write-Output "janela nao encontrada"
}

Get-Process -Id $p.Id -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
