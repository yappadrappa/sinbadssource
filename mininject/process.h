#ifndef __PROCESS_H
#define __PROCESS_H

#include <string>
#include <filesystem>
#include <functional>

#include <windows.h>

namespace Cheddar
{
    namespace Process
    {
        void HandleReceiver(HANDLE *io_port);
        DWORD FindPIDByName(const std::string& executableName);
        HANDLE GetHandle(const std::string& processName, const std::string& parentProcessName = "", DWORD perms = PROCESS_ALL_ACCESS);
        void* FindBaseAddress(HANDLE process, const std::string& baseImageName);
        void IterateMemory(HANDLE process, std::function<bool(void*, const std::vector<char>& data)> callback);
        bool InjectDLL(HANDLE process, const std::string& dllPath);
    }
}

#endif
