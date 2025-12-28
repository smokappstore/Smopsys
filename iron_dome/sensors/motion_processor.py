# -*- coding: utf-8 -*-
import numpy as np

class MotionProcessor:
    """
    Mock Motion Processor for Layer 2 confirmation.
    Simulates triangulation and movement detection.
    """
    def __init__(self):
        self.last_coordinates = (0.0, 0.0)
        
    def confirm_movement(self, acoustic_signature, source_direction=None):
        """
        Confirms if there is physical movement corresponding to the acoustic source.
        In a real system, this would interface with PIR or Radar sensors.
        """
        # Mock logic: If energy is above 0.2, we "detect" motion
        if acoustic_signature.rms_energy > 0.15:
            # Simulate a coordinate based on the frequency as a mock seed
            x = np.cos(acoustic_signature.dominant_freq) * 10.0
            y = np.sin(acoustic_signature.dominant_freq) * 10.0
            self.last_coordinates = (x, y)
            return True, self.last_coordinates
            
        return False, None
