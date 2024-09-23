/*
Lucas Nogueira e Wellington Almeida 2023
Programa para juntar dois arquivos binários
Missão:
    Completar quadrículas faltantes do laboratório
    com dados do GPCC, fazendo uma suavização
    na transição de um conjunto de dados para outro
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>        //mult thread
#include <unistd.h>     //getopt
#include <string.h>     //strncpy
#include "c_ctl.h"


//Códigos de erro
#define ARG_ERR 1   //Erro com parâmetros
#define ARQ_ERR 2   //Erro com arquivos
#define MEM_ERR 3   //Erro com memória
#define FUN_ERR 4   //Erro na função

//Funções matemáticas
#define MIN(a,b)        (((a)<(b))?(a):(b))         //menor valor entre a e b
#define MAX(a,b)        (((a)>(b))?(a):(b))         //maior valor entre a e b
#define F_ERROR         (0.0001f)                   //Erro permitido (float)
#define EQ_FLOAT(a,b)   (fabs((a)-(b)) < F_ERROR)   //Se dois floats são iguais



/* Função principal do programa, junta dados de 'p' e 's'
 * preferencia para dados de 'p' delimitados pelo quadrado (xi,xf):(yi,yf)
 * dados faltantes de 'p' são completados com valores de 's'
 * para cada valor 's' é feita uma média com valores de 'p' adjacentes
*/
binary_data* compose_data (binary_data* p, binary_data* s, float xi, float xf, float yi, float yf);


/* Função auxiliar para verificar se uma coordenada está dentro da área 
 * Retorna 1 se a coordenada (x,y) está dentro da área delimitada por (xi,xf,yi,yf).
 * Retorna 0 caso contrário.
*/
int inside_area(float x, float y, float xi, float xf, float yi, float yf);

/* Função auxiliar para verificar se um valor está dentro do intervalo
 * Retorna 1 se o valor 'val' está contido no intervalo (inclusivo).
 * Retorna 0 caso contrário.
*/
int inside_axis(float val, float initial, float final);

/* Verifica se dois grids são compatíveis.
 * Retorna 1 se os dois são compatíveis, retorna 0 caso contrário.
*/
int compat_grid(info_ctl* a, info_ctl* b);

// Print Error: imprime uma mensagem de erro na saída padrão de erros
int perro (int err_cod);

// Print Error: imprime uma mensagem de erro na saída padrão de erros com um comentário
int perro_com (int err_cod, const char* comment);



int main(int argc, char *argv[]) {
    
    binary_data* lab;        //dados do laboratório

    binary_data* extra;      //dados complementares que serão adicionados

    binary_data* out_data;   //arquivo de saída

    //coordenadas da america do sul
    float xi = -89.5f,
        xf = -31.5f,
        yi = -56.5f,
        yf = 14.5f;

    char pri_name[STR_SIZE] = {'\0'};
    char sec_name[STR_SIZE] = {'\0'};
    char out_name[STR_SIZE] = {'\0'};

    int opt;



    //Lendo argumentos
    while((opt = getopt(argc,argv,"a:b:c:d:")) != -1){
        switch(opt)  {  
            case 'a':
                yi=atof(optarg);
                break;
            case 'b':  
                yf=atof(optarg);
                break;
            case 'c':  
                xi=atof(optarg);
                break;
            case 'd':  
                xf=atof(optarg);
                break;
            default:
                return perro_com(ARG_ERR,"Uso:\n./compose [-a lat_ini] [-b lat_fin] [-c lon_ini] [-d lon_fin] primario.ctl secundario.ctl prefixo_saida\n");
        }
    }

    if (optind > (argc-3)) {
        return perro_com(ARG_ERR,"Uso:\n./compose [-a lat_ini] [-b lat_fin] [-c lon_ini] [-d lon_fin] primario.ctl secundario.ctl prefixo_saida\n");
    }

    // le resto dos argumentos
    strncpy(pri_name,argv[optind++],STR_SIZE);
    strncpy(sec_name,argv[optind++],STR_SIZE);
    strncpy(out_name,argv[optind++],STR_SIZE);


    lab = open_bin_ctl(pri_name);
    //erro ao ler arquivo
    if (lab == NULL){
        return perro_com(MEM_ERR, pri_name);
    }
    
    extra = open_bin_ctl(sec_name);
    //erro ao ler arquivo
    if (extra == NULL){
        free_bin(lab);
        
        return perro_com(MEM_ERR, sec_name);
    }

    printf("Compondo:\nFonte Primária: %s\nFonte Secundária: %s\nLimites:%.2f,%.2f,%.2f,%.2f\nSaida: %s\n",
        lab->info.bin_filename,
        extra->info.bin_filename,
        yi, yf, xi, xf,
        out_name
    );


    //junta os dois dados
    out_data = compose_data(lab,extra,xi,xf,yi,yf);
    
    
    //erro ao ler arquivo
    if (out_data == NULL){
        free_bin(lab);
        free_bin(extra);

        return perro(FUN_ERR);
    }

    printf("Composição concluída.\n");

    //saida
    write_files(out_data,out_name);

    printf("Saída: %s\n",out_data->info.bin_filename);

    free_bin(lab);
    free_bin(extra);
    free_bin(out_data);


    return 0;
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

/* Junta dados de 'p' (primário) com 's' (secundário), dando preferencia para os
 * os dados primários. Quando não houver dado em 'p', faz uma média de 's' com os
 * dados de 'p' que estão em volta. A operação apenas será realizada dentro da área
 * delimitada pelo quadrado com inicio em (xi,yi) e final em (xf,yf)
 * retorna uma estrutura com os dados já unificados
*/
binary_data* compose_data (binary_data* p, binary_data* s, float xi, float xf, float yi, float yf){

    info_ctl ctl;

    size_t pos;
    float undef;

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
    bin_data = aloca_bin(ctl.x.def,ctl.y.def,ctl.tdef);

    if(!bin_data->data){
        fprintf(stderr,"ERRO: Alocação para matriz de resultado falhou.\n");
        return NULL;
    }

    // copia as dimensões obtidas para a estrutura de dados
    cp_ctl(&(bin_data->info),&(ctl));


    undef = bin_data->info.undef;

    // preenchendo dados
    #pragma omp parallel for private(pos)
    for (size_t t = 0; t < ctl.tdef; t++){
        for (size_t y = 0; y < ctl.y.def; y++){
            for (size_t x = 0; x < ctl.x.def; x++){

                // calcula a posição na matriz
                pos = get_pos(&(bin_data->info),x,y,t);

                // Detectando se o dado está dentro da área passada (bounding box)
                float x_pos = wrap_val(x * ctl.x.size + ctl.x.i,MIN_X,MAX_X);
                float y_pos = wrap_val(y * ctl.y.size + ctl.y.i,MIN_Y,MAX_Y);
                
                // Se o dado está fora da área solicitada
                if(!inside_area(x_pos, y_pos, xi, xf, yi, yf)){

                    // Copia o valor do dado secundário 's' para o dado final
                    if(EQ_FLOAT(cp_data_val(bin_data,s,x,y,t),s->info.undef)){
                        // Se o valor for indefinido, usa o UNDEF de bin_data
                        bin_data->data[pos] = undef;
                    }
                }
                else{
                    // Se o valor copiado do dado principal 'p' for indefinido
                    if(EQ_FLOAT(cp_data_val(bin_data,p,x,y,t),undef)){

                        // copia o dado secundário 's'
                        float sum = cp_data_val(bin_data,s,x,y,t);
                        if(!EQ_FLOAT(sum,s->info.undef)){
                            int qt = 1;

                            // somatório com valores adjacentes de 'p'
                            for(int i = -1; i <= 1; i++){
                                for(int j = -1; j <= 1; j++){

                                    
                                    float new_val = get_data_val(bin_data, p, x+i, y+j, t);
                                    
                                    if(!EQ_FLOAT(new_val,undef)){
                                        sum += new_val;
                                        qt++;
                                    }
                                }
                            }

                            // valor final é a média dos valores
                            bin_data->data[pos] = sum / (float)qt;
                        }
                        else{
                            bin_data->data[pos] = undef;
                        }
                    }
                }
            }
        }
    }  

    return bin_data;
}


/* Retorna 1 se a coordenada (x,y) está dentro da área delimitada por (xi,xf,yi,yf).
 * Retorna 0 caso contrário.
*/
int inside_area(float x, float y, float xi, float xf, float yi, float yf){
    return inside_axis(x,xi,xf) && inside_axis(y,yi,yf);
}

/* Retorna 1 se o valor 'val' está contido no intervalo (inclusivo).
 * Retorna 0 caso contrário.
*/
int inside_axis(float val, float initial, float final){

    //Se o valor for igual a um dos limites
    if(EQ_FLOAT(val, initial))  return 1;
    if(EQ_FLOAT(val, final))    return 1;

    //                           <———————————[~~~~]———————————>
    if(initial < final) return (val > initial) && (val < final);

    //                         <~~~~~~~~~~~]————[~~~~~~~~~~~>
    else                return (val < final) || (val > initial);

    return 0;
}

/* Verifica se dois grids são compatíveis.
 * Retorna 1 se os dois são compatíveis, retorna 0 caso contrário.
*/
int compat_grid(info_ctl* a, info_ctl* b){
    return (
        EQ_FLOAT(fmod(fabs(a->x.i - b->x.i),a->x.size),0.0f) &&
        EQ_FLOAT(fmod(fabs(a->y.i - b->y.i),a->y.size),0.0f)
    ); 
}
