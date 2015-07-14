#include "VirtualContext.h"

#define HAX_EMUL_ONE		0x1
#define HAX_EMUL_REAL		0x2
#define HAX_EMUL_HLT		0x4
#define HAX_EMUL_EXITLOOP	0x5

/*
 * Request the HAX kernel module to run the CPU for us until one of
 * the following occurs:
 * 1. Guest crashes or is shut down
 * 2. We need QEMU's emulation like when the guest executes a MMIO
 *    instruction or guest enters emulation mode (non-PG mode)
 * 3. Guest executes HLT
 * 4. Qemu has Signal/event pending
 * 5. An unknown VMX-exit happens
 */
int hax_vcpu_interrupt(hax_vcpu_state *CPU);
int VCpu_Exec(hax_vcpu_state *CPU)
{
	int ret = 0;
    hax_tunnel *ht = CPU->tunnel;

    do
	{
        hax_vcpu_interrupt(CPU);

        int haxResult = hax_vcpu_run(CPU);

        // Simply continue the vcpu_run if system call interrupted
        if (haxResult == -EINTR || haxResult == -EAGAIN)
		{
            printf("IO window interrupted\n");
            continue;
        }

		// Fail if system call failed
        if (haxResult < 0)
        {
            printf("vCPU run failed for vCPU %x\n", CPU->vcpu_id);
            abort();
        }

        switch (ht->_exit_status)
        {
			// Regular I/O
            case HAX_EXIT_IO:
				/*
                {
                    ret = hax_handle_io(env, ht->pio._df, ht->pio._port,
                      ht->pio._direction,
                      ht->pio._size, ht->pio._count, vcpu->iobuf);
                }
				*/
                break;

			// Memory-mapped I/O
            case HAX_EXIT_MMIO:
                ret = HAX_EMUL_ONE;
                break;

			// Fast memory-mapped I/O
            case HAX_EXIT_FAST_MMIO:
                //ret = hax_handle_fastmmio(env, (hax_fastmmio *)vcpu->iobuf);
                break;

			// Guest running in real mode
            case HAX_EXIT_REAL:
                ret = HAX_EMUL_REAL;
                break;

			// Guest state changed, currently only for shutdown
            case HAX_EXIT_STATECHANGE:
                printf("VCPU shutdown request\n");
                ret = HAX_EMUL_EXITLOOP;
                break;

            case HAX_EXIT_UNKNOWN_VMEXIT:
                dprint("Unknown VMX exit %x from guest\n", ht->_exit_reason);
                //qemu_system_reset_request();
                //hax_prepare_emulation(env);
                //cpu_dump_state(env, stderr, fprintf, 0);
                ret = HAX_EMUL_EXITLOOP;
                break;

			// HLT instruction executed
            case HAX_EXIT_HLT:
				/*
                if (!(env->interrupt_request & CPU_INTERRUPT_HARD) &&
                  !(env->interrupt_request & CPU_INTERRUPT_NMI)) {
                    // hlt instruction with interrupt disabled is shutdown
                    env->eflags |= IF_MASK;
                    env->halted = 1;
                    env->exception_index = EXCP_HLT;
                    ret = HAX_EMUL_HLT;
                }
				*/
                break;

                // these situations will continue to the Hax module
            case HAX_EXIT_INTERRUPT:
            case HAX_EXIT_PAUSED:
                break;

            default:
                printf("Unknown exit %x from Hax driver\n", ht->_exit_status);
                ret = HAX_EMUL_EXITLOOP;
                break;
        }
    }while (!ret);

    return ret;
}

#define HAX_RAM_INFO_ROM 0x1

/*
static void set_v8086_seg(struct segment_desc_t *lhs, const SegmentCache *rhs)
{
    memset(lhs, 0, sizeof(struct segment_desc_t ));
    lhs->selector = rhs->selector;
    lhs->base = rhs->base;
    lhs->limit = rhs->limit;
    lhs->type = 3;
    lhs->present = 1;
    lhs->dpl = 3;
    lhs->operand_size = 0;
    lhs->desc = 1;
    lhs->long_mode = 0;
    lhs->granularity = 0;
    lhs->available = 0;
}
*/

void hax_msr_entry_set(struct vmx_msr *item, uint32_t index, uint64_t value)
{
    item->entry = index;
    item->value = value;
}

bool hax_get_fpu(hax_vcpu_state *CPU, fx_layout *FPU)
{
    return hax_sync_fpu(CPU, FPU, 0) >= 0;
}

bool hax_set_fpu(hax_vcpu_state *CPU, fx_layout *FPU)
{
	return hax_sync_fpu(CPU, FPU, 1) >= 0;
}