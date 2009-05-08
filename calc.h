#include "anlex.h"

#define TINT    1
#define TFLOAT  2
#define TOP     3
#define TFREE   4

#define GET_MEM(obj, size) obj = malloc(size); if (obj==0) { \
            printf("malloc fails: %d", __LINE__); \
            exit(-1); \
        }

#define MEM_RESIZE(obj, size)  obj = realloc(obj, size);if (obj==0) { \
            printf("realloc fails: %d", __LINE__); \
            exit(-1); \
        }

#define PREBUF_SIZE 3

#define INIT_PZTOKEN(a) GET_MEM(a, sizeof(ar_token) * PREBUF_SIZE); \
    int tmpi; \
    for (tmpi=0; tmpi < PREBUF_SIZE; tmpi++) { \
        a[tmpi].nsize = a[tmpi].ntoken = 0;\
    }

#define RESIZE_PZTOKEN(a,ini, size) MEM_RESIZE(a, (ini +size) * sizeof(ar_token)); \
        int tmpi; \
        for (tmpi=ini; tmpi < ini + size; tmpi++) { \
            a[tmpi].nsize = a[tmpi].ntoken = 0;\
        }
                

/**
 * esta estrutra represanta cualquier numero 
 * posible
 */
typedef struct {
    union {
        long long int   entero;
        float  flotante;
        int    operation;
    } value;
    int type;
} zval;

/* wrappers para manipular los zval */
#define Z_INT                   1
#define Z_FLOAT                 2
#define Z_OP                    3
#define Z_TYPE(zval)            zval.type
#define Z_SET_TYPE(zval,ztype)  zval.type = ztype 

#define Z_GET_INT(zval)         zval.value.entero

#define Z_GET_FLOAT(zval)       zval.value.flotante

#define Z_TRANS_OP(zval,str)    Z_SET_TYPE(zval, Z_OP); \
                                Z_GET_OP(zval) = str[0];
#define Z_GET_OP(zval)          zval.value.operation
#define Z_GET_NUM               Z_GET_FLOAT

#define Z_VALUE(zval)           (Z_TYPE(zval) == Z_INT ? Z_GET_INT(zval) : Z_GET_FLOAT(zval))



/**
 *  Esta estructura guarda 
 *  los tokens por cada linea
 */
typedef struct {
    int line;
    token * token;
    /* numero de tokens */
    int ntoken;
    /* tamanho de memoria ya reservado */
    int nsize;
} ar_token;

/**
 * Stack
 */
typedef struct {
    zval * values;
    int size;
    int position;
} stack;
#define INIT_STACK(stack, xsize)     stack.size     = xsize; \
                                     stack.position = -1; \
                                     GET_MEM(stack.values, sizeof(zval) * (xsize+1));

#define DESTROY_STACK(stack)         free(stack.values)
#define STACK_NEXT(stack)            stack.position++;

/* backward */
void zval_sub(zval * result, zval a, zval b);
void zval_add(zval * result, zval a, zval b);
void zval_div(zval * result, zval a, zval b);
void zval_mul(zval * result, zval a, zval b);
void var_dump(zval value);
void parse_number(const char * number, zval * rnumber);

