#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include <math.h>
#include "c_ctl.h"

/**
 * @brief Calculate the Root Mean Squared Error (RMSE) between two binary data structs.
 * 
 * @param original The original binary data struct.
 * @param predicted The predicted binary data struct.
 * @param n The number of valid data points.
 * @return The RMSE value.
 */
float rmse(binary_data *original, binary_data *predicted, long int n);

/**
 * @brief Calculate the Mean Absolute Error (MAE) between two binary data structs.
 * 
 * @param original The original binary data struct.
 * @param predicted The predicted binary data struct.
 * @param n The number of valid data points.
 * @return The MAE value.
 */
float mae(binary_data *original, binary_data *predicted, long int n);

/**
 * @brief Calculate the Mean Squared Error (MSE) between two binary data structs.
 * 
 * @param original The original binary data struct.
 * @param predicted The predicted binary data struct.
 * @param n The number of valid data points.
 * @return The MSE value.
 */
float mse(binary_data *original, binary_data *predicted, long int n);

#endif // ERROR_METRICS_H
