#include "hax-all.h"

int hax_handle_fastmmio(hax_vcpu_state *env, struct hax_fastmmio *hft)
{
    uint64_t buf = 0;

    /*
     * With fast MMIO, QEMU need not sync vCPU state with HAXM
     * driver because it will only invoke MMIO handler
     * However, some MMIO operations utilize virtual address like qemu_pipe
     * Thus we need to sync the CR0, CR3 and CR4 so that QEMU
     * can translate the guest virtual address to guest physical
     * address
     */
    buf = hft->value;
    //cpu_physical_memory_rw(hft->gpa, &buf, hft->size, hft->direction);
    if (hft->direction == 0)
        hft->value = buf;

    return 0;
}

int hax_handle_io(hax_vcpu_state *env, uint32_t df, uint16_t port, int direction,
  int size, int count, void *buffer)
{
	/*
    uint8_t *ptr;
    int i;

    if (!df)
        ptr = (uint8_t *)buffer;
    else
        ptr = buffer + size * count - size;
    for (i = 0; i < count; i++)
    {
        if (direction == HAX_EXIT_IO_IN) {
            switch (size) {
                case 1:
                    stb_p(ptr, cpu_inb(port));
                    break;
                case 2:
                    stw_p(ptr, cpu_inw(port));
                    break;
                case 4:
                    stl_p(ptr, cpu_inl(port));
                    break;
            }
        } else {
            switch (size) {
                case 1:
                    cpu_outb(port, ldub_p(ptr));
                    break;
                case 2:
                    cpu_outw(port, lduw_p(ptr));
                    break;
                case 4:
                    cpu_outl(port, ldl_p(ptr));
                    break;
            }
        }
        if (!df)
            ptr += size;
        else
            ptr -= size;
    }
	*/
    return 0;
}

int hax_vcpu_interrupt(hax_vcpu_state *CPU)
{
#if 0
    struct hax_tunnel *ht = CPU->tunnel;

    /*
     * Try to inject an interrupt if the guest can accept it
     * Unlike KVM, the HAX kernel module checks the eflags, instead.
     */
    if (ht->ready_for_interrupt_injection &&
      (env->interrupt_request & CPU_INTERRUPT_HARD))
    {
        int irq;

        env->interrupt_request &= ~CPU_INTERRUPT_HARD;
        irq = -1;//cpu_get_pic_interrupt(env);
        if (irq >= 0) {
            hax_inject_interrupt(env, irq);
        }
    }

    /* 
     * If we have an interrupt pending but the guest is not ready to
     * receive it, request an interrupt window exit.  This will cause
     * a return to userspace as soon as the guest is ready to receive
     * an interrupt.
     */
    if ((env->interrupt_request & CPU_INTERRUPT_HARD))
        ht->request_interrupt_window = 1;
    else
        ht->request_interrupt_window = 0;
#endif
    return 0;
}

void hax_raise_event(hax_vcpu_state *CPU)
{
    CPU->tunnel->user_event_pending = 1;
}