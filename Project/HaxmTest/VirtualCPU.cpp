#include "VirtualCPU.h"
#include "VirtualContext.h"
#include "VMCS.h"

#define HAX_EMULATE_STATE_MMIO		0x1
#define HAX_EMULATE_STATE_REAL		0x2
#define HAX_EMULATE_STATE_NONE		0x3
#define HAX_EMULATE_STATE_INITIAL	0x4

#define SET_SEGMENT_ACCESS(seg, value) \
	(seg).available = ((value) >> 12) & 1; \
	(seg).present = ((value) >> 7) & 1; \
	(seg).dpl = ((value) >> 5) & ~(~0 << (6-5+1)); \
	(seg).desc = ((value) >> 4) & 1; \
	(seg).type = ((value) >> 0) & ~(~0 << (3-0+1));

hax_vcpu_state *VCpu_Create(hax_state *Hax)
{
	if (!Hax->vm)
	{
		printf("vCPU created failed, vm is null\n");
		return nullptr;
	}

	// Find the next free vCPU index
	int cpuId = -1;

	for (int i = 0; i < ARRAYSIZE(Hax->vm->vcpus); i++)
	{
		if (Hax->vm->vcpus[i])
			continue;

		cpuId = i;
		break;
	}

	if (cpuId == -1)
	{
		printf("Maximum number of vCPUs have been allocated for this VM!\n");
		return nullptr;
	}

	// Allocate the virtual CPU instance structure and
	// zero memory
	auto vCPU = (hax_vcpu_state *)malloc(sizeof(hax_vcpu_state));

	if (!vCPU)
	{
		printf("Failed to alloc vCPU state\n");
		return nullptr;
	}

	memset(vCPU, 0, sizeof(hax_vcpu_state));

	// Tell the driver to create the vCPU instance
	vCPU->vcpu_id = cpuId;

	if (hax_host_create_vcpu(Hax->vm->fd, cpuId) < 0)
	{
		printf("Failed to create vCPU %x\n", cpuId);
		goto error;
	}

	// Grab a handle to the driver's instance
	vCPU->fd = hax_host_open_vcpu(Hax->vm->id, cpuId);

	if (hax_invalid_fd(vCPU->fd))
	{
		printf("Failed to open the vCPU handle\n");
		goto error;
	}

	// Mark the CPU index as used with a pointer
	Hax->vm->vcpus[cpuId] = vCPU;

	// Create the tunnel to kernel data
	if (hax_host_setup_vcpu_channel(vCPU) < 0)
	{
		printf("Invalid HAX tunnel size \n");
		goto error;
	}

	return vCPU;

error:
	// vCPU and tunnel will be closed automatically
	if (vCPU && !hax_invalid_fd(vCPU->fd))
		hax_close_fd(vCPU->fd);

	Hax->vm->vcpus[cpuId] = nullptr;
	free(vCPU);
	return nullptr;
}

bool VCpu_Destroy(hax_state *Hax, hax_vcpu_state *CPU)
{
    if (!Hax->vm)
    {
        printf("vCPU %x destroy failed, vm is null\n", CPU->vcpu_id);
        return false;
    }

	// Get a direct pointer to the index
	auto vCPU = Hax->vm->vcpus[CPU->vcpu_id];

	// Check if valid
    if (!vCPU)
        return false;

     // 1. The hax_tunnel is also destroyed at vcpu_destroy
     // 2. hax_close_fd will require the HAX kernel module to free vCPU
    Hax->vm->vcpus[CPU->vcpu_id] = nullptr;

	hax_close_fd(vCPU->fd);
	free(vCPU);
    return true;
}

void VCpu_Init(hax_vcpu_state *CPU)
{
	VCpu_ResetState(CPU);
}

void VCpu_ResetState(hax_vcpu_state *CPU)
{
	CPU->emulation_state = HAX_EMULATE_STATE_INITIAL;
	CPU->tunnel->user_event_pending = 0;
	CPU->tunnel->ready_for_interrupt_injection = 0;
}

void VCpu_WriteVMCS(hax_vcpu_state *CPU, UINT Field, UINT64 Value)
{
	// Query the CPU state
	vcpu_state_t state;

	if (hax_sync_vcpu_state(CPU, &state, 0) < 0)
		__debugbreak();

	// Set the required field
	switch (Field)
	{
	case VMCS_GUEST_ES_SELECTOR:		state._es.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_ES_LIMIT:			state._es.limit = (uint32)Value;		break;
	case VMCS_GUEST_ES_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._es, Value)	break;
	case VMCS_GUEST_ES_BASE:			state._es.base = (uint64)Value;			break;

	case VMCS_GUEST_CS_SELECTOR:		state._cs.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_CS_LIMIT:			state._cs.limit = (uint32)Value;		break;
	case VMCS_GUEST_CS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._cs, Value)	break;
	case VMCS_GUEST_CS_BASE:			state._cs.base = (uint64)Value;			break;

	case VMCS_GUEST_SS_SELECTOR:		state._ss.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_SS_LIMIT:			state._ss.limit = (uint32)Value;		break;
	case VMCS_GUEST_SS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._ss, Value)	break;
	case VMCS_GUEST_SS_BASE:			state._ss.base = (uint64)Value;			break;

	case VMCS_GUEST_DS_SELECTOR:		state._ds.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_DS_LIMIT:			state._ds.limit = (uint32)Value;		break;
	case VMCS_GUEST_DS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._ds, Value)	break;
	case VMCS_GUEST_DS_BASE:			state._ds.base = (uint64)Value;			break;

	case VMCS_GUEST_FS_SELECTOR:		state._fs.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_FS_LIMIT:			state._fs.limit = (uint32)Value;		break;
	case VMCS_GUEST_FS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._fs, Value)	break;
	case VMCS_GUEST_FS_BASE:			state._fs.base = (uint64)Value;			break;

	case VMCS_GUEST_GS_SELECTOR:		state._gs.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_GS_LIMIT:			state._gs.limit = (uint32)Value;		break;
	case VMCS_GUEST_GS_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._gs, Value)	break;
	case VMCS_GUEST_GS_BASE:			state._gs.base = (uint64)Value;			break;

	case VMCS_GUEST_LDTR_SELECTOR:		state._ldt.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_LDTR_LIMIT:			state._ldt.limit = (uint32)Value;		break;
	case VMCS_GUEST_LDTR_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._ldt, Value)	break;
	case VMCS_GUEST_LDTR_BASE:			state._ldt.base = (uint64)Value;		break;

	case VMCS_GUEST_TR_SELECTOR:		state._tr.selector = (uint16_t)Value;	break;
	case VMCS_GUEST_TR_LIMIT:			state._tr.limit = (uint32)Value;		break;
	case VMCS_GUEST_TR_ACCESS_RIGHTS:	SET_SEGMENT_ACCESS(state._tr, Value)	break;
	case VMCS_GUEST_TR_BASE:			state._tr.base = (uint64)Value;			break;

	case VMCS_GUEST_GDTR_LIMIT:			state._gdt.limit = (uint32)Value;		break;
	case VMCS_GUEST_GDTR_BASE:			state._gdt.base = (uint64)Value;		break;

	case VMCS_GUEST_IDTR_LIMIT:			state._idt.limit = (uint32)Value;		break;
	case VMCS_GUEST_IDTR_BASE:			state._idt.base = (uint64)Value;		break;

	case VMCS_GUEST_CR0:				state._cr0 = (uint64)Value;				break;
	case VMCS_GUEST_CR3:				state._cr3 = (uint64)Value;				break;
	case VMCS_GUEST_CR4:				state._cr4 = (uint64)Value;				break;

	case VMCS_GUEST_DR7:				state._dr7 = (uint64)Value;				break;
	case VMCS_GUEST_RSP:				state._rsp = (uint64)Value;				break;
	case VMCS_GUEST_RIP:				state._rip = (uint64)Value;				break;
	case VMCS_GUEST_RFLAGS:				state._rflags = (uint64)Value;			break;

	case VMCS_GUEST_IA32_SYSENTER_CS:	state._sysenter_cs = (uint32)Value;		break;
	case VMCS_GUEST_IA32_SYSENTER_ESP:	state._sysenter_esp = (uint64)Value;	break;
	case VMCS_GUEST_IA32_SYSENTER_EIP:	state._sysenter_eip = (uint64)Value;	break;

	default:
		__debugbreak();
	}

	// Set the new CPU state
	if (hax_sync_vcpu_state(CPU, &state, 1) < 0)
		__debugbreak();
}

bool VCpu_Run(hax_vcpu_state *CPU)
{
	return VCpu_Exec(CPU);
}