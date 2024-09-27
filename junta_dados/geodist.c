#include <math.h> 
#include "geodist.h"


/* Conversão de graus (latitude e longitude) para radianos*/
double to_radians(double degree) {
    return degree * (M_PI / 180.0);
}

/* Distância entre os pontos A e B é o mesmo que
 * a hipotenusa de um triângulo retângulo com catetos (ax-bx) e (ay-by)
 * */
double distance(double ax, double ay, double bx, double by){
    return hypot(ax - bx, ay - by);
}

/* O cálculo da distância euclidiana ao quadrado entre A e B, é bem
 * mais rápido que a distância pois não precisa calcular a raiz quadrada.
 * Útil para quando basta comparar valores (comparação de maior/menor).
**/
double sqr_distance(double ax, double ay, double bx, double by){
    return (ax - bx)*(ax - bx) + (ay - by)*(ay - by);
}

/* Haversine Distance*/
double haversine_distance(double loni, double lati, double lonf, double latf){
    
    // Convertendo latitude e longitude de graus para radianos
    lati = to_radians(lati);
    loni = to_radians(loni);
    latf = to_radians(latf);
    lonf = to_radians(lonf);

    // distancia entre as coordenadas
    double dlat = latf - lati;
    double dlon = lonf - loni;

    // Haversine formula
    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lati) * cos(latf) * sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    // Calculando em quilometros
    return EARTH_RADIUS * c;
}


/* Retorna 1 se a coordenada (x,y) está dentro da área delimitada por (xi,xf,yi,yf).
 * Retorna 0 caso contrário.
**/
int inside_area(double x, double y, double xi, double xf, double yi, double yf){
    return inside_axis(x,xi,xf) && inside_axis(y,yi,yf);
}

/* Retorna 1 se o valor 'val' está contido no intervalo (inclusivo).
 * Retorna 0 caso contrário.
**/
int inside_axis(double val, double initial, double final){

    //Se o valor for igual a um dos limites
    if(EQ_FLOAT(val, initial))  return 1;
    if(EQ_FLOAT(val, final))    return 1;

    //                           <———————————[~~~~]———————————>
    if(initial < final) return (val > initial) && (val < final);

    //                         <~~~~~~~~~~~]————[~~~~~~~~~~~>
    else                return (val < final) || (val > initial);

    return 0;
}
