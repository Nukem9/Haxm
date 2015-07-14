#pragma once

#include "hax-all.h"

bool HaxIsSupported(hax_state *Hax);
bool HaxIsAvailable(hax_state *Hax);
bool HaxGetCapability(hax_state *Hax, hax_capabilityinfo *Caps);

hax_vm *HaxVmCreate(hax_state *Hax);
bool HaxVmDestroy(hax_vm *Vm);