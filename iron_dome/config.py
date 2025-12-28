# -*- coding: utf-8 -*-
"""
Iron Dome: Global Configuration
Defining the metric and symplectic constants for the system.
"""

import numpy as np

# Golden Operator (O_n) Constants
PHI = (1 + np.sqrt(5)) / 2
PI = np.pi

# Metriplectic Physics Configuration
class MetriplecticConfig:
    # Symplectic (Conservative) scale
    H_SCALE = 1.0
    # Metric (Dissipative) scale
    S_SCALE = 0.1
    # Prohibit pure systems (Rule 1.3)
    MIN_DISSIPATION = 1e-4

# IoT Sensor Thresholds
class SensorConfig:
    # Acoustic RMS threshold for target distinction
    ACOUSTIC_NR_THRESHOLD = 0.6  # Normalized Reynolds number proxy
    # Minimum frequency for alerts
    MIN_FREQ_ALERT = 20.0  # Hz
    # Maximum frequency
    MAX_FREQ_ALERT = 20000.0  # Hz

# Deployment Settings
DEBUG = True
SAVE_LOGS = True
LOG_DIR = "logs/"
