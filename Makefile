CC = gcc
CPPFLAGS = -D_GNU_SOURCE
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g3 -Og
LDFLAGS = -lcapstone

BIN = my-kvm
OBJS = \
    bzimage.o \
    my-kvm.o \
    utils.o \
    $(NULL)

$(BIN): $(OBJS)

clean:
	$(RM) $(OBJS) $(BIN)
