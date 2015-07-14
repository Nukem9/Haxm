/*
** Copyright (c) 2011, Intel Corporation
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#ifndef _HAX_I386_H
#define _HAX_I386_H

#include "config.h"

typedef HANDLE hax_fd;

struct hax_vcpu_state
{
	hax_fd fd;
	int vcpu_id;
	int emulation_state;
	struct hax_tunnel *tunnel;
	unsigned char *iobuf;
};

struct hax_state
{
	hax_fd fd; /* the global hax device interface */
	uint32_t version;
	struct hax_vm *vm;
	uint64_t mem_quota;
};

#define HAX_MAX_VCPU 0x10
#define MAX_VM_ID 0x40
#define MAX_VCPU_ID 0x40

struct hax_vm
{
	hax_fd fd;
	int id;
	struct hax_vcpu_state *vcpus[HAX_MAX_VCPU];
};

/* Functions exported to host specific mode */
int valid_hax_tunnel_size(uint32_t size);
int hax_open_device(hax_fd *fd);
hax_fd hax_mod_open(void);
int hax_populate_ram(struct hax_vm *vm, uint64_t va, uint32_t size);
int hax_set_phys_mem(struct hax_state *hax, target_phys_addr_t start_addr, ram_addr_t size, ram_addr_t phys_offset);
int hax_capability(struct hax_state *hax, struct hax_capabilityinfo *cap);
int hax_mod_version(struct hax_state *hax, struct hax_module_version *version);
char *hax_vm_devfs_string(int vm_id);
char *hax_vcpu_devfs_string(int vm_id, int vcpu_id);
int hax_host_create_vm(struct hax_state *hax, int *vmid);
hax_fd hax_host_open_vm(struct hax_state *hax, int vm_id);
int hax_notify_qemu_version(hax_fd vm_fd, struct hax_qemu_version *qversion);
int hax_host_create_vcpu(hax_fd vm_fd, int vcpuid);
hax_fd hax_host_open_vcpu(int vmid, int vcpuid);
int hax_host_setup_vcpu_channel(struct hax_vcpu_state *vcpu);
int hax_vcpu_run(struct hax_vcpu_state* vcpu);
int hax_sync_fpu(struct hax_vcpu_state *env, struct fx_layout *fl, int set);
int hax_sync_msr(struct hax_vcpu_state *env, struct hax_msr_data *msrs, int set);
int hax_sync_vcpu_state(struct hax_vcpu_state *env, struct vcpu_state_t *state, int set);
int hax_inject_interrupt(struct hax_vcpu_state *env, int vector);

#include "hax-windows.h"
#include "hax-interface.h"

#endif