#!/usr/bin/env bash

# Este script é usado para executar o MIE e depois plotar os resultados

# Definir a lista de iterações, porcentagens e métodos
iterations=(50)
percentages=(1 2 5 10 20 30 50)
methods=(--avg --idw --msh)

arq1=$1
arq2=$2
seed=$3

# Verificar se os arquivos foram fornecidos
if [ -z "$arq1" ] || [ -z "$arq2" ]; then
    echo "Usage: $0 <input_file1> <input_file2>"
    exit 1
fi

# Criar arquivo de detalhes da execução
detail_file="details.dat"
echo "Gerando $detail_file com os detalhes da execução"
echo "Run details - $(date)" > $detail_file
echo "Input file 1: $arq1" >> $detail_file
echo "Input file 2: $arq2" >> $detail_file
echo "Iterations: ${iterations[@]}" >> $detail_file
echo "Percentages: ${percentages[@]}" >> $detail_file
echo "Methods: ${methods[@]}" >> $detail_file
echo "" >> $detail_file

# Rodar o MIE
echo "Rodando o MIE"
for iteration in "${iterations[@]}"; do
    echo "Running MIE with ${iteration} iterations"
    for percentage in "${percentages[@]}"; do
        echo "Running MIE with ${percentage}% of the data, ${iteration} iterations"
        ./bin/mie -f $arq1 $arq2 -p ${percentage} -r ${iteration} 
    done
done

# Extrair os resultados
echo "Extraindo os resultados"
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
