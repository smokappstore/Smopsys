// Definición de la estructura Matrix2x2 para representar matrices 2x2
struct Matrix2x2 {
    double m[2][2]; // Matriz 2x2

    // Sobrecarga del operador para multiplicación de matrices
    Matrix2x2 operator*(const Matrix2x2& other) const {
        Matrix2x2 result;
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                result.m[i][j] = m[i][0] * other.m[0][j] + m[i][1] * other.m[1][j];
            }
        }
        return result;
    }

    // Método para calcular el adjunto de la matriz (transpuesta conjugada)
    Matrix2x2 adjoint() const {
        Matrix2x2 result;
        result.m[0][0] = m[0][0];    // C[0][0] = C[0][0] (asumiendo elementos reales)
        result.m[0][1] = m[1][0];    // C[0][1] = C[1][0]
        result.m[1][0] = m[0][1];    // C[1][0] = C[0][1]
        result.m[1][1] = m[1][1];    // C[1][1] = C[1][1]
        return result;
    }
};

// Definición de la función calculate_L
Matrix2x2 calculate_L(const Matrix2x2& rho_t, double Re_measured, double OTOC_measured) {
    // 1. Definir el estado ideal/objetivo basado en tu operador áureo (Meq en tu JS)
    double target_mass = Meq; 

    // 2. Medir la desviación actual del sistema
    double reynolds_threshold = 2300.0; // UMBRAL LAMINAR/TURBULENTO
    double chaos_threshold = 0.5; // UMBRAL CAOS CUANTICO (ejemplo)

    double re_deviation = Re_measured - reynolds_threshold;
    double otoc_deviation = OTOC_measured - chaos_threshold;

    // 3. Calcular un "Factor de Amortiguamiento Metripléctico"
    double damping_factor = 0.0;
    if (re_deviation > 0 || otoc_deviation > 0) {
        damping_factor = std::max(re_deviation / reynolds_threshold, otoc_deviation / chaos_threshold);
    }

    // 4. Definir el operador 'C' (aplicacion fisica)
    Matrix2x2 C; 
    C.m[0][0] = 1.0; // Asignar valores según el operador deseado
    C.m[0][1] = 0.0;
    C.m[1][0] = 0.0;
    C.m[1][1] = 1.0;

    // 5. Construir el Superoperador de Lindblad (L)
    Matrix2x2 C_dagger = C.adjoint(); // Obtener el adjunto de C

    // Calcular el término disipatvo
    Matrix2x2 dissipative_term = (C * rho_t * C_dagger) - (0.5 * (C_dagger * C * rho_t + rho_t * C_dagger * C));

    // Aplicar el factor de amortiguamiento
    dissipative_term = damping_factor * dissipative_term;

    return dissipative_term;
}

