#include <stdio.h>
#define FILAS 20
#define COLUMNAS 20

//operación 7
void generarHTML(const char *nombreArchivo, char tabla[FILAS][COLUMNAS]) {
    FILE *archivo;
    int fila, columna;
    archivo = fopen(nombreArchivo, "w");
    if (archivo == NULL)
    {
        printf("No se pudo crear el archivo.\n");
        return;
    }
    fprintf(archivo, "<html>\n");
    fprintf(archivo, "<head><title>Ataque</title></head>\n");
    fprintf(archivo, "<body>\n");
    fprintf(archivo, "<table border=\"1\">\n");
    for (fila = 0; fila < FILAS; fila++)
    {
        fprintf(archivo, "<tr>\n");
        for (columna = 0; columna < COLUMNAS; columna++)
        {
            fprintf(archivo, "<td>[%c]</td>", tabla[fila][columna]);
        }
        fprintf(archivo, "</tr>\n");
    }
    fprintf(archivo,"</table>\n");
    fprintf(archivo, "</body>\n");
    fprintf(archivo, "</html>\n");

    fclose(archivo);
}

int main() {
    const char *nombreArchivo = "prueba.html";
    char tabla[FILAS][COLUMNAS];
    char caracter = ' ';
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            tabla[i][j] = caracter;
        }
    }
    generarHTML(nombreArchivo, tabla);
    return 0;
}



/*struct Orden
{
unsigned int espera = 0;
bool soltarGranada1 = false;
bool soltarGranada2 = false;
bool ataqueKamikaze = false;
bool aterrizaje = false;
bool despegue = false;
int siguientex =0;
int siguientey= 0;
}

struct OrdenArchivo
{
 int x =0;
 int y =0;
 unsigned int espera = 0;
 bool soltarGranada1 = false;
 bool soltarGranada2 = false;
 bool ataqueKamikaze = false;
 bool aterrizaje = false;
 bool despegue = false;
 int siguientex =0;
 int siguientey= 0;
}*/