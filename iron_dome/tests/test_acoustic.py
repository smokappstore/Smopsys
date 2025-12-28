# -*- coding: utf-8 -*-
import pytest
import numpy as np
from iron_dome.sensors.acoustic_processor import AcousticProcessor

def test_acoustic_to_hamiltonian_mapping():
    processor = AcousticProcessor(system_type="H2")
    
    # Simulate a 440Hz tone
    t = np.linspace(0, 0.1, 4410)
    audio = 0.5 * np.sin(2 * np.pi * 440 * t)
    
    features = processor.analyze_audio_chunk(audio)
    assert np.isclose(features.dominant_freq, 440, atol=10)
    
    H = processor.get_hamiltonian(features)
    # H2 has 2 orbitals -> Hilbert dim is 2^2 = 4
    assert H.shape == (4, 4)
    # H must be Hermitian
    assert np.allclose(H, H.conj().T)

def test_harmonicity_proxy():
    processor = AcousticProcessor(system_type="H2")
    
    # Pure tone (High harmonicity)
    t = np.linspace(0, 0.1, 4410)
    pure_tone = np.sin(2 * np.pi * 440 * t)
    features_pure = processor.analyze_audio_chunk(pure_tone)
    
    # White noise (Low harmonicity)
    noise = np.random.randn(4410)
    features_noise = processor.analyze_audio_chunk(noise)
    
    assert features_pure.harmonicity > features_noise.harmonicity
