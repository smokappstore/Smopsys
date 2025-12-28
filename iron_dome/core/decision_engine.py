# -*- coding: utf-8 -*-
"""
Decision Engine for Iron Dome
Distinguishes Target vs Non-Target using Metriplectic diagnostics.
"""

import numpy as np
from enum import Enum
from iron_dome.config import SensorConfig

class TargetType(Enum):
    UNKNOWN = "Unknown Ambient"
    DRONE = "Drone Motor"
    BIRD = "Bird / Avian"
    CAT = "Cat / Small Animal"
    DOG = "Dog / Domestic"
    SQUIRREL = "Squirrel / Wild"
    NOISE = "Baseline Noise"
    HUMAN = "Human Intruder"

class TrackingState(Enum):
    IDLE = "Idle"
    ACOUSTIC_DETECTION = "Acoustic Detection"
    MOTION_CONFIRMATION = "Motion Confirmation"
    VISION_TRACKING = "Vision Tracking"
    RESOLVED = "Resolved"

# Branching logic
THREAT_BRANCH = [TargetType.DRONE, TargetType.HUMAN, TargetType.UNKNOWN]
ANIMAL_BRANCH = [TargetType.BIRD, TargetType.CAT, TargetType.DOG, TargetType.SQUIRREL]

class DecisionEngine:
    def __init__(self):
        self.alert_history = []
        self.baseline = None
        self.current_state = TrackingState.IDLE
        self.active_target = None
        
    def set_baseline(self, baseline):
        self.baseline = baseline
        
    def evaluate(self, diagnostics, spectral_features):
        """
        Evaluates system state to determine if an alert is needed.
        Rule 3.3: Competition between conservative (symp) and dissipative (metr).
        """
        symp = diagnostics['symp_mag']
        metr = diagnostics['metr_mag']
        
        # Reynolds Informacional (Re_psi) Proxy
        # High Re_psi indicates potential turbulence/uncontrolled information (threat)
        re_psi = symp / (metr + 1e-9)
        
        # Determination logic
        # 1. High energy + High Entropy change -> Target (Unexpected event)
        # 2. Stable H + Low S change -> Non-Target (Ambient noise)
        
        is_target = False
        threat_level = 0.0
        target_type = TargetType.NOISE
        
        if self.baseline:
            rms_threshold = self.baseline['rms_baseline'] + max(0.1, 3 * self.baseline['std_rms'])
        else:
            rms_threshold = SensorConfig.ACOUSTIC_NR_THRESHOLD
            
        if spectral_features.rms_energy > rms_threshold:
            # 1. Acoustic Detection
            target_type = self._classify(spectral_features, re_psi, diagnostics)
            self.current_state = TrackingState.ACOUSTIC_DETECTION
            self.active_target = target_type
            
            # 2. Layer Analysis
            is_threat = target_type in THREAT_BRANCH
            
            if target_type == TargetType.DRONE:
                is_target = True
                threat_level = 0.9
            elif target_type in ANIMAL_BRANCH:
                is_target = False
                threat_level = 0.2
            else:
                if re_psi >= 15.0:
                    is_target = True
                    threat_level = 0.6
        else:
            if re_psi > 20.0:
                 target_type = self._classify(spectral_features, re_psi, diagnostics)
                 self.active_target = target_type
                 
        # Tracking State Logic
        if is_target:
            self.current_state = TrackingState.VISION_TRACKING
        elif target_type != TargetType.NOISE:
            self.current_state = TrackingState.MOTION_CONFIRMATION
        else:
            self.current_state = TrackingState.IDLE

        reason = self._generate_reason(is_target, target_type, re_psi, diagnostics['entropy'])
        
        result = {
            "alert": is_target,
            "threat_level": threat_level,
            "target_type": target_type.value,
            "re_psi": re_psi,
            "state": self.current_state.value,
            "reason": reason
        }
        
        if is_target:
            self.alert_history.append(result)
            
        return result

    def _classify(self, features, re_psi, diagnostics):
        # Heuristic signatures
        freq = features.dominant_freq
        harm = features.harmonicity
        bw = features.spectral_bandwidth
        
        # 1. Drone: Stable high-freq whine, high harmonicity
        if 1000 <= freq <= 6000 and harm > 0.6:
            return TargetType.DRONE
            
        # 2. Bird: Very high freq chirps, high bandwidth
        if freq > 3000 and bw > 1000:
            return TargetType.BIRD
            
        # 3. Cat/Small animal: Low freq impact, low harmonicity
        if freq < 800 and harm < 0.3:
            return TargetType.CAT
            
        # 4. Dog: Medium-low freq, bursty
        if 400 <= freq <= 1200 and harm < 0.4:
            return TargetType.DOG

        return TargetType.UNKNOWN

    def _generate_reason(self, is_target, target_type, re_psi, entropy):
        if not is_target:
            if target_type in ANIMAL_BRANCH:
                return f"Detected {target_type.value}: Dynamic ignored (Non-threat animal)."
            return "Ambient noise within laminar flow limits."
        
        main_prefix = f"ALERT ({self.current_state.value}) - Object: {target_type.value}. "
        
        if re_psi > 50.0:
            return main_prefix + "High Informational Reynolds: Structured drone-like approach."
        if entropy > 0.9:
            return main_prefix + "Metriplectic Instability detected."
        return main_prefix + "Displacement in conservative-dissipative balance."
