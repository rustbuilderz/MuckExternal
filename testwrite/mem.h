#pragma once
#define MEM_H
#include <cstdint>

uintptr_t GetCurrentAddress();
void UpdateAddress();
float GetCurrentFloatValue();
void WriteMemoryLoop(bool value);
void CreateWindowAndRun();
