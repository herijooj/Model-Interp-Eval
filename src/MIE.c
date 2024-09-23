#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "func.h"
#include "c_ctl.h"
#include "error_metrics.h"
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */

// Struct to hold command-line arguments
typedef struct {
    char *original_file;    // original file
    char *interpolated_file; // interpolated file
    int seed;               // seed
    char *output;           // output file
    float percentage;       // percentage
    int help;               // help flag
    int runs;               // number of runs
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
}

Arguments parse_arguments(int argc, char *argv[]) {
    srand(time(NULL)); // Set the seed for the random number generator
    int seed = rand();
    Arguments args = {NULL, NULL, seed, NULL, 2.0, 0, 1}; // Default runs to 1

    int opt;
    while ((opt = getopt(argc, argv, "hf:s:o:p:r:")) != -1) {
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

    // create a file to write the data
    FILE* csv_file = fopen("data.csv", "w");
    if (csv_file == NULL) {
        fprintf(stderr, "Error: unable to create the CSV file\n");
        free(original_bin_data);
        free(interpolated_bin_data);
        exit(1);
    }

    // create a file to write the metrics of each run
    FILE* metrics_file = fopen("metrics.csv", "w");
    if (metrics_file == NULL) {
        fprintf(stderr, "Error: unable to create the metrics CSV file\n");
        free(original_bin_data);
        free(interpolated_bin_data);
        fclose(csv_file);
        exit(1);
    }

    // write the header to the CSV files
    fprintf(csv_file, "run, i, real, predicted, error\n");
    fprintf(metrics_file, "run, RMSE, MAE, MSE\n");

    float total_rmse = 0.0, total_mae = 0.0, total_mse = 0.0;

    for (int run = 0; run < args.runs; run++) {
        printf("Run %d/%d\n", run + 1, args.runs);

        // New file ===================================================================
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

        long int *train_data_points = (long int *) malloc(n_train * sizeof(long int));
        if (train_data_points == NULL) {
            fprintf(stderr, "Error: could not allocate memory\n");
            free(original_bin_data);
            free(interpolated_bin_data);
            fclose(csv_file);
            fclose(metrics_file);
            exit(1);
        }

        // Initialize the vector with -1
        for (long int i = 0; i < n_train; i++) {
            train_data_points[i] = -1;
        }

        // Choose the training data points
        long int j = 0;
        while (j < n_train) {
            long int r = rand() % n_data_points;
            if (original_bin_data->data[r] != original_bin_data->info.undef) {
                int found = 0;
                for (long int i = 0; i < n_train; i++) {
                    if (train_data_points[i] == r) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    train_data_points[j] = r;
                    j++;
                }
            }
        }

        // Copy of the original data
        info_ctl intermediary_info;
        cp_ctl(&intermediary_info, &original_info);

        // Copy of the original binary data
        binary_data* intermediary_bin_data = aloca_bin(original_bin_data->info.x.def, original_bin_data->info.y.def, original_bin_data->info.tdef);
        cp_ctl(&intermediary_bin_data->info, &original_bin_data->info);
        memcpy(intermediary_bin_data->data, original_bin_data->data, sizeof(datatype) * original_bin_data->info.x.def * original_bin_data->info.y.def * original_bin_data->info.tdef);

        // The name of this should be intermediary
        strcpy(intermediary_info.bin_filename, "intermediary.bin");

        // Set the values of the training data points in the intermediary binary data to undefined
        for (long int i = 0; i < n_train; i++) {
            intermediary_bin_data->data[train_data_points[i]] = original_bin_data->info.undef;
        }

        // Write the .bin and .ctl files
        write_files(intermediary_bin_data, "intermediary", "intermediary");

        // INTERPOLATION ==============================================================
        pid_t pid = fork();
        if (pid == 0) { /* child process */
            char *argv[] = {"junta_dados/compose", "intermediary.ctl", args.interpolated_file, "final", NULL};
            execvp(argv[0], argv);
            
            perror("execvp failed");
            exit(127);
        } else { /* pid != 0; parent process */
            waitpid(pid, 0, 0); /* wait for child to exit */
        }

        // Read the final binary data
        info_ctl final_info;
        binary_data* final_bin_data;
        if (!open_files("final.ctl", &final_info, &final_bin_data)) {
            free(original_bin_data);
            free(interpolated_bin_data);
            free(intermediary_bin_data);
            free(train_data_points);
            fclose(csv_file);
            fclose(metrics_file);
            exit(1);
        }

        // write only the training data points
        for (long int i = 0; i < n_train; i++) {
            fprintf(csv_file, "%d, %ld, %f, %f, %f\n", run + 1, train_data_points[i], original_bin_data->data[train_data_points[i]], final_bin_data->data[train_data_points[i]], final_bin_data->data[train_data_points[i]] - original_bin_data->data[train_data_points[i]]);
        }

        // Calculate the error metrics
        float rmsev = rmse(original_bin_data, final_bin_data, n_train);
        float maev = mae(original_bin_data, final_bin_data, n_train);
        float msev = mse(original_bin_data, final_bin_data, n_train);

        // Print the error metrics
        // printf("Run %d/%d\n", run + 1, args.runs);
        // printf("Root Mean Square Error (RMSE): %f\n", rmsev);
        // printf("Mean Absolute Error (MAE): %f\n", maev);
        // printf("Mean Squared Error (MSE): %f\n", msev);

        // Write the error metrics to the metrics file
        fprintf(metrics_file, "%d, %f, %f, %f\n", run + 1, rmsev, maev, msev);

        // Accumulate the metrics for average calculation
        total_rmse += rmsev;
        total_mae += maev;
        total_mse += msev;

        // Free allocated memory for this run
        free(train_data_points);
        free_bin(intermediary_bin_data);
        free_bin(final_bin_data);
    }

    // Calculate the average metrics
    float avg_rmse = total_rmse / args.runs;
    float avg_mae = total_mae / args.runs;
    float avg_mse = total_mse / args.runs;

    // Print the average metrics
    // printf("Average Root Mean Square Error (RMSE): %f\n", avg_rmse);
    // printf("Average Mean Absolute Error (MAE): %f\n", avg_mae);
    // printf("Average Mean Squared Error (MSE): %f\n", avg_mse);

    // Write the average metrics to the metrics file
    fprintf(metrics_file, "Average, %f, %f, %f\n", avg_rmse, avg_mae, avg_mse);

    // Delete the intermediary files
    remove("intermediary.ctl");
    remove("intermediary.bin");
    remove("final.ctl");
    remove("final.bin");

    // Close the CSV files
    fclose(csv_file);
    fclose(metrics_file);

    // Free allocated memory
    free_bin(original_bin_data);
    free_bin(interpolated_bin_data);

    return 0;
}