# -*- coding: utf-8 -*-
import pytest
import numpy as np
from iron_dome.core.decision_engine import DecisionEngine, TrackingState, TargetType
from iron_dome.sensors.acoustic_processor import SpectralFeatures
from iron_dome.core.stat_tracker import StatTracker
from iron_dome.sensors.motion_processor import MotionProcessor
from iron_dome.sensors.vision_processor import VisionProcessor

def test_pipeline_handoff():
    decision = DecisionEngine()
    motion = MotionProcessor()
    vision = VisionProcessor()
    stats = StatTracker()
    
    # Pre-set baseline to allow classification
    decision.set_baseline({'rms_baseline': 0.1, 'std_rms': 0.01, 'harmonicity_baseline': 0.2, 'centroid_baseline': 500})
    
    # 1. Simulate Drone (Acoustic Layer)
    drone_features = SpectralFeatures(
        dominant_freq=2500, spectral_centroid=2500, spectral_bandwidth=200,
        rms_energy=0.5, harmonicity=0.8, timestamp=0
    )
    diag = {'symp_mag': 15.0, 'metr_mag': 1.0, 'entropy': 0.5}
    
    # Acoustic evaluation
    res = decision.evaluate(diag, drone_features)
    assert res['target_type'] == "Drone Motor"
    assert res['state'] == "Vision Tracking" # Because it's a threat
    
    # Motion confirmation (Layer 2)
    confirmed, coords = motion.confirm_movement(drone_features)
    assert confirmed is True
    
    # Vision tracking (Layer 3)
    vision_log = vision.start_tracking(res['target_type'], coords)
    assert "Focused" in vision_log
    
    # Stats logging
    stats.log_event(res['target_type'], res)
    summary = stats.get_summary()
    assert summary['Stats']['TP'] == 1

def test_animal_ignored_by_layers():
    decision = DecisionEngine()
    stats = StatTracker()
    
    # Pre-set baseline
    decision.set_baseline({'rms_baseline': 0.1, 'std_rms': 0.01, 'harmonicity_baseline': 0.2, 'centroid_baseline': 200})
    
    # Simulate Cat (Acoustic Layer)
    cat_features = SpectralFeatures(
        dominant_freq=200, spectral_centroid=300, spectral_bandwidth=100,
        rms_energy=0.25, harmonicity=0.1, timestamp=0
    )
    diag = {'symp_mag': 5.0, 'metr_mag': 1.0, 'entropy': 0.5}
    
    res = decision.evaluate(diag, cat_features)
    assert res['target_type'] == "Cat / Small Animal"
    assert res['alert'] is False
    assert res['state'] == "Motion Confirmation" # Animals trigger motion but not vision tracking alert
    
    stats.log_event(res['target_type'], res)
    summary = stats.get_summary()
    assert summary['Stats']['TN'] == 1
    assert summary['Stats']['TP'] == 0
