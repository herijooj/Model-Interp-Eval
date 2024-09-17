CC = gcc
CFLAGS = -Wall -pedantic -fopenmp -O3
LDLIBS = -lm
VPATH = .

# Directory for executables
BINDIR = ../bin

# Main target
all: $(BINDIR)/mie

# Object files
OBJS = func.o c_ctl.o MIE.o

# Rule for linking
$(BINDIR)/mie: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Rules for compiling
func.o: func.c func.h
	$(CC) $(CFLAGS) -c $<
c_ctl.o: c_ctl.c c_ctl.h
	$(CC) $(CFLAGS) -c $<
MIE.o: MIE.c
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
