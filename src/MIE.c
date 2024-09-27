#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "c_ctl.h"
#include "error_metrics.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char *original_file;    // original file
    char *interpolated_file; // interpolated file
    int seed;               // seed
    char *output;           // output file
    float percentage;       // percentage
    int help;               // help flag
    int runs;               // number of runs
    int print_csv;          // flag to print CSV
} Arguments;

// Function to show the help message
void show_help() {
    printf("Usage: MIE -f [Original] [Interpolated] [OPTIONS]\n");
    printf("Options:\n");
    printf("  -h, --help                   | This is the flag to show the help message\n");
    printf("  -f [Original] [Interpolated] | The files that we want to compare (required)\n");
    printf("  -s [Seed]                    | The seed for the random number generator (default: random value)\n");
    printf("  -o [Output File]             | The output file where the results will be saved (default: standard output)\n");
    printf("  -p [%%]                       | The percentage of the data that will be used for training (default: 2%%)\n");
    printf("  -r [Runs]                    | The number of runs (default: 1)\n");
    printf("  -c                           | Print the results in CSV format\n");
}

Arguments parse_arguments(int argc, char *argv[]) {
    srand(time(NULL)); // Set the seed for the random number generator
    int seed = rand();
    Arguments args = {NULL, NULL, seed, NULL, 2.0, 0, 1, 0}; // Default runs to 1 and print_csv to 0

    int opt;
    while ((opt = getopt(argc, argv, "hf:s:o:p:r:c")) != -1) {
        switch (opt) {
            case 'h':
                args.help = 1;
                break;
            case 'f':
                args.original_file = optarg;
                args.interpolated_file = argv[optind];
                optind++;
                break;
            case 's':
                args.seed = atoi(optarg);
                break;
            case 'o':
                args.output = optarg;
                break;
            case 'p':
                args.percentage = atof(optarg);
                break;
            case 'r':
                args.runs = atoi(optarg);
                break;
            case 'c':
                args.print_csv = 1;
                break;
            default:
                fprintf(stderr, "Error: invalid option\n");
                show_help();
                exit(1);
        }
    }

    if (args.help) {
        show_help();
        exit(0);
    }

    if (args.original_file == NULL || args.interpolated_file == NULL) {
        fprintf(stderr, "Error: original and interpolated files are required\n");
        show_help();
        exit(1);
    }

    return args;
}
// Function to open .ctl and .bin files
int open_files(const char *ctl_file, info_ctl *info, binary_data **bin_data) {
    // Cria uma cópia não constante da string ctl_file
    char ctl_file_copy[strlen(ctl_file) + 1];
    strcpy(ctl_file_copy, ctl_file);

    if (!open_ctl(info, ctl_file_copy)) {
        fprintf(stderr, "Error reading .ctl file (%s:%d).\n", __FILE__, __LINE__);
        return 0;
    }
    *bin_data = open_bin_info(info);
    if (*bin_data == NULL) {
        fprintf(stderr, "Error reading .bin file (%s:%d).\n", __FILE__, __LINE__);
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    // Parse the command-line arguments
    Arguments args = parse_arguments(argc, argv);

    // Open the original files
    info_ctl original_info;
    binary_data *original_bin_data;
    if (!open_files(args.original_file, &original_info, &original_bin_data)) {
        exit(1);
    }

    // Open the interpolated files
    info_ctl interpolated_info;
    binary_data *interpolated_bin_data;
    if (!open_files(args.interpolated_file, &interpolated_info, &interpolated_bin_data)) {
        free(original_bin_data);
        exit(1);
    }

    // Checks ======================================================================
    if (!check_dim(original_bin_data, interpolated_bin_data)) {
        fprintf(stderr, "Error: the files have different dimensions\n");
        free(original_bin_data);
        free(interpolated_bin_data);
        exit(1);
    }
    if (!compat_grid(&original_info, &interpolated_info)) {
        fprintf(stderr, "Error: the files have different grids\n");
        free(original_bin_data);
        free(interpolated_bin_data);
        exit(1);
    }

    // Array de métodos de interpolação
    char *methods[] = {"--avg", "--idw", "--msh"};
    int num_methods = sizeof(methods) / sizeof(methods[0]);

    for (int method_idx = 0; method_idx < num_methods; method_idx++) {
        char *method = methods[method_idx];

        FILE* csv_file = NULL;
        if (args.print_csv) {
            // Create a file to write the data
            char csv_filename[100];
            sprintf(csv_filename, "data_%.2f%%_%d_runs_%s.csv", args.percentage, args.runs, method);
            csv_file = fopen(csv_filename, "w");
            if (csv_file == NULL) {
                fprintf(stderr, "Error: unable to create the CSV file\n");
                free(original_bin_data);
                free(interpolated_bin_data);
                exit(1);
            }
            fprintf(csv_file, "run, i, real, predicted, error\n");
        }

        // Create a file to write the metrics for each run
        char metrics_filename[100];
        sprintf(metrics_filename, "metrics_%.2f%%_%d_runs_%s.csv", args.percentage, args.runs, method);
        FILE* metrics_file = fopen(metrics_filename, "w");
        if (metrics_file == NULL) {
            fprintf(stderr, "Error: unable to create the metrics CSV file\n");
            free(original_bin_data);
            free(interpolated_bin_data);
            if (csv_file) fclose(csv_file);
            exit(1);
        }

        fprintf(metrics_file, "run, RMSE, MAE, MSE\n");

        float total_rmse = 0.0, total_mae = 0.0, total_mse = 0.0;

        // Novo arquivo ===================================================================
        float percentage = args.percentage / 100.0;
        long int n = 0;

        long int n_data_points = original_bin_data->info.tdef * original_bin_data->info.x.def *
                                original_bin_data->info.y.def;

        for (int i = 0; i < n_data_points; i++) {
            if (original_bin_data->data[i] != original_bin_data->info.undef) {
                n++;
            }
        }
        long int n_train = n * percentage;

        printf("Number of data points: %ld\n", n);
        printf("Number of training data points: %ld\n", n_train);

        for (int run = 0; run < args.runs; run++) {
            printf("Run %d/%d Metric %s/%d\n", run + 1, args.runs, method, args.runs);

            long int *train_data_points = (long int *) malloc(n_train * sizeof(long int));
            if (train_data_points == NULL) {
                fprintf(stderr, "Error: could not allocate memory\n");
                free(original_bin_data);
                free(interpolated_bin_data);
                if (csv_file) fclose(csv_file);
                fclose(metrics_file);
                exit(1);
            }

            // Inicializa o vetor com -1
            for (long int i = 0; i < n_train; i++) {
                train_data_points[i] = -1;
            }

            // Array para marcar pontos de dados já escolhidos
            bool *selected = (bool *) calloc(n_data_points, sizeof(bool));
            if (selected == NULL) {
                fprintf(stderr, "Error: could not allocate memory for selected array\n");
                free(original_bin_data);
                free(interpolated_bin_data);
                free(train_data_points);
                if (csv_file) fclose(csv_file);
                fclose(metrics_file);
                exit(1);
            }

            // Escolhe os pontos de dados de treinamento
            long int j = 0;
            while (j < n_train) {
                long int r = rand() % n_data_points;
                if (original_bin_data->data[r] != original_bin_data->info.undef && !selected[r]) {
                    train_data_points[j] = r;
                    selected[r] = true;
                    j++;
                }
            }

            // Libera a memória do array selected
            free(selected);

            // Cópia dos dados originais
            info_ctl intermediary_info;
            cp_ctl(&intermediary_info, &original_info);

            // Cópia dos dados binários originais
            binary_data* intermediary_bin_data = aloca_bin(original_bin_data->info.x.def, original_bin_data->info.y.def, original_bin_data->info.tdef);
            cp_ctl(&intermediary_bin_data->info, &original_bin_data->info);
            memcpy(intermediary_bin_data->data, original_bin_data->data, sizeof(datatype) * original_bin_data->info.x.def * original_bin_data->info.y.def * original_bin_data->info.tdef);

            // O nome deste deve ser intermediário
            strcpy(intermediary_info.bin_filename, "intermediary.bin");

            // Define os valores dos pontos de dados de treinamento nos dados binários intermediários como indefinidos
            for (long int i = 0; i < n_train; i++) {
                intermediary_bin_data->data[train_data_points[i]] = original_bin_data->info.undef;
            }

            // Escreve os arquivos .bin e .ctl
            write_files(intermediary_bin_data, "intermediary", "intermediary");

            // INTERPOLAÇÃO ==============================================================
            pid_t pid = fork();
            if (pid == 0) { /* processo filho */
                char *argv[] = {"junta_dados/compose", "intermediary.ctl", args.interpolated_file, "final", method, NULL};
                execvp(argv[0], argv);
                
                perror("execvp failed");
                exit(127);
            } else { /* pid != 0; processo pai */
                waitpid(pid, 0, 0); /* espera o filho sair */
            }

            // Lê os dados binários finais
            info_ctl final_info;
            binary_data* final_bin_data;
            if (!open_files("final.ctl", &final_info, &final_bin_data)) {
                free(original_bin_data);
                free(interpolated_bin_data);
                free(intermediary_bin_data);
                free(train_data_points);
                if (csv_file) fclose(csv_file);
                fclose(metrics_file);
                exit(1);
            }

            if (args.print_csv) {
                // Escreve apenas os pontos de dados de treinamento
                for (long int i = 0; i < n_train; i++) {
                    fprintf(csv_file, "%d, %ld, %f, %f, %f\n", run + 1, train_data_points[i], original_bin_data->data[train_data_points[i]], final_bin_data->data[train_data_points[i]], final_bin_data->data[train_data_points[i]] - original_bin_data->data[train_data_points[i]]);
                }
            }

            // Calcula as métricas de erro
            float rmsev = rmse(original_bin_data, final_bin_data, train_data_points, n_train);
            float maev = mae(original_bin_data, final_bin_data, train_data_points, n_train);
            float msev = mse(original_bin_data, final_bin_data, train_data_points, n_train);
            
            // Escreve as métricas de erro no arquivo de métricas
            fprintf(metrics_file, "%d, %f, %f, %f\n", run + 1, rmsev, maev, msev);

            // Acumula as métricas para cálculo da média
            total_rmse += rmsev;
            total_mae += maev;
            total_mse += msev;

            // Libera a memória alocada para esta exescução
            free(train_data_points);
            free_bin(intermediary_bin_data);
            free_bin(final_bin_data);
        }

        // Calcula as métricas médias
        float avg_rmse = total_rmse / args.runs;
        float avg_mae = total_mae / args.runs;
        float avg_mse = total_mse / args.runs;

        // Escreve as métricas médias no arquivo de métricas
        fprintf(metrics_file, "Average, %f, %f, %f\n", avg_rmse, avg_mae, avg_mse);

        // Deleta os arquivos intermediários
        remove("intermediary.ctl");
        remove("intermediary.bin");
        remove("final.ctl");
        remove("final.bin");

        // Fecha os arquivos CSV
        if (csv_file) fclose(csv_file);
        fclose(metrics_file);
    }

    // Libera a memória alocada
    free_bin(original_bin_data);
    free_bin(interpolated_bin_data);

    return 0;
}