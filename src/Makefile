CC = gcc
CFLAGS = -Wall -pedantic -fopenmp -O3
LDLIBS = -lm
VPATH = .

# Diretório para executáveis
BINDIR = ../bin

# Alvos principais
all: $(BINDIR)/mie

# Arquivos objeto comuns
OBJS = c_ctl.o error_metrics.o MIE.o

# Programa em float
LIB_DOUBLE = c_ctl
OBJS64 = $(LIB_DOUBLE)_64.o

# Regras de linkagem
$(BINDIR)/mie: MIE.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Regras de compilação
%.o: %.c
	$(CC) $(CFLAGS) -c $<

$(OBJS64): CFLAGS += -DDATATYPE=double
$(OBJS64): $(LIB_DOUBLE).c
	$(CC) $(CFLAGS) -c -o $@ $^

# Compilação com flags de depuração
debug: CFLAGS += -DDEBUG -g
debug: all

# Compilação para dados 'double'
double: $(OBJS64)

# Regras de limpeza
clean:
	rm -f $(OBJS) $(OBJS64) $(BINDIR)/mie

purge: clean

# Regra para executar
run: $(BINDIR)/mie
	$(BINDIR)/mie

.PHONY: all debug double clean purge run