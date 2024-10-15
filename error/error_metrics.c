#include "error_metrics.h"

inline float calculate_squared_error(float diff) {
    return diff * diff;
}

inline float calculate_absolute_error(float diff) {
    return fabs(diff);
}

inline float calculate_percentage_error(float diff, float original_value) {
    return fabs(diff) / fabs(original_value);
}

float calculate_error(binary_data *original, binary_data *predicted, ErrorMetricType type) {
    float sum = 0.0;
    size_t count = 0;
    size_t total_elements = original->info.x.def * original->info.y.def * original->info.tdef;
    float *original_data = original->data;
    float *predicted_data = predicted->data;
    float undef_original = original->info.undef;
    float undef_predicted = predicted->info.undef;

    #pragma omp parallel for reduction(+:sum, count)
    for (size_t i = 0; i < total_elements; i++) {
        float orig_val = original_data[i];
        float pred_val = predicted_data[i];

        if (orig_val != undef_original && pred_val != undef_predicted) {
            float diff = orig_val - pred_val;
            float error = 0.0;

            switch (type) {
                case RMSE:
                case MSE:
                    error = diff * diff;
                    break;
                case MAE:
                    error = fabs(diff);
                    break;
                case PERCENTAGE_ERROR:
                    if (fabs(orig_val) > FLT_EPSILON) { // Evita divisão por zero
                        error = fabs(diff) / fabs(orig_val);
                    } else {
                        continue; // Pula a iteração se a condição não for satisfeita
                    }
                    break;
            }

            sum += error;
            count++;
        }
    }

    if (count == 0) {
        return 0.0;
    }

    switch (type) {
        case RMSE:
            return sqrt(sum / count);
        case MAE:
        case MSE:
            return sum / count;
        case PERCENTAGE_ERROR:
            return (sum / count) * 100;
    }

    return 0.0; // Para evitar warnings de compilação
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

    float rmse_value = calculate_error(original_bin_data, interpolated_bin_data, RMSE);
    float mae_value = calculate_error(original_bin_data, interpolated_bin_data, MAE);
    float mse_value = calculate_error(original_bin_data, interpolated_bin_data, MSE);
    float percentage_error_value = calculate_error(original_bin_data, interpolated_bin_data, PERCENTAGE_ERROR);

    printf("RMSE: %f\n", rmse_value);
    printf("MAE: %f\n", mae_value);
    printf("MSE: %f\n", mse_value);
    printf("Percentage Error: %f%%\n", percentage_error_value);

    free_bin(original_bin_data);
    free_bin(interpolated_bin_data);

    return 0;
}