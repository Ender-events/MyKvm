#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <linux/kvm.h>

#include <capstone/capstone.h>

#include "my-kvm.h"

void disassembly(csh handle, void* guest_mem, uint64_t rip);
void dump_register(struct kvm_regs* regs, struct kvm_sregs* sregs);
csh init_debug(int fd_vcpu);
void debug_dump(csh handle, struct my_kvm *my_kvm);

#endif /* _DEBUG_H_ */
