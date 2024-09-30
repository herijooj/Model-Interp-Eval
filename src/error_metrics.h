#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include <math.h>
#include "c_ctl.h"

/**
 * @brief Calculate the Root Mean Squared Error (RMSE) between two binary data structs.
 * 
 * @param original The original binary data struct.
 * @param predicted The predicted binary data struct.
 * @return The RMSE value.
 */
float rmse(binary_data *original, binary_data *predicted, long int *validation_points, long int n_validation);
/**
 * @brief Calculate the Mean Absolute Error (MAE) between two binary data structs.
 * 
 * @param original The original binary data struct.
 * @param predicted The predicted binary data struct.
 * @return The MAE value.
 */
float mae(binary_data *original, binary_data *predicted, long int *validation_points, long int n_validation);
/**
 * @brief Calculate the Mean Squared Error (MSE) between two binary data structs.
 * 
 * @param original The original binary data struct.
 * @param predicted The predicted binary data struct.
 * @return The MSE value.
 */
float mse(binary_data *original, binary_data *predicted, long int *validation_points, long int n_validation);

/**
 * @brief Calculate the percentage error between two binary data structs.
 * 
 * @param original The original binary data struct.
 * @param predicted The predicted binary data struct.
 * @return The percentage error value.
 */
float percentage_error(binary_data *original, binary_data *predicted, long int *validation_points, long int n_validation);
#endif // ERROR_METRICS_H