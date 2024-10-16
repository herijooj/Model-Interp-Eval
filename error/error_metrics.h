#ifndef ERROR_METRICS_H
#define ERROR_METRICS_H

#include "c_ctl.h"
#include <math.h>
#include <float.h> // Para FLT_EPSILON
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h> // Para paralelização
#include <immintrin.h> // Para SIMD

// Estrutura para armazenar as métricas de erro
typedef struct {
    float rmse;
    float mae;
    float mse;
    float percentage_error;
} ErrorMetrics;

// Função para calcular todas as métricas de erro entre dados originais e previstos
// original: ponteiro para os dados binários originais
// predicted: ponteiro para os dados binários previstos
// Retorna uma estrutura ErrorMetrics contendo todas as métricas calculadas
ErrorMetrics calculate_all_errors(binary_data *original, binary_data *predicted);

#endif // ERROR_METRICS_H