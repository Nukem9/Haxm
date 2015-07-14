#pragma once

#include "hax-all.h"

hax_vcpu_state *VCpu_Create(hax_state *Hax);
bool VCpu_Destroy(hax_state *Hax, hax_vcpu_state *CPU);
void VCpu_Init(hax_vcpu_state *CPU);
void VCpu_ResetState(hax_vcpu_state *CPU);
void VCpu_WriteVMCS(hax_vcpu_state *CPU, UINT Field, UINT64 Value);
bool VCpu_Run(hax_vcpu_state *CPU);
