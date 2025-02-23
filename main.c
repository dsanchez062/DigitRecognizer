#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>  // file handling functions
#include <stdlib.h> // atoi
#include <string.h> // strtok
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
        char *record = strtok(buffer, ",");  // Suponiendo que las columnas están separadas por comas
        int column = 0;

        // Leer cada valor dentro de la fila
        while (record != NULL && column < ncols) {
            aux = strtod(record, NULL) * (float)fac;
            mat[row][column] = aux;
            record = strtok(NULL, ",");  // Leer siguiente valor de la fila
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

int main(int argc, char *argv[]) {
    /*
     * El programa recibe un único argumento, la cantidad de procesos que se
     * emplearán en la paralelización. Por ejemplo, parallel 3 tendrá que dividir
     * la matriz en tres, y lanzar tres procesos paralelos. Cada proceso, deberá
     * procesar un tercio de la matriz de datos
     */

    if (argc != 2) {
        printf("El programa debe tener un único argumento, la cantidad de procesos que se van a generar\n");
        exit(1);
    }

    load_data(my_path);
    
    unload_data();
    return 0;
}
