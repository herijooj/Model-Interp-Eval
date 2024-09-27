#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "c_ctl.h"
#include "error_metrics.h"

typedef struct {
    char *original_file;
    char *interpolated_file;
    int seed;
    char *output;
    float percentage;
    int help;
    int runs;
    int print_csv;
} Arguments;

void show_help() {
    printf("Usage: MIE -f [Original] [Interpolated] [OPTIONS]\n");
    printf("Options:\n");
    printf("  -h, --help                   | Show help message\n");
    printf("  -f [Original] [Interpolated] | Files to compare (required)\n");
    printf("  -s [Seed]                    | Seed for RNG (default: random)\n");
    printf("  -o [Output File]             | Output file (default: stdout)\n");
    printf("  -p [%%]                       | Percentage for training (default: 2%%)\n");
    printf("  -r [Runs]                    | Number of runs (default: 1)\n");
    printf("  -c                           | Print results in CSV format\n");
}

Arguments parse_arguments(int argc, char *argv[]) {
    srand(time(NULL));
    Arguments args = {NULL, NULL, rand(), NULL, 2.0, 0, 1, 0};

    int opt;
    while ((opt = getopt(argc, argv, "hf:s:o:p:r:c")) != -1) {
        switch (opt) {
            case 'h':
                args.help = 1;
                break;
            case 'f':
                args.original_file = optarg;
                args.interpolated_file = argv[optind++];
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

    if (args.help || !args.original_file || !args.interpolated_file) {
        show_help();
        exit(args.help ? 0 : 1);
    }

    return args;
}

int open_files(const char *ctl_file, info_ctl *info, binary_data **bin_data) {
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
    Arguments args = parse_arguments(argc, argv);

    info_ctl original_info, interpolated_info;
    binary_data *original_bin_data, *interpolated_bin_data;

    if (!open_files(args.original_file, &original_info, &original_bin_data) ||
        !open_files(args.interpolated_file, &interpolated_info, &interpolated_bin_data)) {
        free(original_bin_data);
        exit(1);
    }

    if (!check_dim(original_bin_data, interpolated_bin_data) || 
        !compat_grid(&original_info, &interpolated_info)) {
        fprintf(stderr, "Error: incompatible files\n");
        free(original_bin_data);
        free(interpolated_bin_data);
        exit(1);
    }

    char *methods[] = {"--avg", "--idw", "--msh"};
    int num_methods = sizeof(methods) / sizeof(methods[0]);

    long int n_data_points = original_bin_data->info.tdef * original_bin_data->info.x.def * original_bin_data->info.y.def;
    long int n = 0;
    for (int i = 0; i < n_data_points; i++) {
        if (original_bin_data->data[i] != original_bin_data->info.undef) n++;
    }
    long int n_train = n * (args.percentage / 100.0);

    long int *train_data_points = malloc(n_train * sizeof(long int));
    bool *selected = calloc(n_data_points, sizeof(bool));

    if (!train_data_points || !selected) {
        fprintf(stderr, "Error: could not allocate memory\n");
        free(original_bin_data);
        free(interpolated_bin_data);
        free(train_data_points);
        free(selected);
        exit(1);
    }

    for (int method_idx = 0; method_idx < num_methods; method_idx++) {
        char *method = methods[method_idx];
        FILE *csv_file = NULL, *metrics_file = NULL;

        if (args.print_csv) {
            char csv_filename[100];
            sprintf(csv_filename, "data_%.2f%%_%d_runs_%s.csv", args.percentage, args.runs, method);
            csv_file = fopen(csv_filename, "w");
            if (!csv_file) {
                fprintf(stderr, "Error: unable to create CSV file\n");
                free(original_bin_data);
                free(interpolated_bin_data);
                free(train_data_points);
                free(selected);
                exit(1);
            }
            fprintf(csv_file, "run, i, real, predicted, error\n");
        }

        char metrics_filename[100];
        sprintf(metrics_filename, "metrics_%.2f%%_%d_runs_%s.csv", args.percentage, args.runs, method);
        metrics_file = fopen(metrics_filename, "w");
        if (!metrics_file) {
            fprintf(stderr, "Error: unable to create metrics file\n");
            free(original_bin_data);
            free(interpolated_bin_data);
            free(train_data_points);
            free(selected);
            if (csv_file) fclose(csv_file);
            exit(1);
        }
        fprintf(metrics_file, "run, RMSE, MAE, MSE\n");

        float total_rmse = 0.0, total_mae = 0.0, total_mse = 0.0;

        for (int run = 0; run < args.runs; run++) {
            printf("Run %d/%d Metric %s/%d\n", run + 1, args.runs, method, args.runs);

            memset(selected, 0, n_data_points * sizeof(bool));
            long int j = 0;
            while (j < n_train) {
                long int r = rand() % n_data_points;
                if (original_bin_data->data[r] != original_bin_data->info.undef && !selected[r]) {
                    train_data_points[j++] = r;
                    selected[r] = true;
                }
            }

            info_ctl intermediary_info;
            cp_ctl(&intermediary_info, &original_info);
            binary_data *intermediary_bin_data = aloca_bin(original_bin_data->info.x.def, original_bin_data->info.y.def, original_bin_data->info.tdef);
            cp_ctl(&intermediary_bin_data->info, &original_bin_data->info);
            memcpy(intermediary_bin_data->data, original_bin_data->data, sizeof(datatype) * original_bin_data->info.x.def * original_bin_data->info.y.def * original_bin_data->info.tdef);
            strcpy(intermediary_info.bin_filename, "intermediary.bin");

            for (long int i = 0; i < n_train; i++) {
                intermediary_bin_data->data[train_data_points[i]] = original_bin_data->info.undef;
            }

            write_files(intermediary_bin_data, "intermediary", "intermediary");

            pid_t pid = fork();
            if (pid == 0) {
                char *argv[] = {"junta_dados/compose", "intermediary.ctl", args.interpolated_file, "final", method, NULL};
                execvp(argv[0], argv);
                perror("execvp failed");
                exit(127);
            } else {
                waitpid(pid, 0, 0);
            }

            info_ctl final_info;
            binary_data *final_bin_data;
            if (!open_files("final.ctl", &final_info, &final_bin_data)) {
                free(original_bin_data);
                free(interpolated_bin_data);
                free(intermediary_bin_data);
                free(train_data_points);
                free(selected);
                if (csv_file) fclose(csv_file);
                fclose(metrics_file);
                exit(1);
            }

            if (args.print_csv) {
                for (long int i = 0; i < n_train; i++) {
                    fprintf(csv_file, "%d, %ld, %f, %f, %f\n", run + 1, train_data_points[i], original_bin_data->data[train_data_points[i]], final_bin_data->data[train_data_points[i]], final_bin_data->data[train_data_points[i]] - original_bin_data->data[train_data_points[i]]);
                }
            }

            float rmsev = rmse(original_bin_data, final_bin_data, train_data_points, n_train);
            float maev = mae(original_bin_data, final_bin_data, train_data_points, n_train);
            float msev = mse(original_bin_data, final_bin_data, train_data_points, n_train);

            fprintf(metrics_file, "%d, %f, %f, %f\n", run + 1, rmsev, maev, msev);
            total_rmse += rmsev;
            total_mae += maev;
            total_mse += msev;

            free_bin(intermediary_bin_data);
            free_bin(final_bin_data);
        }

        fprintf(metrics_file, "Average, %f, %f, %f\n", total_rmse / args.runs, total_mae / args.runs, total_mse / args.runs);
        remove("intermediary.ctl");
        remove("intermediary.bin");
        remove("final.ctl");
        remove("final.bin");

        if (csv_file) fclose(csv_file);
        fclose(metrics_file);
    }

    free_bin(original_bin_data);
    free_bin(interpolated_bin_data);
    free(train_data_points);
    free(selected);

    return 0;
}