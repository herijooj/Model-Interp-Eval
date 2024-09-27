#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Função auxiliar para ler uma data em texto e converter para 'struct tm'
int str_to_date(struct tm *dest, char *str) {
    char months[12][4] = {"jan", "feb", "mar", "apr", "may", "jun",
                          "jul", "aug", "sep", "oct", "nov", "dec"};
    char mon[4];
    int year;

    mon[3] = '\0';

    sscanf(str, "%2d%3s%4d", &(dest->tm_mday), mon, &year);
    dest->tm_year = year - 1900;

    for (int i = 0; i < 12; i++) {
        if (!strcmp(mon, months[i])) {
            dest->tm_mon = i;
            return 1;
        }
    }
    dest->tm_mon = 0;
    return 0;
}

// Função para ler o arquivo .CTL
void read_ctl_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return;
    }

    char line[256];
    struct tm date;
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "tdef", 4) == 0) {
            char date_str[10];
            sscanf(line, "tdef %*d linear %9s %*s", date_str);
            if (str_to_date(&date, date_str)) {
                printf("Data lida: %02d-%02d-%04d\n", date.tm_mday, date.tm_mon + 1, date.tm_year + 1900);
            } else {
                printf("Erro ao converter a data\n");
            }
        }
    }

    fclose(file);
}

int main() {
    read_ctl_file("lab_0.5x0.5_1950-2020.ctl");
    return 0;
}