#include "anlex.h"

typedef struct {
    union {
        long   entero;
        float  flotante;
        int    operation;
    } foo;
} ePila;

int main(int argc,char* args[])
{
	// inicializar analizador lexico
	int complex=0;
    int lastline;

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
			printf("Lin %d: %s -> %d\n",numLinea,t.pe->lexema,t.compLex);
		}
		fclose(archivo);
	}else{
		printf("Debe pasar como parametro el path al archivo fuente.");
		exit(1);
	}

	return 0;
}
