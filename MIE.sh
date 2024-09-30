#!/usr/bin/env bash

# Este script é usado para executar o MIE e depois plotar os resultados

# Definir a lista de iterações, porcentagens e métodos
iterations=(50)
percentages=(1 2 5 10 20)
methods=(--avg --idw --msh)

arq1=$1
arq2=$2

# Rodar o MIE
echo "Rodando o MIE"
for iteration in "${iterations[@]}"; do
    for percentage in "${percentages[@]}"; do
            echo "Running MIE with ${percentage}% of the data, ${iteration} iterations"
            ./bin/mie -f $arq1 $arq2 -p ${percentage} -r ${iteration}
    done
done

# Extrair os resultados
echo "Extraindo os resultados"

# Vamos plotar para cada porcentagem o RMSE, MAE e MSE para cada método
echo "iterations, percentage, method, RMSE, MAE, MSE, PERROR" > results.dat
for iteration in "${iterations[@]}"; do
    for percentage in "${percentages[@]}"; do
        for method in "${methods[@]}"; do
            # Pegar a última linha do arquivo
            avg_line=$(tail -n 1 metrics_${percentage}.00%_${iteration}_runs_${method}.csv)
            echo "${iteration}, ${percentage}, ${method}, ${avg_line#Average, }" >> results.dat
        done
    done
done

# Plotar os resultados
python3 plot.py results.dat
# gnuplot <<EOF
# set terminal pngcairo enhanced font 'Verdana,10' size 800,600
# set output 'results.png'
# set datafile separator ","
# set key outside
# set title "MIE Results by Method"
# set xlabel "Percentage of Data Used"
# set ylabel "Error Metrics"
# set grid

# # Subplot for RMSE
# set multiplot layout 3, 1 title "Error Metrics by Method"
# set title "RMSE"
# plot 'results.dat' using 2:4 with linespoints title '--avg' lt rgb "blue" lw 2 pt 7, \
#      'results.dat' using 2:4 with linespoints title '--idw' lt rgb "green" lw 2 pt 5, \
#      'results.dat' using 2:4 with linespoints title '--msh' lt rgb "red" lw 2 pt 9

# # Subplot for MAE
# set title "MAE"
# plot 'results.dat' using 2:5 with linespoints title '--avg' lt rgb "blue" lw 2 pt 7, \
#      'results.dat' using 2:5 with linespoints title '--idw' lt rgb "green" lw 2 pt 5, \
#      'results.dat' using 2:5 with linespoints title '--msh' lt rgb "red" lw 2 pt 9

# # Subplot for MSE
# set title "MSE"
# plot 'results.dat' using 2:6 with linespoints title '--avg' lt rgb "blue" lw 2 pt 7, \
#      'results.dat' using 2:6 with linespoints title '--idw' lt rgb "green" lw 2 pt 5, \
#      'results.dat' using 2:6 with linespoints title '--msh' lt rgb "red" lw 2 pt 9

# unset multiplot
# EOF
