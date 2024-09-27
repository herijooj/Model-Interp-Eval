# famoso Makefile do grandessíssimo Prof Maziero
# https://wiki.inf.ufpr.br/maziero/doku.php?id=prog2:o_sistema_make

TARGET=verify_interp
LIB_DOUBLE=c_ctl

# programa em float
OBJS= $(LIB_DOUBLE).o
OBJS64= $(LIB_DOUBLE)_64.o

# programa em double
TARGET64=$(TARGET)_64

# commun objs (independe do tipo)
COBJS =$(TARGET).o geodist.o



#https://stackoverflow.com/questions/1305665/how-to-compile-different-c-files-with-different-cflags-using-makefile

CC=gcc -fopenmp -O3
CFLAGS += -Wall		# gerar "warnings" detalhados e infos de depuração
LDLIBS += -lm


# regra default (primeira regra)
all: $(TARGET)
 

# programa com dado "float"
$(TARGET): $(COBJS) $(OBJS)



# programa com dado "double"
$(TARGET64): $(COBJS) $(OBJS64)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJS64): CFLAGS +=-DDATATYPE=double
$(OBJS64): $(LIB_DOUBLE).c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $^ 



.PHONY: all debug double clean purge

# compila com flags de depuração 
debug: CFLAGS += -DDEBUG -g -O
debug: all

# compila para dados 'double'
double: $(TARGET64)

# remove arquivos temporários
clean:
	-rm -f $(OBJS) $(OBJS64) $(COBJS) $(TARGET64).o
 
# remove tudo o que não for o código-fonte
purge: clean
	-rm -f $(TARGET) $(TARGET64)
