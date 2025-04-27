# DigitRecognizer

## Descripción
**DigitRecognizer** es un programa en C que reconoce dígitos usando una red neuronal densa de 4 capas.  
Procesa **60000 imágenes** (28x28 píxeles) y predice dígitos (0-9) usando pesos y sesgos cargados de archivos CSV, ReLU y argmax.  
Incluye una versión secuencial y una paralelizada usando `clone()`.

Desarrollado por **David Sánchez** y **Hugo Kowalski**.

## Requisitos
- Linux (probado en ArchLinux y Ubuntu)
- GCC
- SDL2 (`libsdl2-dev`)

### Instalar SDL2
```
sudo apt-get install libsdl2-dev
```

### Compilación

```
git clone https://github.com/dsanchez062/DigitRecognizer.git
cd DigitRecognizer
gcc -Wall -o DigitRecognizer_parallelized DigitRecognizer_parallelized.c -lSDL2
```

### Ejecución

**Paralelizado**:
```
./DigitRecognizer_parallelized <num_processes> <image_to_recognize>
```

**Secuencial**:
```
./DigitRecognizer_sequential <image_to_recognize>
```
