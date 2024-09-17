CC = gcc
CFLAGS = -Wall -pedantic -fopenmp -O3
LDLIBS = -lm
VPATH = .

# Directory for executables
# BINDIR = ../bin
BINDIR = .

# Main target
all: $(BINDIR)/mie

# Object files
OBJS = func.o c_ctl.o MIE.o error_metrics.o

# Rule for linking
$(BINDIR)/mie: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Rules for compiling
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Debug build
debug: CFLAGS += -DDEBUG -g
debug: all

# Clean rule
clean:
	rm -f $(OBJS) $(BINDIR)/mie

# Run rule
run: $(BINDIR)/mie
	$(BINDIR)/mie

.PHONY: all debug clean run