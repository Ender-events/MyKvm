#ifndef _MY_KVM_H
#define _MY_KVM_H

struct my_kvm {
	int fd_kvm;
	int fd_vm;
	int fd_vcpu;
	void *mem_addr;
};

struct my_kvm *init_kvm(const char *bzimage_path);
void run_kvm(struct my_kvm *my_kvm);

#endif /* _MY_KVM_H */
