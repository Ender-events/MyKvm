CC = gcc
CPPFLAGS = -D_GNU_SOURCE
CFLAGS = -Wall -Wextra -pedantic -std=c99

DEBUG ?= 0

BIN = my-kvm
OBJS = \
    bzimage.o \
    my-kvm.o \
    utils.o \
    $(NULL)

ifeq ($(DEBUG), 1)
    CFLAGS += -g3 -O0
    CPPFLAGS += -DDEBUG
    LDFLAGS += -lcapstone
    OBJS += debug.o
endif

$(BIN): $(OBJS)

clean:
	$(RM) $(OBJS) $(BIN)
