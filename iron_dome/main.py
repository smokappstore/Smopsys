# -*- coding: utf-8 -*-
"""
Iron Dome: Main System Loop with TUI Dashboard
"""

import numpy as np
import time
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg') # Headless mode
from iron_dome.core.metriplectic_engine import MetriplecticSystem
from iron_dome.sensors.acoustic_processor import AcousticProcessor
from iron_dome.sensors.motion_processor import MotionProcessor
from iron_dome.sensors.vision_processor import VisionProcessor
from iron_dome.core.decision_engine import DecisionEngine
from iron_dome.core.calibration import CalibrationManager
from iron_dome.core.stat_tracker import StatTracker
from iron_dome.core.monitor import IronDomeMonitor
from iron_dome.config import PHI, MetriplecticConfig

def run_iron_dome():
    # Initialize components
    engine = MetriplecticSystem(dim=4) 
    acoustic = AcousticProcessor(system_type="H2")
    motion = MotionProcessor()
    vision = VisionProcessor()
    decision = DecisionEngine()
    stats = StatTracker()
    monitor = IronDomeMonitor()
    
    # Background Visualization lists
    symp_vals = []
    metr_vals = []
    threat_vals = []
    time_vals = []
    
    # 1. Calibration Phase (Minimal console output before TUI)
    print("🛡️  Iniciando Domo de Hierro Metripléptico...")
    print("📡 Fase de Calibración: Registrando Ruido Blanco (5s)...")
    calibration = CalibrationManager(calibration_duration=5.0)
    cal_start = time.time()
    
    while not calibration.is_calibrated(time.time(), cal_start):
        audio_data = np.random.randn(1024) * 0.1
        features = acoustic.analyze_audio_chunk(audio_data)
        calibration.add_sample(features)
        time.sleep(0.1)
        
    baseline = calibration.finalize()
    decision.set_baseline(baseline)
    print(f"✅ Calibración Completada. Entrando en modo Monitor...")
    time.sleep(1)

    # 2. Main Monitoring Loop with TUI
    start_time = time.time()
    n = 0
    
    with monitor.start_live() as live:
        try:
            while True:
                elapsed = time.time() - start_time
                
                # Simulate Scenarios (Cycle every 40s)
                scenario_cycle = int(elapsed) % 40
                if scenario_cycle < 10:
                    # Calm
                    t_arr = np.linspace(0, 1024/44100, 1024)
                    audio_data = np.random.randn(1024) * 0.05
                    target_label = "Fondo (Calm)"
                elif scenario_cycle < 20:
                    # Drone
                    t_arr = np.linspace(0, 1024/44100, 1024)
                    audio_data = 0.7 * np.sin(2 * np.pi * 3000 * t_arr) + np.random.randn(1024) * 0.1
                    target_label = "Intrusión: Drone"
                elif scenario_cycle < 30:
                    # Bird
                    t_arr = np.linspace(0, 1024/44100, 1024)
                    audio_data = 0.5 * np.sin(2 * np.pi * 5000 * t_arr) + np.random.randn(1024) * 0.2
                    target_label = "Ambiente: Ave"
                else:
                    # Cat
                    t_arr = np.linspace(0, 1024/44100, 1024)
                    audio_data = 0.4 * np.sin(2 * np.pi * 180 * t_arr) + np.random.randn(1024) * 0.1
                    target_label = "Movimiento: Gato"

                # Process Layer 1: Acoustic
                features = acoustic.analyze_audio_chunk(audio_data)
                features.timestamp = elapsed
                
                # Update Metriplectic Dynamics
                H = acoustic.get_hamiltonian(features)
                S = np.eye(H.shape[0]) * (1.0 - features.harmonicity)
                
                engine.step(H, S, dt=0.05, n=n)
                diagnostics = engine.get_diagnostics()
                
                # Evaluate Decision engine
                eval_result = decision.evaluate(diagnostics, features)
                
                # Layer 2 & 3: Motion & Vision Handoff
                if eval_result['state'] == "Acoustic Detection":
                    confirmed, coords = motion.confirm_movement(features)
                    if confirmed:
                        eval_result['state'] = "Motion Confirmation"
                        vision_log = vision.start_tracking(eval_result['target_type'], coords)
                        eval_result['state'] = "Vision Tracking"
                
                # Track statistics
                stats.log_event(eval_result['target_type'], eval_result)
                
                # Update TUI
                log_msg = eval_result['reason'] if eval_result['alert'] else f"Monitoring {target_label}"
                monitor.add_event(eval_result['target_type'], eval_result['state'], log_msg)
                
                stats_summary = stats.get_summary()
                monitor.update_view(diagnostics, stats_summary, eval_result.get('threat_level', 0))
                
                # Background plot update
                symp_vals.append(diagnostics['symp_mag'])
                metr_vals.append(diagnostics['metr_mag'])
                threat_vals.append(eval_result.get('threat_level', 0))
                time_vals.append(elapsed)
                
                if len(time_vals) > 50:
                    symp_vals.pop(0)
                    metr_vals.pop(0)
                    threat_vals.pop(0)
                    time_vals.pop(0)
                
                n += 1
                time.sleep(0.1)

        except KeyboardInterrupt:
            pass
        finally:
            # Save final background plot
            plt.clf()
            fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
            ax1.plot(time_vals, symp_vals, 'b-', label='Conservative (H)')
            ax1.plot(time_vals, metr_vals, 'r-', label='Dissipative (S)')
            ax1.set_title("Competencia Metripléptica (H vs S)")
            ax1.legend()
            
            ax2.plot(time_vals, threat_vals, 'g-')
            ax2.set_ylim(-0.1, 1.1)
            ax2.set_title("Threat Level Historical")
            
            output_path = "/home/jako/smopsys/Smopsys/iron_dome/diagnostics.png"
            plt.savefig(output_path)

if __name__ == "__main__":
    run_iron_dome()
