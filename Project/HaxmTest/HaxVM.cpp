#include "HaxVM.h"

// Current version
UINT32 HaxCurrentVersion = 0x2;

// Minimum HAX kernel version
UINT32 HaxMinVersion = 0x1;

bool HaxIsSupported(hax_state *Hax)
{
	hax_module_version version;
	if (hax_mod_version(Hax, &version) < 0)
		return false;

	if ((HaxMinVersion > version.cur_version) || (HaxCurrentVersion < version.compat_version))
		return false;

	return true;
}

bool HaxIsAvailable(hax_state *Hax)
{
	hax_capabilityinfo capinfo;
	if (!HaxGetCapability(Hax, &capinfo))
		return false;

	if ((capinfo.wstatus & HAX_CAP_WORKSTATUS_MASK) == HAX_CAP_STATUS_NOTWORKING)
	{
		if (capinfo.winfo & HAX_CAP_FAILREASON_VT)
			printf("VT feature is not enabled, HAXM not working.\n");
		else if (capinfo.winfo & HAX_CAP_FAILREASON_NX)
			printf("NX feature is not enabled, HAXM not working.\n");

		return false;
	}

	if (capinfo.wstatus & HAX_CAP_MEMQUOTA)
	{
		if (capinfo.mem_quota < Hax->mem_quota)
		{
			printf("The memory needed by this VM exceeds the driver limit.\n");
			return false;
		}
	}

	return true;
}

bool HaxGetCapability(hax_state *Hax, hax_capabilityinfo *Caps)
{
	if (hax_invalid_fd(Hax->fd))
	{
		printf("Invalid handle for Hax device!\n");
		return false;
	}

	DWORD bytesReturned;
	if (!DeviceIoControl(Hax->fd, HAX_IOCTL_CAPABILITY, nullptr, 0, Caps, sizeof(*Caps), &bytesReturned, nullptr))
	{
		DWORD err = GetLastError();

		if (err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_MORE_DATA)
			printf("Hax capability is too long to hold.\n");

		printf("Failed to get Hax capability: %d\n", err);
		return false;
	}

	return true;
}

hax_vm *HaxVmCreate(hax_state *Hax)
{
	// Handle validate
	if (hax_invalid_fd(Hax->fd))
		return nullptr;

	// Skip this if a VM is already allocated
	if (Hax->vm)
		return Hax->vm;

	// Allocate memory for the virtual machine structure
	// and zero the memory
	auto vm = (hax_vm *)malloc(sizeof(hax_vm));
	
	if (!vm)
		return nullptr;

	memset(vm, 0, sizeof(struct hax_vm));

	// Ask the driver for a VM instance and ID
	int vmId = 0;

	if (hax_host_create_vm(Hax, &vmId) < 0)
	{
		printf("Failed to create vm\n");

		free(vm);
		return nullptr;
	}
	
	vm->id = vmId;
	vm->fd = hax_host_open_vm(Hax, vmId);

	// Validate the VM's handle
	if (hax_invalid_fd(vm->fd))
	{
		printf("Open vm device error\n");

		free(vm);
		return nullptr;
	}

	// Notify the driver which version we are using
	hax_qemu_version qversion;
	qversion.cur_version = HaxCurrentVersion;
	qversion.least_version = HaxMinVersion;
	hax_notify_qemu_version(vm->fd, &qversion);

	Hax->vm = vm;
	return vm;
}

bool HaxVmDestroy(hax_vm *Vm)
{
	if (!Vm)
		return false;

	// Enumerate all vCPUs linked to the VM
	// and check if they are deleted
	for (int i = 0; i < HAX_MAX_VCPU; i++)
	{
		if (Vm->vcpus[i])
		{
			printf("vCPU should be cleaned before vm clean\n");
			return false;
		}
	}

	// Tell the driver to kill the instance
	hax_close_fd(Vm->fd);

	// Free the struct
	free(Vm);
	return true;
}