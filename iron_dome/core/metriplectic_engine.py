# -*- coding: utf-8 -*-
"""
Metriplectic Engine for Iron Dome
Follows "El Mandato Metriplético" (Core Physics)
"""

import numpy as np
from iron_dome.config import PHI, MetriplecticConfig

class MetriplecticSystem:
    def __init__(self, dim=2):
        self.dim = dim
        self.psi = np.random.randn(dim) + 1j * np.random.randn(dim)
        self.psi /= np.linalg.norm(self.psi)
        self.rho = np.outer(self.psi, self.psi.conj())
        self.gamma = MetriplecticConfig.S_SCALE # Dissipation strength
        self.last_symp_mag = 0.0
        self.last_metr_mag = 0.0
        
    def golden_operator(self, n):
        """Regla 2.1: Fondo Estructurado (Operador Áureo)"""
        return np.cos(np.pi * n) * np.cos(np.pi * PHI * n)

    def compute_lagrangian(self, H, S):
        """Regla 3.1: Lagrangiano Explícito"""
        L_symp = H
        L_metr = S
        return L_symp, L_metr

    def step(self, H, S, dt=0.01, n=0):
        """
        Calcula d_symp = {u, H} (Rule 1.1)
        Calcula d_metr = [u, S] (Rule 1.2)
        """
        O_n = self.golden_operator(n)
        
        # Rule 1.3: Prohibición de Singularidades
        s_norm = np.linalg.norm(S)
        if s_norm < MetriplecticConfig.MIN_DISSIPATION:
            S = S + np.eye(self.dim) * MetriplecticConfig.MIN_DISSIPATION
            
        self.last_symp_mag = np.linalg.norm(H)
        self.last_metr_mag = np.linalg.norm(S)
            
        # 1. Symplectic Component (Conservative)
        # d_symp = -i[H, rho]
        comm_h = -1j * (H @ self.rho - self.rho @ H)
        
        # 2. Metric Component (Dissipative)
        # d_metr = -[S, [S, rho]] (Simplified metric bracket)
        # Or more simply for relaxation: -(S*rho + rho*S - 2*Tr(S*rho)*rho)
        # We'll use the anti-commutator form for stability
        comm_s = (S @ self.rho + self.rho @ S - 2 * np.trace(self.rho @ S) * self.rho)
        
        # Total evolution with Golden Operator modulation
        d_rho = (comm_h - self.gamma * comm_s) * (1 + 0.1 * O_n)
        
        # Euler Step
        self.rho += d_rho * dt
        
        # Stability: Ensure Hermitian and Trace-1
        self.rho = (self.rho + self.rho.conj().T) / 2.0
        tr = np.trace(self.rho).real
        if tr < 1e-10 or np.isnan(tr):
            self.rho = np.eye(self.dim, dtype=complex) / float(self.dim)
        else:
            self.rho /= tr
        
    def get_diagnostics(self):
        # Von Neumann Entropy: S = -sum(p_i * log(p_i))
        try:
            eigvals = np.linalg.eigvalsh(self.rho)
            eigvals = np.maximum(eigvals, 1e-12) # Numerical floor
            entropy = -np.sum(eigvals * np.log(eigvals))
            purity = np.sum(eigvals**2)
        except:
            entropy = 0.0
            purity = 1.0
            
        return {
            "symp_mag": self.last_symp_mag,
            "metr_mag": self.last_metr_mag,
            "entropy": float(np.real(entropy)),
            "purity": float(np.real(purity)),
            "H": self.last_symp_mag,
            "S": self.last_metr_mag
        }
