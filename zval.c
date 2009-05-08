#include "calc.h"

static long strpos(const char * str, char letter) {
    int i;
    for (i=0; *str ; *str++,i++) {
       if (*str == letter) {
           return i;
       }
    }
    return -1;
}


/**
 * Simple funcion para simular pow
 */
static long long int mpow(long long int a, unsigned int b) {
    long long int c;
    int i;
    c = 1;
    for (i=0; i < b; i++) {
        c *= a;
    }
    return c;
}

/**
 *  Esta funcion recibe un string "number", y lo transforma
 *  a su similar en zval.
 */
void parse_number(const char * number, zval * rnumber) {
    int i;
    zval value;
    if ((i=strpos(number,'e')) != -1 || (i=strpos(number, 'E')) != -1) {
        char tmp1[80], tmp2[80];
        zval ztmp2;
        long exp;
        strncpy(tmp1, number,i);
        strcpy(tmp2, number + i + 1);
        parse_number(tmp1, &value);
        parse_number(tmp2, &ztmp2);


        Z_GET_INT(ztmp2) = mpow(10, Z_GET_INT(ztmp2) );

        zval_mul(&value, value, ztmp2 );
    }
    else if (strpos(number,'.') != -1) {
        Z_SET_TYPE(value, Z_FLOAT); 
        Z_GET_FLOAT(value) = atof(number);
    } else {
        Z_SET_TYPE(value,Z_INT);
        Z_GET_INT(value) = atol(number);
    }
    *rnumber = value;

}

/**
 * Esta funcion imprime al stdout la variable y su tipo
 */
void var_dump(zval value)
{
    switch(Z_TYPE(value)) {
    case Z_INT:
        printf("entero:%d\n", Z_GET_INT(value));
        break;
    case Z_FLOAT:
        printf("flotante:%f\n", Z_GET_FLOAT(value));
        break;
    default:
        printf("invalid header-type: %d", Z_TYPE(value));
    }
    fflush(stdout);
}

/**
 * Resta
 */
void zval_sub(zval * result, zval a, zval b) {
    zval out;
    if (Z_TYPE(a) == Z_FLOAT || Z_TYPE(b) == Z_FLOAT)  {
        Z_SET_TYPE(out, Z_FLOAT);
        Z_GET_FLOAT(out) = Z_VALUE(a) - Z_VALUE(b);
    } else {
        Z_SET_TYPE(out, Z_INT);
        Z_GET_INT(out) = Z_VALUE(a) - Z_VALUE(b);
    }
    *result = out;
}

/**
 * Suma
 */
void zval_add(zval * result, zval a, zval b) {
    zval out;
    if (Z_TYPE(a) == Z_FLOAT || Z_TYPE(b) == Z_FLOAT)  {
        Z_SET_TYPE(out, Z_FLOAT);
        Z_GET_FLOAT(out) = Z_VALUE(a) + Z_VALUE(b);
    } else {
        Z_SET_TYPE(out, Z_INT);
        Z_GET_INT(out) = Z_VALUE(a) + Z_VALUE(b);
    }
    *result = out;
}

/**
 * Division
 */
void zval_div(zval * result, zval a, zval b) {
    zval out;
    double ta, tb;
    ta = (double) Z_VALUE(a);
    tb = (double) Z_VALUE(b);
    if ( ta/tb == (int)ta/tb) {
        Z_SET_TYPE(out, Z_FLOAT);
        Z_GET_FLOAT(out) = Z_VALUE(a) / Z_VALUE(b);
    } else {
        Z_SET_TYPE(out, Z_INT);
        Z_GET_INT(out) = Z_VALUE(a) / Z_VALUE(b);
    }
    *result = out;
}

/**
 * Multiplicacon
 */
void zval_mul(zval * result, zval a, zval b) {
    zval out;
    if (Z_TYPE(a) == Z_FLOAT || Z_TYPE(b) == Z_FLOAT)  {
        Z_SET_TYPE(out, Z_FLOAT);
        Z_GET_FLOAT(out) = Z_VALUE(a) * Z_VALUE(b);
    } else {
        Z_SET_TYPE(out, Z_INT);
        Z_GET_INT(out) = Z_VALUE(a) * Z_VALUE(b);
    }
    *result = out;
}
