#include <iostream>
#include <cstdio>
#include <cstdlib> // Necesario para la funcion abs() (valor absoluto)
using namespace std;

const int FILAS = 200;
const int COLS = 200;

// ============================================================================
// ESTRUCTURAS DE DATOS (MOLDES)
// ============================================================================

// 1. Molde exacto del registro que viaja al archivo binario (.dat)
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
    bool ocupado = false; // Flag clave para saber si la celda es parte de la ruta activa
};

// ============================================================================
// FUNCIONES AUXILIARES DE VALIDACION (BLINDAJE)
// ============================================================================

// Verifica matematicamente que el siguiente paso sea adyacente y no la misma celda
bool sonAledanas(int x1, int y1, int x2, int y2) {
    int distX = abs(x1 - x2);
    int distY = abs(y1 - y2);
    // El movimiento maximo es 1 en X y 1 en Y, y la distancia no puede ser cero.
    return (distX <= 1 && distY <= 1) && !(distX == 0 && distY == 0);
}

// Verifica que una coordenada (X,Y) no se salga del rango 0 a 199
bool dentroDelMapa(int x, int y) {
    return (x >= 0 && x < FILAS && y >= 0 && y < COLS);
}

// Atrapa errores de teclado (ej: ingresar una letra en vez de numero) y limpia el buffer
int leerEntero(const char *mensaje) {
    int valor;
    cout << mensaje;
    while (!(cin >> valor)) {
        cout << "  [Error] Ingreso invalido. Se esperaba un numero entero: ";
        cin.clear(); // Resetea el estado de error de la consola
        cin.ignore(10000, '\n'); // Descarta la "basura" del buffer hasta el salto de linea
    }
    cin.ignore(10000, '\n'); // Limpia el salto de linea que sobra
    return valor;
}

// Obliga al usuario a ingresar un numero dentro de un rango especifico
int leerEnteroRango(const char *mensaje, int minVal, int maxVal) {
    int valor;
    do {
        valor = leerEntero(mensaje);
        if (valor < minVal || valor > maxVal) {
            cout << "  [Error] Fuera de limite. Ingrese un valor entre " << minVal << " y " << maxVal << "." << endl;
        }
    } while (valor < minVal || valor > maxVal);
    return valor;
}

// Impide ingresar tiempos de espera negativos
unsigned int leerEspera(const char *mensaje) {
    int valor;
    do {
        valor = leerEntero(mensaje);
        if (valor < 0) {
            cout << "  [Error] El tiempo de espera no puede ser menor a 0." << endl;
        }
    } while (valor < 0);
    return (unsigned int)valor;
}

// Fuerza al usuario a contestar solo 1 (Verdadero) o 0 (Falso)
bool leerBool(const char *mensaje) {
    return leerEnteroRango(mensaje, 0, 1) == 1;
}

// Lee nombres de archivo usando getline para aceptar espacios y validando ingresos vacios
void leerRuta(const char *mensaje, char ruta[260]) {
    do {
        cout << mensaje;
        cin.getline(ruta, 260);
        if (cin.fail()) { // Si el usuario pego una ruta larguisima que rompio el buffer
            cin.clear();
            cin.ignore(10000, '\n');
            ruta[0] = '\0';
            cout << "  [Error] La ruta excede el limite de 260 caracteres." << endl;
        } else if (ruta[0] == '\0') {
            cout << "  [Error] El nombre del archivo no puede estar en blanco." << endl;
        }
    } while (ruta[0] == '\0');
}

// Solicita "Siguiente X/Y" forzando en vivo a que sea una celda vecina dentro de la matriz
void pedirSiguienteValido(int actualX, int actualY, int &sigX, int &sigY) {
    bool valido = false;
    do {
        sigX = leerEnteroRango("Siguiente X (0 a 199): ", 0, FILAS - 1);
        sigY = leerEnteroRango("Siguiente Y (0 a 199): ", 0, COLS - 1);

        if (!sonAledanas(actualX, actualY, sigX, sigY)) {
            cout << "  [Error] Movimiento invalido: Debe ser una celda aledana a (" << actualX << "," << actualY << ")." << endl;
        } else {
            valido = true;
        }
    } while (!valido);
}

// Comprueba si el dron se quedo encerrado sin casilleros libres para avanzar
bool tieneVecinaLibre(int x, int y, bool usado[FILAS][COLS]) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (dentroDelMapa(nx, ny) && !usado[nx][ny]) {
                return true; // Encontro una salida
            }
        }
    }
    return false; // Esta atrapado
}

// Verifica si existe alguna ruta cargada en la matriz principal
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
        cout << "Error: No se encontro el archivo '" << rutaArchivo << "'." << endl;
        return false;
    }

    // Usamos una matriz temporal para evitar corromper la memoria real si el archivo es invalido
    static Orden temp[FILAS][COLS];
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            temp[i][j] = Orden();
        }
    }

    OrdenArchivo r;
    int totalRegistros = 0, cantDespegues = 0, cantFin = 0;
    int inicioX = -1, inicioY = -1;

    // FASE 1: Volcado del archivo binario a la matriz temporal con validacion de reglas basicas
    while (fread(&r, sizeof(OrdenArchivo), 1, f) == 1) {
        totalRegistros++;

        if (!dentroDelMapa(r.x, r.y)) {
            cout << "Error de carga: Coordenadas fuera de limites en el archivo." << endl;
            fclose(f);
            return false;
        }
        if (temp[r.x][r.y].ocupado) {
            cout << "Error de carga: Coordenada (" << r.x << "," << r.y << ") duplicada en el archivo." << endl;
            fclose(f);
            return false;
        }
        if (r.despegue) {
            cantDespegues++;
            inicioX = r.x;
            inicioY = r.y;
            // El despegue no puede tener acciones ofensivas ni ser el aterrizaje a la vez
            if (r.soltarGranada1 || r.soltarGranada2 || r.ataqueKamikaze || r.aterrizaje) {
                cout << "Error de carga: Se detectaron ordenes incompatibles con el Despegue." << endl;
                fclose(f);
                return false;
            }
        }
        if (r.aterrizaje && r.ataqueKamikaze) {
            cout << "Error de carga: No se puede aterrizar y hacer Kamikaze al mismo tiempo." << endl;
            fclose(f);
            return false;
        }
        if (r.aterrizaje || r.ataqueKamikaze) {
            cantFin++;
        }

        // Si paso las pruebas iniciales, lo guardamos en el temporal
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
        cout << "Error de carga: El archivo '" << rutaArchivo << "' esta vacio." << endl;
        return false;
    }

    if (cantDespegues != 1 || cantFin != 1) {
        cout << "Error de carga: Archivo corrupto. Debe existir exactamente 1 Despegue y 1 Final." << endl;
        return false;
    }

    // FASE 2: Validacion de continuidad de ruta. Simulamos el vuelo para atrapar huecos o bucles.
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

        if (celdaActual.aterrizaje || celdaActual.ataqueKamikaze) break; // Termino bien

        int sigX = celdaActual.siguientex;
        int sigY = celdaActual.siguientey;

        if (!dentroDelMapa(sigX, sigY)) {
            cout << "Error de ruta: El dron intenta salir del mapa desde (" << actualX << "," << actualY << ")." << endl;
            return false;
        }
        if (!sonAledanas(actualX, actualY, sigX, sigY)) {
            cout << "Error de ruta: Salto ilegal entre (" << actualX << "," << actualY << ") y (" << sigX << "," << sigY << ")." << endl;
            return false;
        }
        if (!temp[sigX][sigY].ocupado) {
            cout << "Error de ruta: Hueco detectado. Apunta a (" << sigX << "," << sigY << "), pero esa celda no existe." << endl;
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
        cout << "Error de carga: Hay registros sueltos que no forman parte de la ruta del vuelo." << endl;
        return false;
    }

    // FASE 3: Aprobacion final. Se vuelca la matriz temporal a la matriz oficial de la RAM.
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            memoriaDron[i][j] = temp[i][j];
        }
    }

    cout << "Exito: Mision cargada en memoria y validada correctamente (" << totalRegistros << " registros procesados)." << endl;
    return true;
}

// ============================================================================
// OPERACION 2: Mostrar ataque cargado
// ============================================================================
void mostrarAtaque(Orden memoriaDron[FILAS][COLS]) {
    int actualX = -1, actualY = -1;

    // Busca el punto de despegue para iniciar el reporte
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
        cout << "Aviso: No hay ninguna mision activa en la memoria RAM." << endl;
        return;
    }

    // Matriz de seguridad para evitar que errores introducidos con la Opcion 5 traben el reporte
    static bool visitado[FILAS][COLS];
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) visitado[i][j] = false;
    }

    int numeroPaso = 1, pasosPorPagina = 5, contadorPagina = 0, paginaActual = 1;

    cout << "\n=== REPORTE DE VUELO EN MEMORIA (Pag. " << paginaActual << ") ===" << endl;

    while (true) {
        if (!dentroDelMapa(actualX, actualY)) {
            cout << "\n[!] Error critico en RAM: La ruta apunta fuera del mapa. Abortando visualizacion." << endl;
            break;
        }

        Orden celda = memoriaDron[actualX][actualY];

        if (!celda.ocupado) {
            cout << "\n[!] Error critico en RAM: La ruta apunta a un vacio en (" << actualX << "," << actualY << ")." << endl;
            break;
        }

        if (visitado[actualX][actualY]) {
            cout << "\n[!] Error critico en RAM: Bucle infinito por edicion indebida en (" << actualX << "," << actualY << ")." << endl;
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
            cout << "\n=== FIN DE LA MISION ===" << endl;
            break;
        }

        if (!sonAledanas(actualX, actualY, celda.siguientex, celda.siguientey)) {
            cout << "\n[!] Error critico en RAM: Salto no aledano detectado. La ruta fue adulterada." << endl;
            break;
        }

        cout << "  -> 7. MOVER A: (" << celda.siguientex << ", " << celda.siguientey << ")" << endl;

        // Paginacion
        contadorPagina++;
        if (contadorPagina == pasosPorPagina) {
            cout << "\n--- Presione ENTER para ver la siguiente pagina ---";
            cin.get();
            paginaActual++;
            contadorPagina = 0;
            cout << "\n=== REPORTE DE VUELO EN MEMORIA (Pag. " << paginaActual << ") ===" << endl;
        }

        actualX = celda.siguientex;
        actualY = celda.siguientey;
        numeroPaso++;
    }
}

// ============================================================================
// OPERACION 3: Crear un archivo de ataque nuevo
// ============================================================================
void crearArchivoNuevo() {
    char nombreArchivo[260];
    leerRuta("Nombre del nuevo archivo a crear (ej: mision.dat): ", nombreArchivo);

    FILE *f = fopen(nombreArchivo, "wb");
    if (f == NULL) {
        cout << "Error: No se tienen permisos para crear el archivo." << endl;
        return;
    }

    // Rastrea las celdas utilizadas en vivo para impedir que el usuario provoque bucles durante la carga
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
        cout << "\n--- PROGRAMANDO PASO #" << (cantidad + 1) << " ---" << endl;

        if (cantidad == 0) {
            cout << "[!] Primer paso: Despegue automatico (Ataques deshabilitados)." << endl;
            r.x = leerEnteroRango("Coordenada de Despegue X (0 a 199): ", 0, FILAS - 1);
            r.y = leerEnteroRango("Coordenada de Despegue Y (0 a 199): ", 0, COLS - 1);
            usadoEnCreacion[r.x][r.y] = true;

            r.despegue = true;
            r.aterrizaje = false;
            r.ataqueKamikaze = false;
            r.soltarGranada1 = false;
            r.soltarGranada2 = false;
            r.espera = leerEspera("Espera inicial (0 o mas): ");

            do {
                pedirSiguienteValido(r.x, r.y, r.siguientex, r.siguientey);
                if (usadoEnCreacion[r.siguientex][r.siguientey]) {
                    cout << "  [Error] Esa celda ya fue visitada. Elija otra direccion." << endl;
                }
            } while (usadoEnCreacion[r.siguientex][r.siguientey]);

            proxX = r.siguientex;
            proxY = r.siguientey;
        } else {
            r.x = proxX;
            r.y = proxY;
            usadoEnCreacion[r.x][r.y] = true;
            cout << "[+] Dron posicionado en (" << r.x << ", " << r.y << ")." << endl;

            r.despegue = false;

            // Si el dron se metio en un callejon sin salida, se fuerza el fin de la ruta
            if (!tieneVecinaLibre(r.x, r.y, usadoEnCreacion)) {
                cout << "[!] ALERTA: No quedan celdas vecinas libres. El dron debe finalizar mision aqui." << endl;
                r.ataqueKamikaze = leerBool("Terminar con Kamikaze? (1=Si, 0=Aterrizaje): ");
                r.aterrizaje = !r.ataqueKamikaze;
            } else {
                r.ataqueKamikaze = leerBool("Ejecutar Ataque Kamikaze (1=Si, 0=No): ");
                if (!r.ataqueKamikaze) {
                    r.aterrizaje = leerBool("Ejecutar Aterrizaje      (1=Si, 0=No): ");
                } else {
                    r.aterrizaje = false;
                }
            }

            r.espera = leerEspera("Tiempo de espera (0 o mas): ");
            r.soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
            r.soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");

            if (r.ataqueKamikaze || r.aterrizaje) {
                r.siguientex = 0;
                r.siguientey = 0;
                seguir = false; // Corta el bucle de carga de manera prolija
                cout << "[!] Archivo de mision completado y cerrado." << endl;
            } else {
                do {
                    pedirSiguienteValido(r.x, r.y, r.siguientex, r.siguientey);
                    if (usadoEnCreacion[r.siguientex][r.siguientey]) {
                        cout << "  [Error] La celda ya fue visitada en esta ruta. Elija otra." << endl;
                    }
                } while (usadoEnCreacion[r.siguientex][r.siguientey]);

                proxX = r.siguientex;
                proxY = r.siguientey;
                cout << "  -> El dron avanzara a (" << proxX << ", " << proxY << ")." << endl;
            }
        }

        fwrite(&r, sizeof(OrdenArchivo), 1, f);
        cantidad++;
    }

    fclose(f);
    cout << "Exito: Se guardaron " << cantidad << " registros conectados en el disco." << endl;
}

// ============================================================================
// OPERACION 4: Corregir un registro del archivo
// ============================================================================
void corregirRegistroArchivo() {
    char nombreArchivo[260];
    leerRuta("Ruta y nombre del archivo a corregir: ", nombreArchivo);

    // Modo "rb+" lee y escribe el archivo binario sin borrar lo que ya tiene
    FILE *f = fopen(nombreArchivo, "rb+");
    if (f == NULL) {
        cout << "Error: No se encontro el archivo especificado." << endl;
        return;
    }

    int buscarX = leerEnteroRango("Coordenada X a editar (0-199): ", 0, FILAS - 1);
    int buscarY = leerEnteroRango("Coordenada Y a editar (0-199): ", 0, COLS - 1);

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

            cout << "\n--- INGRESE NUEVOS PARAMETROS ---" << endl;
            r.espera = leerEspera("Nueva espera (0 o mas): ");

            if (r.despegue) {
                cout << "[!] Bloqueo: La celda de Despegue no admite ataques." << endl;
                r.soltarGranada1 = false;
                r.soltarGranada2 = false;
            } else {
                r.soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
                r.soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");
            }

            // Mantenemos la coherencia de la ruta: Solo deja alterar el tipo de finalizacion
            if (r.aterrizaje || r.ataqueKamikaze) {
                cout << "[!] Casillero Final detectado." << endl;
                r.ataqueKamikaze = leerBool("Desea que finalice con Kamikaze? (1=Si, 0=Aterrizaje): ");
                r.aterrizaje = !r.ataqueKamikaze;
                r.siguientex = 0;
                r.siguientey = 0;
            } else {
                cout << "Paso siguiente registrado: (" << r.siguientex << ", " << r.siguientey << ")" << endl;
                if (leerBool("Desea desviar la ruta hacia otra coordenada vecina? (1=Si, 0=No): ")) {
                    pedirSiguienteValido(r.x, r.y, r.siguientex, r.siguientey);
                }
            }

            // Retrocede 1 bloque en el archivo para pisar el registro recien leido
            fseek(f, -(long)sizeof(OrdenArchivo), SEEK_CUR);
            fwrite(&r, sizeof(OrdenArchivo), 1, f);
            fflush(f); // Vaciado de buffer de disco necesario tras combinar fseek y fwrite

            cout << "Exito: El registro fue actualizado fisicamente en el archivo." << endl;
            cout << "AVISO: Para utilizar esta nueva ruta debe ejecutar la Opcion 1 (Cargar a Memoria)." << endl;
            break;
        }
    }

    if (!encontrado) {
        cout << "Aviso: La coordenada solicitada no formaba parte de este archivo." << endl;
    }
    fclose(f);
}

// ============================================================================
// OPERACION 5: Corregir un registro en memoria
// ============================================================================
void corregirRegistroMemoria(Orden memoriaDron[FILAS][COLS]) {
    if (!hayAtaqueEnMemoria(memoriaDron)) {
        cout << "Aviso: La memoria RAM del dron esta vacia." << endl;
        return;
    }

    int x = leerEnteroRango("Coordenada X a modificar (0-199): ", 0, FILAS - 1);
    int y = leerEnteroRango("Coordenada Y a modificar (0-199): ", 0, COLS - 1);

    if (!memoriaDron[x][y].ocupado) {
        cout << "Error: La celda (" << x << ", " << y << ") no es utilizada por la mision actual." << endl;
        return;
    }

    cout << "\n--- DATOS EN RAM (" << x << ", " << y << ") ---" << endl;
    cout << "Espera: " << memoriaDron[x][y].espera << " | Granada1: " << memoriaDron[x][y].soltarGranada1
         << " | Granada2: " << memoriaDron[x][y].soltarGranada2 << " | Kamikaze: " << memoriaDron[x][y].ataqueKamikaze
         << " | Aterrizaje: " << memoriaDron[x][y].aterrizaje << " | Despegue: " << memoriaDron[x][y].despegue
         << " | Siguiente: (" << memoriaDron[x][y].siguientex << "," << memoriaDron[x][y].siguientey << ")" << endl;

    cout << "\n--- INGRESE NUEVOS PARAMETROS ---" << endl;
    memoriaDron[x][y].espera = leerEspera("Nueva espera (0 o mas): ");

    if (memoriaDron[x][y].despegue) {
        cout << "[!] Bloqueo: La celda de Despegue no admite ataques." << endl;
        memoriaDron[x][y].soltarGranada1 = false;
        memoriaDron[x][y].soltarGranada2 = false;
    } else {
        memoriaDron[x][y].soltarGranada1 = leerBool("Soltar Granada 1 (1=Si, 0=No): ");
        memoriaDron[x][y].soltarGranada2 = leerBool("Soltar Granada 2 (1=Si, 0=No): ");
    }

    if (memoriaDron[x][y].aterrizaje || memoriaDron[x][y].ataqueKamikaze) {
        cout << "[!] Casillero Final detectado." << endl;
        memoriaDron[x][y].ataqueKamikaze = leerBool("Desea que finalice con Kamikaze? (1=Si, 0=Aterrizaje): ");
        memoriaDron[x][y].aterrizaje = !memoriaDron[x][y].ataqueKamikaze;
        memoriaDron[x][y].siguientex = 0;
        memoriaDron[x][y].siguientey = 0;
    } else {
        cout << "Paso siguiente registrado: (" << memoriaDron[x][y].siguientex << ", " << memoriaDron[x][y].siguientey << ")" << endl;
        if (leerBool("Desea redirigir el paso hacia OTRA celda ocupada? (1=Si, 0=No): ")) {
            int nuevoSigX, nuevoSigY;
            bool pasoValido = false;
            do {
                pedirSiguienteValido(x, y, nuevoSigX, nuevoSigY);
                // Si la va a desviar en RAM, debe asegurarse que apunte a un pedazo de ruta existente
                if (!memoriaDron[nuevoSigX][nuevoSigY].ocupado) {
                    cout << "  [Error] Ruptura de ruta: Debe apuntar a una celda ocupada por la mision en RAM." << endl;
                } else if (memoriaDron[nuevoSigX][nuevoSigY].despegue) {
                    cout << "  [Error] Movimiento ciclico invalido: No puede regresar al Despegue." << endl;
                } else {
                    pasoValido = true;
                }
            } while (!pasoValido);

            memoriaDron[x][y].siguientex = nuevoSigX;
            memoriaDron[x][y].siguientey = nuevoSigY;
        }
    }

    cout << "Exito: Los cambios fueron guardados en la memoria RAM." << endl;
}

// ============================================================================
// OPERACION 6: Guardar memoria en un archivo nuevo
// ============================================================================
void guardarMemoriaEnArchivo(Orden memoriaDron[FILAS][COLS]) {
    if (!hayAtaqueEnMemoria(memoriaDron)) {
        cout << "Aviso: La memoria esta vacia. No hay mision para exportar." << endl;
        return;
    }

    char nombreArchivo[260];
    leerRuta("Ruta y nombre del archivo binario a generar (ej: backup.dat): ", nombreArchivo);

    FILE *f = fopen(nombreArchivo, "wb");
    if (f == NULL) {
        cout << "Error: No se tienen permisos para crear el archivo en esa ruta." << endl;
        return;
    }

    int guardados = 0;
    // Barre secuencialmente la matriz exportando las celdas marcadas como activas
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
    cout << "Exito: Mision exportada con " << guardados << " registros en '" << nombreArchivo << "'." << endl;
}

// ============================================================================
// OPERACION 7: Visualizar el ataque en HTML
// ============================================================================
void generarHTML(Orden memoriaDron[FILAS][COLS]) {
    if (!hayAtaqueEnMemoria(memoriaDron)) {
        cout << "Aviso: No se encontro ninguna mision en memoria para diagramar." << endl;
        return;
    }

    char nombreArchivo[260];
    leerRuta("Nombre del archivo HTML a exportar (ej: reporte.html): ", nombreArchivo);

    FILE *f = fopen(nombreArchivo, "w"); // "w" para texto plano
    if (f == NULL) {
        cout << "Error critico al intentar crear el documento HTML." << endl;
        return;
    }

    // Cabeceras y estilos CSS inyectados para formatear la cuadrícula oscura
    fprintf(f, "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>Visor de Ataque</title>\n");
    fprintf(f, "<style>\n");
    fprintf(f, "body { background-color: #0f111a; color: #fff; font-family: sans-serif; display: flex; flex-direction: column; align-items: center; margin: 0; padding: 20px; }\n");
    fprintf(f, ".header { background-color: #1e212b; padding: 12px 30px; border-radius: 6px; margin-bottom: 10px; font-size: 15px; letter-spacing: 1px; box-shadow: 0 4px 6px rgba(0,0,0,0.5); border: 1px solid #333; }\n");
    fprintf(f, ".leyenda { font-size: 12px; color: #8892b0; margin-bottom: 25px; }\n");
    fprintf(f, "table { border-collapse: collapse; background-color: #050505; box-shadow: 0 0 20px rgba(0,0,0,0.9); }\n");
    fprintf(f, "td { width: 22px; height: 20px; border: 1px solid #222; text-align: center; vertical-align: middle; font-size: 12px; font-weight: bold; cursor: crosshair; }\n");
    fprintf(f, "td:hover { background-color: #333; border-color: #666; }\n");
    // Paleta de colores 
    fprintf(f, ".D { background: #4caf50; color: #fff; }\n");
    fprintf(f, ".A { background: #2196f3; color: #fff; }\n");
    fprintf(f, ".G1{ background: #ff9800; color: #fff; }\n");
    fprintf(f, ".G2{ background: #ff5722; color: #fff; }\n");
    fprintf(f, ".K { background: #f44336; color: #fff; }\n");
    fprintf(f, ".E { background: #ffeb3b; color: #000; }\n");
    fprintf(f, ".M { background: #9e9e9e; color: #000; }\n");
    fprintf(f, "</style>\n</head>\n<body>\n");

    fprintf(f, "<div class=\"header\">PANEL RADAR | CUADRANTE 200x200</div>\n");
    fprintf(f, "<div class=\"leyenda\">D: Despegue | A: Aterrizaje | 1/2: Granadas | K: Kamikaze | E: Espera | *: Movimiento</div>\n");

    fprintf(f, "<table>\n");
    for (int i = 0; i < FILAS; i++) {
        fprintf(f, "<tr>\n");
        for (int j = 0; j < COLS; j++) {
            if (memoriaDron[i][j].ocupado) {
                // El if-else-if estricto asegura que se imprima la orden de MAYOR PRECEDENCIA
                if (memoriaDron[i][j].despegue)            fprintf(f, "<td class=\"D\">D</td>");
                else if (memoriaDron[i][j].aterrizaje)     fprintf(f, "<td class=\"A\">A</td>");
                else if (memoriaDron[i][j].soltarGranada1) fprintf(f, "<td class=\"G1\">1</td>");
                else if (memoriaDron[i][j].soltarGranada2) fprintf(f, "<td class=\"G2\">2</td>");
                else if (memoriaDron[i][j].ataqueKamikaze) fprintf(f, "<td class=\"K\">K</td>");
                else if (memoriaDron[i][j].espera > 0)     fprintf(f, "<td class=\"E\">E</td>");
                else                                       fprintf(f, "<td class=\"M\">*</td>"); // Movimiento liso
            } else {
                fprintf(f, "<td></td>");
            }
        }
        fprintf(f, "</tr>\n");
    }
    fprintf(f, "</table>\n</body>\n</html>\n");

    fclose(f);
    cout << "Generacion Exitosa: Localice y abra el archivo '" << nombreArchivo << "' en su navegador web." << endl;
}

// ============================================================================
// MAIN - BUCLE DEL SISTEMA
// ============================================================================
int main() {
    // Declaracion 'static' envia los 640KB de la matriz al Segmento de Datos, salvando el Stack (Pila).
    static Orden memoriaDron[FILAS][COLS];
    int opcion;

    do {
        cout << "\n=== MENU SISTEMA DRON MILITAR ===" << endl;
        cout << "1. Cargar archivo de ataque en memoria" << endl;
        cout << "2. Mostrar ataque cargado" << endl;
        cout << "3. Crear un archivo de ataque nuevo" << endl;
        cout << "4. Corregir un registro del archivo" << endl;
        cout << "5. Corregir un registro en memoria" << endl;
        cout << "6. Guardar memoria en un archivo nuevo" << endl;
        cout << "7. Visualizar un archivo de ataque en HTML" << endl;
        cout << "0. Salir" << endl;

        opcion = leerEntero(">> Ingrese una operacion (0-7): ");

        switch (opcion) {
        case 1: {
            char ruta[260];
            leerRuta("Ruta y nombre del archivo binario a cargar: ", ruta);
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
            cout << "Cerrando el sistema... Adios." << endl;
            break;
        default:
            cout << "Opcion no valida. Por favor, seleccione un numero del 0 al 7." << endl;
        }
    } while (opcion != 0);

    return 0;
}