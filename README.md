#include <iostream>
#include <cstdio>
#include <cstdlib> // Para abs()
using namespace std;

const int FILAS = 200;
const int COLS = 200;

// 1. Molde de cada registro en el archivo binario (.dat)
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

// 2. Molde de cada casillero en la memoria RAM del dron (Matriz 200x200)
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
// FUNCIONES AUXILIARES BLINDADAS
// ============================================================================

// Verifica que la distancia sea de maximo 1 casillero y que NO sea la misma celda
bool sonAledanas(int x1, int y1, int x2, int y2) {
    int distX = abs(x1 - x2);
    int distY = abs(y1 - y2);
    return (distX <= 1 && distY <= 1) && !(distX == 0 && distY == 0);
}

// Verifica que una coordenada este dentro de los limites de la matriz (0 a 199)
bool dentroDelMapa(int x, int y) {
    return (x >= 0 && x < FILAS && y >= 0 && y < COLS);
}

// Lectura de enteros a prueba de letras o simbolos que limpia el buffer
int leerEntero(const char *mensaje) {
    int valor;
    cout << mensaje;
    while (!(cin >> valor)) {
        cout << "  [Error] Entrada invalida. Ingrese un numero entero: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }
    cin.ignore(10000, '\n');
    return valor;
}

// Lectura de enteros validando un rango minimo y maximo
int leerEnteroRango(const char *mensaje, int minVal, int maxVal) {
    int valor;
    do {
        valor = leerEntero(mensaje);
        if (valor < minVal || valor > maxVal) {
            cout << "  [Error] El valor debe estar entre " << minVal << " y " << maxVal << "." << endl;
        }
    } while (valor < minVal || valor > maxVal);
    return valor;
}

// Lectura de tiempo de espera validando que no sea negativo
unsigned int leerEspera(const char *mensaje) {
    int valor;
    do {
        valor = leerEntero(mensaje);
        if (valor < 0) {
            cout << "  [Error] El tiempo de espera no puede ser negativo (ingrese 0 o mas)." << endl;
        }
    } while (valor < 0);
    return (unsigned int)valor;
}

// Lectura estricta para valores booleanos (solo acepta 0 o 1)
bool leerBool(const char *mensaje) {
    return leerEnteroRango(mensaje, 0, 1) == 1;
}

// Lee nombres o rutas de archivo evitando entradas vacias o desbordes
void leerRuta(const char *mensaje, char ruta[260]) {
    do {
        cout << mensaje;
        cin.getline(ruta, 260);
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            ruta[0] = '\0';
            cout << "  [Error] La ruta excede el limite de caracteres." << endl;
        } else if (ruta[0] == '\0') {
            cout << "  [Error] El nombre del archivo no puede estar vacio." << endl;
        }
    } while (ruta[0] == '\0');
}

// Pide las coordenadas siguientes validando que sean vecinas y dentro del mapa
void pedirSiguienteValido(int actualX, int actualY, int &sigX, int &sigY) {
    bool valido = false;
    do {
        sigX = leerEnteroRango("Siguiente X (0 a 199): ", 0, FILAS - 1);
        sigY = leerEnteroRango("Siguiente Y (0 a 199): ", 0, COLS - 1);

        if (!sonAledanas(actualX, actualY, sigX, sigY)) {
            cout << "  [Error] Movimiento invalido: Debe ser una celda vecina a (" << actualX << "," << actualY << ")." << endl;
        } else {
            valido = true;
        }
    } while (!valido);
}

// Verifica si existe al menos una celda vecina libre alrededor de (x, y)
bool tieneVecinaLibre(int x, int y, bool usado[FILAS][COLS]) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (dentroDelMapa(nx, ny) && !usado[nx][ny]) {
                return true;
            }
        }
    }
    return false;
}

// Verifica si la memoria RAM del dron tiene una mision cargada
bool hayAtaqueEnMemoria(Orden memoriaDron[FILAS][COLS]) {
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (memoriaDron[i][j].ocupado) return true;
        }
    }
    return false;
}

// ============================================================================
// OPERACION 1: Cargar archivo de ataque en memoria
// ============================================================================
bool cargarArchivoEnMemoria(char rutaArchivo[], Orden memoriaDron[FILAS][COLS]) {
    FILE *f = fopen(rutaArchivo, "rb");
    if (f == NULL) {
        cout << "Error: No se pudo abrir el archivo '" << rutaArchivo << "'." << endl;
        return false;
    }

    // Matriz temporal estatica para no agotar la Pila (Stack) ni alterar la RAM si el archivo falla
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

        if (!dentroDelMapa(r.x, r.y)) {
            cout << "Error de validacion: Coordenadas fuera del mapa." << endl;
            fclose(f);
            return false;
        }
        if (temp[r.x][r.y].ocupado) {
            cout << "Error de validacion: Coordenada (" << r.x << "," << r.y << ") repetida en el archivo." << endl;
            fclose(f);
            return false;
        }
        if (r.despegue) {
            cantDespegues++;
            inicioX = r.x;
            inicioY = r.y;
            if (r.soltarGranada1 || r.soltarGranada2 || r.ataqueKamikaze || r.aterrizaje) {
                cout << "Error de validacion: El despegue no puede tener ataques ni aterrizaje simultaneo." << endl;
                fclose(f);
                return false;
            }
        }
        if (r.aterrizaje && r.ataqueKamikaze) {
            cout << "Error de validacion: Aterrizaje y kamikaze simultaneos en (" << r.x << "," << r.y << ")." << endl;
            fclose(f);
            return false;
        }
        if (r.aterrizaje || r.ataqueKamikaze) {
            cantFin++;
        }

        // Volcado a la matriz temporal
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

    if (totalRegistros == 0) {
        cout << "Error de validacion: El archivo esta vacio." << endl;
        return false;
    }

    if (cantDespegues != 1 || cantFin != 1) {
        cout << "Error de validacion: La ruta debe tener exactamente 1 despegue y 1 final (aterrizaje o kamikaze)." << endl;
        return false;
    }

    // Seguimiento de la ruta paso a paso para verificar conexion continua y ausencia de bucles
    static bool visitado[FILAS][COLS];
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            visitado[i][j] = false;
        }
    }

    int actualX = inicioX, actualY = inicioY, pasosDados = 0;

    while (true) {
        visitado[actualX][actualY] = true;
        pasosDados++;
        Orden celdaActual = temp[actualX][actualY];

        if (celdaActual.aterrizaje || celdaActual.ataqueKamikaze) break;

        int sigX = celdaActual.siguientex;
        int sigY = celdaActual.siguientey;

        if (!dentroDelMapa(sigX, sigY)) {
            cout << "Error de ruta: El dron intenta salir del mapa desde (" << actualX << "," << actualY << ")." << endl;
            return false;
        }
        if (!sonAledanas(actualX, actualY, sigX, sigY)) {
            cout << "Error de ruta: Salto ilegal entre (" << actualX << "," << actualY << ") y (" << sigX << "," << sigY << "). No son aledanas." << endl;
            return false;
        }
        if (!temp[sigX][sigY].ocupado) {
            cout << "Error de ruta: Hueco detectado. La celda (" << sigX << "," << sigY << ") no existe." << endl;
            return false;
        }
        if (visitado[sigX][sigY]) {
            cout << "Error de ruta: Bucle infinito. El dron vuelve a pisar la celda (" << sigX << "," << sigY << ")." << endl;
            return false;
        }

        actualX = sigX;
        actualY = sigY;
    }

    if (pasosDados != totalRegistros) {
        cout << "Error de validacion: Hay ordenes sueltas en el archivo que no forman parte del camino." << endl;
        return false;
    }

    // Copia definitiva a la memoria RAM oficial del dron
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            memoriaDron[i][j] = temp[i][j];
        }
    }

    cout << "Exito: Archivo validado y cargado en memoria correctamente (" << totalRegistros << " pasos)." << endl;
    return true;
}

// ============================================================================
// OPERACION 2: Mostrar ataque cargado
// ============================================================================
void mostrarAtaque(Orden memoriaDron[FILAS][COLS]) {
    int actualX = -1, actualY = -1;

    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (memoriaDron[i][j].ocupado && memoriaDron[i][j].despegue) {
                actualX = i;
                actualY = j;
                break;
            }
        }
        if (actualX != -1) break;
    }

    if (actualX == -1) {
        cout << "Aviso: No hay ningun ataque cargado en memoria." << endl;
        return;
    }

    static bool visitado[FILAS][COLS];
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            visitado[i][j] = false;
        }
    }

    int numeroPaso = 1, pasosPorPagina = 5, contadorPagina = 0, paginaActual = 1;

    cout << "\n=== REPORTE DE ATAQUE EN MEMORIA (Pag. " << paginaActual << ") ===" << endl;

    while (true) {
        if (!dentroDelMapa(actualX, actualY)) {
            cout << "\nError fatal: La ruta apunta fuera del mapa (" << actualX << "," << actualY << "). Abortando lectura." << endl;
            break;
        }

        Orden celda = memoriaDron[actualX][actualY];

        if (!celda.ocupado) {
            cout << "\nError fatal: La ruta apunta a una celda vacia (" << actualX << "," << actualY << "). Abortando lectura." << endl;
            break;
        }

        if (visitado[actualX][actualY]) {
            cout << "\nError fatal: Bucle detectado en memoria al repetir la celda (" << actualX << "," << actualY << "). Abortando lectura." << endl;
            break;
        }
        visitado[actualX][actualY] = true;

        cout << "\n[Paso #" << numeroPaso << "] Coordenada (" << actualX << ", " << actualY << "):" << endl;

        if (celda.despegue)       cout << "  -> 1. INSTRUCCION: Despegue" << endl;
        if (celda.aterrizaje)     cout << "  -> 2. INSTRUCCION: Aterrizaje" << endl;
        if (celda.soltarGranada1) cout << "  -> 3. ACCION: Soltar Granada 1" << endl;
        if (celda.soltarGranada2) cout << "  -> 4. ACCION: Soltar Granada 2" << endl;
        if (celda.ataqueKamikaze) cout << "  -> 5. ACCION: Ataque Kamikaze" << endl;
        if (celda.espera > 0)     cout << "  -> 6. ESPERA: " << celda.espera << " unid." << endl;

        if (celda.aterrizaje || celda.ataqueKamikaze) {
            cout << "\n=== FIN DEL RECORRIDO ===" << endl;
            break;
        }

        if (!sonAledanas(actualX, actualY, celda.siguientex, celda.siguientey)) {
            cout << "\nError fatal: El siguiente paso (" << celda.siguientex << "," << celda.siguientey << ") no es una celda aledana." << endl;
            break;
        }

        cout << "  -> 7. MOVER A: (" << celda.siguientex << ", " << celda.siguientey << ")" << endl;

        contadorPagina++;
        if (contadorPagina == pasosPorPagina) {
            cout << "\n--- Presione ENTER para ver la siguiente pagina ---";
            cin.get();
            paginaActual++;
            contadorPagina = 0;
            cout << "\n=== REPORTE DE ATAQUE EN MEMORIA (Pag. " << paginaActual << ") ===" << endl;
        }

        actualX = celda.siguientex;
        actualY = celda.siguientey;
        numeroPaso++;
    }
}

// ============================================================================
// OPERACION 3: Crear un archivo de ataque nuevo (Guiado, sin huecos ni bucles)
// ============================================================================
void crearArchivoNuevo() {
    char nombreArchivo[260];
    leerRuta("Ruta y nombre del archivo a crear (ej: ataque.dat): ", nombreArchivo);

    FILE *f = fopen(nombreArchivo, "wb");
    if (f == NULL) {
        cout << "Error: No se pudo crear el archivo." << endl;
        return;
    }

    // Registra las celdas ya usadas en esta creacion para impedir coordenadas repetidas o bucles
    static bool usadoEnCreacion[FILAS][COLS];
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            usadoEnCreacion[i][j] = false;
        }
    }

    int cantidad = 0;
    bool seguir = true;
    int proxX = 0, proxY = 0;

    while (seguir) {
        OrdenArchivo r;
        cout << "\n--- REGISTRO #" << (cantidad + 1) << " ---" << endl;

        if (cantidad == 0) {
            cout << "[!] Primer registro: Se establece DESPEGUE automaticamente (Sin ataques)." << endl;
            r.x = leerEnteroRango("Coordenada inicial X (0 a 199): ", 0, FILAS - 1);
            r.y = leerEnteroRango("Coordenada inicial Y (0 a 199): ", 0, COLS - 1);
            usadoEnCreacion[r.x][r.y] = true;

            r.despegue = true;
            r.aterrizaje = false;
            r.ataqueKamikaze = false;
            r.soltarGranada1 = false;
            r.soltarGranada2 = false;
            r.espera = leerEspera("Espera (0 o mas): ");

            do {
                pedirSiguienteValido(r.x, r.y, r.siguientex, r.siguientey);
                if (usadoEnCreacion[r.siguientex][r.siguientey]) {
                    cout << "  [Error] Esa celda ya fue visitada. Elija otra para no crear un bucle." << endl;
                }
            } while (usadoEnCreacion[r.siguientex][r.siguientey]);

            proxX = r.siguientex;
            proxY = r.siguientey;
        } else {
            r.x = proxX;
            r.y = proxY;
            usadoEnCreacion[r.x][r.y] = true;
            cout << "[!] Posicion actual del dron fijada en: (" << r.x << ", " << r.y << ")" << endl;

            r.despegue = false;

            // Si el dron se encerro solo y no tiene celdas vecinas libres, debe finalizar aca
            if (!tieneVecinaLibre(r.x, r.y, usadoEnCreacion)) {
                cout << "[!] Aviso: No quedan celdas vecinas libres sin visitar. Debe finalizar la mision aqui." << endl;
                r.ataqueKamikaze = leerBool("Finalizar con Ataque Kamikaze? (1=Kamikaze, 0=Aterrizaje): ");
                r.aterrizaje = !r.ataqueKamikaze;
            } else {
                r.ataqueKamikaze = leerBool("Ataque Kamikaze  (1=Si, 0=No): ");
                if (!r.ataqueKamikaze) {
                    r.aterrizaje = leerBool("Aterrizaje       (1=Si, 0=No): ");
                } else {
                    r.aterrizaje = false;
                }
            }

            r.espera = leerEspera("Espera (0 o mas): ");
            r.soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
            r.soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");

            if (r.ataqueKamikaze || r.aterrizaje) {
                r.siguientex = 0;
                r.siguientey = 0;
                seguir = false;
                cout << "[!] Mision finalizada (Aterrizaje/Kamikaze registrado). Cerrando archivo..." << endl;
            } else {
                do {
                    pedirSiguienteValido(r.x, r.y, r.siguientex, r.siguientey);
                    if (usadoEnCreacion[r.siguientex][r.siguientey]) {
                        cout << "  [Error] La celda (" << r.siguientex << "," << r.siguientey << ") ya fue visitada. Elija otra." << endl;
                    }
                } while (usadoEnCreacion[r.siguientex][r.siguientey]);

                proxX = r.siguientex;
                proxY = r.siguientey;
                cout << "[!] El dron avanza hacia (" << proxX << ", " << proxY << ")." << endl;
            }
        }

        fwrite(&r, sizeof(OrdenArchivo), 1, f);
        cantidad++;
    }

    fclose(f);
    cout << "Exito: Archivo creado con " << cantidad << " registros perfectamente conectados." << endl;
}

// ============================================================================
// OPERACION 4: Corregir un registro del archivo
// ============================================================================
void corregirRegistroArchivo() {
    char nombreArchivo[260];
    leerRuta("Ruta y nombre del archivo a corregir: ", nombreArchivo);

    FILE *f = fopen(nombreArchivo, "rb+");
    if (f == NULL) {
        cout << "Error: No se pudo abrir el archivo." << endl;
        return;
    }

    int buscarX = leerEnteroRango("Coordenada X a buscar (0-199): ", 0, FILAS - 1);
    int buscarY = leerEnteroRango("Coordenada Y a buscar (0-199): ", 0, COLS - 1);

    OrdenArchivo r;
    bool encontrado = false;

    while (fread(&r, sizeof(OrdenArchivo), 1, f) == 1) {
        if (r.x == buscarX && r.y == buscarY) {
            encontrado = true;
            cout << "\n--- DATOS ENCONTRADOS EN (" << r.x << ", " << r.y << ") ---" << endl;
            cout << "Espera: " << r.espera << " | Granada1: " << r.soltarGranada1
                 << " | Granada2: " << r.soltarGranada2 << " | Kamikaze: " << r.ataqueKamikaze
                 << " | Aterrizaje: " << r.aterrizaje << " | Despegue: " << r.despegue
                 << " | Siguiente: (" << r.siguientex << "," << r.siguientey << ")" << endl;

            cout << "\n--- INGRESE NUEVOS DATOS ---" << endl;
            r.espera = leerEspera("Nueva espera (0 o mas): ");

            if (r.despegue) {
                cout << "[!] Celda de Despegue: No se permiten ataques ni aterrizaje aqui." << endl;
                r.soltarGranada1 = false;
                r.soltarGranada2 = false;
            } else {
                r.soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
                r.soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");
            }

            // Si es la celda final, permite alternar entre Kamikaze y Aterrizaje sin perder el fin de ruta
            if (r.aterrizaje || r.ataqueKamikaze) {
                cout << "[!] Esta es la celda de FIN DE MISION." << endl;
                r.ataqueKamikaze = leerBool("Modo de fin (1=Ataque Kamikaze, 0=Aterrizaje): ");
                r.aterrizaje = !r.ataqueKamikaze;
                r.siguientex = 0;
                r.siguientey = 0;
            } else {
                cout << "Siguiente actual: (" << r.siguientex << ", " << r.siguientey << ")" << endl;
                if (leerBool("Desea cambiar la coordenada siguiente? (1=Si, 0=No): ")) {
                    pedirSiguienteValido(r.x, r.y, r.siguientex, r.siguientey);
                }
            }

            fseek(f, -(long)sizeof(OrdenArchivo), SEEK_CUR);
            fwrite(&r, sizeof(OrdenArchivo), 1, f);
            fflush(f);

            cout << "Exito: Registro actualizado en el disco duro." << endl;
            cout << "AVISO: Para ver este cambio reflejado en el sistema, vuelva a cargar el archivo (Opcion 1)." << endl;
            break;
        }
    }

    if (!encontrado) {
        cout << "Aviso: La coordenada (" << buscarX << ", " << buscarY << ") no existe en el archivo." << endl;
    }
    fclose(f);
}

// ============================================================================
// OPERACION 5: Corregir un registro en memoria
// ============================================================================
void corregirRegistroMemoria(Orden memoriaDron[FILAS][COLS]) {
    if (!hayAtaqueEnMemoria(memoriaDron)) {
        cout << "Aviso: No hay ningun ataque cargado en memoria para modificar." << endl;
        return;
    }

    int x = leerEnteroRango("Coordenada X a modificar (0-199): ", 0, FILAS - 1);
    int y = leerEnteroRango("Coordenada Y a modificar (0-199): ", 0, COLS - 1);

    if (!memoriaDron[x][y].ocupado) {
        cout << "Error: La coordenada (" << x << ", " << y << ") esta vacia. No pertenece a la ruta activa." << endl;
        return;
    }

    cout << "\n--- DATOS ACTUALES EN RAM (" << x << ", " << y << ") ---" << endl;
    cout << "Espera: " << memoriaDron[x][y].espera << " | Granada1: " << memoriaDron[x][y].soltarGranada1
         << " | Granada2: " << memoriaDron[x][y].soltarGranada2 << " | Kamikaze: " << memoriaDron[x][y].ataqueKamikaze
         << " | Aterrizaje: " << memoriaDron[x][y].aterrizaje << " | Despegue: " << memoriaDron[x][y].despegue
         << " | Siguiente: (" << memoriaDron[x][y].siguientex << "," << memoriaDron[x][y].siguientey << ")" << endl;

    cout << "\n--- INGRESE NUEVOS DATOS ---" << endl;
    memoriaDron[x][y].espera = leerEspera("Nueva espera (0 o mas): ");

    if (memoriaDron[x][y].despegue) {
        cout << "[!] Celda de Despegue: No se permiten ataques ni aterrizaje aqui." << endl;
        memoriaDron[x][y].soltarGranada1 = false;
        memoriaDron[x][y].soltarGranada2 = false;
    } else {
        memoriaDron[x][y].soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
        memoriaDron[x][y].soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");
    }

    if (memoriaDron[x][y].aterrizaje || memoriaDron[x][y].ataqueKamikaze) {
        cout << "[!] Esta es la celda de FIN DE MISION." << endl;
        memoriaDron[x][y].ataqueKamikaze = leerBool("Modo de fin (1=Ataque Kamikaze, 0=Aterrizaje): ");
        memoriaDron[x][y].aterrizaje = !memoriaDron[x][y].ataqueKamikaze;
        memoriaDron[x][y].siguientex = 0;
        memoriaDron[x][y].siguientey = 0;
    } else {
        cout << "Siguiente paso actual: (" << memoriaDron[x][y].siguientex << ", " << memoriaDron[x][y].siguientey << ")" << endl;
        if (leerBool("Desea redirigir el siguiente paso a otra celda activa? (1=Si, 0=No): ")) {
            int nuevoSigX, nuevoSigY;
            bool pasoValido = false;
            do {
                pedirSiguienteValido(x, y, nuevoSigX, nuevoSigY);
                if (!memoriaDron[nuevoSigX][nuevoSigY].ocupado) {
                    cout << "  [Error] La celda (" << nuevoSigX << "," << nuevoSigY << ") esta vacia en RAM. Debe apuntar a una celda ocupada." << endl;
                } else if (memoriaDron[nuevoSigX][nuevoSigY].despegue) {
                    cout << "  [Error] No puede volver a apuntar a la celda de Despegue." << endl;
                } else {
                    pasoValido = true;
                }
            } while (!pasoValido);

            memoriaDron[x][y].siguientex = nuevoSigX;
            memoriaDron[x][y].siguientey = nuevoSigY;
        }
    }

    cout << "Exito: Registro actualizado en memoria RAM." << endl;
}

// ============================================================================
// OPERACION 6: Guardar memoria en un archivo nuevo
// ============================================================================
void guardarMemoriaEnArchivo(Orden memoriaDron[FILAS][COLS]) {
    if (!hayAtaqueEnMemoria(memoriaDron)) {
        cout << "Aviso: La memoria esta vacia. Cargue un ataque antes de guardar." << endl;
        return;
    }

    char nombreArchivo[260];
    leerRuta("Ruta y nombre del archivo binario a crear (ej: backup.dat): ", nombreArchivo);

    FILE *f = fopen(nombreArchivo, "wb");
    if (f == NULL) {
        cout << "Error: No se pudo crear el archivo." << endl;
        return;
    }

    int guardados = 0;
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (memoriaDron[i][j].ocupado) {
                OrdenArchivo r;
                r.x = i;
                r.y = j;
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
    cout << "Exito: Se guardaron " << guardados << " registros en '" << nombreArchivo << "'." << endl;
}

// ============================================================================
// OPERACION 7: Visualizar el ataque en HTML
// ============================================================================
void generarHTML(Orden memoriaDron[FILAS][COLS]) {
    if (!hayAtaqueEnMemoria(memoriaDron)) {
        cout << "Aviso: No hay ningun ataque cargado en memoria para visualizar." << endl;
        return;
    }

    char nombreArchivo[260];
    leerRuta("Nombre del archivo HTML a generar (ej: ataque.html): ", nombreArchivo);

    FILE *f = fopen(nombreArchivo, "w");
    if (f == NULL) {
        cout << "Error al crear el archivo HTML." << endl;
        return;
    }

    fprintf(f, "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>Visor de Ataque</title>\n");
    fprintf(f, "<style>\n");
    fprintf(f, "body { background-color: #0f111a; color: #fff; font-family: sans-serif; display: flex; flex-direction: column; align-items: center; margin: 0; padding: 20px; }\n");
    fprintf(f, ".header { background-color: #1e212b; padding: 12px 30px; border-radius: 6px; margin-bottom: 10px; font-size: 15px; letter-spacing: 1px; box-shadow: 0 4px 6px rgba(0,0,0,0.5); border: 1px solid #333; }\n");
    fprintf(f, ".leyenda { font-size: 12px; color: #8892b0; margin-bottom: 25px; }\n");
    fprintf(f, "table { border-collapse: collapse; background-color: #050505; box-shadow: 0 0 20px rgba(0,0,0,0.9); }\n");
    fprintf(f, "td { width: 22px; height: 20px; border: 1px solid #222; text-align: center; vertical-align: middle; font-size: 12px; font-weight: bold; cursor: crosshair; }\n");
    fprintf(f, "td:hover { background-color: #333; border-color: #666; }\n");
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
                // Orden de precedencia estricto
                if (memoriaDron[i][j].despegue)            fprintf(f, "<td class=\"D\">D</td>");
                else if (memoriaDron[i][j].aterrizaje)     fprintf(f, "<td class=\"A\">A</td>");
                else if (memoriaDron[i][j].soltarGranada1) fprintf(f, "<td class=\"G1\">1</td>");
                else if (memoriaDron[i][j].soltarGranada2) fprintf(f, "<td class=\"G2\">2</td>");
                else if (memoriaDron[i][j].ataqueKamikaze) fprintf(f, "<td class=\"K\">K</td>");
                else if (memoriaDron[i][j].espera > 0)     fprintf(f, "<td class=\"E\">E</td>");
                else                                       fprintf(f, "<td class=\"M\">*</td>");
            } else {
                fprintf(f, "<td></td>");
            }
        }
        fprintf(f, "</tr>\n");
    }
    fprintf(f, "</table>\n</body>\n</html>\n");

    fclose(f);
    cout << "Exito: Archivo HTML '" << nombreArchivo << "' generado. Abrelo en tu navegador." << endl;
}

// ============================================================================
// MAIN - MENU PRINCIPAL
// ============================================================================
int main() {
    // Declaramos como static para alojar la matriz en el segmento de datos y proteger la Pila (Stack)
    static Orden memoriaDron[FILAS][COLS];
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
            leerRuta("Ingrese ruta y nombre del archivo a cargar: ", ruta);
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
            cout << "Opcion invalida. Intente nuevamente." << endl;
        }
    } while (opcion != 0);

    return 0;
}
