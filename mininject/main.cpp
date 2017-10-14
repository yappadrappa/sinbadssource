#include <iostream>
#include <string>
#include <filesystem>
#include <thread>

const std::string processName = "FortniteClient-Win64-Shipping.exe";
const std::string parentName = "EpicGamesLauncher.exe";

#include "process.h"

int main()
{
    std::cout << "Looking for \"" << processName << "\" (parent: \"" << parentName << "\")" << std::endl;

    using namespace Cheddar;

    while (!Process::FindPIDByName(parentName))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto handle = Process::GetHandle(processName, parentName, PROCESS_ALL_ACCESS);
    if (handle == nullptr)
    {
        std::cout << "Failed to get a process handle :(" << std::endl;
        return 1;
    }

    std::cout << "Got a handle to the process!" << std::endl;

    char pathS[MAX_PATH];
    GetModuleFileName(NULL, pathS, MAX_PATH);
    auto path = std::experimental::filesystem::path(pathS);
    path.remove_filename();
    auto dllPath = path / "totallynotahack.dll";

    std::string pp = dllPath.generic_u8string();

    if (Process::InjectDLL(handle, pp))
    {
        std::cout << "Injection success!" << std::endl;
    }
    else
    {
        std::cout << "Injection failed :(" << std::endl;
    }

    return 0;
}
