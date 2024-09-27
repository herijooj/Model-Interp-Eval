# O que é?

O programa `compose` ou Junta Dados foi criado para juntar dados em ponto de grade, de duas fontes diferentes.
Fazendo uma transição entre a fonte da dado primária e a secundária.

A **fonte primária** terá os dados copiados para o arquivo de saída, ou seja, serão **inalterados**.
A **fonte secundária** será utilizada somente onde **não** houverem dados da fonte primária.
Nas quadrículas de transição entre os dois conjuntos de dados, os valores da **fonte secundária** sofrerão
alteração com o intuito de suavizar a transição entre os dados.

Como resultado será gerado um arquivo binário com seu respectivo .ctl, com as maiores dimensões possíveis, ou seja:
 - A data inicial do arquivo de saída será a menor data inicial entre os dois arquivos de entrada.
 - A data final do arquivo de saída será a maior data final entre os dois arquivos de entrada.
 - A dimensão espacial é a composta de:
   - longitude inicial do arquivo de saída é a longitude inicial mais a oeste entre as longitudes iniciais dos arquivos de entrada.
   - longitude final do arquivo de saída é a longitude final mais a leste entre as longitudes finais dos arquivos de entrada.
   - latitude inicial do arquivo de saída é a latitude inicial mais a norte entre as latitudes iniciais dos arquivos de entrada.
   - latitude final do arquivo de saída é a latitude final mais a sul entre as latitudes finais dos arquivos de entrada.

# Execução básica

Execute o arquivo para ver os parâmetros exigidos

    ./compose

São necessários pelo menos 3 parâmetros na seguinte ordem:

    ./compose entrada_principal.ctl entrada_secundária.ctl nome_prefixo_saída

 - Primeiro argumento: nome (caminho) do arquivo .ctl de dado principal.
 - Segundo argumento: nome (caminho) do arquivo .ctl de dado complementar.
 - Terceiro argumento: nome (caminho) do arquivo .ctl de saída.

## Opções Disponíveis
Para exibir um resumo das opções da linha de comando:

    ./compose -h

Opções curtas têm apenas um `-` como em `-h`. Opções longas são precedidas de dois traços `--` como em `--help`.
Opções longas podem ser encurtadas, desde que continuem únicas ex.: `--he` é equivalente a `--help`. Seguem todas as opções:
Opções com parâmetro:
 - `--lati` ou `--yi`: Latitudade inicial. Valor padrão = -56.5.
 - `--latf` ou `--yf`: Latitudade final. Valor padrão = 14.5.
 - `--loni` ou `--xi`: Longitude inicial. Valor padrão = -89.5.
 - `--lonf` ou `--xf`: Longitude final. Valor padrão = -31.5.


Opções sem parâmetro:
Apenas uma das seguintes terá efeito, mais informações sobre os métodos de interpolação ver `Interpolação.tex`:
 -  `-a` ou `--avg`: Usa médias simples entre quadrículas adjacentes para calcular a transição. Método antigo.
 -  `-i` ou `--idw`: [Inverse distance weighting](https://en.wikipedia.org/wiki/Inverse_distance_weighting). Usa média ponderada pelo inverso da distância entre quadrículas adjacentes para calcular a transição.
    Melhoria do método de Médias. Quadrículas na diagonal estão mais distantes, portanto recebem um peso menor. 
 -  `-m` ou `--msh`: [Modified Shepard](https://en.wikipedia.org/wiki/Inverse_distance_weighting#Modified_Shepard's_method). Usa um raio de busca para decidir quais quadrículas serão utilizadas no cálculo. Método **padrão**.
 -  `-n` ou `--none`: Nenhum. Copia quadrículas da fonte primária, se não houver, copia do dado secundário.

Outras Opções:

 - `-h` ou `--help`: mostra opções disponíveis.
 - `-D` ou `--debug`: O arquivo de saída gerado contém apenas quadrículas que sofreram alteração. Utilizado para testar a interpolação.


## Compilando

Caso o arquivo executável  `compose` ainda não tenha sido criado, vá até o diretório dos arquivos fonte,
isto é, o diretório que contém o arquivo `Makefile`, e execute:

    make

Caso os arquivos de entrada que serão utilizados foram escritos com precisão dupla, o arquivo executável
apropriado é o `compose_64`, gerado com o comando:

    make double
