#include "calc.h"

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
                zval_mul(&result, num1, num2);
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
