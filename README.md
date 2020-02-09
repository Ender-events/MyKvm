# MyKvm

## Usage

```
my-kvm [OPTIONS] bzImage [KERNEL_COMMAND_LINE]
```

## Options

* -h: Display usage message
* --initrd PATH\_TO\_INITRD: Initial ram disk image
* -m: RAM size

## Examples

```
my-kvm bzImage
my-kvm -h
my-kvm bzImage root=/dev/vda
my-kvm --initrd initramfs bzImage root=/dev/vda
my-kvm -m $ram --initrd initramfs bzImage console=ttyS0
```
