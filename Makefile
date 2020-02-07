CC = gcc
CPPFLAGS = -D_GNU_SOURCE
CFLAGS = -Wall -Wextra -pedantic -std=c99
LDFLAGS = -lcapstone

DEBUG ?= 1

ifeq ($(DEBUG), 1)
    CFLAGS += -g3 -O0
endif

BIN = my-kvm
OBJS = \
    bzimage.o \
    debug.o \
    my-kvm.o \
    utils.o \
    $(NULL)

$(BIN): $(OBJS)

clean:
	$(RM) $(OBJS) $(BIN)
