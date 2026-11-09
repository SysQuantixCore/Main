#include "stdafx.h"
#include "SystemCompatibility.h"

#include <intrin.h>
#include <winioctl.h>
#include <ntddstor.h>
#include <winternl.h>
#include <new>

#ifndef STORAGE_DEVICE_SEEK_PENALTY_DESCRIPTOR
typedef struct _STORAGE_DEVICE_SEEK_PENALTY_DESCRIPTOR
{
    DWORD Version;
    DWORD Size;
    BOOLEAN IncursSeekPenalty;
} STORAGE_DEVICE_SEEK_PENALTY_DESCRIPTOR,
* PSTORAGE_DEVICE_SEEK_PENALTY_DESCRIPTOR;
#endif

namespace
{
    typedef LONG(WINAPI* RtlGetVersionFunction)(PRTL_OSVERSIONINFOW);

    bool QueryWindowsVersion(SystemCompatibility::SystemProfile& profile)
    {
        OSVERSIONINFOW version = {};
        version.dwOSVersionInfoSize = sizeof(version);

        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll)
        {
            RtlGetVersionFunction rtlGetVersion =
                reinterpret_cast<RtlGetVersionFunction>(GetProcAddress(ntdll, "RtlGetVersion"));

            if (rtlGetVersion && rtlGetVersion(reinterpret_cast<PRTL_OSVERSIONINFOW>(&version)) == 0)
            {
                profile.windowsBuild = version.dwBuildNumber;

                if (version.dwMajorVersion == 6 && version.dwMinorVersion == 1)
                    profile.windowsVersion = SystemCompatibility::WindowsVersion::Windows7;
                else if (version.dwMajorVersion == 6 && version.dwMinorVersion == 2)
                    profile.windowsVersion = SystemCompatibility::WindowsVersion::Windows8;
                else if (version.dwMajorVersion == 6 && version.dwMinorVersion == 3)
                    profile.windowsVersion = SystemCompatibility::WindowsVersion::Windows81;
                else if (version.dwMajorVersion == 10 && version.dwBuildNumber >= 22000)
                    profile.windowsVersion = SystemCompatibility::WindowsVersion::Windows11;
                else if (version.dwMajorVersion == 10)
                    profile.windowsVersion = SystemCompatibility::WindowsVersion::Windows10;
                else if (version.dwMajorVersion > 10)
                    profile.windowsVersion = SystemCompatibility::WindowsVersion::Newer;
                else
                    profile.windowsVersion = SystemCompatibility::WindowsVersion::Unknown;

                return true;
            }
        }

        profile.windowsVersion = SystemCompatibility::WindowsVersion::Unknown;
        profile.windowsBuild = 0;
        return false;
    }

    void QueryCpu(SystemCompatibility::SystemProfile& profile)
    {
        int registers[4] = {};
        __cpuid(registers, 0);

        const int maxLeaf = registers[0];
        memcpy(profile.cpuVendor, &registers[1], 4);
        memcpy(profile.cpuVendor + 4, &registers[3], 4);
        memcpy(profile.cpuVendor + 8, &registers[2], 4);
        profile.cpuVendor[12] = '\0';

        if (maxLeaf >= 1)
        {
            __cpuid(registers, 1);
            profile.sse2 = (registers[3] & (1 << 26)) != 0;
            profile.sse41 = (registers[2] & (1 << 19)) != 0;

            const bool osxsave = (registers[2] & (1 << 27)) != 0;
            const bool avxCpu = (registers[2] & (1 << 28)) != 0;
            bool avxOs = false;

            if (osxsave && avxCpu)
            {
                const unsigned __int64 xcr0 = _xgetbv(0);
                avxOs = (xcr0 & 0x6) == 0x6;
            }

            profile.avx = avxCpu && avxOs;
        }

        if (maxLeaf >= 7)
        {
            __cpuidex(registers, 7, 0);
            profile.avx2 = profile.avx && ((registers[1] & (1 << 5)) != 0);
        }

        __cpuid(registers, static_cast<int>(0x80000000));
        const unsigned int maxExtendedLeaf = static_cast<unsigned int>(registers[0]);

        if (maxExtendedLeaf >= 0x80000004)
        {
            int brand[12] = {};
            __cpuid(brand, static_cast<int>(0x80000002));
            __cpuid(brand + 4, static_cast<int>(0x80000003));
            __cpuid(brand + 8, static_cast<int>(0x80000004));
            memcpy(profile.cpuBrand, brand, sizeof(brand));
            profile.cpuBrand[sizeof(profile.cpuBrand) - 1] = '\0';
        }
    }

    void QueryProcessorTopology(SystemCompatibility::SystemProfile& profile)
    {
        SYSTEM_INFO systemInfo = {};
        GetNativeSystemInfo(&systemInfo);
        profile.logicalProcessors = systemInfo.dwNumberOfProcessors;

        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!kernel32)
            return;

        typedef BOOL(WINAPI* GetLogicalProcessorInformationExFunction)(
            LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);

        GetLogicalProcessorInformationExFunction query =
            reinterpret_cast<GetLogicalProcessorInformationExFunction>(
                GetProcAddress(kernel32, "GetLogicalProcessorInformationEx"));

        if (!query)
        {
            profile.physicalCores = profile.logicalProcessors;
            return;
        }

        DWORD required = 0;
        query(RelationProcessorCore, NULL, &required);
        if (required == 0)
        {
            profile.physicalCores = profile.logicalProcessors;
            return;
        }

        BYTE* buffer = new (std::nothrow) BYTE[required];
        if (!buffer)
        {
            profile.physicalCores = profile.logicalProcessors;
            return;
        }

        if (query(RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer), &required))
        {
            DWORD offset = 0;
            while (offset < required)
            {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX entry =
                    reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer + offset);

                if (entry->Relationship == RelationProcessorCore)
                    ++profile.physicalCores;

                if (entry->Size == 0)
                    break;

                offset += entry->Size;
            }
        }

        delete[] buffer;

        if (profile.physicalCores == 0)
            profile.physicalCores = profile.logicalProcessors;
    }

    void QueryMemory(SystemCompatibility::SystemProfile& profile)
    {
        MEMORYSTATUSEX memory = {};
        memory.dwLength = sizeof(memory);

        if (GlobalMemoryStatusEx(&memory))
        {
            profile.totalPhysicalMemory = memory.ullTotalPhys;
            profile.availablePhysicalMemory = memory.ullAvailPhys;
            profile.totalVirtualMemory = memory.ullTotalVirtual;
        }
    }

    void QueryGpu(SystemCompatibility::SystemProfile& profile)
    {
        DISPLAY_DEVICEA display = {};
        display.cb = sizeof(display);

        for (DWORD index = 0; EnumDisplayDevicesA(NULL, index, &display, 0); ++index)
        {
            if ((display.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) == 0 &&
                display.DeviceString[0] != '\0')
            {
                strncpy_s(profile.gpuName, display.DeviceString, _TRUNCATE);
                break;
            }

            ZeroMemory(&display, sizeof(display));
            display.cb = sizeof(display);
        }
    }

    SystemCompatibility::StorageType QueryStorage()
    {
        char executablePath[MAX_PATH] = {};
        if (!GetModuleFileNameA(NULL, executablePath, MAX_PATH))
            return SystemCompatibility::StorageType::Unknown;

        char rootPath[MAX_PATH] = {};
        if (!GetVolumePathNameA(executablePath, rootPath, MAX_PATH))
            return SystemCompatibility::StorageType::Unknown;

        const UINT driveType = GetDriveTypeA(rootPath);
        if (driveType == DRIVE_REMOVABLE)
            return SystemCompatibility::StorageType::Removable;
        if (driveType == DRIVE_REMOTE)
            return SystemCompatibility::StorageType::Network;
        if (driveType != DRIVE_FIXED)
            return SystemCompatibility::StorageType::Unknown;

        char devicePath[] = "\\\\.\\C:";
        const char* root = rootPath;
        if (root[0] && root[1] == ':')
            devicePath[4] = root[0];

        HANDLE device = CreateFileA(devicePath, 0,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (device == INVALID_HANDLE_VALUE)
            return SystemCompatibility::StorageType::Unknown;

        STORAGE_PROPERTY_QUERY query = {};
        query.PropertyId = StorageDeviceSeekPenaltyProperty;
        query.QueryType = PropertyStandardQuery;

        STORAGE_DEVICE_SEEK_PENALTY_DESCRIPTOR descriptor = {};
        DWORD returned = 0;
        const BOOL success = DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY,
            &query, sizeof(query), &descriptor, sizeof(descriptor), &returned, NULL);

        CloseHandle(device);

        if (!success || returned < sizeof(descriptor))
            return SystemCompatibility::StorageType::Unknown;

        return descriptor.IncursSeekPenalty
            ? SystemCompatibility::StorageType::HardDisk
            : SystemCompatibility::StorageType::SolidState;
    }
}

namespace SystemCompatibility
{
    bool Initialize(SystemProfile& profile)
    {
        ZeroMemory(&profile, sizeof(profile));

        QueryWindowsVersion(profile);
        QueryCpu(profile);
        QueryProcessorTopology(profile);
        QueryMemory(profile);
        QueryGpu(profile);
        profile.storageType = QueryStorage();

        return profile.logicalProcessors != 0 || profile.totalPhysicalMemory != 0;
    }

    const char* GetWindowsVersionName(WindowsVersion version)
    {
        switch (version)
        {
        case WindowsVersion::Windows7: return "Windows 7";
        case WindowsVersion::Windows8: return "Windows 8";
        case WindowsVersion::Windows81: return "Windows 8.1";
        case WindowsVersion::Windows10: return "Windows 10";
        case WindowsVersion::Windows11: return "Windows 11";
        case WindowsVersion::Newer: return "Windows newer than 11";
        default: return "Unknown Windows version";
        }
    }

    const char* GetStorageTypeName(StorageType type)
    {
        switch (type)
        {
        case StorageType::HardDisk: return "HDD";
        case StorageType::SolidState: return "SSD / low-seek storage";
        case StorageType::Removable: return "Removable";
        case StorageType::Network: return "Network";
        default: return "Unknown";
        }
    }

    HANDLE CreateSequentialReadHandleA(const char* path, DWORD shareMode)
    {
        if (!path || !path[0])
            return INVALID_HANDLE_VALUE;

        return CreateFileA(path, GENERIC_READ, shareMode, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    }
}
