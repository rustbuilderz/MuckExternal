#include <windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <thread>
#include "mem.h" 

void WriteMemoryLoop(bool value) {
    if (!value) { 
        return;
    }

    const char* targetProcess = "Muck.exe";
    DWORD PID = 0;
    int valToWrite = 1120410373; 

    
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32W proc;
    proc.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snap, &proc)) {
        do {
            char procName[MAX_PATH] = { 0 };
            size_t convertedChars = 0;
            wcstombs_s(&convertedChars, procName, MAX_PATH, proc.szExeFile, _TRUNCATE);

            if (_stricmp(procName, targetProcess) == 0) {
                PID = proc.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &proc));
    }
    CloseHandle(snap);

    if (!PID) {
        return;
    }

    HANDLE handle = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, PID);
    if (!handle) {
        return;
    }

    
    while (value) {  
        uintptr_t addr = GetCurrentAddress();

        if (addr != 0) { 
            WriteProcessMemory(handle, (LPVOID)addr, &valToWrite, sizeof(valToWrite), nullptr);
        }

        Sleep(200); 
    }

    CloseHandle(handle);
}
