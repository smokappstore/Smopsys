# -*- coding: utf-8 -*-
"""
Iron Dome: Main System Loop
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
from iron_dome.config import PHI, MetriplecticConfig

def run_iron_dome():
    print("🛡️  Iniciando Domo de Hierro Metripléptico...")
    
    # Initialize components
    engine = MetriplecticSystem(dim=4) 
    acoustic = AcousticProcessor(system_type="H2")
    motion = MotionProcessor()
    vision = VisionProcessor()
    decision = DecisionEngine()
    stats = StatTracker()
    
    # Visualization setup
    plt.ion()
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    
    symp_vals = []
    metr_vals = []
    times = []
    
    # Calibration Phase
    print("📡 Fase de Calibración: Registrando Ruido Blanco (5s)...")
    calibration = CalibrationManager(calibration_duration=5.0)
    cal_start = time.time()
    
    while not calibration.is_calibrated(time.time(), cal_start):
        audio_data = np.random.randn(1024) * 0.1 # Real ambient noise simulation
        features = acoustic.analyze_audio_chunk(audio_data)
        calibration.add_sample(features)
        time.sleep(0.1)
        
    baseline = calibration.finalize()
    decision.set_baseline(baseline)
    print(f"✅ Calibración Completada. Baseline RMS: {baseline['rms_baseline']:.4f}")
    
    # Main loop setup
    start_time = time.time()
    n = 0
    
    try:
        while True:
            # 1. Simulate audio data (In real IoT, this is mic input)
            elapsed = time.time() - start_time
            
            # Simulate different scenarios based on time
            scenario = int(elapsed) % 40
            
            if scenario < 10:
                # Baseline
                audio_data = np.random.randn(1024) * 0.1
                mode_str = "Fondo (Calm)"
            elif scenario < 20:
                # Drone (Harmonic 1kHz - 4kHz)
                t_arr = np.linspace(0, 1024/44100, 1024)
                audio_data = 0.6 * np.sin(2 * np.pi * 2500 * t_arr) + np.random.randn(1024) * 0.05
                mode_str = "Intrusión: Drone"
            elif scenario < 30:
                # Bird (High freq, high bandwidth)
                t_arr = np.linspace(0, 1024/44100, 1024)
                audio_data = 0.4 * np.sin(2 * np.pi * 5000 * t_arr * (1 + 0.5 * np.sin(100 * t_arr)))
                mode_str = "Ambiente: Ave"
            else:
                # Cat (Low freq steps)
                t_arr = np.linspace(0, 1024/44100, 1024)
                audio_data = 0.3 * np.sin(2 * np.pi * 150 * t_arr) + np.random.randn(1024) * 0.1
                mode_str = "Movimiento: Gato"
                
            # 2. Process acoustics to Hamiltonian
            features = acoustic.analyze_audio_chunk(audio_data)
            features.timestamp = elapsed
            H = acoustic.get_hamiltonian(features)
            
            # 3. Define Entropy Potential S (related to stability)
            # S is high if the signal is non-harmonic (Rule 1.2)
            S = np.eye(H.shape[0]) * (1.0 - features.harmonicity)
            
            # 4. Engine step
            engine.step(H, S, dt=0.05, n=n)
            diagnostics = engine.get_diagnostics()
            
            # 5. Multi-Layer Evaluation
            eval_result = decision.evaluate(diagnostics, features)
            
            # 6. Sensor Confirmation Handoff
            if eval_result['state'] == "Acoustic Detection":
                # Trigger Layer 2: Motion
                confirmed, coords = motion.confirm_movement(features)
                if confirmed:
                    # Simulation update of state (normally set by DecisionEngine but here we handle the handoff)
                    eval_result['state'] = "Motion Confirmation"
                    # Trigger Layer 3: Vision (only for significant threats or persistent cats/birds)
                    vision_log = vision.start_tracking(eval_result['target_type'], coords)
                    eval_result['state'] = "Vision Tracking"
                    eval_result['vision_log'] = vision_log
            
            # Log results in StatTracker
            stats.log_event(eval_result['target_type'], eval_result)
            
            # 7. Logging and Output
            print(f"[{elapsed:.1f}s] {mode_str} | {eval_result['target_type']} | State: {eval_result['state']} | Re_psi: {eval_result['re_psi']:.2f}")
            if eval_result['alert']:
                print(f"🚨 ALERT: {eval_result['reason']}")
            elif "Animal" in eval_result['target_type'] or "Bird" in eval_result['target_type']:
                print(f"ℹ️  Info: {eval_result['reason']}")

            # Summary stats every 40s
            if int(elapsed) > 0 and int(elapsed) % 40 == 39:
                print("\n📊 --- RESUMEN DE SEGURIDAD (Layer Statistics) ---")
                summary = stats.get_summary()
                print(f"Accuracy: {summary['Accuracy']} | TP: {summary['Stats']['TP']} | FP: {summary['Stats']['FP']} | TN: {summary['Stats']['TN']}")
                print("--------------------------------------------------\n")
            
            # 7. Visualization Update
            symp_vals.append(diagnostics['symp_mag'])
            metr_vals.append(diagnostics['metr_mag'])
            times.append(elapsed)
            
            if len(times) > 50:
                times = times[-50:]
                symp_vals = symp_vals[-50:]
                metr_vals = metr_vals[-50:]
                
            ax1.clear()
            ax1.plot(times, symp_vals, 'b-', label='Conservative (H)')
            ax1.plot(times, metr_vals, 'r-', label='Dissipative (S)')
            ax1.set_title("Competencia Metripléptica (H vs S)")
            ax1.legend()
            
            ax2.clear()
            ax2.plot(times, [eval_result['threat_level'] for _ in range(len(times))], 'g-')
            ax2.set_ylim(-0.1, 1.1)
            ax2.set_title("Threat Level")
            
            plt.pause(0.1)
            n += 0.1
            
    except KeyboardInterrupt:
        print("\n🛑 Sistema detenido por el usuario.")
    finally:
        plt.ioff()
        output_path = "/home/jako/smopsys/Smopsys/iron_dome/diagnostics.png"
        plt.savefig(output_path)
        print(f"📊 Gráfica diagnóstica guardada en: {output_path}")

if __name__ == "__main__":
    run_iron_dome()
