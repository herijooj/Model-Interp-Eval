import matplotlib.pyplot as plt
import csv
import sys

# Read data from command line argument file
file_path = sys.argv[1]

percentages = []
rmse_avg = []
rmse_idw = []
rmse_msh = []
mae_avg = []
mae_idw = []
mae_msh = []
perror_avg = []
perror_idw = []
perror_msh = []

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
        perror = float(row['PERROR'].strip())
        
        if method == '--avg':
            percentages.append(percentage)
            rmse_avg.append(rmse)
            mae_avg.append(mae)
            perror_avg.append(perror)
        elif method == '--idw':
            rmse_idw.append(rmse)
            mae_idw.append(mae)
            perror_idw.append(perror)
        elif method == '--msh':
            rmse_msh.append(rmse)
            mae_msh.append(mae)
            perror_msh.append(perror)

# Debugging: Print the collected data
print("Percentages:", percentages)
print("RMSE Avg:", rmse_avg)
print("RMSE IDW:", rmse_idw)
print("RMSE MSH:", rmse_msh)
print("MAE Avg:", mae_avg)
print("MAE IDW:", mae_idw)
print("MAE MSH:", mae_msh)
print("PERROR Avg:", perror_avg)
print("PERROR IDW:", perror_idw)
print("PERROR MSH:", perror_msh)

# Plotting
plt.figure(figsize=(10, 9))

# RMSE Comparison
plt.subplot(3, 1, 1)
plt.plot(percentages, rmse_avg, label='--avg (RMSE)', marker='o')
plt.plot(percentages, rmse_idw, label='--idw (RMSE)', marker='o')
plt.plot(percentages, rmse_msh, label='--msh (RMSE)', marker='o')
plt.ylabel('RMSE')
plt.title('RMSE, MAE, and PERROR Comparison between Methods')
plt.legend()

# MAE Comparison
plt.subplot(3, 1, 2)
plt.plot(percentages, mae_avg, label='--avg (MAE)', marker='o')
plt.plot(percentages, mae_idw, label='--idw (MAE)', marker='o')
plt.plot(percentages, mae_msh, label='--msh (MAE)', marker='o')
plt.ylabel('MAE')
plt.legend()

# PERROR Comparison
plt.subplot(3, 1, 3)
plt.plot(percentages, perror_avg, label='--avg (PERROR)', marker='o')
plt.plot(percentages, perror_idw, label='--idw (PERROR)', marker='o')
plt.plot(percentages, perror_msh, label='--msh (PERROR)', marker='o')
plt.xlabel('Percentage')
plt.ylabel('PERROR')
plt.legend()

plt.tight_layout()
plt.show()