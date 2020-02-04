#include "bzimage.h"

#include <asm/bootparam.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

int bzimage_init(char* filename, struct bzimage* bzImage)
{
    int fd_bzImage= open(filename, O_RDONLY);
    struct stat st;
    fstat(fd_bzImage, &st);
    bzImage->size = st.st_size;
    bzImage->data = mmap(0, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE,
                        fd_bzImage, 0);
    close(fd_bzImage);
    return 0;
}

void bzimage_boot_protocol(struct setup_header* HdrS)
{
    HdrS->vid_mode = 0xffff; // VGA
    HdrS->type_of_loader = 0xFF; // undefined, maybe Qemu ?
    HdrS->loadflags = 0x01;
    HdrS->ramdisk_image = 0x0;
    HdrS->ramdisk_size = 0x0;
    // HdrS->heap_end_ptr = 0x9800 - 0x200;
    HdrS->ext_loader_ver = 0x0;
    HdrS->cmd_line_ptr = CMDLINE_PTR;
}

void boot_param_init(struct boot_params* boot_params, struct bzimage* bzImage)
{
    memset(boot_params, 0, sizeof(*boot_params));
    struct setup_header* HdrS = ptr_offset(bzImage->data, 0x1f1);
    unsigned char *bzimg = bzImage->data;
    size_t setup_size = bzimg[0x201] + 0x202;
    memcpy(&boot_params->hdr, HdrS, setup_size);
    bzimage_boot_protocol(&boot_params->hdr);
    boot_params->ext_cmd_line_ptr = CMDLINE_PTR;
}

void load_bzimage(char* filename, void* hw)
{
    struct bzimage bzImage;
    memset(ptr_offset(hw, CMDLINE_PTR), 0, 255);
    bzimage_init(filename, &bzImage);
    size_t kernel_size = bzImage.size;
    struct boot_params* boot_params = ptr_offset(hw, BOOT_PARAMS_PTR);
    boot_param_init(boot_params, &bzImage);
    struct setup_header* HdrS = &boot_params->hdr;
    size_t setup_sects = HdrS->setup_sects;
    if (setup_sects == 0)
        setup_sects = 4;
    size_t setup_size = (setup_sects + 1) << 9;
    kernel_size -= setup_size;
    void* kernel = ptr_offset(bzImage.data, setup_size);
    memcpy(ptr_offset(hw, PM_ADDR), kernel, kernel_size);
}
