#include "stdafx.h"

void TestDOS(hax_state *State, hax_vcpu_state *Cpu)
{
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_ACCESS_RIGHTS, 0x9b);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_ACCESS_RIGHTS, 0x10000);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_ACCESS_RIGHTS, 0x83);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GDTR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GDTR_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_IDTR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_IDTR_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CR0, 0x20 | 1);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CR3, 0x0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CR4, 0x2000);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RSP, 0x100);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RIP, 0x100);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RFLAGS, 0x200 | 0x2);

	LPVOID mem = VirtualAlloc(nullptr, 16384, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memset(mem, 0x90, 16384);
	*(BYTE *)((PBYTE)mem + 500) = 0x0F;
	*(BYTE *)((PBYTE)mem + 501) = 0x01;
	*(BYTE *)((PBYTE)mem + 502) = 0xD1;
	*(BYTE *)((PBYTE)mem + 503) = 0xCC;
	//memcpy(mem, data, sizeof(data));

	hax_populate_ram(State->vm, (uint64)mem, 16384);
	hax_set_phys_mem(State, 0, 16384, (uint64)mem);

	VCpu_Run(Cpu);

	vcpu_state_t state;
	memset(&state, 0, sizeof(vcpu_state_t));

	if (hax_sync_vcpu_state(Cpu, &state, 0) < 0)
		__debugbreak();
}

int main(int argc, char *argv[])
{
	// Is everything loaded and compatible?
	if (!VM_HaxEnabled())
		return 1;

	// Create the VM instance with 16kb RAM
	auto state = VM_CreateInstance(16384);

	if (!state)
		return 1;

	// Set up the virtual processor
	auto cpu = VCpu_Create(state);

	if (!cpu)
		return 1;

	VCpu_Init(cpu);
	TestDOS(state, cpu);

	getchar();
	VCpu_Destroy(state, cpu);
	VM_DestroyInstance(state);

	getchar();
	return 0;
}