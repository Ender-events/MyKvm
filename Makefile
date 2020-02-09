CC = gcc
CPPFLAGS = -D_GNU_SOURCE
CFLAGS = -Wall -Wextra -pedantic -std=c99
VPATH = src

DEBUG ?= 0

BIN = my-kvm
OBJS = \
    bzimage.o \
    my-kvm.o \
    options.o \
    utils.o \
    $(NULL)

OBJS_DEBUG = debug.o

ifeq ($(DEBUG), 1)
    CFLAGS += -g3 -O0
    CPPFLAGS += -DDEBUG
    LDFLAGS += -lcapstone
    OBJS += $(OBJS_DEBUG)
endif

$(BIN): $(OBJS)

clean:
	$(RM) $(OBJS) $(OBJS_DEBUG) $(BIN)
