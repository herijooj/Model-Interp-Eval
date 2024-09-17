#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "func.h"
#include "c_ctl.h"
#include "error_metrics.h"

// Struct to hold command-line arguments
// [train_file] | this is the file that we want to compare
// [test_file]  | this is the file that we consider as the ground truth
// [seed]       | this is the seed for the random number generator (if not provided, the seed is set to a random value)
// [output]     | this is the output file where the results will be saved (if not provided, the results are printed to the standard output)
// [%]          | this is the percentage of the data that will be used for training (default: 80%)
// [help]       | this is the flag to show the help message
typedef struct {
    char *train_file; // train file
    char *test_file;  // test file
    int seed;         // seed
    char *output;     // output file
    float percentage;   // percentage
    int help;         // help flag

} Arguments;

// Function to show the help message
// Usage: MIE [OPTIONS]
// Options:
//   -h, --help               | this is the flag to show the help message
//   [Train File] [Test File] [Seed] [Output File] [%]
void show_help() {
    printf("Usage: MIE [Train File] [Test File] [Seed] [Output File] [%%]\n");
    printf("Options:\n");
    printf("  -h, --help               | This is the flag to show the help message\n");
    printf("  [Train File]             | The file that we want to compare\n");
    printf("  [Test File]              | The file that we consider as the ground truth\n");
    printf("  [Seed]                   | The seed for the random number generator (default: random value)\n");
    printf("  [Output File]            | The output file where the results will be saved (default: standard output)\n");
    printf("  [%%]                      | The percentage of the data that will be used for training (default: 80%%)\n");
}

// Function to parse the command-line arguments
Arguments parse_arguments(int argc, char *argv[]) {
    Arguments args = {NULL, NULL, -1, NULL, 0.1, 0};

    // parse the command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
            case 'h':
                args.help = 1;
                break;
            default:
                break;
        }
    }

    // check if the help flag is set
    if (args.help) {
        show_help();
        exit(0);
    }

    // check if the number of arguments is correct
    if (argc < 3 || argc > 6) {
        fprintf(stderr, "Error: invalid number of arguments\n");
        show_help();
        exit(1);
    }

    // set the command-line arguments
    args.train_file = argv[1];
    args.test_file = argv[2];

    if (argc >= 4) {
        args.seed = atoi(argv[3]);
    }

    if (argc >= 5) {
        args.output = argv[4];
    }

    if (argc == 6) {
        args.percentage = atoi(argv[5]);
    }

    return args;
}

// open_ctl and bin
void read_data(info_ctl* info, binary_data* bin_data, char* name){
    if (!open_ctl(info, name)) {
        fprintf(stderr, "Error reading .ctl file (%s:%d).\n", __FILE__, __LINE__);
        exit(1);
    }
    if ((bin_data = open_bin_info(info)) == NULL) {
        fprintf(stderr, "Error reading .bin file (%s:%d).\n", __FILE__, __LINE__);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    // parse the command-line arguments
    Arguments args = parse_arguments(argc, argv);

    // open the files
    info_ctl train_info;
    if (!open_ctl(&train_info, args.train_file)) {
        fprintf(stderr, "Error reading .ctl file (%s:%d).\n", __FILE__, __LINE__);
        exit(1);
    }
    binary_data* train_bin_data = open_bin_info(&train_info);
    if (train_bin_data == NULL) {
        fprintf(stderr, "Error reading .bin file (%s:%d).\n", __FILE__, __LINE__);
        exit(1);
    }
    info_ctl test_info;
    if (!open_ctl(&test_info, args.test_file)) {
        fprintf(stderr, "Error reading .ctl file (%s:%d).\n", __FILE__, __LINE__);
        exit(1);
    }
    binary_data* test_bin_data = open_bin_info(&test_info);
    if (test_bin_data == NULL) {
        fprintf(stderr, "Error reading .bin file (%s:%d).\n", __FILE__, __LINE__);
        exit(1);
    }

    // check if the files have the same dimensions
    if (!check_dim(train_bin_data, test_bin_data)) {
        fprintf(stderr, "Error: the files have different dimensions\n");
        exit(1);
    }

    // set the seed for the random number generator
    if (args.seed == -1) {
        srand(time(NULL));
    } else {
        srand(args.seed);
    }

    // calculate the number of undefined quadriculas
    size_t n_undef = 0;
    for (size_t i = 0; i < train_bin_data->info.tdef * train_bin_data->info.x.def * train_bin_data->info.y.def; i++) {
        if (train_bin_data->data[i] == train_info.undef) {
            n_undef++;
        }
    }

    // calculate the number of quadriculas to be used for training
    size_t data_percentage = (size_t)(n_undef * args.percentage / 100);

    // allocate memory for the array to store the undefined quadriculas
    size_t* undef = (size_t*)malloc(data_percentage * sizeof(size_t));
    if (undef == NULL) {
        fprintf(stderr, "Error: unable to allocate memory\n");
        exit(1);
    }

    // store the indices of the undefined quadriculas
    size_t count = 0;
    for (size_t i = 0; i < train_bin_data->info.tdef * train_bin_data->info.x.def * train_bin_data->info.y.def; i++) {
        if (train_bin_data->data[i] == train_info.undef) {
            undef[count] = i;
            count++;
            if (count == data_percentage) {
                break;
            }
        }
    }

    // shuffle the undefined quadriculas
    for (size_t i = 0; i < data_percentage; i++) {
        size_t j = rand() % data_percentage;
        size_t temp = undef[i];
        undef[i] = undef[j];
        undef[j] = temp;
    }

    // ERROR PRINTING
    printf("SEED: %d\n", args.seed);
    printf("PERCENTAGE: %f\n", args.percentage);
    printf("TRAIN FILE: %s\n", args.train_file);
    printf("TEST FILE: %s\n", args.test_file);
    printf("undef: %zu\n", n_undef);
    printf("data_percentage: %zu\n", data_percentage);

    // create a file to write the data
    FILE* csv_file = fopen("data.csv", "w");
    if (csv_file == NULL) {
        fprintf(stderr, "Error: unable to create the CSV file\n");
        exit(1);
    }

    // write the header to the CSV file
    fprintf(csv_file, "i, real, predicted, error, position\n");

    // write the data to the CSV file
    for (size_t i = 0; i < data_percentage; i++) {
        fprintf(csv_file, "%zu, %f, %f, %f, %zu\n", i, test_bin_data->data[undef[i]], train_bin_data->data[undef[i]], test_bin_data->data[undef[i]] - train_bin_data->data[undef[i]], undef[i]);
    }

    // close the CSV file
    fclose(csv_file);

    // FREE MEMORY =====================================================================================================
    free(undef);

    // FREE MEMORY =====================================================================================================
    free_bin(train_bin_data);
    free_bin(test_bin_data);

    return 0;
}