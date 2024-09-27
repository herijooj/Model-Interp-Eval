# Junta-Dados
Programa para juntar dados climáticos em pontos de grade (binário).
No laboratório, foi utilizado para juntar dados do GPCC com dados observados de estação.

---
> [!NOTE]
> Este é um arquivo com o máximo de informações sobre o programa reunidas, para uma referência rápida ver [Resumo.md](Resumo.md) e para uma explicação matemática dos métodos utilizados: [Interpolação.tex](Interpolação.tex) ou `Interpolação.pdf`.

# Funcionamento

O programa lê dois arquivos (.ctl) no formato [GrADS Data Descriptor File](http://cola.gmu.edu/grads/gadoc/descriptorfile.html),
e cria um novo arquivo binário com seu respectivo .ctl que corresponde à junção de ambos.

## Compilação
Para gerar o executável, basta utilizar o comando `make` no diretório `src/` que contém o arquivo `Makefile`.

    make all

De forma alternativa, para gerar um executável de depuração (veja seção [Modo Depuração](#depuração) ) utilizar:

    make debug

Caso os arquivos binários foram escritos com precisão dupla (`double`) ao invés de `float`, é preciso gerar o executável compatível com o comando:

    make double

Para excluir os arquivos objetos gerados durante a compilação:

    make clean

Para excluir tanto os arquivos objeto quanto os executáveis:

    make purge
    
## Executando
Para executar o programa com valores padrão:

    ./compose arquivo_princial.ctl arquivo_secundario.ctl prefixo_saída
    
É possível modificar os valores padrão utilizando opções na linha de comando. Para ver todas as opções disponíveis:

    ./compose -h
    ./compose --help

### Tipos de arquivos aceitos

São aceitos como entrada arquivos binários desde que sejam de um mesmo tipo, atualmente são aceitos:
 
 - Dados diários, tipo de data no arquivo `.ctl`: `dy`.
 - Dados mensais, tipo de data no arquivo `.ctl`: `mo`.
 - Dados anuais, tipo de data no arquivo `.ctl`: `yr`.

Além disso, os dois arquivos de entrada precisam ter o mesmo tamanho de quadrícula, bem como latitudes e longitudes inicias compatíveis.
Caso alguma das restrições não sejam atendidas, o programa acusará o erro encontrado.

### Formatação dos arquivos de Entrada

> [!IMPORTANT]
> Para executar a junção de dados é necessário formatar os arquivos `.ctl` de entrada.

Os arquivos precisam ser de apenas 1 variável e 1 nível, contendo os campos na seguinte ordem:

> dset `<file>`  
> title Valor Ignorado  
> undef `<value>`  
> xdef `<int_value>` linear `<value>` `<value>`  
> ydef `<int_value>` linear `<value>` `<value>`  
> zdef `<int_value>` linear 1 1  
> tdef `<int_value>` linear `<date>` `<step>`  
> vars 1  
> <name>  Restante é ignorado  
> endvars

Onde `<int_value>` são valores inteiros e `<value>` são valores em ponto flutuante. Veja [GrADS Data Descriptor File](http://cola.gmu.edu/grads/gadoc/descriptorfile.html) para mais informações.
E `<date>` precisa estar no formato `ddmmmaaaa`, ou seja:

 - `dd`: 2 dígitos para dia.
 - `mmm`: 3 letras em minúsculo para abreviação do mês.
 - `aaaa`: 4 dígitos para o ano.

Exemplos de arquivo de entrada podem ser encontrados no diretório [exemplos/](exemplos/).

Alternativamente o script [ajusta_ctl.sh](helper/ajusta_ctl.sh) pode ser utilizado para ajustar os arquivos `.ctl`.

### Depuração

Ao executar o script com a opção `-D` ou `--debug`, o arquivo de saída conterá apenas as quadrículas que tiveram seu valor modificado.
Tipicamente serão as quadrículas de transição entre os dois conjuntos de dados. Demais valores serão marcados como undef.

> [!IMPORTANT]
> O dado gerado com a opção de depuração serve apenas para verificar o funcionamento do programa. Não faz sentido utilizá-lo como dado climatológico.
