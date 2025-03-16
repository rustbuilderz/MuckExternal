#include <iostream>
#include <thread>
#include <windows.h>
#include "mem.h"

int main() {
    std::thread updateThread(UpdateAddress);
    std::thread windowThread(CreateWindowAndRun);

    updateThread.detach();
    windowThread.detach();

    while (true) {
        uintptr_t addr = GetCurrentAddress();

        if (addr != 0) {
            std::cout << std::hex << std::uppercase << addr << std::endl;
        }

        Sleep(200);
    }

    return 0;
}
