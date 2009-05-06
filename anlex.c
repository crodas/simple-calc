/*
 *    Analizador Léxico    
 *    Curso: Compiladores y Lenguajes de Bajo de Nivel
 *    Práctica de Programación Nro. 1
 *    
 *    Descripcion:
 *    Implementa un analizador léxico que reconoce números, identificadores, 
 *     palabras reservadas, operadores y signos de puntuación para un lenguaje
 *     con sintaxis tipo Pascal.
 *    
 */

#include "anlex.h"
/************* Variables globales **************/

int consumir;            /* 1 indica al analizador lexico que debe devolver
                        el sgte componente lexico, 0 debe devolver el actual */

char cad[5*TAMLEX];        // string utilizado para cargar mensajes de error
token t;                // token global para recibir componentes del Analizador Lexico

// variables para el analizador lexico

FILE *archivo;            // Fuente pascal
char buff[2*TAMBUFF];    // Buffer para lectura de archivo fuente
char id[TAMLEX];        // Utilizado por el analizador lexico
int delantero=-1;        // Utilizado por el analizador lexico
int fin=0;                // Utilizado por el analizador lexico
int numLinea=1;            // Numero de Linea

/************** Prototipos *********************/


void sigLex();        // Del analizador Lexico

/*****/
#ifdef LINUX
int stricmp(const char * str1, const char * str2)
{
    return strcmp(str1, str2);
    int l1, l2, i;
    l1 = strlen(str1);
    l2 = strlen(str2);

    for (i=0; i < l1 && i < l2; i++)
    {
        if (tolower(str1[i]) > tolower(str2[i]))
        {
            return -1;
        } else {
            return 1;
        }
    }
    return 0;
}
#endif

/**************** Funciones **********************/

/*********************HASH************************/
entrada *tabla;                //declarar la tabla de simbolos
int tamTabla=TAMHASH;        //utilizado para cuando se debe hacer rehash
int elems=0;                //utilizado para cuando se debe hacer rehash

int h(char* k, int m)
{
    unsigned h=0,g;
    int i;
    for (i=0;i<strlen(k);i++)
    {
        h=(h << 4) + k[i];
        if (g=h&0xf0000000){
            h=h^(g>>24);
            h=h^g;
        }
    }
    return h%m;
}
void insertar(entrada e);

void initTabla()
{    
    int i=0;
    
    tabla=(entrada*)malloc(tamTabla*sizeof(entrada));
    for(i=0;i<tamTabla;i++)
    {
        tabla[i].compLex=-1;
    }
}

int esprimo(int n)
{
    int i;
    for(i=3;i*i<=n;i+=2)
        if (n%i==0)
            return 0;
    return 1;
}

int siguiente_primo(int n)
{
    if (n%2==0)
        n++;
    for (;!esprimo(n);n+=2);

    return n;
}

//en caso de que la tabla llegue al limite, duplicar el tamaño
void rehash()
{
    entrada *vieja;
    int i;
    vieja=tabla;
    tamTabla=siguiente_primo(2*tamTabla);
    initTabla();
    for (i=0;i<tamTabla/2;i++)
    {
        if(vieja[i].compLex!=-1)
            insertar(vieja[i]);
    }        
    free(vieja);
}

//insertar una entrada en la tabla
void insertar(entrada e)
{
    int pos;
    if (++elems>=tamTabla/2)
        rehash();
    pos=h(e.lexema,tamTabla);
    while (tabla[pos].compLex!=-1)
    {
        pos++;
        if (pos==tamTabla)
            pos=0;
    }
    tabla[pos]=e;

}
//busca una clave en la tabla, si no existe devuelve NULL, posicion en caso contrario
entrada* buscar(char *clave)
{
    int pos;
    entrada *e;
    pos=h(clave,tamTabla);
    while(tabla[pos].compLex!=-1 && stricmp(tabla[pos].lexema,clave)!=0 )
    {
        pos++;
        if (pos==tamTabla)
            pos=0;
    }
    return &tabla[pos];
}

void insertTablaSimbolos(char *s, int n)
{
    entrada e;
    sprintf(e.lexema,s);
    e.compLex=n;
    insertar(e);
}

void initTablaSimbolos()
{
    int i;
    entrada pr,*e;
    char *vector[]={
        "program",
        "type",
        "var",
        "array",
        "begin",
        "end",
        "do",
        "to",
        "downto",
        "then",
        "of",
        "function",
        "procedure", 
        "integer", 
        "real", 
        "boolean", 
        "char", 
        "for", 
        "if", 
        "else", 
        "while", 
        "repeat", 
        "until", 
        "case", 
        "record", 
        "writeln",
        "write",
        "const"
    };
     for (i=0;i<28;i++)
    {
        insertTablaSimbolos(vector[i],i+256);
    }
    insertTablaSimbolos(",",',');
    insertTablaSimbolos(".",'.');
    insertTablaSimbolos(":",':');
    insertTablaSimbolos(";",';');
    insertTablaSimbolos("(",'(');
    insertTablaSimbolos(")",')');
    insertTablaSimbolos("[",'[');
    insertTablaSimbolos("]",']');
    insertTablaSimbolos("true",BOOL);
    insertTablaSimbolos("false",BOOL);
    insertTablaSimbolos("not",NOT);
    insertTablaSimbolos("<",OPREL);
    insertTablaSimbolos("<=",OPREL);
    insertTablaSimbolos("<>",OPREL);
    insertTablaSimbolos(">",OPREL);
    insertTablaSimbolos(">=",OPREL);
    insertTablaSimbolos("=",OPREL);
    insertTablaSimbolos("+",OPSUMA);
    insertTablaSimbolos("-",OPSUMA);
    insertTablaSimbolos("or",OPSUMA);
    insertTablaSimbolos("*",OPMULT);
    insertTablaSimbolos("/",OPMULT);
    insertTablaSimbolos("div",OPMULT);
    insertTablaSimbolos("mod",OPMULT);
    insertTablaSimbolos(":=",OPASIGNA);
}

// Rutinas del analizador lexico

void error(char* mensaje)
{
    printf("Lin %d: Error Lexico. %s.\n",numLinea,mensaje);    
}

void sigLex()
{
    int i=0, longid=0;
    char c=0;
    int acepto=0;
    int estado=0;
    char msg[41];
    entrada e;

    while((c=fgetc(archivo))!=EOF)
    {
        
        if (c==' ' || c=='\t')
            continue;    //eliminar espacios en blanco
        else if(c=='\n')
        {
            //incrementar el numero de linea
            numLinea++;
            continue;
        }
        else if (isalpha(c))
        {
            //es un identificador (o palabra reservada)
            i=0;
            do{
                id[i]=c;
                i++;
                c=fgetc(archivo);
                if (i>=TAMLEX)
                    error("Longitud de Identificador excede tamaño de buffer");
            }while(isalpha(c) || isdigit(c));
            id[i]='\0';
            if (c!=EOF)
                ungetc(c,archivo);
            else
                c=0;
            t.pe=buscar(id);
            t.compLex=t.pe->compLex;
            if (t.pe->compLex==-1)
            {
                sprintf(e.lexema,id);
                e.compLex=ID;
                insertar(e);
                t.pe=buscar(id);
                t.compLex=ID;
            }
            break;
        }
        else if (isdigit(c))
        {
                //es un numero
                i=0;
                estado=0;
                acepto=0;
                id[i]=c;
                
                while(!acepto)
                {
                    switch(estado){
                    case 0: //una secuencia netamente de digitos, puede ocurrir . o e
                        c=fgetc(archivo);
                        if (isdigit(c))
                        {
                            id[++i]=c;
                            estado=0;
                        }
                        else if(c=='.'){
                            id[++i]=c;
                            estado=1;
                        }
                        else if(tolower(c)=='e'){
                            id[++i]=c;
                            estado=3;
                        }
                        else{
                            estado=6;
                        }
                        break;
                    
                    case 1://un punto, debe seguir un digito (caso especial de array, puede venir otro punto)
                        c=fgetc(archivo);                        
                        if (isdigit(c))
                        {
                            id[++i]=c;
                            estado=2;
                        }
                        else if(c=='.')
                        {
                            i--;
                            fseek(archivo,-1,SEEK_CUR);
                            estado=6;
                        }
                        else{
                            sprintf(msg,"No se esperaba '%c'",c);
                            estado=-1;
                        }
                        break;
                    case 2://la fraccion decimal, pueden seguir los digitos o e
                        c=fgetc(archivo);
                        if (isdigit(c))
                        {
                            id[++i]=c;
                            estado=2;
                        }
                        else if(tolower(c)=='e')
                        {
                            id[++i]=c;
                            estado=3;
                        }
                        else
                            estado=6;
                        break;
                    case 3://una e, puede seguir +, - o una secuencia de digitos
                        c=fgetc(archivo);
                        if (c=='+' || c=='-')
                        {
                            id[++i]=c;
                            estado=4;
                        }
                        else if(isdigit(c))
                        {
                            id[++i]=c;
                            estado=5;
                        }
                        else{
                            sprintf(msg,"No se esperaba '%c'",c);
                            estado=-1;
                        }
                        break;
                    case 4://necesariamente debe venir por lo menos un digito
                        c=fgetc(archivo);
                        if (isdigit(c))
                        {
                            id[++i]=c;
                            estado=5;
                        }
                        else{
                            sprintf(msg,"No se esperaba '%c'",c);
                            estado=-1;
                        }
                        break;
                    case 5://una secuencia de digitos correspondiente al exponente
                        c=fgetc(archivo);
                        if (isdigit(c))
                        {
                            id[++i]=c;
                            estado=5;
                        }
                        else{
                            estado=6;
                        }break;
                    case 6://estado de aceptacion, devolver el caracter correspondiente a otro componente lexico
                        if (c!=EOF)
                            ungetc(c,archivo);
                        else
                            c=0;
                        id[++i]='\0';
                        acepto=1;
                        t.pe=buscar(id);
                        if (t.pe->compLex==-1)
                        {
                            sprintf(e.lexema,id);
                            e.compLex=NUM;
                            insertar(e);
                            t.pe=buscar(id);
                        }
                        t.compLex=NUM;
                        break;
                    case -1:
                        if (c==EOF)
                            error("No se esperaba el fin de archivo");
                        else
                            error(msg);
                        exit(1);
                    }
                }
            break;
        }
        else if (c=='<') 
        {
            //es un operador relacional, averiguar cual
            c=fgetc(archivo);
            if (c=='>'){
                t.compLex=OPREL;
                t.pe=buscar("<>");
            }
            else if (c=='='){
                t.compLex=OPREL;
                t.pe=buscar("<=");
            }
            else{
                ungetc(c,archivo);
                t.compLex=OPREL;
                t.pe=buscar("<");
            }
            break;
        }
        else if (c=='>')
        {
            //es un operador relacional, averiguar cual
                c=fgetc(archivo);
            if (c=='='){
                t.compLex=OPREL;
                t.pe=buscar(">=");
            }
            else{
                ungetc(c,archivo);
                t.compLex=OPREL;
                t.pe=buscar(">");
            }
            break;
        }
        else if (c==':')
        {
            //puede ser un : o un operador de asignacion
            c=fgetc(archivo);
            if (c=='='){
                t.compLex=OPASIGNA;
                t.pe=buscar(":=");
            }
            else{
                ungetc(c,archivo);
                t.compLex=':';
                t.pe=buscar(":");
            }
            break;
        }
        else if (c=='+')
        {
            t.compLex=OPSUMA;
            t.pe=buscar("+");
            break;
        }
        else if (c=='-')
        {
            t.compLex=OPSUMA;
            t.pe=buscar("-");
            break;
        }
        else if (c=='*')
        {
            t.compLex=OPMULT;
            t.pe=buscar("*");
            break;
        }
        else if (c=='/')
        {
            t.compLex=OPMULT;
            t.pe=buscar("/");
            break;
        }
        else if (c=='=')
        {
            t.compLex=OPREL;
            t.pe=buscar("=");
            break;
        }
        else if (c==',')
        {
            t.compLex=',';
            t.pe=buscar(",");
            break;
        }
        else if (c==';')
        {
            t.compLex=';';
            t.pe=buscar(";");
            break;
        }
        else if (c=='.')
        {
            t.compLex='.';
            t.pe=buscar(".");
            break;
        }
        else if (c=='(')
        {
            if ((c=fgetc(archivo))=='*')
            {//es un comentario
                while(c!=EOF)
                {
                    c=fgetc(archivo);
                    if (c=='*')
                    {
                        if ((c=fgetc(archivo))==')')
                        {
                            break;
                        }
                        ungetc(c,archivo);
                    }
                }
                if (c==EOF)
                    error("Se llego al fin de archivo sin finalizar un comentario");
                continue;
            }
            else
            {
                ungetc(c,archivo);
                t.compLex='(';
                t.pe=buscar("(");
            }
            break;
        }
        else if (c==')')
        {
            t.compLex=')';
            t.pe=buscar(")");
            break;
        }
        else if (c=='[')
        {
            t.compLex='[';
            t.pe=buscar("[");
            break;
        }
        else if (c==']')
        {
            t.compLex=']';
            t.pe=buscar("]");
            break;
        }
        else if (c=='\'')
        {//un caracter o una cadena de caracteres
            i=0;
            id[i]=c;
            i++;
            do{
                c=fgetc(archivo);
                if (c=='\'')
                {
                    c=fgetc(archivo);
                    if (c=='\'')
                    {
                        id[i]=c;
                        i++;
                        id[i]=c;
                        i++;
                    }
                    else
                    {
                        id[i]='\'';
                        i++;
                        break;
                    }
                }
                else if(c==EOF)
                {
                    error("Se llego al fin de archivo sin finalizar un literal");
                }
                else{
                    id[i]=c;
                    i++;
                }
            }while(isascii(c));
            id[i]='\0';
            if (c!=EOF)
                ungetc(c,archivo);
            else
                c=0;
            t.pe=buscar(id);
            t.compLex=t.pe->compLex;
            if (t.pe->compLex==-1)
            {
                sprintf(e.lexema,id);
                if (strlen(id)==3 || strcmp(id,"''''")==0)
                    e.compLex=CAR;
                else
                    e.compLex=LITERAL;
                insertar(e);
                t.pe=buscar(id);
                t.compLex=e.compLex;
            }
            break;
        }
        else if (c=='{')
        {
            //elimina el comentario
            while(c!=EOF)
            {
                c=fgetc(archivo);
                if (c=='}')
                    break;
            }
            if (c==EOF)
                error("Se llego al fin de archivo sin finalizar un comentario");
        }
        else if (c!=EOF)
        {
            sprintf(msg,"%c no esperado",c);
            error(msg);
        }
    }
    if (c==EOF)
    {
        t.compLex=EOF;
        sprintf(e.lexema,"EOF");
        t.pe=&e;
    }
    
}


