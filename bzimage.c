#include "bzimage.h"

#include <asm/bootparam.h>
#include <asm/e820.h>
#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

static int bzimage_init(const char *filename, struct bzimage *bzImage)
{
	int fd_bzImage = open(filename, O_RDONLY);
	struct stat st;
	fstat(fd_bzImage, &st);
	bzImage->size = st.st_size;
	bzImage->data = mmap(0, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
			     fd_bzImage, 0);
	close(fd_bzImage);
	return 0;
}

static void bzimage_boot_protocol(struct setup_header *HdrS)
{
	HdrS->vid_mode = 0xFFFF; // VGA
	HdrS->type_of_loader = 0xFF; // undefined, maybe Qemu ?
	HdrS->ramdisk_image = 0x0;
	HdrS->ramdisk_size = 0x0;
	HdrS->loadflags |= CAN_USE_HEAP | 0x01 | KEEP_SEGMENTS;
	HdrS->heap_end_ptr = 0xFE00;
	HdrS->ext_loader_ver = 0x0;
	HdrS->cmd_line_ptr = CMDLINE_PTR;
}

static void e820_init(struct boot_e820_entry *entry)
{
	entry[0] = (struct boot_e820_entry){
		.addr = 0x0,
		.size = 0x1000,
		.type = E820_UNUSABLE
	};
	entry[1] = (struct boot_e820_entry){
		.addr = 0x1000,
		.size = (1 << 30) - 0x1000,
		.type = E820_RAM
	};
}

static void boot_param_init(struct boot_params *boot_params,
			    struct bzimage *bzImage)
{
	memset(boot_params, 0, sizeof(*boot_params));
	memcpy(boot_params, bzImage->data, sizeof(struct boot_params));
	bzimage_boot_protocol(&boot_params->hdr);
	e820_init(boot_params->e820_table);
}

static void load_initramfs(const char *initramfs_path,
						   struct setup_header *HdrS,
						   void *hw)
{
	if (HdrS->initrd_addr_max < INITRAMFS_ADDR)
		errx(1, "Can't load initramfs to the wanted address");
	int fd_initramfs = open(initramfs_path, O_RDONLY);
	if (fd_initramfs == -1)
		errx(1, "Can't open initramfs");
	struct stat st;
	fstat(fd_initramfs, &st);
	uint64_t initramfs_size = st.st_size;
	void *initramfs_data = mmap(0, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
						  fd_initramfs, 0);
	close(fd_initramfs);
	memcpy(ptr_offset(hw, INITRAMFS_ADDR), initramfs_data, initramfs_size);
	HdrS->ramdisk_image = INITRAMFS_ADDR;
	HdrS->ramdisk_size = initramfs_size;
	munmap(initramfs_data, initramfs_size);
}

void load_bzimage(const char *filename, const char *initramfs_path, void *hw)
{
	struct bzimage bzImage;
	bzimage_init(filename, &bzImage);
	size_t kernel_size = bzImage.size;
	struct boot_params *boot_params = ptr_offset(hw, BOOT_PARAMS_PTR);
	boot_param_init(boot_params, &bzImage);
	struct setup_header *HdrS = &boot_params->hdr;
	memset(ptr_offset(hw, CMDLINE_PTR), 0, HdrS->cmdline_size);
	//strcpy(ptr_offset(hw, CMDLINE_PTR), "earlyprintk=serial,ttyS0,115200");
	strcpy(ptr_offset(hw, CMDLINE_PTR), "root=/dev/ram0 console=ttyS0");
	size_t setup_sects = HdrS->setup_sects;
	if (setup_sects == 0)
		setup_sects = 4;
	size_t setup_size = (setup_sects + 1) << 9;
	kernel_size -= setup_size;
	void *kernel = ptr_offset(bzImage.data, setup_size);
	memcpy(ptr_offset(hw, PM_ADDR), kernel, kernel_size);
	load_initramfs(initramfs_path, HdrS, hw);
}
