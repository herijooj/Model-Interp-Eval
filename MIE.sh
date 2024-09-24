#!/usr/bin/env bash

# Este script é usado para executar o MIE e depois plotar os resultados

# Definir a lista de iterações e porcentagens
iterations=(50)
percentages=(2 5 10 20)

# arq1=$1
# arq2=$2

# # Rodar o MIE
# echo "Rodando o MIE"
# for iteration in "${iterations[@]}"; do
#     for percentage in "${percentages[@]}"; do
#         echo "Running MIE with ${percentage}% of the data and ${iteration} iterations"
#         ./bin/mie -f $arq1 $arq2 -p ${percentage} -r ${iteration}
#     done
# done

# Extrair os resultados
echo "Extraindo os resultados"

# Os resultados são armazenados no arquivo metrics_[percentage]%_[num of runs]_runs.csv
# A última linha do arquivo contém a média das métricas
# run, RMSE, MAE, MSE
# Average, 4.518883, 0.304393, 20.457901
# Vamos plotar para cada porcentagem o RMSE, MAE e MSE
echo "iterations, percentage, RMSE, MAE, MSE" > results.dat
for iteration in "${iterations[@]}"; do
    for percentage in "${percentages[@]}"; do
        # Pegar a última linha do arquivo
        avg_line=$(tail -n 1 metrics_${percentage}.00%_${iteration}_runs.csv)
        echo "${iteration}, ${percentage}, ${avg_line#Average, }" >> results.dat
    done
done

# Plotar os resultados
gnuplot <<EOF
set terminal pngcairo enhanced font 'Verdana,10' size 800,600
set output 'results.png'
set datafile separator ","
set key outside
set title "MIE Results"
set xlabel "Percentage of Data Used"
set ylabel "Error Metrics"
set grid
plot 'results.dat' using 2:3 with linespoints title 'RMSE' lt rgb "blue" lw 2 pt 7, \
     'results.dat' using 2:4 with linespoints title 'MAE' lt rgb "green" lw 2 pt 5, \
     'results.dat' using 2:5 with linespoints title 'MSE' lt rgb "red" lw 2 pt 9
EOF
