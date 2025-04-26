#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define stacksize 1048576
#define CLONE_FLAGS (CLONE_VM | CLONE_FILES | SIGCHLD)

// Variables globales
static double **data;
int data_nrows = 60000;
int data_ncols = 784;
char *my_path = "/home/dsanchez062/Desktop/DigitRecognizer/"; // Cambia al path correcto

int seed = 6;
int matrices_rows[4] = {784, 200, 100, 50};
int matrices_columns[4] = {200, 100, 50, 10};
int vector_rows[4] = {200, 100, 50, 10};
char *str;

static double *digits;
static double **mat1;
static double **mat2;
static double **mat3;
static double **mat4;
static double *vec1;
static double *vec2;
static double *vec3;
static double *vec4;

// Estructura para pasar argumentos a los procesos hijo
typedef struct {
    int start_row;
    int end_row;
    double **data;
    double **mat1, **mat2, **mat3, **mat4;
    double *vec1, *vec2, *vec3, *vec4;
    double **capa0, **capa1, **capa2, **capa3;
    int *predicciones;
    double *child_time; // Para almacenar el tiempo del proceso hijo
} ChildArgs;

// Prototipo de funciones
double measure_time_timeval(struct timespec start, struct timespec end);

// Función para leer una matriz
int read_matrix(double **mat, char *file, int nrows, int ncols, int fac) {
    char *buffer = (char *)malloc(4096 * sizeof(char));
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

    while (fgets(buffer, 4096, fstream) != NULL && row < nrows) {
        char *record = strtok(buffer, " ");
        int column = 0;

        while (record != NULL && column < ncols) {
            aux = strtod(record, NULL) * (float)fac;
            mat[row][column] = aux;
            record = strtok(NULL, " ");
            column++;
        }
        row++;
    }

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

// Función para leer un vector
int read_vector(double *vect, char *file, int nrows) {
    char *buffer = (char *)malloc(4096 * sizeof(char));
    FILE *fstream = fopen(file, "r");

    if (!fstream) {
        printf("Error: No se pudo abrir el archivo %s\n", file);
        free(buffer);
        exit(1);
    }

    double aux;
    for (int row = 0; row < nrows; row++) {
        if (fgets(buffer, 4096, fstream) == NULL) {
            printf("Error: No se pudo leer la línea %d en %s\n", row, file);
            free(buffer);
            fclose(fstream);
            exit(1);
        }
        aux = strtod(buffer, NULL);
        vect[row] = aux;
    }

    fclose(fstream);
    free(buffer);
    return 0;
}

// Función para imprimir una matriz
void print_matrix(double **mat, int nrows, int ncols, int offset_row, int offset_col) {
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            printf("%f ", mat[row + offset_row][col + offset_col]);
        }
        printf("\n");
    }
}

// Función para imprimir un vector
void print_vector(double *vect, int nrows) {
    for (int i = 0; i < nrows; i++) {
        printf("%lf\n", vect[i]);
    }
}

// Función para cargar datos
void load_data(char *path) {
    str = malloc(4096 * sizeof(char));
    if (!str) {
        printf("Error: No se pudo asignar memoria para str\n");
        exit(1);
    }

    digits = malloc(data_nrows * sizeof(double));
    sprintf(str, "%sdata/digits.csv", path);
    read_vector(digits, str, data_nrows);

    data = malloc(data_nrows * sizeof(double *));
    for (int i = 0; i < data_nrows; i++) {
        data[i] = malloc(data_ncols * sizeof(double));
    }
    sprintf(str, "%sdata/data.csv", path);
    read_matrix(data, str, data_nrows, data_ncols, 1);

    mat1 = malloc(matrices_rows[0] * sizeof(double *));
    for (int i = 0; i < matrices_rows[0]; i++) {
        mat1[i] = malloc(matrices_columns[0] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 0, seed);
    read_matrix(mat1, str, matrices_rows[0], matrices_columns[0], 1);

    mat2 = malloc(matrices_rows[1] * sizeof(double *));
    for (int i = 0; i < matrices_rows[1]; i++) {
        mat2[i] = malloc(matrices_columns[1] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 1, seed);
    read_matrix(mat2, str, matrices_rows[1], matrices_columns[1], 1);

    mat3 = malloc(matrices_rows[2] * sizeof(double *));
    for (int i = 0; i < matrices_rows[2]; i++) {
        mat3[i] = malloc(matrices_columns[2] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 2, seed);
    read_matrix(mat3, str, matrices_rows[2], matrices_columns[2], 1);

    mat4 = malloc(matrices_rows[3] * sizeof(double *));
    for (int i = 0; i < matrices_rows[3]; i++) {
        mat4[i] = malloc(matrices_columns[3] * sizeof(double));
    }
    sprintf(str, "%sdata/weights/csv/weights%d_%d.csv", path, 3, seed);
    read_matrix(mat4, str, matrices_rows[3], matrices_columns[3], 1);

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

// Función para liberar memoria
void unload_data() {
    for (int i = 0; i < data_nrows; i++) {
        free(data[i]);
    }
    for (int i = 0; i < matrices_rows[0]; i++) {
        free(mat1[i]);
    }
    for (int i = 0; i < matrices_rows[1]; i++) {
        free(mat2[i]);
    }
    for (int i = 0; i < matrices_rows[2]; i++) {
        free(mat3[i]);
    }
    for (int i = 0; i < matrices_rows[3]; i++) {
        free(mat4[i]);
    }
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
void mat_mul(double **A, double **B, double **result, int rows, int A_cols, int B_cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < B_cols; j++) {
            result[i][j] = 0;
            for (int k = 0; k < A_cols; k++) {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Función para sumar un vector
void sum_vect(double **matrix, double *vector, double **result, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            result[i][j] = matrix[i][j] + vector[j];
        }
    }
}

// Función ReLU
void relu(double **matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] < 0) {
                matrix[i][j] = 0;
            }
        }
    }
}

// Función ArgMax
void argmax(double **matrix, int *predictions, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        int max_index = 0;
        for (int j = 1; j < cols; j++) {
            if (matrix[i][j] > matrix[i][max_index]) {
                max_index = j;
            }
        }
        predictions[i] = max_index;
    }
}

// Función para mostrar la imagen
void show_image(SDL_Renderer *renderer, double *image_data) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 28, 28, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_LockSurface(surface);

    for (int i = 0; i < 28; i++) {
        for (int j = 0; j < 28; j++) {
            int pixel_index = i * 28 + j;
            unsigned char color_value = (unsigned char)(image_data[pixel_index] * 255);
            Uint32 color = SDL_MapRGB(surface->format, color_value, color_value, color_value);
            ((Uint32 *)surface->pixels)[i * 28 + j] = color;
        }
    }

    SDL_UnlockSurface(surface);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(texture);
}

// Función para inicializar SDL
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

// Función para manejar eventos
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

// Función para medir tiempo (para procesos hijo)
double measure_time(clock_t start, clock_t end) {
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Función para medir tiempo con timespec (para main)
double measure_time_timeval(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Función ejecutada por cada proceso hijo
int perform_multiplications(void *arg) {
    ChildArgs *args = (ChildArgs *)arg;
    int rows = args->end_row - args->start_row;

    // Depuración: Imprimir el subrango procesado
    printf("Proceso hijo: Procesando filas %d a %d (%d filas)\n", args->start_row, args->end_row - 1, rows);

    clock_t start = clock();

    // Capa 0
    mat_mul(args->data, args->mat1, args->capa0, rows, 784, 200);
    sum_vect(args->capa0, args->vec1, args->capa0, rows, 200);
    relu(args->capa0, rows, 200);

    // Capa 1
    mat_mul(args->capa0, args->mat2, args->capa1, rows, 200, 100);
    sum_vect(args->capa1, args->vec2, args->capa1, rows, 100);
    relu(args->capa1, rows, 100);

    // Capa 2
    mat_mul(args->capa1, args->mat3, args->capa2, rows, 100, 50);
    sum_vect(args->capa2, args->vec3, args->capa2, rows, 50);
    relu(args->capa2, rows, 50);

    // Capa 3
    mat_mul(args->capa2, args->mat4, args->capa3, rows, 50, 10);
    sum_vect(args->capa3, args->vec4, args->capa3, rows, 10);
    relu(args->capa3, rows, 10);

    // Predicciones
    argmax(args->capa3, args->predicciones, rows, 10);

    clock_t end = clock();
    *args->child_time = measure_time(start, end);

    // Depuración: Imprimir tiempo del proceso hijo
    printf("Proceso hijo (filas %d-%d): %.6f segundos\n", args->start_row, args->end_row - 1, *args->child_time);

    return 0;
}

// Función principal
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <num_processes> <image_to_recognize>\n", argv[0]);
        exit(1);
    }

    int num_processes = atoi(argv[1]);
    int image_to_recognize = atoi(argv[2]);

    // Validar argumentos
    if (num_processes < 1 || num_processes > 60000) {
        printf("Error: El número de procesos debe estar entre 1 y 60000\n");
        exit(1);
    }
    if (image_to_recognize < 0 || image_to_recognize >= 60000) {
        printf("Error: El índice de la imagen debe estar entre 0 y 59999\n");
        exit(1);
    }

    struct timespec start, end;

    // Inicializar SDL
    SDL_Window *window = init_window("Imagen de 28x28", 400, 400);
    SDL_Renderer *renderer = init_renderer(window);

    // Cargar datos
    clock_gettime(CLOCK_MONOTONIC, &start);
    load_data(my_path);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Tiempo de carga de datos: %.6f segundos\n", measure_time_timeval(start, end));

    // Asignar memoria
    double **capa0 = malloc(60000 * sizeof(double *));
    double **capa1 = malloc(60000 * sizeof(double *));
    double **capa2 = malloc(60000 * sizeof(double *));
    double **capa3 = malloc(60000 * sizeof(double *));
    int *predicciones = malloc(60000 * sizeof(int));
    for (int i = 0; i < 60000; i++) {
        capa0[i] = malloc(200 * sizeof(double));
        capa1[i] = malloc(100 * sizeof(double));
        capa2[i] = malloc(50 * sizeof(double));
        capa3[i] = malloc(10 * sizeof(double));
    }

    // Configurar procesos hijo
    int rows_per_process = 60000 / num_processes;
    pid_t *pids = malloc(num_processes * sizeof(pid_t));
    ChildArgs *args = malloc(num_processes * sizeof(ChildArgs));
    void **stacks = malloc(num_processes * sizeof(void *));
    double *child_times = malloc(num_processes * sizeof(double));

    // Crear procesos hijo
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_processes; i++) {
        int start_row = i * rows_per_process;
        int end_row = (i == num_processes - 1) ? 60000 : (i + 1) * rows_per_process;

        args[i].start_row = start_row;
        args[i].end_row = end_row;
        args[i].data = data + start_row;
        args[i].mat1 = mat1;
        args[i].mat2 = mat2;
        args[i].mat3 = mat3;
        args[i].mat4 = mat4;
        args[i].vec1 = vec1;
        args[i].vec2 = vec2;
        args[i].vec3 = vec3;
        args[i].vec4 = vec4;
        args[i].capa0 = capa0 + start_row;
        args[i].capa1 = capa1 + start_row;
        args[i].capa2 = capa2 + start_row;
        args[i].capa3 = capa3 + start_row;
        args[i].predicciones = predicciones + start_row;
        args[i].child_time = &child_times[i];

        void *stack = malloc(stacksize);
        if (!stack) {
            printf("Error: No se pudo asignar memoria para la pila\n");
            exit(1);
        }
        stacks[i] = stack;

        pids[i] = clone(perform_multiplications, stack + stacksize, CLONE_FLAGS, &args[i]);
        if (pids[i] == -1) {
            printf("Error: No se pudo crear el proceso hijo %d: %s\n", i, strerror(errno));
            free(stack);
            exit(1);
        }
    }

    // Esperar a los procesos hijo
    for (int i = 0; i < num_processes; i++) {
        waitpid(pids[i], NULL, 0);
        free(stacks[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calcular el tiempo total (máximo de los tiempos de los hijos)
    double max_child_time = 0;
    for (int i = 0; i < num_processes; i++) {
        if (child_times[i] > max_child_time) {
            max_child_time = child_times[i];
        }
    }

    printf("Tiempo de procesamiento paralelo (máximo): %.6f segundos\n", max_child_time);
    printf("Tiempo total (creación y espera): %.6f segundos\n", measure_time_timeval(start, end));

    // Mostrar resultados
    printf("Predicción: %d\n", predicciones[image_to_recognize]);
    show_image(renderer, data[image_to_recognize]);

    // Mantener la ventana abierta
    handle_events();

    // Liberar memoria
    for (int i = 0; i < 60000; i++) {
        free(capa0[i]);
        free(capa1[i]);
        free(capa2[i]);
        free(capa3[i]);
    }
    free(capa0);
    free(capa1);
    free(capa2);
    free(capa3);
    free(predicciones);
    free(pids);
    free(args);
    free(stacks);
    free(child_times);
    unload_data();

    // Cerrar SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}