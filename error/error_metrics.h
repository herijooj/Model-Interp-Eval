#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include "c_ctl.h"
#include <math.h>
#include <float.h> // Para FLT_EPSILON
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h> // Para paralelização

// Enumeração para os diferentes tipos de métricas de erro
typedef enum {
    RMSE,             // Root Mean Square Error
    MAE,              // Mean Absolute Error
    MSE,              // Mean Square Error
    PERCENTAGE_ERROR  // Percentage Error
} ErrorMetricType;

float calculate_squared_error(float diff);
float calculate_absolute_error(float diff);
float calculate_percentage_error(float diff, float original_value);
float calculate_error(binary_data *original, binary_data *predicted, ErrorMetricType type);

// Função para calcular a métrica de erro entre dados originais e previstos
// original: ponteiro para os dados binários originais
// predicted: ponteiro para os dados binários previstos
// type: tipo da métrica de erro a ser calculada
// Retorna o valor da métrica de erro calculada
float calculate_error(binary_data *original, binary_data *predicted, ErrorMetricType type);

#endif // ERROR_METRICS_H