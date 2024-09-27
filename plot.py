import matplotlib.pyplot as plt
import csv
import sys
import csv

# Read data from command line argument file
file_path = sys.argv[1]

percentages = []
rmse_avg = []
rmse_idw = []
rmse_msh = []
mae_avg = []
mae_idw = []
mae_msh = []

with open(file_path, 'r') as file:
    reader = csv.DictReader(file)
    # Remove leading/trailing spaces from fieldnames
    reader.fieldnames = [field.strip() for field in reader.fieldnames]
    print("Available columns:", reader.fieldnames)
    for row in reader:
        percentage = int(row['percentage'].strip())
        method = row['method'].strip()
        rmse = float(row['RMSE'].strip())
        mae = float(row['MAE'].strip())
        
        if method == '--avg':
            percentages.append(percentage)
            rmse_avg.append(rmse)
            mae_avg.append(mae)
        elif method == '--idw':
            rmse_idw.append(rmse)
            mae_idw.append(mae)
        elif method == '--msh':
            rmse_msh.append(rmse)
            mae_msh.append(mae)

# Debugging: Print the collected data
print("Percentages:", percentages)
print("RMSE Avg:", rmse_avg)
print("RMSE IDW:", rmse_idw)
print("RMSE MSH:", rmse_msh)
print("MAE Avg:", mae_avg)
print("MAE IDW:", mae_idw)
print("MAE MSH:", mae_msh)

# Plotting
plt.figure(figsize=(10, 6))

# RMSE Comparison
plt.subplot(2, 1, 1)
plt.plot(percentages, rmse_avg, label='--avg (RMSE)', marker='o')
plt.plot(percentages, rmse_idw, label='--idw (RMSE)', marker='o')
plt.plot(percentages, rmse_msh, label='--msh (RMSE)', marker='o')
plt.ylabel('RMSE')
plt.title('RMSE and MAE Comparison between Methods')
plt.legend()

# MAE Comparison
plt.subplot(2, 1, 2)
plt.plot(percentages, mae_avg, label='--avg (MAE)', marker='o')
plt.plot(percentages, mae_idw, label='--idw (MAE)', marker='o')
plt.plot(percentages, mae_msh, label='--msh (MAE)', marker='o')
plt.xlabel('Percentage')
plt.ylabel('MAE')
plt.legend()

plt.tight_layout()
plt.show()
