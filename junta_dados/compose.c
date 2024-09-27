/*
Lucas Nogueira e Wellington Almeida 2023
Programa para juntar dois arquivos binários
Missão:
    Completar quadrículas faltantes do laboratório
    com dados do GPCC, fazendo uma suavização
    na transição de um conjunto de dados para outro
**/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>        //mult thread
#include <getopt.h>     //getopt
#include <string.h>     //strncpy
#include "c_ctl.h"
#include "geodist.h"


//Códigos de erro
#define ARG_ERR 2   //Erro com parâmetros
#define ARQ_ERR 3   //Erro com arquivos
#define MEM_ERR 4   //Erro com memória
#define FUN_ERR 5   //Erro na função

//Funções matemáticas
#define MIN(a,b)        (((a)<(b))?(a):(b))         //menor valor entre a e b
#define MAX(a,b)        (((a)>(b))?(a):(b))         //maior valor entre a e b
#ifndef BETA
#define BETA            2                           //Potência utilizada na interpolação (BETA maior deixa mais suave)
#endif
#ifndef MAJOR_RADIUS
#define MAJOR_RADIUS    300                         //Raio maior de busca por quadrículas
#endif
#ifndef MINOR_RADIUS
#define MINOR_RADIUS    200                         //Distância minima para realizar interpolação
#endif
#ifndef MAX_SIZE
#define MAX_SIZE        1.0                         //Tamanho de quadrícula máximo recomendado
#endif
#ifndef MIN_NGAUGE
#define MIN_NGAUGE      1                           //Quantidade minima de estações (gauges) permitidas ao usar arquivo 'ngauge'
#endif
#ifndef MIN_GRIDPOINTS
#define MIN_GRIDPOINTS  2                           //Quantidade minima de quadrículas ao ignorar valor secundário por coisa de 'MIN_NGAUGE'
#endif

// flags usadas na escolha da função de interpolação
#define NON_FLAG 0
#define AVG_FLAG 1
#define IDW_FLAG 2
#define MSH_FLAG 3


#define EXEC_MSG "primario.ctl secundario.ctl prefixo_saida"
#define HELP_MSG "-h ou --help\tpara mostrar as opções. Mais informações no arquivo LEIAME."
#define OPTS_MSG "OPTIONS"\
    "\n\t--lati LAT_INI, --yi LAT_INI"\
    "\n\t--latf LAT_FIN, --yf LAT_FIN"\
    "\n\t--loni LON_INI, --xi LON_INI"\
    "\n\t--lonf LON_FIN, --xf LON_FIN\n"\
    "\n\t--s-ngauge secundario_numgauge.ctl\tarquivo com número de estações de chuva por quadrícula do dado secundário.\n"\
    "\n\t-a, --avg\t\tUsa método de médias entre as quadriculas para interpolação."\
    "\n\t-i, --idw\t\tUsa método de peso inverso à distância (IDW) para interpolação."\
    "\n\t-m, --msh\t\tUsa método de Shepard Modificado para interpolação."\
    "\n\t-n, --none\t\tApenas junta as quadrículas, sem interpolação, preferência para os dados primários."\
    "\n\t-d, --debug\t\tSaída gerada contém apenas quadrículas que sofreram alteração, demais valores serão undef."
#define EXEM_MSG "--xi -89.5 --xf -31.5 --yi -56.5f --yf 14.5f --msh"


/* Função principal do programa, junta dados de 'p' (primario) e 's' (secundario)
 * preferencia para dados de 'p' delimitados pelo quadrado xi a xf e yi a yf
 * dados faltantes de 'p' são completados com valores de 's'
 * para cada valor 's' é feita cum calculo dependendo da função de interpolação escolhida
**/
binary_data* compose_data (binary_data* p, binary_data* s, coordtype xi, coordtype xf, coordtype yi, coordtype yf, binary_data* ngauge);


/* Calcula a distancia entre quadriculas para latitudes diferentes.
 * Recebe como entrada um ctl e a função de distancia.
 * Retorna o ponteiro para a matriz de distância ou NULL em erro.
 * */
coordtype** calc_dist(info_ctl* info,double (*dist)(double,double,double,double));

/* Alocação dinamica de matriz
 * Retorna o ponteiro para matriz ou NULL em erro.
 * */
coordtype** alloc_dist_matrix(int lin, int col);

/* Desaloca a matriz de distância.
 * */
void free_dist_matrix(coordtype** mat);

/* Retorna o peso baseado na distância entre a quadricula (x,y) e (x+dx,y+dy)
 * */
coordtype get_weight(size_t x, size_t y, int dx, int dy);



// Print Error: imprime uma mensagem de erro na saída padrão de erros
int perro (int err_cod);

// Print Error: imprime uma mensagem de erro na saída padrão de erros com um comentário adicional
int perro_com (int err_cod, const char* comment);




/*= FUNÇÕES DE INTERPOLAÇÃO =*/

/* Método mais simples, utilizado nas primeiras versões do programa.
 * Quadrícula resultante é a média simples entre valor da quadrícula do dado secundário
 * com as quadrículas adjacentes do dado primário
**/
int average_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge);

/* Inverse Distance Weighting (IDW)
**/
int idweight_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge);

/* Modified Shepard
**/
int mshepard_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge);

/* Nenhuma
 * */
int none_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge);

/* Mais informações:
 * https://en.wikipedia.org/wiki/Inverse_distance_weighting
 * https://iri.columbia.edu/~rijaf/CDTUserGuide/html/interpolation_methods.html
 *===========================*/

/*= FUNÇÕES DE PESO =*/
coordtype inverse_power(double value)   {return 1/(coordtype)pow(value,BETA);}

coordtype inverse_power_2(double value) {return 1/(coordtype)(value*value);}

coordtype inverse_value(double value)   {return 1/(coordtype)value;}
/*===================*/



/* == VARIAVEIS GLOBAIS == */
// I will not say: do not use global variables;
// for not all global variables are an evil.

// armazena a função que será usada na interpolação
int (*interpolation) (binary_data*,binary_data*,binary_data*,size_t,size_t,size_t,binary_data*);

// armazena a função de distancia
double (*dist) (double, double, double, double);

// armazena a função de peso
coordtype (*weight) (double);

// armazena as distancias já calculadas
coordtype** g_dist_matrix;

// altura da quadrícula
coordtype g_height;

int g_debug = 0;

/* ======================= */



int main(int argc, char *argv[]) {

    binary_data* lab = NULL;        //dados do laboratório

    binary_data* extra = NULL;      //dados complementares que serão adicionados

    binary_data* out_data = NULL;   //arquivo de saída

    binary_data* sngauge = NULL;    //dados complementares que serão adicionados

    //coordenadas da america do sul
    coordtype xi = -89.5f,
        xf = -31.5f,
        yi = -56.5f,
        yf = 14.5f;

    char pri_name[STR_SIZE] = {'\0'};
    char sec_name[STR_SIZE] = {'\0'};
    char out_name[STR_SIZE] = {'\0'};

    char sngauge_name[STR_SIZE] = {'\0'};


    // por padrão usa a interpolação Modified Shepard
    int interp_method = MSH_FLAG;


    //Lendo argumentos: https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html
    while(1){
        static struct option long_options[] =
        {
            {"help" , no_argument, NULL, 'h'},
            {"debug", no_argument, NULL, 'D'},

            {"avg"  , no_argument, NULL, 'a'},
            {"idw"  , no_argument, NULL, 'i'},
            {"msh"  , no_argument, NULL, 'm'},
            {"none"  , no_argument, NULL, 'n'},

            {"loni", required_argument, NULL, 'w'},
            {"lonf", required_argument, NULL, 'x'},
            {"lati", required_argument, NULL, 'y'},
            {"latf", required_argument, NULL, 'z'},

            {"s-ngauge", required_argument, NULL, 'g'},

            {"xi"  , required_argument, NULL, 'w'},
            {"xf"  , required_argument, NULL, 'x'},
            {"yi"  , required_argument, NULL, 'y'},
            {"yf"  , required_argument, NULL, 'z'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int opt = getopt_long (argc, argv, "aimnw:x:y:z:g:hD",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (opt == -1) break;

        switch (opt){
            case 'a':
                interp_method = AVG_FLAG;
                break;
            case 'i':
                interp_method = IDW_FLAG;
                break;
            case 'm':
                interp_method = MSH_FLAG;
                break;
            case 'n':
                interp_method = NON_FLAG;
                break;

            case 'w':
                xi=atof(optarg);
                break;
            case 'x':
                xf=atof(optarg);
                break;
            case 'y':
                yi=atof(optarg);
                break;
            case 'z':
                yf=atof(optarg);
                break;

            case 'g':
                strncpy(sngauge_name, optarg, STR_SIZE - 1);
                sngauge_name[STR_SIZE-1]='\0';
                break;


            case 'D':
                printf(" ==> MODO DE DEPURAÇÃO: apenas quadrículas interpoladas serão salvas.\n");
                g_debug = 1;
                break;

            case 'h':
                fprintf(stderr,
                        OPTS_MSG
                        "\n\nSe nenhuma opção for passada será usado:"
                        "\n\t%s "EXEC_MSG" "EXEM_MSG"\n"
                        , argv[0]);
                return 1;

            default:
                fprintf(stderr,"Uso: %s " EXEC_MSG "\n", argv[0]);
                return perro(ARG_ERR);
        }
    }


    if (optind > (argc-3)) {
        fprintf(stderr,"Uso: %s " EXEC_MSG "\n" HELP_MSG "\n", argv[0]);
        return ARG_ERR;
    }

    // le resto dos argumentos
    strncpy(pri_name,argv[optind++],STR_SIZE-1);
    pri_name[STR_SIZE-1]='\0';

    strncpy(sec_name,argv[optind++],STR_SIZE-1);
    sec_name[STR_SIZE-1]='\0';

    strncpy(out_name,argv[optind++],STR_SIZE-1);
    out_name[STR_SIZE-1]='\0';



    lab = open_bin_ctl(pri_name);
    if (lab == NULL){
        return perro_com(MEM_ERR, pri_name);
    }

    extra = open_bin_ctl(sec_name);
    if (extra == NULL){
        free_bin(lab);
        return perro_com(MEM_ERR, sec_name);
    }

    if ( strlen(sngauge_name) > 0 ){

        sngauge = open_bin_ctl(sngauge_name);

        if (sngauge == NULL){
            free_bin(lab);
            free_bin(extra);
            return perro_com(MEM_ERR, sngauge_name);
        }
    }


    // Saida na tela com as opções

    printf("Compondo:\n\tFonte Primária: %s\n\tFonte Secundária: %s\n\tLimites:%.2f,%.2f,%.2f,%.2f\n\tSaida: %s\n",
        lab->info.bin_filename,
        extra->info.bin_filename,
        yi, yf, xi, xf,
        out_name
    );

    printf("Método de interpolação:");

    switch (interp_method){
        case NON_FLAG:
            printf(" **SEM** Interpolação.\n");
            interpolation = none_interpolation;
            break;
        case AVG_FLAG:
            interpolation = average_interpolation;
            printf(" Média de quadriculas adjacentes.\n");
            break;
        case IDW_FLAG:
            interpolation = idweight_interpolation;
            printf(" Inverse distance weighting (IDW).\n");
            break;
        case MSH_FLAG:
            printf(" Modified Shepard.\n");
            interpolation = mshepard_interpolation;
            break;
    }

    if( sngauge ) printf("  Arquivo de número de estações: %s\n", sngauge_name);

    // funções adicionais
    dist = haversine_distance;
    weight = inverse_power_2;



    //junta os dois dados
    out_data = compose_data(lab,extra,xi,xf,yi,yf,sngauge);



    //erro ao ler arquivo
    if (out_data == NULL){
        free_bin(lab);
        free_bin(extra);

        return perro(FUN_ERR);
    }

    printf("Composição concluída.\n");

    //saida
    write_files(out_data,out_name,"Composição de dados");

    printf("Saída: %s\n",out_data->info.bin_filename);

    free_bin(lab);
    free_bin(extra);
    free_bin(out_data);
    free_bin(sngauge);
    free_dist_matrix(g_dist_matrix);


    return 0;
}


/* Junta dados de 'p' (primário) com 's' (secundário), dando preferencia para os
 * os dados primários. Quando não houver dado em 'p', faz uma interpolação em 's' com os
 * dados de 'p' que estão em volta. A operação apenas será realizada dentro da área
 * delimitada pelo quadrado com inicio em (xi,yi) e final em (xf,yf)
 * retorna uma estrutura com os dados já unificados
**/
binary_data* compose_data (binary_data* p, binary_data* s, coordtype xi, coordtype xf, coordtype yi, coordtype yf, binary_data* ngauge){

    info_ctl ctl;

    datatype undef;

    binary_data* bin_data; //dado que será retornado pela função

    if(p->info.ttype != s->info.ttype){
        fprintf(stderr,"ERRO: Arquivos não tem o mesmo tipo de dado.\n");
        return NULL;
    }

    // testando se as quadriculas são do mesmo tamanho
    if(!EQ_FLOAT(p->info.x.size,s->info.x.size) || !EQ_FLOAT(p->info.y.size,s->info.y.size)){
        fprintf(stderr,"ERRO: Tamanhos de quadrículas diferentes:x(%.2f & %.2f), y(%.2f & %.2f).\n",
            p->info.x.size,
            s->info.x.size,
            p->info.y.size,
            s->info.y.size
        );

        return NULL;
    }
    if(MAX(p->info.x.size,s->info.x.size) > MAX_SIZE || MAX(p->info.y.size,s->info.y.size) > MAX_SIZE){
        fprintf(stderr,"AVISO: Recomenda-se utilizar quadrículas de tamanho até '%.2f' para melhores resultados.\n",MAX_SIZE);
    }

    //testando se os grids são compatíveis
    if(!compat_grid(&(p->info),&(s->info))){
        fprintf(stderr,"ERRO: grids são incompatíveis, verifique as posições iniciais de lat e lon.\n");
        return NULL;
    }

    if (ngauge && !compat_grid(&(s->info),&(ngauge->info))){
        fprintf(stderr,"ERRO: (NUM_GAUGE) grids são incompatíveis, verifique as posições iniciais de lat e lon.\n");
        return NULL;

    }

    // Garantindo que as coordenadas estão dentro do globo
    xi = wrap_val(xi,MIN_X,MAX_X);
    xf = wrap_val(xf,MIN_X,MAX_X);
    yi = wrap_val(yi,MIN_Y,MAX_Y);
    yf = wrap_val(yf,MIN_Y,MAX_Y);


    // inicializa o ctl com valores da entrada primária
    cp_ctl(&(ctl),&(p->info));

    // ponto mais à esquerda
    ctl.x.i = MIN(p->info.x.i,s->info.x.i);
    ctl.y.i = MIN(p->info.y.i,s->info.y.i);
    // ponto mais à direita
    ctl.x.f = MAX(p->info.x.f, s->info.x.f);
    ctl.y.f = MAX(p->info.y.f, s->info.y.f);
    // quantidade de quadriculas entre o ponto inicial e o ponto final
    ctl.x.def = (int)((ctl.x.f - ctl.x.i) / ctl.x.size);
    ctl.y.def = (int)((ctl.y.f - ctl.y.i) / ctl.y.size);

    // caso o dado secundário comece antes do primário
    if(s->info.t_from_date_i < p->info.t_from_date_i){
        cp_date_ctl(&ctl,&(s->info));
    }


    // final - inicial
    ctl.tdef = MAX(p->info.t_from_date_i + p->info.tdef, s->info.t_from_date_i + s->info.tdef) - ctl.t_from_date_i;

    // aloca a matriz de dados
    bin_data = aloca_bin(ctl.x.def,ctl.y.def,ctl.tdef);

    if(!bin_data->data){
        perro_com(MEM_ERR,"Matriz de resultados");
        return NULL;
    }

    // copia as dimensões obtidas para a estrutura de dados
    cp_ctl(&(bin_data->info),&(ctl));


    g_dist_matrix = calc_dist(&(bin_data->info),dist);
    if (!g_dist_matrix){
        perro_com(MEM_ERR,"Matriz de distâncias");
        return NULL;
    }


    undef = bin_data->info.undef;


    // preenchendo dados
    #pragma omp parallel for
    for (size_t t = 0; t < ctl.tdef; t++){
        for (size_t y = 0; y < ctl.y.def; y++){
            for (size_t x = 0; x < ctl.x.def; x++){

                int modified = 0;

                // Detectando se o dado está dentro da área passada (bounding box)
                coordtype x_pos = wrap_val(x * ctl.x.size + ctl.x.i,MIN_X,MAX_X);
                coordtype y_pos = wrap_val(y * ctl.y.size + ctl.y.i,MIN_Y,MAX_Y);

                // Se o dado está fora da área solicitada
                if(!inside_area(x_pos, y_pos, xi, xf, yi, yf)){
                    if(EQ_FLOAT(cp_data_val(bin_data,s,x,y,t),undef)){
                        cp_data_val(bin_data,p,x,y,t);
                    }
                }
                else{
                    // Se o valor copiado do dado principal 'p' for indefinido
                    if(EQ_FLOAT(cp_data_val(bin_data,p,x,y,t),undef)){

                        // Executa a função de interpolação
                        modified = interpolation(bin_data,p,s,x,y,t,ngauge);
                    }
                }
                if(g_debug && !modified) set_data_val(bin_data,x,y,t,undef);
            }
        }
    }

    return bin_data;
}


/*
 * Interpolação simples utilizando a média das quadriculas adjacentes
 * Retorna 1 se ocorreu interpolação, retorna 0 caso contrário
**/
int average_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge){

    // copia o dado secundário 's_src' no ponto (x,y,t)
    datatype val = cp_data_val(dest,s_src,x,y,t);

    if(!EQ_FLOAT(val,dest->info.undef)){

        datatype sum = 0;
        int qt = 1;

        // somatório com valores adjacentes de 'p_src'
        for(int i = -1; i <= 1; i++){
            for(int j = -1; j <= 1; j++){


                datatype new_val = get_data_val(dest, p_src, x+i, y+j, t);

                if(!EQ_FLOAT(new_val,p_src->info.undef)){
                    sum += new_val;
                    qt++;
                }
            }
        }

        if ( qt > 1){
            // se a quadricula do dado secundário não tem estações suficiente
            if ( ngauge && (get_data_val(dest,ngauge,x,y,t)  < MIN_NGAUGE)){

                // valor do dado secundário não será adicionado à soma
                val = 0;
                qt--;
            }

            // soma o valor do dado secundário com os valores do primário
            sum += val;

            // valor final é a média dos valores
            set_data_val(dest,x,y,t, sum / (datatype)qt);

            return 1;
        }
    }

    return 0;
}



/* Ou invés de pesos iguais para todas as quadrículas adjacentes
 * utilizamos um peso inverso à distância, ou seja
 * quadrículas na diagonal tem peso menor que quadrículas diretamente do lado
 * */
int idweight_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge){

    datatype val = cp_data_val(dest,s_src,x,y,t);

    // se o houver um valor na quadrícula
    if(!EQ_FLOAT(val,dest->info.undef)){

        // acumuladores (somatório) & valor mínimo
        datatype    sum = 0;
        coordtype w_sum = 0;
        coordtype min_w = 1;

        // buscamos nas quadrículas adjacentes
        for(int i = -1; i <= 1; i++){
            for(int j = -1; j <= 1; j++){

                datatype neighbor = get_data_val(dest, p_src, x + i, y + j, t);
                coordtype w = 0;

                // queremos o peso apenas se o valor da quadricula não for indefinido
                // e se não for a própria quadricula
                if(!EQ_FLOAT(neighbor, p_src->info.undef) && !(i == 0 && j == 0)){

                    //atribui o valor ao peso e salva o menor valor encontrado
                    if(min_w > (w = get_weight(x,y,i,j))){
                        min_w = w;
                    }

                    // somatório dos valores e dos pesos
                    sum += neighbor * w;
                    w_sum += w;
                }
            }
        }

        if ( sum > 0){

            // se a quadricula do dado secundário não tem estações suficiente
            // não iremos usar esse valor na conta
            if ( ngauge && (get_data_val(dest,ngauge,x,y,t) < MIN_NGAUGE)) min_w = 0;


            // adicionamos o valor secundário com o menor peso encontrado
            sum += val * min_w;
            w_sum += min_w;

            set_data_val(dest,x,y,t, sum / w_sum);

            // retorna 1 apenas se o valor foi modificado
            return 1;
        }
    }

    return 0;
}



/* Tendo um raio maximo de busca, encontra as quadrículas ao redor
 * e faz uma função que da peso maior para as quadrículas mais próximas
 * */
int mshepard_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge){

    datatype val = cp_data_val(dest,s_src,x,y,t);

    // se o houver um valor na quadrícula
    if(!EQ_FLOAT(val,dest->info.undef)){

        // acumuladores (somatório)
        double sum = 0;
        double w_sum = 0;

        // lon e lat da quadrícula
        coordtype lon  = dest->info.x.i + x*dest->info.x.size;
        coordtype lat  = dest->info.y.i + y*dest->info.y.size;


        // RADIUS/width
        int steps = (int) (MAJOR_RADIUS/dist(0,lat,dest->info.x.size,lat));
        int qt = 1;

        // buscamos nas quadrículas adjacentes
        for(int i = -steps; i <= steps; i++){
            for(int j = -steps; j <= steps; j++){

                datatype neighbor = get_data_val(dest, p_src, x + i, y + j, t);

                // queremos o peso apenas se o valor da quadricula não for indefinido
                // e se não for a própria quadricula
                if(!EQ_FLOAT(neighbor, p_src->info.undef) && !(i == 0 && j == 0)){

                    double d = dist(lon, lat, lon + i*dest->info.x.size, lat + j*dest->info.y.size);
                    if (d < MAJOR_RADIUS){

                        double w = pow((MAJOR_RADIUS - d)/(MAJOR_RADIUS*d), BETA);

                        // somatório dos valores e dos pesos
                        sum += neighbor * w;
                        w_sum += w;

                        if (d < MINOR_RADIUS) qt++;
                    }
                }
            }
        }

        if(qt > 1){

            double w = 1/(double)qt;


            // Se houver pelo menos outras MIN_GRIDPOINTS quadrículas
            // além do dado secundário, testamos se foi
            // passado o arquivo de numgauge
            if ( (qt > MIN_GRIDPOINTS) && ngauge ){

                // se a quadricula do dado secundário não tem estações suficiente
                if (get_data_val(dest,ngauge,x,y,t) < MIN_NGAUGE){

                    // valor do dado secundário não será adicionado à soma
                    w = 0;
                }
            }

            if(g_debug){
                printf("[%lu,%lu,%lu]\nSEC: %.4f\n\tINTERP: %.4f\tQT: %d\nRESULT: %.4f\n\n",
                    x,
                    y,
                    t,
                    val,
                    (sum / w_sum),
                    qt,
                    (sum / w_sum)*(1-w) + val*w
                );
            }

            //((sum / w_sum)*qt + val)/(qt+1)
            set_data_val(dest,x,y,t,(sum / w_sum)*(1-w) + val*w);

            return 1;
        }
    }

    return 0;
}


int none_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, size_t x, size_t y, size_t t, binary_data* ngauge){
    cp_data_val(dest,s_src,x,y,t);
    return 0;
}


coordtype get_weight(size_t x, size_t y, int dx, int dy){
    if (!g_dist_matrix) return 0;

    if (dx == 0) return g_height;


    return g_dist_matrix[dy + 1][y];
}

/*
* Calculamos uma matriz de distâncias previamente,
* uma vez que as distancias entre quadriculas
* só variam em decorrência das latitudes.
* Assim não precisamos recalcular o mesmo valor
* para cada quadrícula individual
**/
coordtype** calc_dist(info_ctl* info,double (*dist)(double,double,double,double)){
    int y = info->y.def;
    coordtype** data;

    if(! (data = alloc_dist_matrix(3,y))) return NULL;


    coordtype lon_i = info->x.i;
    coordtype lon_f = lon_i + info->x.size;
    coordtype lat_i = info->y.i;

    // salva a altura na variavel global
    g_height = weight(dist(0, 0, 0, info->y.size));

    for(size_t i = 0; i < y; i++){

        // distancia para a quadricula na diagonal superior
        data[0][i] = weight(dist(lon_i, lat_i, lon_f, lat_i - info->y.size));

        // distancia para a quadricula ao lado
        data[1][i] = weight(dist(lon_i, lat_i, lon_f, lat_i));

        // distancia para a quadricula na diagonal inferior
        data[2][i] = weight(dist(lon_i, lat_i, lon_f, lat_i + info->y.size));

        lat_i += info->y.size;
    }

    return data;
}

coordtype** alloc_dist_matrix(int lin, int col){
    coordtype** mat;

    if(! (mat = malloc (lin * sizeof (coordtype*)))){
        return NULL;
    }

    // aloca um vetor com todos os elementos da matriz
    if (! (mat[0] = malloc(lin * col * sizeof(coordtype)))){
        free (mat[0]);
        return NULL;
    }

    // ajusta os demais ponteiros de linhas (i > 0)
    for (int i = 1; i < lin; i++)
        mat[i] = mat[0] + i * col;


    return mat;
}

void free_dist_matrix(coordtype** mat){
    if (mat){
        free (mat[0]);
        free (mat);
    }
}


// ૮・ﻌ・ა
int perro(int err_cod){
    return perro_com(err_cod,"");
}


int perro_com (int err_cod, const char* comment){
    switch (err_cod){
        case ARG_ERR:
            fprintf(stderr,"ERRO: argumentos inválidos.\n%s\n",comment);
        break;

        case ARQ_ERR:
            fprintf(stderr,"ERRO: não foi possível abrir arquivo.\n%s\n", comment);
        break;

        case MEM_ERR:
            fprintf(stderr,"ERRO: falha na alocação.\n%s\n",comment);
        break;

        case FUN_ERR:
            fprintf(stderr,"ERRO: função de juntar dados não pode ser executada.\n%s\n",comment);
        break;

        default:
            //fprintf(stderr,"ERRO:%s\n.",comment);
            return 0;
        break;
    }

    fprintf(stderr,"Abortando.\n");
    return err_cod;
}
