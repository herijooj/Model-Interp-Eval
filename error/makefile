# Nome do executável
EXEC = error_metrics

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fopenmp -O3

# Arquivos fonte e objeto
SRCS = error_metrics.c c_ctl.c
OBJS = $(SRCS:.c=.o)

# Regra padrão para compilar o executável
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS) -lm

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os arquivos objeto e executável
clean:
	rm -f $(OBJS) $(EXEC)

# Regra para rodar o programa
run: $(EXEC)
	./$(EXEC) original_file interpolated_file

.PHONY: clean run