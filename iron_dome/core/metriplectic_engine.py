# -*- coding: utf-8 -*-
"""
Metriplectic Engine for Iron Dome
Follows "El Mandato Metriplético" (Core Physics)
"""

import numpy as np
import matplotlib.pyplot as plt
from iron_dome.config import PHI, MetriplecticConfig

class MetriplecticSystem:
    def __init__(self, dim=2):
        self.dim = dim
        self.psi = np.random.randn(dim) + 1j * np.random.randn(dim)
        self.psi /= np.linalg.norm(self.psi)
        self.rho = np.outer(self.psi, self.psi.conj())
        self.v = np.zeros(dim)
        
    def golden_operator(self, n):
        """Regla 2.1: Fondo Estructurado (Operador Áureo)"""
        return np.cos(np.pi * n) * np.cos(np.pi * PHI * n)

    def compute_lagrangian(self, H, S):
        """Regla 3.1: Lagrangiano Explícito"""
        # L_symp generates conservative motion (Reversible)
        L_symp = H
        # L_metr generates relaxation towards an attractor (Dissipative)
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
            # Inject minimal dissipation to prevent numerical explosion
            S = S + np.eye(self.dim) * MetriplecticConfig.MIN_DISSIPATION
            
        # Rule 3.3: Visualización Diagnóstica (Internal tracking)
        # Tracking after rule 1.3 check to reflect minimal dissipation
        self.last_symp_mag = np.linalg.norm(H)
        self.last_metr_mag = np.linalg.norm(S)
            
        # Simplificatic integration for demonstration
        # Heisenberg/Schrodinger like for H (Symplectic)
        # Gradient flow like for S (Metric)
        
        # Hamiltonian update (Unitary-ish)
        dH = -1j * (H @ self.rho - self.rho @ H)
        
        # Entropy update (Dissipative)
        dS = -(S @ self.rho + self.rho @ S - 2 * np.trace(self.rho @ S) * self.rho)
        
        # Structural vacuum modulation
        self.rho += (MetriplecticConfig.H_SCALE * dH + MetriplecticConfig.S_SCALE * dS) * dt * (1 + O_n)
        
        # Re-normalize to preserve probability (Rule 1.1)
        self.rho /= np.trace(self.rho)
        
    def get_diagnostics(self):
        return {
            "symp_mag": self.last_symp_mag,
            "metr_mag": self.last_metr_mag,
            "entropy": -np.real(np.trace(self.rho @ np.log(self.rho + 1e-12))),
            "purity": np.real(np.trace(self.rho @ self.rho))
        }
