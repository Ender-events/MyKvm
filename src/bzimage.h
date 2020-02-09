#ifndef _BZIMAGE_H_
#define _BZIMAGE_H_

#include <stddef.h>

#include "options.h"

struct bzimage {
	size_t size;
	void *data;
};

void load_bzimage(struct opts opts, void *hw);

#endif /* _BZIMAGE_H_ */
