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

/* funciona faltantes en Linux */
#ifdef LINUX
int stricmp(const char * s1, const char * s2)
{
    for(; toupper(*s1) == toupper(*s2); ++s1, ++s2)
        if(*s1 == 0)
            return 0;
    return *(unsigned char *)toupper(s1) < *(unsigned char *)toupper(s2) ? -1 : 1;

}
#endif

/* backwards */
void zval_mult(zval * result, zval a, zval b);
void zval_add(zval * result, zval a, zval b);
void zval_sub(zval * result, zval a, zval b);
void zval_div(zval * result, zval a, zval b);

/**
 *  Encolar el token actual, y asignarle la linea a la cual 
 *  pertenece
 */
void token_queue(ar_token * gtoken, int lineno, token pztoken)
{
    ar_token * current;
    token    * curTok;
    if (gtoken == 0) {
        return;
    }
    current = & gtoken[lineno-1];
    if (current->nsize == 0) {
        GET_MEM(current->token, sizeof(token) * PREBUF_SIZE);
        current->nsize  = PREBUF_SIZE;
    }

    if (current->ntoken+1 == current->nsize) {
        current->nsize += PREBUF_SIZE;
        MEM_RESIZE(current->token, current->nsize * sizeof(token));
    }

    curTok = & current->token[current->ntoken];

    /* copiar pztoken a nuestra pila de tokens */
    curTok->compLex = pztoken.compLex;
    GET_MEM(curTok->pe, sizeof(entrada)); 
    curTok->pe->compLex =  pztoken.pe->compLex;
    strcpy(curTok->pe->lexema, pztoken.pe->lexema);

    current->ntoken++;
}

void stack_push(stack * pStack, zval value) {
    pStack->values[++pStack->position] = value;
}

zval stack_get(stack * pStack) {
    return pStack->values[pStack->position];
}

zval stack_pop(stack * pStack) {
    return pStack->values[ pStack->position-- ];
}

/**
 *  Esta funcion realiza los procesos para cada linea
 */ 
static int get_precedence(int op) {
    int i;
    switch (op) {
    case '+':
    case '-':
        i = 1;
        break;
    case '/':
    case '*':
        i = 2;
        break;
    case '(':
        i = 3;
        break;
    default:
        i = 0;
    }
    return i;
}

/**
 *  Ejecuta una expresion post fija.
 *
 */
void postfix_exec(stack * svalues, zval * return_value, int * status)
{
    int i;
    zval value;
    zval result, num1, num2;
    stack estack;

    Z_GET_NUM(result) = 0;

    INIT_STACK(estack, svalues->position+1);

    for (i=0; i <= svalues->position; i++) {
        value = svalues->values[i];
        switch( Z_TYPE(value) ) {
        case Z_OP:
            switch (Z_GET_OP(value)) {
            case '(':
            case ')':
                continue;
            }
            num2 = stack_pop(&estack);
            num1 = stack_pop(&estack);
            switch (Z_GET_OP(value)) {
            case '+':
                zval_add(&result, num1, num2);
                break;
            case '-':
                zval_sub(&result, num1, num2);
                break;
            case '/':
                zval_div(&result, num1, num2);
                break;
            case '*':
                zval_mult(&result, num1, num2);
                break;
            }
            stack_push(&estack, result);
            break;
        case Z_INT:
        case Z_FLOAT:
            stack_push(&estack, value);
            break;
        }
    }

    *return_value = stack_pop(&estack);

    if (estack.position != -1) {
        *status = ERR_EXPR;
    }

    DESTROY_STACK(estack);
}

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
 *
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
 *
 */
void zval_mult(zval * result, zval a, zval b) {
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

long long int mpow(long long int a, unsigned int b) {
    long long int c;
    int i;
    c = 1;
    for (i=0; i < b; i++) {
        c *= a;
    }
    return c;
}

/**
 * 
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

        zval_mult(&value, value, ztmp2 );
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
 * Realiza los procesos por cada linea.
 *
 * Parentesis no reconce ()
 */
void token_queue_single_process(ar_token * tokline, zval * return_value, int * status)
{
    int i;
    token * tok;
    int parentesis=0, pred, expected = 0;
    stack svalues, sop; 
    zval value;
    
    
    *status = OK;

    INIT_STACK(svalues, tokline->ntoken);
    INIT_STACK(sop, tokline->ntoken);

    for (i=0; i < tokline->ntoken && *status==OK ; i++) {
        tok = &tokline->token[i];
        switch (tok->pe->compLex) {
        case '(':
            parentesis++;
            break;
        case ')':
            parentesis--;
            break;
        case NUM:
            if (expected != 0) {
                *status = ERR_UNEXPECTED_NUM;
                break;
            }
            parse_number(tok->pe->lexema, &value);
            stack_push(&svalues, value);
            expected = 1;
            break;
        case OPMULT:
        case OPSUMA:
            if (expected != 1) {
                *status = ERR_UNEXPECTED_SYM;
                break;
            }
            Z_TRANS_OP(value, tok->pe->lexema);
            pred = get_precedence( Z_GET_OP(value) );
            while (sop.position > -1 && pred <= get_precedence( Z_GET_OP(stack_get(&sop))  )  ) {
                stack_push(&svalues, stack_pop(&sop));
            }
            stack_push(&sop, value);
            expected = 0;
            break;
        case 8: /* EOF */
            break;
        default:
            printf("Valor inesperado (%s)\n", tok->pe->lexema);
            *status = ERR_UNEXPECTED_VALUE;
            break;
        }
    }
    
    if (expected != 1) {
         *status = ERR_UNEXPECTED_SYM;
    }

    while (sop.position > -1) {
        stack_push(&svalues, stack_pop(&sop));
    }

    if (parentesis != 0) {
        *status = ERR_UNBALANCED_PAR;
    } 
   
    if (*status == OK) {
        postfix_exec(&svalues, return_value, status); 
    }

    DESTROY_STACK(svalues);
    DESTROY_STACK(sop);
}

const char * get_error_str(int errorno) {
    char * errstr;
    switch (errorno) {
    case OK:
        errstr = "No hay errores";
        break;
    case ERR_UNEXPECTED_VALUE:
        errstr = "No se esperaba el token";
        break;
    case ERR_UNBALANCED_PAR:
        errstr = "Parentesis no balanceados";
        break;
    case ERR_UNEXPECTED_NUM:
        errstr = "No se esperaba un numero";
        break;
    case ERR_UNEXPECTED_SYM:
        errstr = "No se esperaba un simbolo";
        break;
    case ERR_EXPR:
        errstr = "Error, expresion invalida";
        break;
    default:
        errstr = "Error desconocido";
    }
    return errstr;
}

/**
 * Esta funcion ejecuta linea por linea las expresiones
 */
void token_queue_process(ar_token * gtoken, int size)
{
    int i;
    zval valor;
    int error;
    for (i=0; i < size-1; i++) {
        error = 0;
        //printf("processing line %d\n", i);
        token_queue_single_process(&gtoken[i], &valor, &error);
        if (error != OK) {
            printf("Error en la linea %d [%s]\n", i+1, get_error_str(error));
            continue;
        } else {
            if (Z_TYPE(valor) == Z_INT) {
                printf("Línea %d: %d\n", i+1, Z_GET_INT(valor));
            } else {
                printf("Línea %d: %f (float)\n", i+1, Z_GET_FLOAT(valor));
            }
            fflush(stdout);
        }
    }
}

/**
 *  Libera la memoria asignada  para nuestra cola de 
 *  tokens por linea.
 */
void token_queue_destroy(ar_token * gtoken, int size)
{
    int i, e;
    for (i=0; i < size; i++) {
        for (e=0; e < gtoken[i].ntoken; e++) {
            free(gtoken[i].token[e].pe);
        }
        free(gtoken[i].token);
    }
    free(gtoken);
}


int main(int argc,char* args[])
{
    // inicializar analizador lexico
    ar_token * pzToken;
    int complex=0;
    int lastline;

    INIT_PZTOKEN(pzToken);

    initTabla();
    initTablaSimbolos();
    
    if(argc > 1)
    {
        if (!(archivo=fopen(args[1],"rt")))
        {
            printf("Archivo no encontrado.");
            exit(1);
        }
        while (t.compLex!=EOF){
            sigLex();
            token_queue(pzToken, numLinea, t);
            if ( (numLinea) % PREBUF_SIZE == 0) {
                RESIZE_PZTOKEN(pzToken, numLinea, PREBUF_SIZE);
            }
        }
        /* procesar los datos */
        token_queue_process(pzToken, numLinea);

        /* liberar la memoria */
        token_queue_destroy(pzToken, numLinea);

        fclose(archivo);
    }else{
        printf("Debe pasar como parametro el path al archivo fuente.");
        exit(1);
    }

    return 0;
}
