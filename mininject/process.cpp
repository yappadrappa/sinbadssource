#include <iostream>

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include "process.h"

#pragma warning(disable: 4311) // silence pointer truncation warning
#pragma warning(disable: 4302)

namespace Cheddar
{
    namespace Internal
    {
        std::string g_TargetFilename;
        HANDLE g_TargetHandle = nullptr;
        DWORD g_TargetPerms = PROCESS_ALL_ACCESS;
    }

    void Process::HandleReceiver(HANDLE *io_port)
    {
        DWORD nOfBytes;
        ULONG_PTR cKey;
        LPOVERLAPPED pid;
        char buffer[MAX_PATH];

        while (GetQueuedCompletionStatus(*io_port, &nOfBytes, &cKey, &pid, -1))
        {
            if (nOfBytes != 6)
            {
                continue;
            }

            auto handle = OpenProcess(Internal::g_TargetPerms, false, (DWORD)pid);
            GetModuleFileNameExA(handle, 0, buffer, MAX_PATH);

            auto path = std::experimental::filesystem::path(buffer);
            std::cout << "[INFO] Found " << path << std::endl;

            if (path.filename() == Internal::g_TargetFilename)
            {
                Internal::g_TargetHandle = handle;
                break;
            }

            CloseHandle(handle);
        }
    }

    DWORD Process::FindPIDByName(const std::string& executableName)
    {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        auto pid = 0UL;

        if (executableName.length() == 0)
        {
            GetWindowThreadProcessId(GetShellWindow(), &pid);
        }
        else
        {
            if (Process32First(snapshot, &entry))
            {
                while (Process32Next(snapshot, &entry))
                {
                    if (executableName == entry.szExeFile)
                    {
                        pid = entry.th32ProcessID;
                        break;
                    }
                }
            }
        }

        return pid;
    }

    HANDLE Process::GetHandle(const std::string& processName, const std::string& parentProcessName, DWORD perms)
    {
        Internal::g_TargetFilename = processName;
        Internal::g_TargetPerms = perms;

        auto pid = FindPIDByName(parentProcessName);
        if (pid == 0)
        {
            return nullptr;
        }

        auto exp_handle = OpenProcess(PROCESS_ALL_ACCESS, true, pid);
        auto io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
        auto job_object = CreateJobObjectW(0, 0);
        auto job_io_port = JOBOBJECT_ASSOCIATE_COMPLETION_PORT{ 0, io_port };
        auto result = SetInformationJobObject(job_object, JobObjectAssociateCompletionPortInformation, &job_io_port, sizeof(job_io_port));
        result = AssignProcessToJobObject(job_object, exp_handle);

        auto threadHandle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HandleReceiver, &io_port, 0, 0);
        WaitForSingleObject(threadHandle, -1);
        CloseHandle(exp_handle);

        return Internal::g_TargetHandle;
    }

    void* Process::FindBaseAddress(HANDLE process, const std::string& baseImageName)
    {
        HMODULE modules[4096];

        DWORD bytesTotal;
        if (!EnumProcessModules(process, modules, sizeof(modules), &bytesTotal))
        {
            return nullptr;
        }

        auto count = bytesTotal / sizeof(HMODULE);

        for (auto i = 0u; i < count; i++)
        {
            char fName[MAX_PATH];
            if (!GetModuleFileNameEx(process, modules[i], fName, MAX_PATH))
            {
                continue;
            }

            auto path = std::experimental::filesystem::path(fName);

            if (baseImageName == path.filename())
            {
                return modules[i];
            }
        }

        return nullptr;
    }

    void Process::IterateMemory(HANDLE process, std::function<bool(void*, const std::vector<char>& data)> callback)
    {
        unsigned char *p = nullptr;
        MEMORY_BASIC_INFORMATION info;
        std::vector<char> buffer;

        for (p = nullptr; VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info); p += info.RegionSize)
        {
            buffer.clear();

            if (info.State != MEM_COMMIT || (info.Type != MEM_MAPPED && info.Type != MEM_PRIVATE))
            {
                continue;
            }

            buffer.resize(info.RegionSize);

            SIZE_T bytesRead;
            ReadProcessMemory(process, p, &buffer[0], info.RegionSize, &bytesRead);

            buffer.resize(bytesRead);

            if (bytesRead == 0)
            {
                continue;
            }

            if (!callback(p, buffer))
            {
                return;
            }
        }
    }

    bool Process::InjectDLL(HANDLE process, const std::string& dllPath)
    {
        auto pathLen = dllPath.length();

        auto address = VirtualAllocEx(process, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (address == nullptr)
        {
            std::cout << "Failed to allocate memory in victim process :(" << std::endl;
            return false;
        }

        SIZE_T bytesWritten;
        if (!WriteProcessMemory(process, address, dllPath.data(), pathLen, &bytesWritten))
        {
            std::cout << "Failed to copy DLL contents into process memory :(. WriteProcessMemory failed with " << GetLastError() << std::endl;
            return false;
        }

        auto kernel32 = GetModuleHandle("kernel32.dll");
        auto loadLibraryAddress = GetProcAddress(kernel32, "LoadLibraryA");
        if (loadLibraryAddress == nullptr)
        {
            std::cout << "Failed to find LoadLibraryW in kernel32.dll :(" << std::endl;
            return false;
        }

        auto threadId = CreateRemoteThread(process, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddress),
            address, 0, nullptr);

        if (threadId == nullptr)
        {
            std::cout << "Failed to create remote thread :(" << std::endl;
            return 1;
        }

        WaitForSingleObject(threadId, INFINITE);
        VirtualFreeEx(process, address, pathLen, MEM_RELEASE);
        return true;
    }

}
