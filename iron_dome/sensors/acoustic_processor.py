# -*- coding: utf-8 -*-
"""
Acoustic Processor for Iron Dome
Maps FFT features to Quantum Chemistry Hamiltonian
"""

import numpy as np
from dataclasses import dataclass
from .quantum_chemistry import (
    MolecularParameters, 
    AcousticMolecularMapping, 
    QuantumChemistryHamiltonian,
    create_hydrogen_molecule_system
)

@dataclass
class SpectralFeatures:
    dominant_freq: float
    spectral_centroid: float
    spectral_bandwidth: float
    rms_energy: float
    harmonicity: float
    timestamp: float

class AcousticProcessor:
    def __init__(self, system_type="H2"):
        # We use the user-provided logic for H2
        if system_type == "H2":
            self.mol_params = create_hydrogen_molecule_system()
        else:
            raise ValueError(f"System {system_type} not yet supported in this context")
            
        self.mapping = AcousticMolecularMapping(
            energy_to_hopping=lambda e: e * 10.0,
            freq_to_gap=lambda f: f / 500.0,
            harmonicity_to_correlation=lambda h: h * 3.0,
            bandwidth_to_spinorbit=lambda b: b / 5000.0,
            centroid_to_efield=lambda c: (c - 1000.0) / 2000.0,
            zcr_to_temperature=lambda z: z * 400.0
        )
        self.qc_hamiltonian = QuantumChemistryHamiltonian(self.mol_params)

    def analyze_audio_chunk(self, audio_data, samplerate=44100):
        """
        Mock FFT pipeline to spectral features.
        In a real scenario, this would use librosa or scipy.fft
        """
        # FFT simulation
        fft_res = np.fft.rfft(audio_data)
        freqs = np.fft.rfftfreq(len(audio_data), 1.0/samplerate)
        mags = np.abs(fft_res)
        
        dominant_freq = freqs[np.argmax(mags)]
        rms_energy = np.sqrt(np.mean(audio_data**2))
        
        # Simple centroid calculation
        centroid = np.sum(freqs * mags) / np.sum(mags) if np.sum(mags) > 0 else 0
        
        # Harmonicity proxy (ratio of peak to mean)
        harmonicity = np.max(mags) / (np.mean(mags) + 1e-6)
        harmonicity = np.clip(harmonicity / 100.0, 0, 1) # Normalize
        
        if np.sum(mags) > 0:
            avg_f2 = np.sum((freqs**2) * mags) / np.sum(mags)
            spectral_bandwidth = np.sqrt(max(0, avg_f2 - centroid**2))
        else:
            spectral_bandwidth = 0
            
        return SpectralFeatures(
            dominant_freq=dominant_freq,
            spectral_centroid=centroid,
            spectral_bandwidth=spectral_bandwidth,
            rms_energy=rms_energy,
            harmonicity=harmonicity,
            timestamp=0.0 # Will be set by main loop
        )

    def get_hamiltonian(self, features: SpectralFeatures):
        """Builds the Hamiltonian based on acoustic features"""
        acoustic_params = {
            'rms_energy': features.rms_energy,
            'dominant_freq': features.dominant_freq,
            'harmonicity': features.harmonicity,
            'spectral_bandwidth': features.spectral_bandwidth,
            'spectral_centroid': features.spectral_centroid
        }
        return self.qc_hamiltonian.build_acoustic_hamiltonian(acoustic_params, self.mapping)
