#ifndef _UTILS_H_
#define _UTILS_H_

#define BOOT_PARAMS_PTR 0x10000
#define CMDLINE_PTR 0x20000
#define PM_ADDR 0x100000

#define MAX_KVM_CPUID_ENTRIES 100

void* ptr_offset(char* ptr, int offset);

#endif /* _UTILS_H_ */
