#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include <stddef.h>

double calculate_rmse(const double *predictions, const double *targets, size_t length);
double calculate_mae(const double *predictions, const double *targets, size_t length);
double calculate_mse(const double *predictions, const double *targets, size_t length);

#endif // ERROR_METRICS_H