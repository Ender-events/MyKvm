#ifndef _OPTIONS_H_
#define _OPTIONS_H_

struct opts
{
    const char *bzimage_path;
    const char *initrd_path;
    char **cmd_line;
    unsigned long ram_size;
};

struct opts parse_options(int argc, char **argv);

#endif /* _OPTIONS_H_ */
