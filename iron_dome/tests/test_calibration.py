# -*- coding: utf-8 -*-
import pytest
import numpy as np
from iron_dome.core.calibration import CalibrationManager
from iron_dome.sensors.acoustic_processor import SpectralFeatures
from iron_dome.core.decision_engine import DecisionEngine

def test_calibration_baseline():
    cal = CalibrationManager(calibration_duration=1.0)
    
    # Simulate consistent noise
    for _ in range(10):
        features = SpectralFeatures(
            dominant_freq=100,
            spectral_centroid=500,
            spectral_bandwidth=100,
            rms_energy=0.1,
            harmonicity=0.2,
            timestamp=0
        )
        cal.add_sample(features)
        
    baseline = cal.finalize()
    assert np.isclose(baseline['rms_baseline'], 0.1)
    assert np.isclose(baseline['std_rms'], 0.0)

def test_decision_with_baseline():
    decision = DecisionEngine()
    baseline = {
        'rms_baseline': 0.1,
        'std_rms': 0.01,
        'harmonicity_baseline': 0.2,
        'centroid_baseline': 500
    }
    decision.set_baseline(baseline)
    
    # Test 1: Noise slightly above baseline should NOT trigger alert
    features_low = SpectralFeatures(
        dominant_freq=100, spectral_centroid=500, spectral_bandwidth=100,
        rms_energy=0.12, harmonicity=0.2, timestamp=0
    )
    diag = {'symp_mag': 5.0, 'metr_mag': 1.0, 'entropy': 0.5}
    res_low = decision.evaluate(diag, features_low)
    assert res_low['alert'] is False
    
    # Test 2: Sound significantly above baseline (3 * std_rms + 0.1 buffer) SHOULD trigger
    # Threshold defined as baseline + max(0.1, 3 * std_rms) = 0.1 + 0.1 = 0.2
    features_high = SpectralFeatures(
        dominant_freq=2500, spectral_centroid=1000, spectral_bandwidth=200,
        rms_energy=0.3, harmonicity=0.8, timestamp=0
    )
    # Target re_psi = symp/metr = 15.0 / 1.0 = 15.0 (> 10)
    diag_high = {'symp_mag': 15.0, 'metr_mag': 1.0, 'entropy': 0.5}
    res_high = decision.evaluate(diag_high, features_high)
    assert res_high['alert'] is True
    assert res_high['target_type'] == "Drone Motor"

def test_animal_distinction():
    decision = DecisionEngine()
    # Mock baseline to 0 to avoid threshold blockage in test
    decision.set_baseline({'rms_baseline': 0.0, 'std_rms': 0.0, 'harmonicity_baseline': 0, 'centroid_baseline': 0})
    # Bird signature (High freq, high bandwidth)
    features_bird = SpectralFeatures(
        dominant_freq=5000, spectral_centroid=5000, spectral_bandwidth=1500,
        rms_energy=0.3, harmonicity=0.4, timestamp=0
    )
    diag = {'symp_mag': 10.0, 'metr_mag': 1.0, 'entropy': 0.5}
    res_bird = decision.evaluate(diag, features_bird)
    # Birds should not trigger alert in non-aggressive dome
    assert res_bird['alert'] is False
    assert res_bird['target_type'] == "Bird / Avian"

    # Cat signature (Low freq, low harmonicity)
    features_cat = SpectralFeatures(
        dominant_freq=200, spectral_centroid=300, spectral_bandwidth=100,
        rms_energy=0.25, harmonicity=0.1, timestamp=0
    )
    res_cat = decision.evaluate(diag, features_cat)
    assert res_cat['alert'] is False
    assert res_cat['target_type'] == "Cat / Small Animal"
