#include "debug.h"

#include <err.h>
#include <sys/ioctl.h>

#include "utils.h"

void disassembly(csh handle, void *guest_mem, uint64_t rip)
{
	cs_insn *insn;
	size_t count;
	count = cs_disasm(handle, ptr_offset(guest_mem, rip), 16, rip, 0,
			  &insn);
	if (count > 0) {
		size_t j;
		if (count > 2)
			count = 2;
		for (j = 0; j < count; j++) {
			printf("0x%" PRIx64 ":\t%s\t\t%s\n", insn[j].address,
			       insn[j].mnemonic, insn[j].op_str);
		}

		cs_free(insn, count);
	} else
		printf("ERROR: Failed to disassemble given code!\n");
}

void dump_register(struct kvm_regs *regs, struct kvm_sregs *sregs)
{
	printf("rax: %08llx\trbx: %08llx\trcx: %08llx\trdx: %08llx\n"
	       "rsi: %08llx\trdi: %08llx\trsp: %08llx\trbp: %08llx\n"
	       "rip: %08llx\trflags: %08llx\n",
	       regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi,
	       regs->rsp, regs->rbp, regs->rip, regs->rflags);
	if (sregs != NULL) {
		printf("cs: %08llx\tds: %08llx\tes: %08llx\tfs: %08llx\n"
		       "gs: %08llx\tss: %08llx\ttr: %08llx\tldt: %08llx\n",
		       sregs->cs.base, sregs->ds.base, sregs->es.base,
		       sregs->fs.base, sregs->gs.base, sregs->ss.base,
		       sregs->tr.base, sregs->ldt.base);
	}
}

csh init_debug(int fd_vcpu)
{
	struct kvm_guest_debug kvm_guest_debug = {
		.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP,
		.pad = 0
	};
	ioctl(fd_vcpu, KVM_SET_GUEST_DEBUG, &kvm_guest_debug);

	csh handle;
	if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
		err(1, "unable to initialize capstone");
	cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);

	return handle;
}

void debug_dump(csh handle, struct my_kvm *my_kvm)
{
	struct kvm_regs regs;
	struct kvm_sregs sregs;
	ioctl(my_kvm->fd_vcpu, KVM_GET_REGS, &regs);
	disassembly(handle, my_kvm->mem_addr, regs.rip);
	ioctl(my_kvm->fd_vcpu, KVM_GET_SREGS, &sregs);
	dump_register(&regs, &sregs);
}
