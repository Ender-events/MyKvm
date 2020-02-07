#include "my-kvm.h"

#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <linux/kvm_para.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "bzimage.h"
#include "debug.h"
#include "utils.h"

#define COM1 0x3f8
#define LSR 5

unsigned char out_o[] = {
	/* begin: */
	0xb0, 0x61, /* mov $0x61, %al */
	0x66, 0xba, 0xf8, 0x03, /* mov $0x3f8, %dx */
	0xee, /* out %al, (%dx) */
	0xeb, 0xf7 /* jmp begin */
};

static void handle_io(struct kvm_run *run_state)
{
	uint64_t offset = run_state->io.data_offset;
	if (run_state->io.port == COM1 &&
			run_state->io.direction == KVM_EXIT_IO_OUT) {
		uint32_t size = run_state->io.size;
		write(STDOUT_FILENO, (char *)run_state + offset, size * run_state->io.count);
	}
	else if (run_state->io.port == COM1 + LSR &&
			run_state->io.direction == KVM_EXIT_IO_IN) {
		char* value = (char *)run_state + offset;
		//*value = 0x20;
	}
}

static int init_vm(int fd_kvm)
{
	int fd_vm = ioctl(fd_kvm, KVM_CREATE_VM, 0);
	if (fd_vm < 0) {
		err(1, "unable to create vm");
	}

	ioctl(fd_vm, KVM_SET_TSS_ADDR, 0xffffd000);
	__u64 map_addr = 0xffffc000;
	ioctl(fd_vm, KVM_SET_IDENTITY_MAP_ADDR, &map_addr);

	ioctl(fd_vm, KVM_CREATE_IRQCHIP, 0);

	return fd_vm;
}

static void init_registers(int fd_vcpu)
{
	struct kvm_sregs sregs;
	ioctl(fd_vcpu, KVM_GET_SREGS, &sregs);

#define set_segment_selector(Seg, Base, Limit, G)                              \
	do {                                                                   \
		Seg.base = Base;                                               \
		Seg.limit = Limit;                                             \
		Seg.g = G;                                                     \
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
}

static void init_cpuid(int fd_kvm, int fd_vcpu)
{
	struct kvm_cpuid2 *kvm_cpuid = calloc(
		1, sizeof(*kvm_cpuid) +
			   MAX_KVM_CPUID_ENTRIES * sizeof(*kvm_cpuid->entries));
	kvm_cpuid->nent = MAX_KVM_CPUID_ENTRIES;
	ioctl(fd_kvm, KVM_GET_SUPPORTED_CPUID, kvm_cpuid);

	for (unsigned int i = 0; i < kvm_cpuid->nent; ++i) {
		struct kvm_cpuid_entry2 *entry = &kvm_cpuid->entries[i];
		if (entry->function == 0) {
			entry->eax = KVM_CPUID_FEATURES;
			entry->ebx = 0x4b4d564b; // KVMK
			entry->ecx = 0x564b4d56; // VMKV
			entry->edx = 0x4d; // M
		}
	}
	ioctl(fd_vcpu, KVM_SET_CPUID2, kvm_cpuid);
}

struct my_kvm *init_kvm(const char *bzimage_path)
{
	struct my_kvm *my_kvm = calloc(1, sizeof(*my_kvm));
	if (!my_kvm)
		err(1, "unable to calloc my_kvm struct");

	int fd_kvm = open("/dev/kvm", O_RDWR);
	if (fd_kvm < 0) {
		err(1, "unable to open /dev/kvm");
	}

	int fd_vm = init_vm(fd_kvm);

	void *mem_addr = mmap(NULL, 1 << 30, PROT_READ | PROT_WRITE,
			      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	//memcpy(mem_addr, out_o, sizeof(out_o));
	load_bzimage(bzimage_path, mem_addr);

	struct kvm_userspace_memory_region region = {
		.slot = 0,
		.flags = 0,
		.guest_phys_addr = 0x0,
		.memory_size = 1 << 30,
		.userspace_addr = (__u64)mem_addr,
	};
	ioctl(fd_vm, KVM_SET_USER_MEMORY_REGION, &region);

	int fd_vcpu = ioctl(fd_vm, KVM_CREATE_VCPU, 0);

	init_registers(fd_vcpu);
	init_cpuid(fd_kvm, fd_vcpu);

	my_kvm->fd_kvm = fd_kvm;
	my_kvm->fd_vm = fd_vm;
	my_kvm->fd_vcpu = fd_vcpu;
	my_kvm->mem_addr = mem_addr;
	return my_kvm;
}

void run_kvm(struct my_kvm *my_kvm)
{
	int kvm_run_size = ioctl(my_kvm->fd_kvm, KVM_GET_VCPU_MMAP_SIZE, 0);
	struct kvm_run *run_state =
		mmap(0, kvm_run_size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
		     my_kvm->fd_vcpu, 0);

#ifdef DEBUG
	csh handle = init_debug(my_kvm->fd_vcpu);
#endif

	for (;;) {
		int rc = ioctl(my_kvm->fd_vcpu, KVM_RUN, 0);

		if (rc < 0)
			warn("KVM_RUN");
		switch (run_state->exit_reason) {
		case KVM_EXIT_IO:
#ifdef DEBUG
			puts("KVM_EXIT_IO");
#endif
			handle_io(run_state);
			break;
		case KVM_EXIT_SHUTDOWN:
			puts("KVM_EXIT_SHUTDOWN");
#ifdef DEBUG
			debug_dump(handle, my_kvm);
#endif
			return;
		default:
#ifdef DEBUG
			printf("exit reason: %d\n", run_state->exit_reason);
			debug_dump(handle, my_kvm);
#endif
			break;
		}
	}
}

int main(int argc, char **argv)
{
	(void)argc;
	struct my_kvm *my_kvm = init_kvm(argv[1]);
	run_kvm(my_kvm);
	return 0;
}
