CC = gcc
CFLAGS = -Wall -Wextra -g

BIN = example-kvm
OBJS = \
    bzimage.o \
    example-kvm.o \
    $(NULL)

$(BIN): $(OBJS)

clean:
	$(RM) $(OBJS) $(BIN)
