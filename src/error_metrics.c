#include "error_metrics.h"

float rmse(binary_data *original, binary_data *predicted, long int *validation_points, long int n_validation) {
    float sum = 0.0;
    long int count = 0;
    for (long int i = 0; i < original->info.x.def * original->info.y.def * original->info.tdef; i++) {
        if (original->data[i] != original->info.undef && predicted->data[i] != predicted->info.undef) {
            sum += (original->data[i] - predicted->data[i]) * (original->data[i] - predicted->data[i]);
            count++;
        }
    }
    if (count == 0) {
        return 0.0; // Return 0 if there are no valid data points
    }
    return sqrt(sum / count);
}

float mae(binary_data *original, binary_data *predicted, long int *validation_points, long int n_validation) {
    float sum = 0.0;
    long int count = 0;
    for (long int i = 0; i < original->info.x.def * original->info.y.def * original->info.tdef; i++) {
        if (original->data[i] != original->info.undef && predicted->data[i] != predicted->info.undef) {
            sum += fabs(original->data[i] - predicted->data[i]);
            count++;
        }
    }
    if (count == 0) {
        return 0.0; // Return 0 if there are no valid data points
    }
    return sum / count;
}

float mse(binary_data *original, binary_data *predicted, long int *validation_points, long int n_validation) {
    float sum = 0.0;
    long int count = 0;
    for (long int i = 0; i < original->info.x.def * original->info.y.def * original->info.tdef; i++) {
        if (original->data[i] != original->info.undef && predicted->data[i] != predicted->info.undef) {
            sum += (original->data[i] - predicted->data[i]) * (original->data[i] - predicted->data[i]);
            count++;
        }
    }
    if (count == 0) {
        return 0.0; // Return 0 if there are no valid data points
    }
    return sum / count;
}