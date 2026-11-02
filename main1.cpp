#include <iostream>
#include <cstdio>
#include <cstdlib> // Para abs()
#include <iomanip> // Para setw() al leer rutas de archivo largas
using namespace std;

const int FILAS = 200;
const int COLS = 200;

// 1. Molde de lo que viene en el archivo binario
struct OrdenArchivo {
    int x = 0;
    int y = 0;
    unsigned int espera = 0;
    bool soltarGranada1 = false;
    bool soltarGranada2 = false;
    bool ataqueKamikaze = false;
    bool aterrizaje = false;
    bool despegue = false;
    int siguientex = 0;
    int siguientey = 0;
};

// 2. Molde de cada casillero en la memoria RAM del dron
struct Orden {
    unsigned int espera = 0;
    bool soltarGranada1 = false;
    bool soltarGranada2 = false;
    bool ataqueKamikaze = false;
    bool aterrizaje = false;
    bool despegue = false;
    int siguientex = 0;
    int siguientey = 0;
    bool ocupado = false;
};

// ============================================================================
// FUNCIONES AUXILIARES Y HERRAMIENTAS BLINDADAS
// ============================================================================

bool sonAledanas(int x1, int y1, int x2, int y2) {
    int distX = abs(x1 - x2);
    int distY = abs(y1 - y2);
    return (distX <= 1 && distY <= 1) && !(distX == 0 && distY == 0);
}

void limpiarEntrada() {
    cin.clear();
    cin.ignore(1000, '\n');
}

int leerEntero(const char *mensaje) {
    int valor;
    cout << mensaje;
    while (!(cin >> valor)) {
        cout << "Entrada invalida. Ingrese un numero entero: ";
        limpiarEntrada();
    }
    return valor;
}

bool leerBool(const char *mensaje) {
    int v;
    do {
        v = leerEntero(mensaje);
        if (v != 0 && v != 1) cout << "Error. Ingrese 0 o 1: " << endl;
    } while (v != 0 && v != 1);
    return v == 1;
}

// ============================================================================
// OPERACION 1: Cargar archivo de ataque en memoria
// ============================================================================
bool cargarArchivoEnMemoria(char rutaArchivo[], Orden memoriaDron[FILAS][COLS]) {
    FILE *f = fopen(rutaArchivo, "rb");
    if (f == NULL) {
        cout << "Error: No se pudo abrir el archivo." << endl;
        return false;
    }

    // Usamos memoria estatica (no Heap) para evitar desbordar la Pila
    static Orden temp[FILAS][COLS];
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            temp[i][j] = Orden();
        }
    }

    OrdenArchivo r;
    int totalRegistros = 0, cantDespegues = 0, cantFin = 0;
    int inicioX = -1, inicioY = -1;

    while (fread(&r, sizeof(OrdenArchivo), 1, f) == 1) {
        totalRegistros++;

        if (r.x < 0 || r.x >= FILAS || r.y < 0 || r.y >= COLS) {
            cout << "Error: Coordenadas fuera del mapa." << endl;
            fclose(f);
            return false;
        }
        if (temp[r.x][r.y].ocupado) {
            cout << "Error: Coordenada repetida." << endl;
            fclose(f);
            return false;
        }
        if (r.despegue) {
            cantDespegues++;
            inicioX = r.x;
            inicioY = r.y;
            if (r.soltarGranada1 || r.soltarGranada2 || r.ataqueKamikaze) {
                cout << "Error: Despegue con ataques simultaneos." << endl;
                fclose(f);
                return false;
            }
        }
        if (r.aterrizaje && r.ataqueKamikaze) {
            cout << "Error: Aterrizaje y kamikaze en la misma celda." << endl;
            fclose(f);
            return false;
        }
        if (r.aterrizaje || r.ataqueKamikaze) cantFin++;

        temp[r.x][r.y].espera = r.espera;
        temp[r.x][r.y].soltarGranada1 = r.soltarGranada1;
        temp[r.x][r.y].soltarGranada2 = r.soltarGranada2;
        temp[r.x][r.y].ataqueKamikaze = r.ataqueKamikaze;
        temp[r.x][r.y].aterrizaje = r.aterrizaje;
        temp[r.x][r.y].despegue = r.despegue;
        temp[r.x][r.y].siguientex = r.siguientex;
        temp[r.x][r.y].siguientey = r.siguientey;
        temp[r.x][r.y].ocupado = true;
    }
    fclose(f);

    if (cantDespegues != 1 || cantFin != 1) {
        cout << "Error: La ruta debe tener 1 despegue y 1 final (aterrizaje o kamikaze)." << endl;
        return false;
    }

    bool visitado[FILAS][COLS] = {false};
    int actualX = inicioX, actualY = inicioY, pasosDados = 0;

    while (true) {
        visitado[actualX][actualY] = true;
        pasosDados++;
        Orden celdaActual = temp[actualX][actualY];

        if (celdaActual.aterrizaje || celdaActual.ataqueKamikaze) break;

        int sigX = celdaActual.siguientex;
        int sigY = celdaActual.siguientey;

        if (sigX < 0 || sigX >= FILAS || sigY < 0 || sigY >= COLS) {
            cout << "Error: El dron intenta salir del mapa." << endl;
            return false;
        }
        if (!sonAledanas(actualX, actualY, sigX, sigY)) {
            cout << "Error: Ruta con salto ilegal. No son aledanas." << endl;
            return false;
        }
        if (!temp[sigX][sigY].ocupado) {
            cout << "Error: Hueco en la ruta." << endl;
            return false;
        }
        if (visitado[sigX][sigY]) {
            cout << "Error: Bucle infinito. El dron pisa una celda visitada." << endl;
            return false;
        }

        actualX = sigX;
        actualY = sigY;
    }

    if (pasosDados != totalRegistros) {
        cout << "Error: Hay ordenes sueltas." << endl;
        return false;
    }

    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            memoriaDron[i][j] = temp[i][j];
        }
    }

    cout << "Archivo validado y cargado en memoria." << endl;
    return true;
}

// ============================================================================
// OPERACION 2: Mostrar ataque cargado
// ============================================================================
void mostrarAtaque(Orden memoriaDron[FILAS][COLS]) {
    int actualX = -1, actualY = -1;

    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (memoriaDron[i][j].despegue) {
                actualX = i; actualY = j; break;
            }
        }
        if (actualX != -1) break;
    }

    if (actualX == -1) {
        cout << "Aviso: No hay ataque cargado." << endl;
        return;
    }

    int numeroPaso = 1, pasosPorPagina = 5, contadorPagina = 0, paginaActual = 1;
    limpiarEntrada();

    cout << "=== REPORTE DE ATAQUE EN MEMORIA (Pag. " << paginaActual << ") ===" << endl;

    int maxPasos = FILAS * COLS;
    int contadorSeguridad = 0;

    while (true) {
        if (actualX < 0 || actualX >= FILAS || actualY < 0 || actualY >= COLS) {
            cout << "\nError fatal: La coordenada apunta fuera del mapa. Abortando lectura." << endl;
            break;
        }

        Orden celda = memoriaDron[actualX][actualY];

        if (!celda.ocupado) {
            cout << "\nError fatal: La ruta apunta a un vacio. Abortando lectura." << endl;
            break;
        }

        cout << "\n[Paso #" << numeroPaso << "] Coordenada (" << actualX << ", " << actualY << "):" << endl;

        if (celda.despegue) cout << "  -> 1. INSTRUCCION: Despegue" << endl;
        if (celda.aterrizaje) cout << "  -> 2. INSTRUCCION: Aterrizaje" << endl;
        if (celda.soltarGranada1) cout << "  -> 3. ACCION: Soltar Granada 1" << endl;
        if (celda.soltarGranada2) cout << "  -> 4. ACCION: Soltar Granada 2" << endl;
        if (celda.ataqueKamikaze) cout << "  -> 5. ACCION: Ataque Kamikaze" << endl;
        if (celda.espera > 0) cout << "  -> 6. ESPERA: " << celda.espera << " unid." << endl;

        if (celda.aterrizaje || celda.ataqueKamikaze) {
            cout << "\n=== FIN DEL RECORRIDO ===" << endl;
            break;
        }

        cout << "  -> 7. MOVER A: (" << celda.siguientex << ", " << celda.siguientey << ")" << endl;

        contadorPagina++;
        if (contadorPagina == pasosPorPagina) {
            cout << "\n--- Presione ENTER para siguiente pagina ---";
            cin.get();
            paginaActual++; contadorPagina = 0;
            cout << "\n=== REPORTE DE ATAQUE (Pag. " << paginaActual << ") ===" << endl;
        }

        actualX = celda.siguientex;
        actualY = celda.siguientey;
        numeroPaso++;

        contadorSeguridad++;
        if(contadorSeguridad > maxPasos) {
            cout << "\nError: Bucle infinito detectado por edicion manual." << endl;
            break;
        }
    }
}

// ============================================================================
// OPERACION 3: Crear un archivo de ataque nuevo
// ============================================================================
void crearArchivoNuevo() {
    char nombreArchivo[260];
    cout << "Ruta y nombre a crear: ";
    cin >> setw(260) >> nombreArchivo;
    limpiarEntrada();

    FILE *f = fopen(nombreArchivo, "wb");
    if (f == NULL) {
        cout << "Error: No se pudo crear el archivo." << endl;
        return;
    }

    int cantidad = 0;
    bool seguir = true;

    while (seguir) {
        OrdenArchivo r;
        cout << "\n--- REGISTRO #" << (cantidad + 1) << " ---" << endl;

        do {
            r.x = leerEntero("X (0 a 199): ");
        } while (r.x < 0 || r.x >= FILAS);

        do {
            r.y = leerEntero("Y (0 a 199): ");
        } while (r.y < 0 || r.y >= COLS);

        int esperaAux;
        do {
            esperaAux = leerEntero("Espera (0 o mas): ");
        } while (esperaAux < 0);
        r.espera = (unsigned int)esperaAux;

        r.soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
        r.soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");
        r.ataqueKamikaze = leerBool("Ataque Kamikaze  (1=Si, 0=No): ");
        r.aterrizaje     = leerBool("Aterrizaje       (1=Si, 0=No): ");
        r.despegue       = leerBool("Despegue         (1=Si, 0=No): ");
        r.siguientex     = leerEntero("Siguiente X: ");
        r.siguientey     = leerEntero("Siguiente Y: ");

        fwrite(&r, sizeof(OrdenArchivo), 1, f);
        cantidad++;

        seguir = leerBool("\nCargar otro? (1=Si, 0=No): ");
    }

    fclose(f);
    cout << "Archivo creado con " << cantidad << " registros." << endl;
}

// ============================================================================
// OPERACION 4: Corregir un registro del archivo
// ============================================================================
void corregirRegistroArchivo() {
    char nombreArchivo[260];
    cout << "Ruta y nombre del archivo a corregir: ";
    cin >> setw(260) >> nombreArchivo;
    limpiarEntrada();

    FILE *f = fopen(nombreArchivo, "rb+");
    if (f == NULL) {
        cout << "Error: No se pudo abrir el archivo." << endl;
        return;
    }

    int buscarX, buscarY;
    do { buscarX = leerEntero("Coordenada X a buscar (0-199): "); } while(buscarX < 0 || buscarX >= FILAS);
    do { buscarY = leerEntero("Coordenada Y a buscar (0-199): "); } while(buscarY < 0 || buscarY >= COLS);

    OrdenArchivo r;
    bool encontrado = false;

    while (fread(&r, sizeof(OrdenArchivo), 1, f) == 1) {
        if (r.x == buscarX && r.y == buscarY) {
            encontrado = true;
            cout << "\n--- DATOS ENCONTRADOS ---" << endl;
            cout << "Espera: " << r.espera << " | Granada1: " << r.soltarGranada1
                 << " | Granada2: " << r.soltarGranada2 << " | Kamikaze: " << r.ataqueKamikaze
                 << " | Aterrizaje: " << r.aterrizaje << " | Despegue: " << r.despegue
                 << " | Siguiente: (" << r.siguientex << "," << r.siguientey << ")" << endl;

            cout << "\n--- NUEVOS DATOS ---" << endl;
            int espAux;
            do { espAux = leerEntero("Nueva espera (0 o mas): "); } while(espAux < 0);
            r.espera = (unsigned int)espAux;

            r.soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
            r.soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");
            r.ataqueKamikaze = leerBool("Ataque Kamikaze  (1=Si, 0=No): ");
            r.aterrizaje     = leerBool("Aterrizaje       (1=Si, 0=No): ");
            r.despegue       = leerBool("Despegue         (1=Si, 0=No): ");
            r.siguientex     = leerEntero("Nuevo Siguiente X: ");
            r.siguientey     = leerEntero("Nuevo Siguiente Y: ");

            fseek(f, - (long)sizeof(OrdenArchivo), SEEK_CUR);
            fwrite(&r, sizeof(OrdenArchivo), 1, f);

            cout << "Registro actualizado en el archivo." << endl;
            break;
        }
    }

    if (!encontrado) cout << "Registro no encontrado en el archivo." << endl;
    fclose(f);
}

// ============================================================================
// OPERACION 5: Corregir un registro en memoria
// ============================================================================
void corregirRegistroMemoria(Orden memoriaDron[FILAS][COLS]) {
    int x, y;
    do { x = leerEntero("Coordenada X (0-199): "); } while(x < 0 || x >= FILAS);
    do { y = leerEntero("Coordenada Y (0-199): "); } while(y < 0 || y >= COLS);

    cout << "\n--- DATOS ACTUALES ---" << endl;
    cout << "Espera: " << memoriaDron[x][y].espera << " | Granada1: " << memoriaDron[x][y].soltarGranada1
         << " | Granada2: " << memoriaDron[x][y].soltarGranada2 << " | Kamikaze: " << memoriaDron[x][y].ataqueKamikaze
         << " | Aterrizaje: " << memoriaDron[x][y].aterrizaje << " | Despegue: " << memoriaDron[x][y].despegue
         << " | Siguiente: (" << memoriaDron[x][y].siguientex << "," << memoriaDron[x][y].siguientey << ")" << endl;

    cout << "\n--- NUEVOS DATOS ---" << endl;
    int espAux;
    do { espAux = leerEntero("Nueva espera (0 o mas): "); } while(espAux < 0);
    memoriaDron[x][y].espera = (unsigned int)espAux;

    memoriaDron[x][y].soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
    memoriaDron[x][y].soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");
    memoriaDron[x][y].ataqueKamikaze = leerBool("Ataque Kamikaze  (1=Si, 0=No): ");
    memoriaDron[x][y].aterrizaje     = leerBool("Aterrizaje       (1=Si, 0=No): ");
    memoriaDron[x][y].despegue       = leerBool("Despegue         (1=Si, 0=No): ");
    memoriaDron[x][y].siguientex     = leerEntero("Nuevo Siguiente X: ");
    memoriaDron[x][y].siguientey     = leerEntero("Nuevo Siguiente Y: ");
    memoriaDron[x][y].ocupado        = true;

    cout << "Registro actualizado en RAM." << endl;
}

// ============================================================================
// OPERACION 6: Guardar memoria en un archivo nuevo
// ============================================================================
void guardarMemoriaEnArchivo(Orden memoriaDron[FILAS][COLS]) {
    char nombreArchivo[260];
    cout << "Ruta y nombre del archivo a crear: ";
    cin >> setw(260) >> nombreArchivo;
    limpiarEntrada();

    FILE *f = fopen(nombreArchivo, "wb");
    if (f == NULL) {
        cout << "Error al crear el archivo." << endl;
        return;
    }

    int guardados = 0;
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (memoriaDron[i][j].ocupado) {
                OrdenArchivo r;
                r.x = i; r.y = j;
                r.espera = memoriaDron[i][j].espera;
                r.soltarGranada1 = memoriaDron[i][j].soltarGranada1;
                r.soltarGranada2 = memoriaDron[i][j].soltarGranada2;
                r.ataqueKamikaze = memoriaDron[i][j].ataqueKamikaze;
                r.aterrizaje = memoriaDron[i][j].aterrizaje;
                r.despegue = memoriaDron[i][j].despegue;
                r.siguientex = memoriaDron[i][j].siguientex;
                r.siguientey = memoriaDron[i][j].siguientey;
                fwrite(&r, sizeof(OrdenArchivo), 1, f);
                guardados++;
            }
        }
    }
    fclose(f);
    cout << "Se guardaron " << guardados << " registros en disco." << endl;
}

// ============================================================================
// OPERACION 7: Visualizar el ataque en HTML
// ============================================================================
void generarHTML(Orden memoriaDron[FILAS][COLS]) {
    char nombreArchivo[260];
    cout << "Nombre del archivo HTML (ej: ataque.html): ";
    cin >> setw(260) >> nombreArchivo;
    limpiarEntrada();

    FILE *f = fopen(nombreArchivo, "w");
    if (f == NULL) {
        cout << "Error al crear archivo HTML." << endl;
        return;
    }

    fprintf(f, "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>Visor de Ataque</title>\n");
    fprintf(f, "<style>\n");
    fprintf(f, "body { background-color: #0f111a; color: #fff; font-family: sans-serif; display: flex; flex-direction: column; align-items: center; margin: 0; padding: 20px; }\n");

    fprintf(f, ".header { background-color: #1e212b; padding: 12px 30px; border-radius: 6px; margin-bottom: 10px; font-size: 15px; letter-spacing: 1px; box-shadow: 0 4px 6px rgba(0,0,0,0.5); border: 1px solid #333; }\n");
    fprintf(f, ".leyenda { font-size: 12px; color: #8892b0; margin-bottom: 25px; }\n");

    // Celdas de 22x20 px para que las letras respiren perfectas
    fprintf(f, "table { border-collapse: collapse; background-color: #050505; box-shadow: 0 0 20px rgba(0,0,0,0.9); }\n");
    fprintf(f, "td { width: 22px; height: 20px; border: 1px solid #222; text-align: center; vertical-align: middle; font-size: 12px; font-weight: bold; }\n");

    // Colores de los casilleros
    fprintf(f, ".D { background: #4caf50; color: #fff; }\n");
    fprintf(f, ".A { background: #2196f3; color: #fff; }\n");
    fprintf(f, ".G1{ background: #ff9800; color: #fff; }\n");
    fprintf(f, ".G2{ background: #ff5722; color: #fff; }\n");
    fprintf(f, ".K { background: #f44336; color: #fff; }\n");
    fprintf(f, ".E { background: #ffeb3b; color: #000; }\n");
    fprintf(f, ".M { background: #9e9e9e; color: #000; }\n");
    fprintf(f, "</style>\n</head>\n<body>\n");

    fprintf(f, "<div class=\"header\">INFORME DE VUELO | MATRIZ 200x200</div>\n");
    fprintf(f, "<div class=\"leyenda\">D: Despegue | A: Aterrizaje | 1/2: Granadas | K: Kamikaze | E: Espera | *: Movimiento</div>\n");

    fprintf(f, "<table>\n");
    for (int i = 0; i < FILAS; i++) {
        fprintf(f, "<tr>\n");
        for (int j = 0; j < COLS; j++) {
            if (memoriaDron[i][j].ocupado) {
                if (memoriaDron[i][j].despegue) fprintf(f, "<td class=\"D\">D</td>");
                else if (memoriaDron[i][j].aterrizaje) fprintf(f, "<td class=\"A\">A</td>");
                else if (memoriaDron[i][j].soltarGranada1) fprintf(f, "<td class=\"G1\">1</td>");
                else if (memoriaDron[i][j].soltarGranada2) fprintf(f, "<td class=\"G2\">2</td>");
                else if (memoriaDron[i][j].ataqueKamikaze) fprintf(f, "<td class=\"K\">K</td>");
                else if (memoriaDron[i][j].espera > 0) fprintf(f, "<td class=\"E\">E</td>");
                else fprintf(f, "<td class=\"M\">*</td>");
            } else {
                fprintf(f, "<td></td>");
            }
        }
        fprintf(f, "</tr>\n");
    }
    fprintf(f, "</table>\n</body>\n</html>\n");

    fclose(f);
    cout << "Archivo HTML generado con exito. Abrelo en tu navegador." << endl;
}
// ============================================================================
// MAIN - MENU PRINCIPAL
// ============================================================================
int main() {
    // Declaramos la memoria principal del dron (matriz de 200x200) inicializada en cero
    Orden memoriaDron[FILAS][COLS] = {};
    int opcion;

    do {
        cout << "\n=== MENU DRON MILITAR ===" << endl;
        cout << "1. Cargar archivo de ataque en memoria" << endl;
        cout << "2. Mostrar ataque cargado" << endl;
        cout << "3. Crear un archivo de ataque nuevo" << endl;
        cout << "4. Corregir un registro del archivo" << endl;
        cout << "5. Corregir un registro en memoria" << endl;
        cout << "6. Guardar memoria en un archivo nuevo" << endl;
        cout << "7. Visualizar un archivo de ataque en HTML" << endl;
        cout << "0. Salir" << endl;

        opcion = leerEntero("Seleccione una operacion: ");

        switch (opcion) {
        case 1: {
                char ruta[260];
                cout << "Ingrese ruta y nombre del archivo a cargar: ";
                cin >> setw(260) >> ruta;
                limpiarEntrada();
                cargarArchivoEnMemoria(ruta, memoriaDron);
                break;
        }
        case 2:
            mostrarAtaque(memoriaDron);
            break;
        case 3:
            crearArchivoNuevo();
            break;
        case 4:
            corregirRegistroArchivo();
            break;
        case 5:
            corregirRegistroMemoria(memoriaDron);
            break;
        case 6:
            guardarMemoriaEnArchivo(memoriaDron);
            break;
        case 7:
            generarHTML(memoriaDron);
            break;
        case 0:
            cout << "Saliendo del sistema..." << endl;
            break;
        default:
            cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);

    return 0;
}
