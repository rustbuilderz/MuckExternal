#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include "mem.h"  
#include <iostream>

uintptr_t FinalStaminaAddress = 0;
float FinalStaminaValue = 0.0f;

DWORD GetProcessIdByName(const std::wstring& processName) {
    DWORD processId = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                if (processName == pe.szExeFile) {
                    processId = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
    return processId;
}

uintptr_t GetModuleBaseAddress(DWORD processId, const std::wstring& moduleName) {
    uintptr_t baseAddress = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me;
        me.dwSize = sizeof(me);
        if (Module32FirstW(snap, &me)) {
            do {
                if (moduleName == me.szModule) {
                    baseAddress = (uintptr_t)me.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
    }
    return baseAddress;
}


uintptr_t GetCurrentAddress() {
    return FinalStaminaAddress;
}


float GetCurrentFloatValue() {
    return FinalStaminaValue;
}
void UpdateAddress() {
    std::wstring targetProcessName = L"Muck.exe";
    std::wstring moduleName = L"mono-2.0-bdwgc.dll";
    DWORD processId = GetProcessIdByName(targetProcessName);

    if (!processId) return;

    uintptr_t moduleBase = GetModuleBaseAddress(processId, moduleName);
    if (!moduleBase) return;

    uintptr_t pointerAddress = moduleBase + 0x00496DA8;
    uintptr_t offsets[] = { 0x70, 0xE20, 0xB0, 0x60, 0x20, 0xB8, 0x64 };

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return;

    while (true) {
        uintptr_t currentAddress = 0;

        
        if (!ReadProcessMemory(hProcess, (LPCVOID)pointerAddress, &currentAddress, sizeof(uintptr_t), nullptr)) {
            Sleep(200);
            continue;
        }

        
        for (int i = 0; i < sizeof(offsets) / sizeof(offsets[0]) - 1; i++) {
            uintptr_t tempAddress = 0;

            if (!ReadProcessMemory(hProcess, (LPCVOID)(currentAddress + offsets[i]), &tempAddress, sizeof(uintptr_t), nullptr)) {
                Sleep(200);
                continue;
            }

            currentAddress = tempAddress;
        }

        
        FinalStaminaAddress = currentAddress + offsets[sizeof(offsets) / sizeof(offsets[0]) - 1];

        Sleep(200); 
    }

    CloseHandle(hProcess);
}

