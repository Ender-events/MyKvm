#include "options.h"

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void print_usage(const char *program_name)
{
	printf("Usage: %s [OPTIONS] bzImage [KERNEL_COMMAND_LINE]\n",
	       program_name);
}

struct opts parse_options(int argc, char **argv)
{
	struct opts opts = { NULL, NULL, NULL, 0 };
	int opt;
	int longindex = 0;
	static const struct option long_options[] = {
		{ "initrd", required_argument, 0, 0 }, { 0, 0, 0, 0 }
	};

	while ((opt = getopt_long(argc, argv, "hm:", long_options,
				  &longindex)) != -1) {
		switch (opt) {
		case 0:
			if (longindex == 0)
				opts.initrd_path = optarg;
			else
				err(1, "unknown long opt: %s", argv[optind]);
			break;
		case 'h':
			print_usage(argv[0]);
			exit(0);
			break;
		case 'm':
			opts.ram_size = strtoul(optarg, NULL, 10);
			break;
		default:
			// Error message already displayed by getopt
			exit(1);
		}
	}

	if (optind >= argc)
		errx(1, "no bzImage given");

	opts.bzimage_path = argv[optind];
	opts.cmd_line = argv + optind + 1;

	return opts;
}
