#include "error_metrics.h"
#include <math.h>

// Calculates the Root Mean Square Error (RMSE) between two arrays of predictions and targets.
// RMSE = sqrt(sum((predictions[i] - targets[i])^2) / length)
double calculate_rmse(const double *predictions, const double *targets, size_t length) {
    double sum = 0.0;
    for (size_t i = 0; i < length; ++i) {
        double diff = predictions[i] - targets[i];
        sum += diff * diff;
    }
    return sqrt(sum / length);
}

// Calculates the Mean Absolute Error (MAE) between two arrays of predictions and targets.
// MAE = sum(|predictions[i] - targets[i]|) / length
double calculate_mae(const double *predictions, const double *targets, size_t length) {
    double sum = 0.0;
    for (size_t i = 0; i < length; ++i) {
        sum += fabs(predictions[i] - targets[i]);
    }
    return sum / length;
}

// Calculates the Mean Squared Error (MSE) between two arrays of predictions and targets.
// MSE = sum((predictions[i] - targets[i])^2) / length
double calculate_mse(const double *predictions, const double *targets, size_t length) {
    double sum = 0.0;
    for (size_t i = 0; i < length; ++i) {
        double diff = predictions[i] - targets[i];
        sum += diff * diff;
    }
    return sum / length;
}