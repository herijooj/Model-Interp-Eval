/*
Lucas Nogueira 2024
    Programa para testar resultados da interpolação
    de dados de diversas fontes.
    O programa irá retirar quadrículas aleatórias,
    realizar a interpolação e então comparar os
    resultados da interpolação com os valores antes
    da retirada.
**/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
#define MAJOR_RADIUS    200                         //Raio maior de busca por quadrículas
#endif
#ifndef MINOR_RADIUS
#define MINOR_RADIUS    100                         //Distância minima para realizar interpolação
#endif
#ifndef MAX_SIZE
#define MAX_SIZE        1.0                         //Tamanho de quadrícula máximo recomendado
#endif
#ifndef MIN_NGAUGE
#define MIN_NGAUGE      1                           //Quantidade minima de estações (gauges) permitidas ao usar arquivo 'ngauge'
#endif
#ifndef POINTS_QT
#define POINTS_QT       100000
#endif



typedef struct struct_idx3{
    size_t x;
    size_t y;
    size_t z;
} idx3;

idx3* sel_rand_points(binary_data* ref_data, size_t qt);
datatype metric(datatype obs,datatype predicted);


/* Função principal do programa, junta dados de 'p' (primario) e 's' (secundario)
 * preferencia para dados de 'p' delimitados pelo quadrado xi a xf e yi a yf
 * dados faltantes de 'p' são completados com valores de 's'
 * para cada valor 's' é feita cum calculo dependendo da função de interpolação escolhida
**/
binary_data* compose_data (binary_data* p, binary_data* s, coordtype xi, coordtype xf, coordtype yi, coordtype yf, idx3* points);


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
coordtype get_weight(int x, int y, int dx, int dy);



// Print Error: imprime uma mensagem de erro na saída padrão de erros
int perro (int err_cod);

// Print Error: imprime uma mensagem de erro na saída padrão de erros com um comentário adicional
int perro_com (int err_cod, const char* comment);




/*= FUNÇÕES DE INTERPOLAÇÃO =*/

/* Método mais simples, utilizado nas primeiras versões do programa.
 * Quadrícula resultante é a média simples entre valor da quadrícula do dado secundário
 * com as quadrículas adjacentes do dado primário
**/
int average_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t);

/* Inverse Distance Weighting (IDW)
**/
int idweight_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t);

/* Modified Shepard
**/
int mshepard_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t);

/* Nenhuma
 * */
int none_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t);

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
int (*interpolation) (binary_data*,binary_data*,binary_data*,int,int,int);

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

    binary_data* pri_bin = NULL;
    binary_data* sec_bin = NULL;
    binary_data* out_bin = NULL;

    char pri_name[STR_SIZE] = {'\0'};
    char sec_name[STR_SIZE] = {'\0'};
    char out_name[STR_SIZE] = {'\0'};

    //coordenadas da america do sul
    coordtype xi = -89.5f,
        xf = -31.5f,
        yi = -56.5f,
        yf = 14.5f;

    //if (argc < 4){
    if (argc < 4){
        fprintf(stderr,"Uso: %s primario.ctl secundario.ctl\n",argv[0]);
        return 1;
    }
    else{
        strncpy(pri_name,argv[1],STR_SIZE-1);
        pri_name[STR_SIZE-1]='\0';

        strncpy(sec_name,argv[2],STR_SIZE-1);
        sec_name[STR_SIZE-1]='\0';

        strncpy(out_name,argv[3],STR_SIZE-1);
        out_name[STR_SIZE-1]='\0';

    }


    if (!(pri_bin = open_bin_ctl(pri_name))){
        fprintf(stderr,"ERRO: falha na alocação (%s).\n",pri_name);
        return 1;
    }

    ;
    if (!(sec_bin = open_bin_ctl(sec_name))){
        free_bin(pri_bin);
        fprintf(stderr,"ERRO: falha na alocação (%s).\n",sec_name);
        return 1;
    }

    // funções adicionais
    interpolation = mshepard_interpolation;
    dist = haversine_distance;
    weight = inverse_power_2;

    srandom(time(NULL));

    idx3* points;
    if(!(points = sel_rand_points(pri_bin,POINTS_QT))){
        fprintf(stderr,"Alguma coisa ai deu ruim viu\n");
        return 1;
    }

    //junta os dois dados
    out_bin = compose_data(pri_bin,sec_bin,xi,xf,yi,yf,points);


    //erro ao ler arquivo
    if (!out_bin){
        free_bin(pri_bin);
        free_bin(sec_bin);

        fprintf(stderr,"ERRO: falha na composição.\n");
        return 1;
    }


    //saida
    write_files(out_bin,out_name);


    free_bin(pri_bin);
    free_bin(sec_bin);
    free_bin(out_bin);
    free(points);
    free_dist_matrix(g_dist_matrix);

    return 0;
}

idx3* sel_rand_points(binary_data* ref_data, size_t qt){
    idx3* idx_array = (idx3*) malloc(sizeof(idx3) * qt);

    if(!idx_array) return NULL;

    size_t x_max = ref_data->info.x.def;
    size_t y_max = ref_data->info.y.def;
    size_t t_max = ref_data->info.tdef;

    size_t att = 0;
    for(size_t i = 0; i < qt;att++){
        idx_array[i].x = random()%x_max;
        idx_array[i].y = random()%y_max;
        idx_array[i].z = random()%t_max;

        size_t pos = get_pos(&(ref_data->info),idx_array[i].x,idx_array[i].y,idx_array[i].z);
        if(!EQ_FLOAT(ref_data->data[pos],ref_data->info.undef)) i++;
    }
    
    printf("%d pontos de grade gerados após %lu tentatativas",POINTS_QT,att);
    return idx_array;
}


/* Junta dados de 'p' (primário) com 's' (secundário), dando preferencia para os
 * os dados primários. Quando não houver dado em 'p', faz uma interpolação em 's' com os
 * dados de 'p' que estão em volta. A operação apenas será realizada dentro da área
 * delimitada pelo quadrado com inicio em (xi,yi) e final em (xf,yf)
 * retorna uma estrutura com os dados já unificados
**/
binary_data* compose_data (binary_data* p, binary_data* s, coordtype xi, coordtype xf, coordtype yi, coordtype yf, idx3* points){

    info_ctl ctl;

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
    bin_data = aloca_bin_fill(ctl.x.def,ctl.y.def,ctl.tdef,ctl.undef);

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


    
    double sum=0, sqr_sum=0;

    // preenchendo dados
    for (size_t i = 0; i < POINTS_QT; i++){

        size_t x = get_equivalent_x(p,bin_data,points[i].x);
        size_t y = get_equivalent_y(p,bin_data,points[i].y);
        size_t t = get_equivalent_t(p,bin_data,points[i].z);


        // Executa a função de interpolação
        interpolation(bin_data,p,s,x,y,t);
        
        datatype real_val = p->data[get_pos(&(p->info), points[i].x, points[i].y, points[i].z)];
        datatype predicted_val = bin_data->data[get_pos(&(bin_data->info),x,y,t)];

        // calc RMSE e |BIAS|
        sum += fabs(predicted_val - real_val);
        sqr_sum += (predicted_val - real_val)*(predicted_val - real_val);

        set_data_val(bin_data,x,y,t,metric(real_val,predicted_val));
    }


    // Resumo
    printf("\n==================\n");
    printf("Tamanho da amostra: %d\n|BIAS|: %f\nRMSE: %f\n",POINTS_QT,sum/POINTS_QT,sqrt(sqr_sum/POINTS_QT));
    printf("==================\n");

    return bin_data;
}

datatype metric(datatype obs,datatype predicted){
    //return fabs(predicted-obs)/obs;
    return (EQ_FLOAT(0,obs)) ? (predicted-obs)/(double)obs : 0;
}


/*
 * Interpolação simples utilizando a média das quadriculas adjacentes
 * Retorna 1 se ocorreu interpolação, retorna 0 caso contrário
**/
int average_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t){

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
            //// se a quadricula do dado secundário não tem estações suficiente
            //if ( ngauge && (get_data_val(dest,ngauge,x,y,t)  < MIN_NGAUGE)){

            //    // valor do dado secundário não será adicionado à soma
            //    val = 0;
            //    qt--;
            //}

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
int idweight_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t){

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

            //// se a quadricula do dado secundário não tem estações suficiente
            //// não iremos usar esse valor na conta
            //if ( ngauge && (get_data_val(dest,ngauge,x,y,t) < MIN_NGAUGE)) min_w = 0;


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
int mshepard_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t){

    datatype val = cp_data_val(dest,s_src,x,y,t);

    // se o houver um valor na quadrícula
    if(!EQ_FLOAT(val,dest->info.undef)){

        // acumuladores (somatório)
        datatype    sum = 0;
        coordtype w_sum = 0;

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


            //// Se houver pelo menos outras MIN_GRIDPOINTS quadrículas
            //// além do dado secundário, testamos se foi
            //// passado o arquivo de numgauge
            //if ( (qt > MIN_GRIDPOINTS) && ngauge ){

            //    // se a quadricula do dado secundário não tem estações suficiente
            //    if (get_data_val(dest,ngauge,x,y,t) < MIN_NGAUGE){

            //        // valor do dado secundário não será adicionado à soma
            //        w = 0;
            //    }
            //}

            //((sum / w_sum)*qt + val)/qt
            set_data_val(dest,x,y,t,(sum / w_sum)*(1-w) + val*w);

            return 1;
        }
    }

    return 0;
}


int none_interpolation(binary_data* dest, binary_data* p_src, binary_data* s_src, int x, int y, int t){
    cp_data_val(dest,s_src,x,y,t);
    return 0;
}


coordtype get_weight(int x, int y, int dx, int dy){
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
