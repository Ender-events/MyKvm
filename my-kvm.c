#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <linux/kvm_para.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <capstone/capstone.h>

#include "bzimage.h"
#include "utils.h"

#define COM1 0x3f8

unsigned char out_o[] = {	/* begin: */
    0xb0, 0x61,				/* mov $0x61, %al */
    0x66, 0xba, 0xf8, 0x03,	/* mov $0x3f8, %dx */
    0xee,					/* out %al, (%dx) */
    0xeb, 0xf7				/* jmp begin */
};

void handle_io(struct kvm_run *run_state)
{
    if (run_state->io.port == COM1 && run_state->io.direction == KVM_EXIT_IO_OUT)
    {
        uint64_t offset = run_state->io.data_offset;
        uint32_t size = run_state->io.size;
        write(STDOUT_FILENO, (char*)run_state + offset, size);
    }
}

void disassembly(csh handle, void* guest_mem, uint64_t rip)
{
    cs_insn *insn;
    size_t count;
    count = cs_disasm(handle, ptr_offset(guest_mem, rip), 16, rip, 0, &insn);
    if (count > 0) {
        size_t j;
        if (count > 2) count = 2;
        for (j = 0; j < count; j++) {
            printf("0x%"PRIx64":\t%s\t\t%s\n",
                    insn[j].address, insn[j].mnemonic,
                    insn[j].op_str);
        }

        cs_free(insn, count);
    } else
        printf("ERROR: Failed to disassemble given code!\n");
}

void dump_register(struct kvm_regs* regs, struct kvm_sregs* sregs)
{
    printf("rax: %08llx\trbx: %08llx\trcx: %08llx\trdx: %08llx\n"
           "rsi: %08llx\trdi: %08llx\trsp: %08llx\trbp: %08llx\n"
           "rip: %08llx\trflags: %08llx\n",
           regs->rax, regs->rbx, regs->rcx, regs->rdx,
           regs->rsi, regs->rdi, regs->rsp, regs->rbp,
           regs->rip, regs->rflags);
    if (sregs != NULL)
    {
        printf("cs: %08llx\tds: %08llx\tes: %08llx\tfs: %08llx\n"
               "gs: %08llx\tss: %08llx\ttr: %08llx\tldt: %08llx\n",
               sregs->cs.base, sregs->ds.base, sregs->es.base, sregs->fs.base,
               sregs->gs.base, sregs->ss.base, sregs->tr.base, sregs->ldt.base);
    }
}

int main(int argc, char **argv)
{
    int fd_kvm = open("/dev/kvm", O_RDWR);
    if (fd_kvm < 0) {
        err(1, "unable to open /dev/kvm");
    }

    int kvm_run_size = ioctl(fd_kvm, KVM_GET_VCPU_MMAP_SIZE, 0);

    int fd_vm = ioctl(fd_kvm, KVM_CREATE_VM, 0);
    if (fd_vm < 0) {
        err(1, "unable to create vm");
    }

    ioctl(fd_vm, KVM_SET_TSS_ADDR, 0xffffd000);
    __u64 map_addr = 0xffffc000;
    ioctl(fd_vm, KVM_SET_IDENTITY_MAP_ADDR, &map_addr);

    ioctl(fd_vm, KVM_CREATE_IRQCHIP, 0);

    void *mem_addr = mmap(NULL, 1 << 30, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    //memcpy(mem_addr, out_o, sizeof(out_o));
    load_bzimage(argv[1], mem_addr);

    struct kvm_userspace_memory_region region = {
        .slot = 0,
        .flags = 0,
        .guest_phys_addr = 0x0,
        .memory_size = 1 << 30,
        .userspace_addr = (__u64)mem_addr,
    };

    ioctl(fd_vm, KVM_SET_USER_MEMORY_REGION, &region);

    int fd_vcpu = ioctl(fd_vm, KVM_CREATE_VCPU, 0);

    struct kvm_sregs sregs;
    ioctl(fd_vcpu, KVM_GET_SREGS, &sregs);

#define set_segment_selector(Seg, Base, Limit, G) \
    do { \
        Seg.base = Base; \
        Seg.limit = Limit; \
        Seg.g = G; \
    } while (0)

    set_segment_selector(sregs.cs, 0, ~0, 1);
    set_segment_selector(sregs.ds, 0, ~0, 1);
    set_segment_selector(sregs.ss, 0, ~0, 1);
    set_segment_selector(sregs.es, 0, ~0, 1);
    set_segment_selector(sregs.fs, 0, ~0, 1);
    set_segment_selector(sregs.gs, 0, ~0, 1);

    sregs.cs.db = 1;
    sregs.ss.db = 1;
#undef set_segment_selector
    sregs.cr0 |= 1;
    ioctl(fd_vcpu, KVM_SET_SREGS, &sregs);

    struct kvm_regs regs;
    ioctl(fd_vcpu, KVM_GET_REGS, &regs);
    regs.rflags = 2;
    regs.rip = PM_ADDR;
    regs.rsi = BOOT_PARAMS_PTR;
    ioctl(fd_vcpu, KVM_SET_REGS, &regs);

    struct kvm_cpuid2 *kvm_cpuid = calloc(1, sizeof(*kvm_cpuid)
            + MAX_KVM_CPUID_ENTRIES * sizeof(*kvm_cpuid->entries));
    kvm_cpuid->nent = MAX_KVM_CPUID_ENTRIES;
    if (ioctl(fd_kvm, KVM_GET_SUPPORTED_CPUID, kvm_cpuid) < 0)
        perror("ioctl KVM_GET_SUPPORTED_CPUID");
    for (unsigned int i = 0; i < kvm_cpuid->nent; ++i) {
        struct kvm_cpuid_entry2 *entry = &kvm_cpuid->entries[i];
        if (entry->function == 0) {
            entry->eax = KVM_CPUID_FEATURES;
            entry->ebx = 0x4b4d564b; // KVMK
            entry->ecx = 0x564b4d56; // VMKV
            entry->edx = 0x4d;       // M
        }
    }
    if (ioctl(fd_vcpu, KVM_SET_CPUID2, kvm_cpuid) < 0)
        perror("ioctl KVM_SET_CPUID2");

    struct kvm_guest_debug kvm_guest_debug = {
        .control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP,
        .pad = 0
    };
    ioctl(fd_vcpu, KVM_SET_GUEST_DEBUG, &kvm_guest_debug);
    struct kvm_run *run_state =
        mmap(0, kvm_run_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd_vcpu, 0);


    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
        return -1;
    cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);

    for (;;) {
        int rc = ioctl(fd_vcpu, KVM_RUN, 0);

        if (rc < 0)
            warn("KVM_RUN");
        switch (run_state->exit_reason)
        {
            case KVM_EXIT_IO:
                printf("KVM_EXIT_IO\n");
                handle_io(run_state);
                break;
            case KVM_EXIT_SHUTDOWN:
                ioctl(fd_vcpu, KVM_GET_REGS, &regs);
                ioctl(fd_vcpu, KVM_GET_SREGS, &sregs);
                disassembly(handle, mem_addr, regs.rip);
                dump_register(&regs, NULL);
                return 1;
            default:
                printf("exit reason: %d\n", run_state->exit_reason);
                ioctl(fd_vcpu, KVM_GET_REGS, &regs);
                disassembly(handle, mem_addr, regs.rip);
                ioctl(fd_vcpu, KVM_GET_SREGS, &sregs);
                dump_register(&regs, &sregs);
                break;
        }

        printf("vm exit, sleeping 1s\n");
    }

    return 0;
}
