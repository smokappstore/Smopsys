# -*- coding: utf-8 -*-
"""
Calibration Manager for Iron Dome
Establishes the "white noise" baseline for environmental distinction.
"""

import numpy as np
import time
from iron_dome.sensors.acoustic_processor import SpectralFeatures

class CalibrationManager:
    def __init__(self, calibration_duration=5.0):
        self.duration = calibration_duration
        self.collected_features = []
        self.baseline = None
        
    def add_sample(self, features: SpectralFeatures):
        self.collected_features.append(features)
        
    def finalize(self):
        """Calculates the baseline averages from collected samples."""
        if not self.collected_features:
            return None
            
        avg_rms = np.mean([f.rms_energy for f in self.collected_features])
        avg_harmonicity = np.mean([f.harmonicity for f in self.collected_features])
        avg_centroid = np.mean([f.spectral_centroid for f in self.collected_features])
        
        self.baseline = {
            "rms_baseline": avg_rms,
            "harmonicity_baseline": avg_harmonicity,
            "centroid_baseline": avg_centroid,
            "std_rms": np.std([f.rms_energy for f in self.collected_features])
        }
        return self.baseline

    def is_calibrated(self, current_time, start_time):
        return (current_time - start_time) >= self.duration
