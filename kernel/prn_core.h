#ifndef PRN_CORE_H
#define PRN_CORE_H

#include <stdint.h>

// Constantes Físicas y Matemáticas (Formato Punto Fijo 16.16)
// 1.61803398875 * 65536 = 106039.3
#define PHI_FIXED       106039
// 0.8 * 65536 = 52428.8
#define ENTROPY_THRESH  52428
#define WINDOW_SIZE     256

// Simple fixed point complex number
typedef struct {
    int32_t real;
    int32_t imag;
} complex_fixed_t;

typedef struct {
    // Estado Bayesiano (16.16 fixed point)
    int32_t coherence_weight;
    int32_t entropy_weight;
    int32_t noise_influence;
    
    // Historial para análisis de tendencias (Buffer circular simplificado)
    int32_t entropy_history[16];
    int32_t coherence_history[16];
    uint8_t history_idx;

    // Buffer de señal
    complex_fixed_t spectral_buffer[WINDOW_SIZE];
} PRN_State;

// Funciones en Assembly (Optimizadas con SIMD)
// Aplica una transformación no lineal para "estirar" picos y suprimir ruido
extern void asm_spectral_sharpen(complex_fixed_t* buffer, uint32_t size, int32_t threshold);

// Calcula entropía aproximada usando logaritmos enteros/tablas
extern int32_t asm_calculate_entropy_fast(complex_fixed_t* buffer, uint32_t size);

// Ejecuta paso de mariposa o FFT completa (dependiendo de la implementación asm)
extern void asm_fft_butterfly(complex_fixed_t* buffer, uint32_t size);

// C-API
void PRN_Init(void);
int32_t PRN_ProcessSignal(int32_t* signal_data, uint32_t size);
// Ingest a single sample, buffering until window size is reached, then process
void PRN_InputSample(int32_t sample);

#endif
