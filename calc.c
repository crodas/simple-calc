#include "anlex.h"

#define TINT    1
#define TFLOAT  2
#define TOP     3
#define TFREE   4

#define GET_MEM(obj, size) obj = malloc(size); if (obj==0) { \
            printf("malloc fails: %d", __LINE__); \
            exit(-1); \
        }

#define INIT_PZTOKEN(a) GET_MEM(a, sizeof(ar_token) * 40); \
    int tmpi; \
    for (tmpi=0; tmpi < 40; tmpi++) { \
        a[tmpi].nsize = a[tmpi].ntoken = 0;\
    }
        
/**
 * esta estrutra represanta cualquier numero 
 * posible
 */
typedef struct {
    union {
        long   entero;
        float  flotante;
        int    operation;
    } value;
    int type;
} zval;

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
        GET_MEM(current->token, sizeof(token) * 40);
        current->nsize  = 40;
    }

    if ((current->ntoken+1)%40 == 0) {
        printf("out of stack");
        return;
    }

    curTok = & current->token[current->ntoken];

    /* copiar pztoken a nuestra pila de tokens */
    curTok->compLex = pztoken.compLex;
    GET_MEM(curTok->pe, sizeof(entrada)); 
    curTok->pe->compLex =  pztoken.pe->compLex;
    strcpy(curTok->pe->lexema, pztoken.pe->lexema);

    current->ntoken++;
}

/**
 *  Esta funcion realiza los procesos para cada linea
 */ 
void token_queue_single_process(ar_token * tokline, zval * return_value, int * status)
{
    int i;
    token * tok;
    for (i=0; i < tokline->ntoken ; i++) {
        tok = &tokline->token[i];
        printf("\t %s -> %d\n",tok->pe->lexema,tok->compLex);
    }
    
}

/**
 * Esta funcion ejecuta linea por linea las expresiones
 */
void token_queue_process(ar_token * gtoken, int size)
{
    int i;
    zval valor;
    int error;
    for (i=0; i < size; i++) {
        error = 0;
        printf("processing line %d\n", i);
        token_queue_single_process(&gtoken[i], &valor, &error);
        if (error) {
            printf("Error en la linea %d\n", i);
            continue;
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
