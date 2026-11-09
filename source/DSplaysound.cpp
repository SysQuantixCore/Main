///////////////////////////////////////////////////////////////////////////////
// DirectSound
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// File: DSPlaySound.cpp
//
// Desc: Gerenciamento de sons com DirectSound. Cada som é um buffer estático
//       carregado de um arquivo WAVE, com N canais (buffers) para reprodução
//       simultânea e suporte opcional a som 3D.
//
// Copyright (c) 1999 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include <objbase.h>
#include <mmreg.h>
#include <dsound.h>
#include <cstring>
#include <vector>
#include "DSwaveio.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "DSPlaySound.h"
#include "./Utilities/Log/ErrorReport.h"
#include "./Utilities/Log/muConsoleDebug.h"

//-----------------------------------------------------------------------------
// Estado global
//-----------------------------------------------------------------------------
bool                    g_EnableSound = false;
bool                    g_Enable3DSound = false;
int                     SoundLoadCount = 0;
long                    g_MasterVolume = 0L;

LPDIRECTSOUND           g_lpDS = NULL;
LPDIRECTSOUND3DLISTENER g_lpDS3DListener = NULL;

LPDIRECTSOUNDBUFFER     g_lpDSBuffer[MAX_BUFFER][MAX_CHANNEL];
LPDIRECTSOUND3DBUFFER   g_lpDS3DBuffer[MAX_BUFFER][MAX_CHANNEL];

OBJECT* Object3DSound[MAX_BUFFER][MAX_CHANNEL];
char                    BufferName[MAX_BUFFER][MAX_PATH];
int                     BufferChannel[MAX_BUFFER];
int                     MaxBufferChannel[MAX_BUFFER];
bool                    Enable3DSound[MAX_BUFFER];

static bool             g_ComInitialized = false;

extern vec3_t CameraAngle;

//-----------------------------------------------------------------------------
// Helpers internos
//-----------------------------------------------------------------------------
static inline bool IsSoundReady()
{
    return g_EnableSound && g_lpDS != NULL;
}

static inline bool IsValidBuffer(int Buffer)
{
    return Buffer >= 0 && Buffer < MAX_BUFFER;
}

static inline bool IsValidChannel(int Channel)
{
    return Channel >= 0 && Channel < MAX_CHANNEL;
}

static bool IsChannelPlaying(int Buffer, int Channel)
{
    LPDIRECTSOUNDBUFFER pBuf = g_lpDSBuffer[Buffer][Channel];
    if (pBuf == NULL)
        return false;

    DWORD dwStatus = 0;
    if (FAILED(pBuf->GetStatus(&dwStatus)))
        return false;

    return (dwStatus & DSBSTATUS_PLAYING) != 0;
}

// Libera todos os canais de um buffer (inclusive interfaces 3D).
static void ReleaseChannels(int Buffer)
{
    for (int ch = 0; ch < MAX_CHANNEL; ++ch)
    {
        if (g_lpDSBuffer[Buffer][ch] != NULL)
            g_lpDSBuffer[Buffer][ch]->Stop();

        SAFE_RELEASE(g_lpDS3DBuffer[Buffer][ch]);
        SAFE_RELEASE(g_lpDSBuffer[Buffer][ch]);
        Object3DSound[Buffer][ch] = NULL;
    }
}

// Lê o WAVE inteiro (cabeçalho + dados) para a memória.
static HRESULT LoadWaveData(const char* strFileName, WAVEFORMATEX& wfx, std::vector<BYTE>& data)
{
    waveIO wave(INPUT);

    // const_cast: mantém compatibilidade caso LoadWaveHeader receba char*.
    if (!wave.LoadWaveHeader(const_cast<char*>(strFileName)))
        return E_FAIL;

    wfx = wave.GetWaveFormatEx();

    DWORD dwSize = wave.GetDataSize();
    if (dwSize == 0)
        return E_FAIL;

    data.resize(dwSize);

    HRESULT hr = wave.ReadWaveData(reinterpret_cast<char*>(data.data()), dwSize);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

// Copia os dados para um buffer já criado, travando e destravando corretamente.
static HRESULT FillSoundBuffer(LPDIRECTSOUNDBUFFER pBuf, const BYTE* pData, DWORD dwBytes)
{
    LPVOID p1 = NULL, p2 = NULL;
    DWORD  s1 = 0, s2 = 0;

    HRESULT hr = pBuf->Lock(0, dwBytes, &p1, &s1, &p2, &s2, 0);
    if (FAILED(hr))
        return hr;

    memcpy(p1, pData, s1);
    if (p2 != NULL && s2 > 0)
        memcpy(p2, pData + s1, s2);

    pBuf->Unlock(p1, s1, p2, s2);
    return S_OK;
}

// Cria um canal (buffer estático) com os dados fornecidos.
static HRESULT CreateChannel(int Buffer, int Channel, const WAVEFORMATEX& wfx,
    const std::vector<BYTE>& data, bool bEnable3D)
{
    DSBUFFERDESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwBufferBytes = static_cast<DWORD>(data.size());
    desc.lpwfxFormat = const_cast<WAVEFORMATEX*>(&wfx);

    const DWORD dwBase = DSBCAPS_STATIC |
        (bEnable3D ? (DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME)
            : (DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY));

    HRESULT hr = E_FAIL;
    LPDIRECTSOUNDBUFFER pBuf = NULL;

    if (bEnable3D)
    {
        // Tenta hardware; se não houver suporte, cai para software.
        desc.dwFlags = dwBase | DSBCAPS_LOCHARDWARE;
        hr = g_lpDS->CreateSoundBuffer(&desc, &pBuf, NULL);
        if (FAILED(hr))
        {
            desc.dwFlags = dwBase | DSBCAPS_LOCSOFTWARE;
            hr = g_lpDS->CreateSoundBuffer(&desc, &pBuf, NULL);
        }
    }
    else
    {
        desc.dwFlags = dwBase;
        hr = g_lpDS->CreateSoundBuffer(&desc, &pBuf, NULL);
    }

    if (FAILED(hr))
        return hr;

    // Atribui já aqui: em caso de erro, o chamador faz a limpeza via ReleaseChannels.
    g_lpDSBuffer[Buffer][Channel] = pBuf;

    if (bEnable3D)
    {
        hr = pBuf->QueryInterface(IID_IDirectSound3DBuffer,
            (void**)&g_lpDS3DBuffer[Buffer][Channel]);
        if (FAILED(hr))
            return hr;

        // Posição inicial: frente-esquerda do listener.
        hr = g_lpDS3DBuffer[Buffer][Channel]->SetPosition(-2.0f, 0.0f, 2.0f, DS3D_IMMEDIATE);
        if (FAILED(hr))
            return hr;
    }

    return FillSoundBuffer(pBuf, data.data(), desc.dwBufferBytes);
}

// Cria o primary buffer, obtém o listener 3D e define o formato de saída.
static HRESULT SetupPrimaryBuffer()
{
    LPDIRECTSOUNDBUFFER pPrimary = NULL;

    DSBUFFERDESC dsbd;
    ZeroMemory(&dsbd, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;

    HRESULT hr = g_lpDS->CreateSoundBuffer(&dsbd, &pPrimary, NULL);
    if (FAILED(hr))
        return hr;

    hr = pPrimary->QueryInterface(IID_IDirectSound3DListener, (void**)&g_lpDS3DListener);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pPrimary);
        return hr;
    }

    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(wfx));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 22050;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    hr = pPrimary->SetFormat(&wfx);

    // O listener continua válido após liberar o primary buffer.
    SAFE_RELEASE(pPrimary);
    return hr;
}

//-----------------------------------------------------------------------------
// Name: InitDirectSound()
// Desc: Inicializa DirectSound. Em caso de falha, libera tudo o que foi criado.
//-----------------------------------------------------------------------------
HRESULT InitDirectSound(HWND hDlg)
{
    if (g_lpDS != NULL)
    {
        g_EnableSound = true;
        return S_OK;
    }

    HRESULT hr = CoInitialize(NULL);
    g_ComInitialized = SUCCEEDED(hr);   // S_FALSE também exige CoUninitialize

    hr = DirectSoundCreate(NULL, &g_lpDS, NULL);
    if (FAILED(hr))
    {
        g_ErrorReport.Write("Init - DirectSoundCreate Error\r\n");
        FreeDirectSound();
        return hr;
    }

    hr = g_lpDS->SetCooperativeLevel(hDlg, DSSCL_PRIORITY);
    if (FAILED(hr))
    {
        g_ErrorReport.Write("Init - DS SetCooperativeLevel Error\r\n");
        FreeDirectSound();
        return hr;
    }

    hr = SetupPrimaryBuffer();
    if (FAILED(hr))
    {
        g_ErrorReport.Write("Init - Primary buffer / listener Error\r\n");
        FreeDirectSound();
        return hr;
    }

    g_EnableSound = true;
    return S_OK;
}

void SetEnableSound(bool b)
{
    g_EnableSound = b;
}

//-----------------------------------------------------------------------------
// Name: LoadWaveFile()
// Desc: Carrega um WAVE e cria MaxChannel buffers estáticos com os mesmos dados.
//-----------------------------------------------------------------------------
HRESULT LoadWaveFile(int Buffer, const char* strFileName, int MaxChannel, bool bEnable3D)
{
    if (!IsSoundReady())
        return S_FALSE;

    if (!IsValidBuffer(Buffer) || strFileName == NULL)
        return E_INVALIDARG;

    if (MaxBufferChannel[Buffer] > 0)
        return S_FALSE;                       // já carregado

    if (MaxChannel < 1)            MaxChannel = 1;
    if (MaxChannel > MAX_CHANNEL)  MaxChannel = MAX_CHANNEL;
    if (!g_Enable3DSound)          bEnable3D = false;

    WAVEFORMATEX     wfx;
    std::vector<BYTE> data;

    HRESULT hr = LoadWaveData(strFileName, wfx, data);
    if (FAILED(hr))
    {
        g_ErrorReport.Write("Failed to load wave file [%s]\r\n", strFileName);
        return hr;
    }

    for (int ch = 0; ch < MaxChannel; ++ch)
    {
        hr = CreateChannel(Buffer, ch, wfx, data, bEnable3D);
        if (FAILED(hr))
        {
            g_ErrorReport.Write("Failed to create sound buffer [%s] channel %d\r\n", strFileName, ch);
            ReleaseChannels(Buffer);
            return hr;
        }
    }

    MaxBufferChannel[Buffer] = MaxChannel;
    BufferChannel[Buffer] = 0;
    Enable3DSound[Buffer] = bEnable3D;

    strncpy(BufferName[Buffer], strFileName, sizeof(BufferName[Buffer]) - 1);
    BufferName[Buffer][sizeof(BufferName[Buffer]) - 1] = '\0';

    ++SoundLoadCount;

    SetVolume(Buffer, g_MasterVolume);
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer()
// Desc: Libera todos os canais de um som.
//-----------------------------------------------------------------------------
HRESULT ReleaseBuffer(int Buffer)
{
    if (!IsValidBuffer(Buffer))
        return E_INVALIDARG;

    if (MaxBufferChannel[Buffer] <= 0)
        return S_FALSE;                       // nunca carregado

    ReleaseChannels(Buffer);

    MaxBufferChannel[Buffer] = 0;
    BufferChannel[Buffer] = 0;
    Enable3DSound[Buffer] = false;
    BufferName[Buffer][0] = '\0';

    --SoundLoadCount;
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: FreeDirectSound()
// Desc: Libera todos os buffers, o listener, o DirectSound e o COM.
//-----------------------------------------------------------------------------
void FreeDirectSound()
{
    for (int i = 0; i < MAX_BUFFER; ++i)
    {
        ReleaseChannels(i);
        MaxBufferChannel[i] = 0;
        BufferChannel[i] = 0;
        Enable3DSound[i] = false;
        BufferName[i][0] = '\0';
    }
    SoundLoadCount = 0;

    SAFE_RELEASE(g_lpDS3DListener);
    SAFE_RELEASE(g_lpDS);

    if (g_ComInitialized)
    {
        CoUninitialize();
        g_ComInitialized = false;
    }

    g_EnableSound = false;
}

//-----------------------------------------------------------------------------
// Name: RestoreBuffers()
// Desc: Restaura um canal perdido e recarrega seus dados do arquivo original.
//-----------------------------------------------------------------------------
HRESULT RestoreBuffers(int Buffer, int Channel)
{
    if (g_lpDS == NULL)
        return S_FALSE;

    if (!IsValidBuffer(Buffer) || !IsValidChannel(Channel))
        return E_INVALIDARG;

    LPDIRECTSOUNDBUFFER pBuf = g_lpDSBuffer[Buffer][Channel];
    if (pBuf == NULL)
        return S_FALSE;

    DWORD dwStatus = 0;
    HRESULT hr = pBuf->GetStatus(&dwStatus);
    if (FAILED(hr))
        return hr;

    if (!(dwStatus & DSBSTATUS_BUFFERLOST))
        return S_OK;

    // A aplicação pode ter acabado de ser ativada; tenta por tempo limitado.
    for (int tries = 0; tries < 100; ++tries)
    {
        hr = pBuf->Restore();
        if (hr != DSERR_BUFFERLOST)
            break;
        Sleep(10);
    }
    if (FAILED(hr))
        return hr;

    // Restore() só devolve a memória; o conteúdo precisa ser recarregado.
    DSBCAPS caps;
    ZeroMemory(&caps, sizeof(caps));
    caps.dwSize = sizeof(caps);
    hr = pBuf->GetCaps(&caps);
    if (FAILED(hr))
        return hr;

    WAVEFORMATEX      wfx;
    std::vector<BYTE> data;

    hr = LoadWaveData(BufferName[Buffer], wfx, data);
    if (FAILED(hr))
        return hr;

    if (data.size() != caps.dwBufferBytes)
        return E_FAIL;

    return FillSoundBuffer(pBuf, data.data(), caps.dwBufferBytes);
}

//-----------------------------------------------------------------------------
// Name: PlayBuffer()
// Desc: Toca o som em um canal livre.
//       - Som em loop já tocando: não dispara outra instância.
//       - Mesmo objeto já tocando este som: não reinicia.
//       - Todos os canais ocupados: descarta (não reinicia o som).
//-----------------------------------------------------------------------------
HRESULT PlayBuffer(int Buffer, OBJECT* Object, BOOL bLooped)
{
    if (!IsSoundReady())
        return S_FALSE;

    if (!IsValidBuffer(Buffer))
        return E_INVALIDARG;

    const int count = MaxBufferChannel[Buffer];
    if (count <= 0)
        return E_FAIL;                          // não carregado

    // 1) Já existe uma instância tocando que não deve ser duplicada?
    for (int c = 0; c < count; ++c)
    {
        if (!IsChannelPlaying(Buffer, c))
            continue;

        if (bLooped)
            return S_OK;                        // loop já ativo

        if (Object != NULL && Object3DSound[Buffer][c] == Object)
            return S_OK;                        // mesmo objeto já tocando
    }

    // 2) Procura canal livre a partir do próximo da fila.
    const int start = BufferChannel[Buffer];
    int channel = -1;
    for (int n = 0; n < count; ++n)
    {
        int c = (start + n) % count;
        if (!IsChannelPlaying(Buffer, c))
        {
            channel = c;
            break;
        }
    }

    // 3) Todos ocupados: descarta em vez de reiniciar.
    if (channel < 0)
        return S_OK;

    LPDIRECTSOUNDBUFFER pBuf = g_lpDSBuffer[Buffer][channel];
    if (pBuf == NULL)
        return E_FAIL;

    const DWORD dwFlags = bLooped ? DSBPLAY_LOOPING : 0;

    pBuf->SetCurrentPosition(0);

    HRESULT hr = pBuf->Play(0, 0, dwFlags);
    if (hr == DSERR_BUFFERLOST)
    {
        if (SUCCEEDED(RestoreBuffers(Buffer, channel)))
            hr = pBuf->Play(0, 0, dwFlags);
    }

    if (FAILED(hr))
    {
        g_ConsoleDebug->Write(MCD_ERROR, "Play Sound: buffer=%d channel=%d hr=0x%08lX",
            Buffer, channel, (unsigned long)hr);
        return hr;
    }

    Object3DSound[Buffer][channel] = Object;
    BufferChannel[Buffer] = (channel + 1) % count;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: IsSoundPlaying()
// Desc: Retorna TRUE se o canal indicado estiver tocando.
//-----------------------------------------------------------------------------
BOOL IsSoundPlaying(int Buffer, int Channel)
{
    if (g_lpDS == NULL || !IsValidBuffer(Buffer) || !IsValidChannel(Channel))
        return FALSE;

    return IsChannelPlaying(Buffer, Channel) ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
// Name: StopBuffer()
// Desc: Para todos os canais de um som.
//-----------------------------------------------------------------------------
void StopBuffer(int Buffer, BOOL bResetPosition)
{
    if (g_lpDS == NULL || !IsValidBuffer(Buffer))
        return;

    for (int c = 0; c < MAX_CHANNEL; ++c)
    {
        LPDIRECTSOUNDBUFFER pBuf = g_lpDSBuffer[Buffer][c];
        if (pBuf == NULL)
            continue;

        pBuf->Stop();
        if (bResetPosition)
            pBuf->SetCurrentPosition(0);

        Object3DSound[Buffer][c] = NULL;
    }
}

void AllStopSound(void)
{
    if (g_lpDS == NULL)
        return;

    for (int i = 0; i < MAX_BUFFER; ++i)
        StopBuffer(i, TRUE);
}

//-----------------------------------------------------------------------------
// Name: SetVolume() / SetMasterVolume()
//-----------------------------------------------------------------------------
void SetVolume(int Buffer, long vol)
{
    if (g_lpDS == NULL || !IsValidBuffer(Buffer))
        return;

    if (vol > DSBVOLUME_MAX) vol = DSBVOLUME_MAX;
    if (vol < DSBVOLUME_MIN) vol = DSBVOLUME_MIN;

    for (int c = 0; c < MaxBufferChannel[Buffer]; ++c)
    {
        if (g_lpDSBuffer[Buffer][c] != NULL)
            g_lpDSBuffer[Buffer][c]->SetVolume(vol);
    }
}

void SetMasterVolume(long vol)
{
    if (vol > DSBVOLUME_MAX) vol = DSBVOLUME_MAX;
    if (vol < DSBVOLUME_MIN) vol = DSBVOLUME_MIN;

    g_MasterVolume = vol;

    for (int i = 0; i < MAX_BUFFER; ++i)
        SetVolume(i, g_MasterVolume);
}

//-----------------------------------------------------------------------------
// Name: Set3DSoundPosition()
// Desc: Atualiza a posição dos sons 3D relativa ao herói e à câmera.
//-----------------------------------------------------------------------------
void Set3DSoundPosition()
{
    if (!IsSoundReady() || !g_Enable3DSound || Hero == NULL)
        return;

    vec3_t Angle;
    float  Matrix[3][4];
    Vector(0.f, 0.f, CameraAngle[2], Angle);
    AngleMatrix(Angle, Matrix);

    for (int i = 0; i < MAX_BUFFER; ++i)
    {
        if (!Enable3DSound[i])
            continue;

        for (int j = 0; j < MaxBufferChannel[i]; ++j)
        {
            OBJECT* pObj = Object3DSound[i][j];
            LPDIRECTSOUND3DBUFFER p3d = g_lpDS3DBuffer[i][j];

            if (pObj == NULL || p3d == NULL)
                continue;

            // Som já terminou: solta a referência ao objeto.
            if (!IsChannelPlaying(i, j))
            {
                Object3DSound[i][j] = NULL;
                continue;
            }

            vec3_t SoundPosition;
            VectorCopy(pObj->Position, SoundPosition);
            VectorSubtract(Hero->Object.Position, SoundPosition, SoundPosition);
            VectorRotate(SoundPosition, Matrix, SoundPosition);
            VectorScale(SoundPosition, 0.004f, SoundPosition);

            p3d->SetPosition(-SoundPosition[0], 0.f, -SoundPosition[1], DS3D_IMMEDIATE);
        }
    }
}