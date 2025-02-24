#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>  // file handling functions
#include <stdlib.h> // atoi
#include <string.h> // strtok
#include <time.h>
#include <SDL2/SDL.h>
#include <sys/wait.h>
#define stacksize 1048576

static double **data;
int data_nrows = 60000;
int data_ncols = 784;
char *my_path = "/home/dsanchez062/Desktop/DigitRecognizer/"; // tendréis que poner vuestro path

int seed = 6;
int matrices_rows[4] = {784, 200, 100, 50};
int matrices_columns[4] = {200, 100, 50, 10};
int vector_rows[4] = {200, 100, 50, 10};
char *str;
int rows_per_div;

static double *digits;
static double **mat1;
static double **mat2;
static double **mat3;
static double **mat4;
static double *vec1;
static double *vec2;
static double *vec3;
static double *vec4;

int read_matrix(double **mat, char *file, int nrows, int ncols, int fac) {
    /*
     * Dada una matriz (mat), un nombre de fichero (file), una cantidad de filas
     * (nrows) y columnas (ncols), y un multiplicador (fac, no se usa, es 1), deja en mat la
     * matriz (de dimensión nrows x ncols) de datos contenida en el fichero con
     * nombre file
     */

    char *buffer = (char *)malloc(4096 * sizeof(char)); // Buffer para una fila completa
    if (!buffer) {
        printf("Error: No se pudo asignar memoria para el buffer\n");
        exit(1);
    }

    FILE *fstream = fopen(file, "r");
    if (!fstream) {
        printf("Error: No se pudo abrir el archivo %s\n", file);
        free(buffer);
        exit(1);
    }

    double aux;
    int row = 0;

    // Leer fila por fila del archivo
    while (fgets(buffer, 4096, fstream) != NULL && row < nrows) {
        char *record = strtok(buffer, " ");  // Usar espacio como delimitador
        int column = 0;

        // Leer cada valor dentro de la fila
        while (record != NULL && column < ncols) {
            aux = strtod(record, NULL) * (float)fac;
            mat[row][column] = aux;
            record = strtok(NULL, " ");  // Leer siguiente valor de la fila, usando espacio como delimitador
            column++;
        }

        row++;
    }

    // Asegurarse de que no hemos leído más de lo necesario
    if (row != nrows) {
        printf("Error: El archivo tiene menos filas de las esperadas\n");
        fclose(fstream);
        free(buffer);
        exit(1);
    }

    fclose(fstream);
    free(buffer);
    return 0;
}

int read_vector(double *vect, char *file, int nrows) {
    /*
     * Dado un vector (vect), un nombre de fichero (file), y una cantidad de filas
     * (nrows), deja en vect el vector (de dimensión nrows) de datos contenido en
     * el fichero con nombre file
     */
    char *buffer = (char *)malloc(4096 * sizeof(char)); // Esto contendrá el valor
    FILE *fstream = fopen(file, "r");

    if (!fstream) {
        printf("Error: No se pudo abrir el archivo %s\n", file);
        exit(1);
    }

    double aux;
    // Control de errores

    for (int row = 0; row < nrows; row++) {
        if (fgets(buffer, 4096, fstream) == NULL) {
            printf("Error: No se pudo leer la línea %d en %s\n", row, file);
            exit(1);
        }
        aux = strtod(buffer, NULL);
        vect[row] = aux;
    }



    // Hay que cerrar ficheros y liberar memoria
    free(buffer);
    return 0;
}


void print_matrix(double **mat, int nrows, int ncols, int offset_row,
                  int offset_col) {
    /*
     * Dada una matriz (mat), una cantidad de filas (nrows) y columnas (ncols) a
     * imprimir, y una cantidad de filas (offset_row) y columnas (offset_col) a
     * ignorar, imprime por salida estándar nrows x ncols de la matriz
     */
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            printf("%f ", mat[row + offset_row][col + offset_col]);
        }
        printf("\n");
    }
}

void print_vector(double *vect, int nrows) {
    /*
     * Dado un vector (vect), imprime su contenido.
     */
    for (int i = 0; i < nrows; i++) {
        printf("%lf\n", vect[i]);
    }
}


void load_data(char *path) {
    /*
     * Dado un directorio en el que están los datos y parámetros, los carga en las
     * variables de entorno
     */
    
    str = malloc(4096 * sizeof(char)); // Asegurar un buffer suficiente
    if (!str) {
        printf("Error: No se pudo asignar memoria para str\n");
        exit(1);
    }

    digits = malloc(data_nrows *
                    sizeof(double)); // Los valores que idealmente predeciremos
    sprintf(str, "%sdata/digits.csv", path);
    read_vector(digits, str, data_nrows);

    // Cargar la matriz de datos (imágenes)
    data = malloc(data_nrows * sizeof(double *)); // 60,000 imágenes
    for (int i = 0; i < data_nrows; i++) {
        data[i] = malloc(data_ncols * sizeof(double)); // 784 valores por imagen
    }
    sprintf(str, "%sdata/data.csv", path);  // Ruta al archivo de datos
    read_matrix(data, str, data_nrows, data_ncols, 1);  // Cargar la matriz de datos

    // Reservar memoria para mat1
    mat1 = malloc(matrices_rows[0] * sizeof(double *));
    for (int i = 0; i < matrices_rows[0]; i++) {
        mat1[i] = malloc(matrices_columns[0] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 0, seed);
    read_matrix(mat1, str, matrices_rows[0], matrices_columns[0], 1);

    // Reservar memoria para mat2
    mat2 = malloc(matrices_rows[1] * sizeof(double *));
    for (int i = 0; i < matrices_rows[1]; i++) {
        mat2[i] = malloc(matrices_columns[1] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 1, seed);
    read_matrix(mat2, str, matrices_rows[1], matrices_columns[1], 1);

    // Reservar memoria para mat3
    mat3 = malloc(matrices_rows[2] * sizeof(double *));
    for (int i = 0; i < matrices_rows[2]; i++) {
        mat3[i] = malloc(matrices_columns[2] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 2, seed);
    read_matrix(mat3, str, matrices_rows[2], matrices_columns[2], 1);

    // Reservar memoria para mat4
    mat4 = malloc(matrices_rows[3] * sizeof(double *));
    for (int i = 0; i < matrices_rows[3]; i++) {
        mat4[i] = malloc(matrices_columns[3] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 3, seed);
    read_matrix(mat4, str, matrices_rows[3], matrices_columns[3], 1);

    // Los vectores
    vec1 = malloc(vector_rows[0] * sizeof(double));
    sprintf(str, "%sdata/biases/csv/biases%d_%d.csv", path, 0, seed);
    read_vector(vec1, str, vector_rows[0]);

    vec2 = malloc(vector_rows[1] * sizeof(double));
    sprintf(str, "%sdata/biases/csv/biases%d_%d.csv", path, 1, seed);
    read_vector(vec2, str, vector_rows[1]);

    vec3 = malloc(vector_rows[2] * sizeof(double));
    sprintf(str, "%sdata/biases/csv/biases%d_%d.csv", path, 2, seed);
    read_vector(vec3, str, vector_rows[2]);

    vec4 = malloc(vector_rows[3] * sizeof(double));
    sprintf(str, "%sdata/biases/csv/biases%d_%d.csv", path, 3, seed);
    read_vector(vec4, str, vector_rows[3]);
}

void unload_data() {
    /*
     * Liberar la memoria
     */
    free(digits);
    free(data);
    free(mat1);
    free(mat2);
    free(mat3);
    free(mat4);
    free(vec1);
    free(vec2);
    free(vec3);
    free(vec4);
    free(str);
}

// Función para multiplicación matricial
void mat_mul(double **A, double **B, double **result, int A_rows, int A_cols, int B_cols) {
    for (int i = 0; i < A_rows; i++) {
        for (int j = 0; j < B_cols; j++) {
            result[i][j] = 0;
            for (int k = 0; k < A_cols; k++) {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Función para sumar un vector a cada fila de la matriz
void sum_vect(double **matrix, double *vector, double **result, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            result[i][j] = matrix[i][j] + vector[j];
        }
    }
}

// Función ReLU (reemplaza valores negativos por cero)
void relu(double **matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] < 0) {
                matrix[i][j] = 0;
            }
        }
    }
}

// Función ArgMax (retorna el índice del valor máximo de cada fila)
void argmax(double **matrix, int *predictions, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        int max_index = 0;
        for (int j = 1; j < cols; j++) {
            if (matrix[i][j] > matrix[i][max_index]) {
                max_index = j;
            }
        }
        predictions[i] = max_index; // Predicción del dígito (índice con valor más alto)
    }
}

void print(void *arg) { printf("Hola, soy %d\n", *(int *)arg); }

// Función para mostrar la imagen usando SDL2
void show_image(SDL_Renderer *renderer, double *image_data) {
    // Crear la superficie y textura para mostrar la imagen
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 28, 28, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_LockSurface(surface);

    // Iterar sobre los valores de los píxeles (28x28)
    for (int i = 0; i < 28; i++) {
        for (int j = 0; j < 28; j++) {
            int pixel_index = i * 28 + j;
            unsigned char color_value = (unsigned char)(image_data[pixel_index] * 255);  // Convertir a valor entre 0-255

            // Debug: Imprimir valores de los píxeles
            //printf("Pixel [%d, %d] = %f -> %d\n", i, j, image_data[pixel_index], color_value);

            // Establecer el valor del píxel en la superficie
            Uint32 color = SDL_MapRGB(surface->format, color_value, color_value, color_value);
            ((Uint32 *)surface->pixels)[i * 28 + j] = color;
        }
    }

    SDL_UnlockSurface(surface);

    // Crear una textura a partir de la superficie
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    // Limpiar la pantalla y renderizar la imagen
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(texture);
}


// Función principal
// Función para inicializar SDL y crear la ventana
SDL_Window* init_window(const char *title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error al inicializar SDL2: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window *window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Error al crear la ventana: %s\n", SDL_GetError());
        exit(1);
    }
    return window;
}

// Función para crear el renderizador
SDL_Renderer* init_renderer(SDL_Window *window) {
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Error al crear el renderizador: %s\n", SDL_GetError());
        exit(1);
    }
    return renderer;
}

// Función para manejar los eventos y mantener la ventana abierta
void handle_events() {
    SDL_Event event;
    int quit = 0;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }
    }
}


double measure_time(clock_t start, clock_t end) {
    return (double)(end - start) / CLOCKS_PER_SEC;
}


// Función principal
int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("El programa debe tener un único argumento, la cantidad de procesos que se van a generar\n");
        exit(1);
    }

    int image_to_recognize = atoi(argv[1]);
    clock_t start, end;  // Variables para medir tiempo

    SDL_Window *window = init_window("Imagen de 28x28", 400, 400);
    SDL_Renderer *renderer = init_renderer(window);

    start = clock();
    load_data(my_path);
    end = clock();
    printf("Tiempo de carga de datos: %.6f segundos\n", measure_time(start, end));

    start = clock();

    // Aquí es donde hacemos las operaciones matriciales:
    double **capa0 = malloc(60000 * sizeof(double *));
    double **capa1 = malloc(60000 * sizeof(double *));
    double **capa2 = malloc(60000 * sizeof(double *));
    double **capa3 = malloc(60000 * sizeof(double *));
    for (int i = 0; i < 60000; i++) {
        capa0[i] = malloc(200 * sizeof(double));  // Asumiendo que capa0 tiene 200 valores
        capa1[i] = malloc(100 * sizeof(double));  // Capa intermedia de 100 valores
        capa2[i] = malloc(50 * sizeof(double));   // Capa intermedia de 50 valores
        capa3[i] = malloc(10 * sizeof(double));   // Capa de salida con 10 valores
    }

    // Realizamos las operaciones de la red neuronal
    start = clock();
    mat_mul(data, mat1, capa0, 60000, 784, 200);  // Multiplicación data * mat1
    sum_vect(capa0, vec1, capa0, 60000, 200);     // Suma de bias
    relu(capa0, 60000, 200);                      // ReLU
    end = clock();
    printf("Tiempo de operaciones entre data y mat1: %.6f segundos\n", measure_time(start, end));


    start = clock();
    mat_mul(capa0, mat2, capa1, 60000, 200, 100);  // Multiplicación capa0 * mat2
    sum_vect(capa1, vec2, capa1, 60000, 100);      // Suma de bias //fallal aqui
    relu(capa1, 60000, 100);                       // ReLU
    end = clock();
    printf("Tiempo de operaciones entre capa0 y mat2: %.6f segundos\n", measure_time(start, end));
    
    start = clock();
    mat_mul(capa1, mat3, capa2, 60000, 100, 50);   // Multiplicación capa1 * mat3
    sum_vect(capa2, vec3, capa2, 60000, 50);       // Suma de bias
    relu(capa2, 60000, 50);                        // ReLU
    end = clock();
    printf("Tiempo de operaciones entre capa1 y mat3: %.6f segundos\n", measure_time(start, end));

    start = clock();
    mat_mul(capa2, mat4, capa3, 60000, 50, 10);    // Multiplicación capa2 * mat4
    sum_vect(capa3, vec4, capa3, 60000, 10);       // Suma de bias
    relu(capa3, 60000, 10);                        // ReLU
    end = clock();
    printf("Tiempo de operaciones entre capa2 y mat4: %.6f segundos\n", measure_time(start, end));

    //Predicción
    start = clock();
    int *predicciones = malloc(60000 * sizeof(int));  // Almacena las predicciones
    argmax(capa3, predicciones, 60000, 10);  // Realiza las predicciones con ArgMax
    end = clock();
    printf("Tiempo de predicciones: %.6f segundos\n", measure_time(start, end));

    printf("Predicción: %d\n", predicciones[image_to_recognize]);
    
    show_image(renderer, data[image_to_recognize]);

    // Mantener la ventana abierta hasta que el usuario la cierre
    handle_events();

    unload_data();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
