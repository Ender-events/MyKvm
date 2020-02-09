#ifndef _MY_KVM_H
#define _MY_KVM_H

struct my_kvm {
	int fd_kvm;
	int fd_vm;
	int fd_vcpu;
	void *mem_addr;
};

#endif /* _MY_KVM_H */
