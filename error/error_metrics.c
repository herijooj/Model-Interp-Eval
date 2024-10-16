#include "error_metrics.h"

ErrorMetrics calculate_all_errors(binary_data *original, binary_data *predicted) {
    float sum_squared_error = 0.0;
    float sum_absolute_error = 0.0;
    float sum_percentage_error = 0.0;
    size_t count = 0;
    size_t total_elements = original->info.x.def * original->info.y.def * original->info.tdef;
    float *original_data = original->data;
    float *predicted_data = predicted->data;
    float undef_original = original->info.undef;
    float undef_predicted = predicted->info.undef;

    #pragma omp parallel
    {
        float local_sum_squared_error = 0.0;
        float local_sum_absolute_error = 0.0;
        float local_sum_percentage_error = 0.0;
        size_t local_count = 0;

        #pragma omp for nowait
        for (size_t i = 0; i < total_elements; i++) {
            float orig_val = original_data[i];
            float pred_val = predicted_data[i];

            if (orig_val != undef_original && pred_val != undef_predicted) {
                float diff = orig_val - pred_val;

                local_sum_squared_error += diff * diff;
                local_sum_absolute_error += fabs(diff);
                if (fabs(orig_val) > FLT_EPSILON) { // Evita divisão por zero
                    local_sum_percentage_error += fabs(diff) / fabs(orig_val);
                }
                local_count++;
            }
        }

        #pragma omp critical
        {
            sum_squared_error += local_sum_squared_error;
            sum_absolute_error += local_sum_absolute_error;
            sum_percentage_error += local_sum_percentage_error;
            count += local_count;
        }
    }

    ErrorMetrics metrics = {0.0, 0.0, 0.0, 0.0};
    if (count > 0) {
        metrics.rmse = sqrt(sum_squared_error / count);
        metrics.mae = sum_absolute_error / count;
        metrics.mse = sum_squared_error / count;
        metrics.percentage_error = (sum_percentage_error / count) * 100;
    }

    return metrics;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <original_file> <interpolated_file>\n", argv[0]);
        return 1;
    }

    const char *original_file = argv[1];
    const char *interpolated_file = argv[2];

    info_ctl original_info, interpolated_info;
    binary_data *original_bin_data, *interpolated_bin_data;

    if (!open_files(original_file, &original_info, &original_bin_data) ||
        !open_files(interpolated_file, &interpolated_info, &interpolated_bin_data)) {
        free(original_bin_data);
        free(interpolated_bin_data);
        return 1;
    }

    ErrorMetrics metrics = calculate_all_errors(original_bin_data, interpolated_bin_data);

    printf("RMSE: %f\n", metrics.rmse);
    printf("MAE: %f\n", metrics.mae);
    printf("MSE: %f\n", metrics.mse);
    printf("Percentage Error: %f%%\n", metrics.percentage_error);

    free_bin(original_bin_data);
    free_bin(interpolated_bin_data);

    return 0;
}