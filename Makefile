CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lcapstone

BIN = example-kvm
OBJS = \
    bzimage.o \
    example-kvm.o \
    utils.o \
    $(NULL)

$(BIN): $(OBJS)

clean:
	$(RM) $(OBJS) $(BIN)
