#pragma once

#include "hax-all.h"

bool VM_HaxEnabled();
hax_state *VM_CreateInstance(UINT RAMSize);
bool VM_DestroyInstance(hax_state *Hax);
bool VM_SetRAMSize(hax_state *Hax, UINT Size);