#include "c_ctl.h"
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ERROR (0.000001) // Erro permitido (float)

void saferFree(void **pp){
	if(pp != NULL && *pp != NULL){
		free(*pp);
		*pp = NULL;
	}
}


// função auxiliar para ler uma data em texto e converter para 'struct tm'
int str_to_date(struct tm *dest, char *str) {
    // meses possíveis
    char months[12][4] = {"jan", "feb", "mar", "apr", "may", "jun",
                          "jul", "aug", "sep", "oct", "nov", "dec"};
    char mon[4];
    int year;

    mon[3] = '\0';

    // lendo a string
    sscanf(str, "%d%c%c%c%d", &(dest->tm_mday), mon, mon + 1, mon + 2, &year);

    // pela documentação da 'struct tm' o campo 'tm_year' é: anos depois de 1900
    dest->tm_year = year - 1900;

    for (int i = 0; i < 12; i++) {
        if (!strcmp(mon, months[i])) {
            dest->tm_mon = i;
            return 1;
        }
    }
    // janeiro por padrão
    dest->tm_mon = 0;

    return 0;
}

// função auxiliar que retorna o tipo de arquivo dada a string presente no ctl.
// Ex: tdef     552 linear 01jan1970 1mo "1mo" é a string correspondente a dados
// mensais
int str_to_ttype(char *str) {

    int size = strlen(str);

    if (size > 2) {
        // verifica se os últimos 2 digitos da string
        // correspondem a algum formato
        if (!strcmp(str + size - 2, "yr"))
            return T_YEAR;
        if (!strcmp(str + size - 2, "mo"))
            return T_MONTH;
        if (!strcmp(str + size - 2, "dy"))
            return T_DAY;
    }

    return 0;
}

// Abre o arquivo ctl 'name' e salva as informações em 'info_field'
int open_ctl(info_ctl *info_field, char *name) {

    char buff[BUFF_SIZE];
    char tmp_str[STR_SIZE];

    FILE *ctl_file;

    // abre o arquivo para leitura
    if (!(ctl_file = fopen(name, "r"))) {
        fprintf(stderr, "ERRO: Não foi possível abrir arquivo para leitura (%s:%d).\n", __FILE__, __LINE__);
        return 0;
    }

    // inicializando string
    memset(info_field->dump, 0, BUFF_SIZE);

    // nome do arquivo
    if (fscanf(ctl_file, "%*s %" STR(STR_SIZE) "s\n", buff) == EOF)
        return 0;
    if (buff[0] == '^') {
        // adiciona o caminho do ctl antes do arquivo binario
        strncpy(tmp_str, name, STR_SIZE);
        memcpy(tmp_str, dirname(tmp_str), STR_SIZE);
        strcat(tmp_str, "/");

        if ((strlen(tmp_str) + strlen(buff)) > STR_SIZE) {
            fprintf(stderr, "ERRO: Nome de arquivo muito longo (max %d).\n",
                    STR_SIZE);
            return 0;
        }

        strcpy(info_field->bin_filename, tmp_str);
        strcat(info_field->bin_filename, buff + 1);
    } else {
        // copia o nome do arquivo da forma que está escrito no ctl
        strncpy(info_field->bin_filename, buff, STR_SIZE);
    }

    // pula uma linha
    if (!fgets(buff, BUFF_SIZE, ctl_file))
        return 0;

    // undef
    if (fscanf(ctl_file, "%*s %f\n", &(info_field->undef)) == EOF)
        return 0;

    // xdef
    if (fscanf(ctl_file, "%*s %lu %*s %f %f\n", &(info_field->x.def),
               &(info_field->x.i), &(info_field->x.size)) == EOF)
        return 0;
    info_field->x.i = wrap_val(info_field->x.i, MIN_X, MAX_X);
    info_field->x.f =
        (info_field->x.i) + (info_field->x.def) * (info_field->x.size);

    // ydef
    if (fscanf(ctl_file, "%*s %lu %*s %f %f\n", &(info_field->y.def),
               &(info_field->y.i), &(info_field->y.size)) == EOF)
        return 0;
    info_field->y.i = wrap_val(info_field->y.i, MIN_Y, MAX_Y);
    info_field->y.f =
        (info_field->y.i) + (info_field->y.def) * (info_field->y.size);

    // zdef
    if (fscanf(ctl_file, "%*s %lu", &(info_field->zdef)) == EOF)
        return 0;
    if (!fgets(buff, BUFF_SIZE, ctl_file))
        return 0;

    // tdef
    if (fscanf(ctl_file, "%*s %lu %*s", &(info_field->tdef)) == EOF)
        return 0;
    if (!fgets(info_field->tdesc, STR_SIZE, ctl_file))
        return 0;

    // resto do arquivo
    //(void)! para ignorar o retorno da função
    (void)!fread(info_field->dump, 1, BUFF_SIZE, ctl_file);

    // preenchendo informações adicionais
    sscanf(info_field->tdesc, "%s", tmp_str);

    // Le a data em texto para a struct
    if (!str_to_date(&(info_field->date_i), tmp_str)) {
        fprintf(stderr,
                "ERRO: data do arquivo inválida. Formato aceito: ddmmaaaa (%s:%d).\n", __FILE__, __LINE__);
        return 0;
    }

    // le o incremento em texto e converte para T_TYPE
    sscanf(info_field->tdesc, "%*s %s", tmp_str);
    if (!(info_field->ttype = str_to_ttype(tmp_str))) {
        fprintf(
            stderr,
            "ERRO: Tipo de dado não reconhecido. Tipos aceitos: yr mo dy (%s:%d).\n", __FILE__, __LINE__);
        return 0;
    }

    info_field->t_from_date_i = date_to_t(info_field);

    fclose(ctl_file);
    return 1;
}

// Escreve um arquivo ctl 'name' com as informações em 'info_field'
int write_ctl(info_ctl *info_field, char *name, char *title) {

    FILE *ctl_file;

    // abre o arquivo para escrita
    if (!(ctl_file = fopen(name, "w")))
        return 0;

    // nome do arquivo
    fprintf(ctl_file, "dset ^%s\n", basename(info_field->bin_filename));

    // titulo
    fprintf(ctl_file, "title %s\n", title);

    // undef
    fprintf(ctl_file, "undef %f\n", info_field->undef);

    // xdef
    fprintf(ctl_file, "xdef %lu linear %f %f\n", info_field->x.def,
            info_field->x.i, info_field->x.size);

    // ydef
    fprintf(ctl_file, "ydef %lu linear %f %f\n", info_field->y.def,
            info_field->y.i, info_field->y.size);

    // zdef
    fprintf(ctl_file, "zdef %lu levels 1\n", info_field->zdef);

    // tdef
    fprintf(ctl_file, "tdef %lu linear %s", info_field->tdef,
            info_field->tdesc);

    // resto
    fwrite(info_field->dump, 1, strlen(info_field->dump), ctl_file);

    fclose(ctl_file);
    return 1;
}

// Abre um arquivo .bin com as informações do arquivo .ctl 'name'
// Retorna o ponteiro para a struct de dado, ou NULL em erro
binary_data *open_bin_ctl(char *name) {
    info_ctl info_field;

    if (!open_ctl(&info_field, name))
        return NULL;

    return open_bin_info(&info_field);
}

// Abre um arquivo .bin com as informações contidas na struct 'info'
// Retorna o ponteiro para a struct de dado, ou NULL em erro
binary_data *open_bin_info(info_ctl *info_field) {

    binary_data *data;

    data = open_bin(info_field->bin_filename, info_field->x.def,
                    info_field->y.def, info_field->tdef);

    if (!data)
        return NULL;

    cp_ctl(&(data->info), info_field);

    return data;
}

size_t get_pos(info_ctl *info_field, size_t x, size_t y, size_t t) {
    size_t dx = info_field->x.def;
    size_t dy = info_field->y.def;

    //printf("return (x:%lu+dx:%lu*(y:%lu+dy:%lu*t:%lu))\n", x, dx, y, dy, t);
    return (x + dx * (y + dy * t));
}

binary_data *aloca_bin(size_t x, size_t y, size_t t) {
    binary_data *bin_data;

    if (!(bin_data = malloc(sizeof(binary_data)))){
        fprintf(stderr, "Erro ao alocar memória para bin_data (%s:%d).\n", __FILE__, __LINE__);
        return NULL;
    }
    if (!(bin_data->data = malloc(x * y * t * sizeof(datatype)))) {
        fprintf(stderr, "Erro ao alocar memória para bin_data->data (%s:%d).\n", __FILE__, __LINE__);
        safeFree(bin_data);
        return NULL;
    }

    return bin_data;
}

// Abre um arquivo o .bin 'name' com as informações passadas por parametro
// Retorna o ponteiro para a struct de dado, ou NULL em erro
binary_data *open_bin(char *name, size_t x, size_t y, size_t t) {
    FILE *bin_file;
    binary_data *bin_data;

    // abrindo o arquivo
    bin_file = fopen(name, "r");
    if (!bin_file) {
        fprintf(
            stderr,
            "ERRO: não foi possível abrir arquivo binário para leitura (%s). (%s:%d).\n",
            name,__FILE__, __LINE__);
        return NULL;
    }
    // alocando estrutura
    if (!(bin_data = aloca_bin(x, y, t))) {
        fclose(bin_file);
        return NULL;
    }

    if (fread(bin_data->data, sizeof(datatype), x * y * t, bin_file) <
        x * y * t) {
        fclose(bin_file);
        return NULL;
    }

    fclose(bin_file);
    return bin_data;
}

// Libera a alocação de 'bin_data'
binary_data *free_bin(binary_data *bin_data) {
    safeFree(bin_data->data);
    safeFree(bin_data);

    return NULL;
}

// Imprime a matriz tridimensional na tela
void print_bin(binary_data *bin_data) {
    size_t pos;
    for (size_t t = 0; t < bin_data->info.tdef; t++) {
        for (size_t y = 0; y < bin_data->info.y.def; y++) {
            for (size_t x = 0; x < bin_data->info.x.def; x++) {
                pos = get_pos(&(bin_data->info), x, y, t);
                printf("[%3ld,%3ld,%5ld] %10.6f\n", (x + 1), (y + 1), (t + 1),
                       bin_data->data[pos]);
            }
        }
    }
}

// Escreve um arquivo binário para a matriz 'bin_data' e um ctl para 'info'
int write_bin(binary_data *bin_data) {
    FILE *bin_file;

    // abre o arquivo para escrita em modo binário
    bin_file = fopen(bin_data->info.bin_filename, "wb");
    if (!bin_file) {
        fprintf(stderr, "Erro ao abrir o arquivo %s para escrita (%s:%d).\n", bin_data->info.bin_filename, __FILE__, __LINE__);
        return 0;
    }

    // quantidade de elementos do arquivo
    size_t dims = bin_data->info.x.def * bin_data->info.y.def * bin_data->info.tdef;
    
    if (fwrite(bin_data->data, sizeof(datatype), dims, bin_file) < dims) {
        fprintf(stderr, "Erro ao escrever binario. (%s:%d).\n", __FILE__, __LINE__);
        fclose(bin_file);
        return 0;
    }
    fclose(bin_file);
    return 1;
}

int write_files(binary_data *bin_data, char *name, char *title) {
    char name_bin[STR_SIZE];
    char name_ctl[STR_SIZE];

    // altera o nome dos arquivos
    char *suffix_bin = ".bin";
    strcpy(name_bin, name);
    strcat(name_bin, suffix_bin);

    char *suffix_ctl = ".ctl";
    strcpy(name_ctl, name);
    strcat(name_ctl, suffix_ctl);

    strcpy(bin_data->info.bin_filename, name_bin);

    // executa as funções de criar arquivos
    return (write_bin(bin_data) && write_ctl(&(bin_data->info), name_ctl, title));
}

datatype get_data_val(binary_data *ref, binary_data *src, int x, int y, int t) {
    coordtype x_pos, y_pos;
    int x_src, y_src, t_src;

    // a posição é o ponto inicial + distância do índice até o início
    x_pos = ref->info.x.i + x * ref->info.x.size;
    y_pos = ref->info.y.i + y * ref->info.y.size;

    // a posição de t é a diferença da quantidade de tempo passado desde
    // 01/01/0001 mais o deslocamento em t
    t_src = ref->info.t_from_date_i - src->info.t_from_date_i + t;

    // o índice da matriz de 'src' é o valor global ajustado para o valor local
    // de 'src'
    x_src = (int)((x_pos - src->info.x.i) / src->info.x.size);
    y_src = (int)((y_pos - src->info.y.i) / src->info.y.size);

    // retorna undef caso o ponto (x_src,y_src,t_src) não exista na matriz de
    // dados de 'src'
    if (!contains(src, x_src, y_src, t_src))
        return src->info.undef;

    // retorna o valor da quadrícula equivalente a
    // ref->data[get_pos(&(ref->info),x,y,t)]
    return src->data[get_pos(&(src->info), x_src, y_src, t_src)];
}

datatype set_data_val(binary_data *dest, int x, int y, int t, datatype value) {
    if (!contains(dest, x, y, t))
        return dest->info.undef;
    return dest->data[get_pos(&(dest->info), x, y, t)] = value;
}

// copia as informações de 'src' para 'dest'
void cp_coord(info_coord *dest, info_coord *src) {
    dest->def = src->def;
    dest->size = src->size;
    dest->i = src->i;
    dest->f = src->f;
}

// copia as informações de 'src' para 'dest'
void cp_ctl(info_ctl *dest, info_ctl *src) {
    strncpy(dest->bin_filename, src->bin_filename, STR_SIZE);
    dest->undef = src->undef;

    cp_coord(&(dest->x), &(src->x));
    cp_coord(&(dest->y), &(src->y));

    dest->zdef = src->zdef;
    dest->tdef = src->tdef;

    cp_date_ctl(dest, src);

    strncpy(dest->dump, src->dump, BUFF_SIZE);
}

void cp_date_ctl(info_ctl *dest, info_ctl *src) {
    dest->date_i.tm_mday = src->date_i.tm_mday;
    dest->date_i.tm_mon = src->date_i.tm_mon;
    dest->date_i.tm_year = src->date_i.tm_year;

    dest->ttype = src->ttype;
    dest->t_from_date_i = src->t_from_date_i;

    strncpy(dest->tdesc, src->tdesc, STR_SIZE);
}

/* Copia o valor de 'src' para 'dest', recebendo a posição (x,y,t) de 'dest'
 * Se a coordenada convertida de 'dest' estiver fora da matriz de dados de 'src'
 * ou se o valor de src for indefinido
 * o valor copiado é 'dest->info.undef'.
 * Retorna o valor copiado.
 **/
datatype cp_data_val(binary_data *dest, binary_data *src, int x, int y, int t) {
    if (contains(dest, x, y, t)) {

        datatype value = get_data_val(dest, src, x, y, t);

        if (fabs(value - src->info.undef) < ERROR)
            value = dest->info.undef;

        return set_data_val(dest, x, y, t, value);
    }
    return dest->info.undef;
}

/* Retorna 1 se a coordenada (x,y,t) está dentro dos limites de 'bin_data'
 * Retorna 0 caso contrário.
 */
int contains(binary_data *bin_data, int x, int y, int t) {
    return ((x < bin_data->info.x.def) && (x >= 0) &&
            (y < bin_data->info.y.def) && (y >= 0) &&
            (t < bin_data->info.tdef) && (t >= 0));
}

// Retorna o valor entre "limites circulares"
// Se o valor passar do máximo, volta ao início
// E vice-versa
coordtype wrap_val(coordtype val, coordtype min, coordtype max) {
    return (val < min) ? (max - fmod(min - val, max - min))
                       : (min + fmod(val - min, max - min));
}

// retorna a quantidade de passos t desde 01/01/0001
int date_to_t(info_ctl *ctl) {
    // https://en.cppreference.com/w/c/chrono/tm

    // "+1" pois meses são de 0 a 11
    int mon = ctl->date_i.tm_mon + 1;
    // "+1900" pois 'tm_year' é a quantidade de anos após 1900
    int year = ctl->date_i.tm_year + 1900;

    switch (ctl->ttype) {
    case T_YEAR:
        return year;
    case T_MONTH:
        return year * 12 + mon;
    case T_DAY:
        return date_to_days(ctl->date_i.tm_mday, mon, year);
    }

    return 0;
}

/* Verifica se dois grids são compatíveis.
 * Retorna 1 se os dois são compatíveis, retorna 0 caso contrário.
 */
int compat_grid(info_ctl *a, info_ctl *b) {
    return (fabs((fmod(fabs(a->x.i - b->x.i), a->x.size)) < ERROR) &&
            fabs((fmod(fabs(a->y.i - b->y.i), a->y.size)) < ERROR));
}

// retorna 1 se ano eh bissexto, 0 caso contrario
int eh_bissexto(int ano) {
    return (((!(ano % 4)) && ano % 100) || !(ano % 400));
}

// retorna quantidade de dias desde o inicio do ano ate fim do mes anterior ao
// mes passado por argumento
int sum_days_till_month(int mes) {
    int meses[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int dias = 0;

    for (int i = 0; i < mes; i++) {
        dias += meses[i];
    }

    return dias;
}

// retorna quantidade de dias passados desde o 01/01/0001 ate dia/mes/ano
int date_to_days(int dia, int mes, int ano) {

    int num_dias;
    int dia_bissexto = eh_bissexto(ano) && (mes > 2);

    // quantidade de dias entre 01/01/01 e fim do ano anterior
    num_dias = (ano - 1) * 365 + (int)(ano - 1) / 4 - (int)(ano - 1) / 100 +
               (int)(ano - 1) / 400;
    // quantidade total de dias do ano atual
    num_dias += (sum_days_till_month(mes - 1)) + dia + dia_bissexto;

    return num_dias;
}
