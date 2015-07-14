#include "VM.h"
#include "HaxVM.h"

bool VM_HaxEnabled()
{
	// This function only creates a temporary state for the driver handle
	hax_state hax;
	memset(&hax, 0, sizeof(hax_state));

	hax.fd = hax_mod_open();

	// Is the returned handle valid?
	if (hax_invalid_fd(hax.fd))
		return false;

	// Determine the capabilities from the driver
	if (!HaxIsAvailable(&hax))
	{
		hax_mod_close(&hax);
		return false;
	}

	// Check if the version is supported
	if (!HaxIsSupported(&hax))
	{
		printf("Incompatible HAX version\n");
		hax_mod_close(&hax);
		return false;
	}

	hax_mod_close(&hax);
	return true;
}

hax_state *VM_CreateInstance(UINT RAMSize)
{
	// Allocate a state structure and zero it
	auto instance = (hax_state *)malloc(sizeof(hax_state));

	if (!instance)
		return nullptr;

	memset(instance, 0, sizeof(hax_state));

	// Set device handle and RAM size
	instance->fd = hax_mod_open();
	instance->mem_quota = RAMSize;

	// Link a VM handle to the state
	if (!HaxVmCreate(instance))
	{
		free(instance);
		return nullptr;
	}

	return instance;
}

bool VM_DestroyInstance(hax_state *Hax)
{
	if (HaxVmDestroy(Hax->vm))
	{
		// Free state
		free(Hax);
		return true;
	}

	return false;
}

bool VM_SetRAMSize(hax_state *Hax, UINT Size)
{
	uint64_t oldQuota = Hax->mem_quota;
	Hax->mem_quota = Size;

	// Check if the memory size is allowed
	if (HaxIsAvailable(Hax))
		return true;

	// Otherwise reset and return false
	Hax->mem_quota = oldQuota;
	return false;
}