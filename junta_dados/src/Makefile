# famoso Makefile do grandessíssimo Prof Maziero
# https://wiki.inf.ufpr.br/maziero/doku.php?id=prog2:o_sistema_make

CC=gcc -fopenmp
CFLAGS = -Wall		# gerar "warnings" detalhados e infos de depuração
LFLIBS = -lm        		# bibliotecas a ligar
LDLIBS = -lm

objs = compose.o c_ctl.o
 
# regra default (primeira regra)
all: compose
 
# regras de ligacao
compose: $(objs)
 
# regras de compilação
compose.o:   compose.c c_ctl.h
c_ctl.o: c_ctl.c c_ctl.h

# compila com flags de depuração 
debug: CFLAGS += -DDEBUG -g
debug: all
 
# remove arquivos temporários
clean:
	-rm -f $(objs)
 
# remove tudo o que não for o código-fonte
purge: clean
	-rm -f compose
