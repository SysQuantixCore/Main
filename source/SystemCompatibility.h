#pragma once

#include <windows.h>

namespace SystemCompatibility
{
    enum class WindowsVersion
    {
        Unknown,
        Windows7,
        Windows8,
        Windows81,
        Windows10,
        Windows11,
        Newer
    };

    enum class StorageType
    {
        Unknown,
        HardDisk,
        SolidState,
        Removable,
        Network
    };

    struct SystemProfile
    {
        WindowsVersion windowsVersion;
        DWORD windowsBuild;
        DWORD logicalProcessors;
        DWORD physicalCores;
        ULONGLONG totalPhysicalMemory;
        ULONGLONG availablePhysicalMemory;
        ULONGLONG totalVirtualMemory;
        bool sse2;
        bool sse41;
        bool avx;
        bool avx2;
        char cpuVendor[16];
        char cpuBrand[64];
        char gpuName[128];
        StorageType storageType;
    };

    bool Initialize(SystemProfile& profile);
    const char* GetWindowsVersionName(WindowsVersion version);
    const char* GetStorageTypeName(StorageType type);
    HANDLE CreateSequentialReadHandleA(const char* path, DWORD shareMode = FILE_SHARE_READ);
}
