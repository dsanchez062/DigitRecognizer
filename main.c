#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>  // file handling functions
#include <stdlib.h> // atoi
#include <string.h> // strtok
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

// Función principal
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("El programa debe tener un único argumento, la cantidad de procesos que se van a generar\n");
        exit(1);
    }

    SDL_Window *window = init_window("Imagen de 28x28", 400, 400);
    SDL_Renderer *renderer = init_renderer(window);

    load_data(my_path);
    show_image(renderer, data[0]);

    // Mantener la ventana abierta hasta que el usuario la cierre
    handle_events();

    unload_data();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
