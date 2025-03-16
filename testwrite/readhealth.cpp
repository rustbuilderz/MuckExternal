#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include "mem.h"  
#include <iostream>
// Global variables to store the stamina address and float value
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

// Function to get the final stamina address
uintptr_t GetCurrentAddress() {
    return FinalStaminaAddress;
}

// Function to get the stamina float value
float GetCurrentFloatValue() {
    return FinalStaminaValue;
}
void UpdateAddress() {
    std::wstring targetProcessName = L"Muck.exe";
    std::wstring moduleName = L"UnityPlayer.dll";
    DWORD processId = GetProcessIdByName(targetProcessName);

    if (!processId) return;

    uintptr_t moduleBase = GetModuleBaseAddress(processId, moduleName);
    if (!moduleBase) return;

    uintptr_t pointerAddress = moduleBase + 0x00496DA8;
    uintptr_t offsets[] = { 0x0, 0xD0, 0x8, 0x60, 0x8, 0x920 };

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return;

    while (true) {
        uintptr_t currentAddress = 0;

        // ✅ Read the base pointer (DO NOT READ AS FLOAT)
        if (!ReadProcessMemory(hProcess, (LPCVOID)pointerAddress, &currentAddress, sizeof(uintptr_t), nullptr)) {
            Sleep(200);
            continue;
        }

        // ✅ Traverse the pointer chain correctly and STOP at the final pointer address
        for (int i = 0; i < sizeof(offsets) / sizeof(offsets[0]) - 1; i++) {  // <-- STOP BEFORE LAST OFFSET
            uintptr_t tempAddress = 0;

            if (!ReadProcessMemory(hProcess, (LPCVOID)(currentAddress + offsets[i]), &tempAddress, sizeof(uintptr_t), nullptr)) {
                Sleep(200);
                continue;
            }

            currentAddress = tempAddress;  // Move deeper into pointer chain
        }

        // ✅ Now apply the last offset manually to get the **actual pointer address**
        FinalStaminaAddress = currentAddress + offsets[sizeof(offsets) / sizeof(offsets[0]) - 1];

        Sleep(200);  // Refresh every 200ms
    }

    CloseHandle(hProcess);
}

