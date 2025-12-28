# -*- coding: utf-8 -*-
import logging
import time

class StatTracker:
    """
    Tracks True/False Positives and Negatives for system calibration analysis.
    """
    def __init__(self):
        self.counters = {
            "TP": 0, # True Positive (Threat confirmed)
            "TN": 0, # True Negative (Environment/Animal ignored)
            "FP": 0, # False Positive (Alert triggered by mistake)
            "FN": 0  # False Negative (Missed threat - rare in simulation)
        }
        self.event_log = []

    def log_event(self, target_type, result, confirmed_threat=None):
        """
        Logs a detection event.
        confirmed_threat: Bool or None if unresolved.
        """
        event = {
            "timestamp": time.time(),
            "target": target_type,
            "alert_triggered": result['alert'],
            "threat_level": result['threat_level']
        }
        
        # Determine stats
        if result['alert']:
            if "Drone" in target_type:
                self.counters["TP"] += 1
            else:
                self.counters["FP"] += 1
        else:
            if "Noise" in target_type or "Animal" in target_type or "Bird" in target_type:
                self.counters["TN"] += 1
            else:
                self.counters["FN"] += 1
                
        self.event_log.append(event)

    def get_summary(self):
        total = sum(self.counters.values())
        if total == 0: return "No data."
        
        accuracy = (self.counters["TP"] + self.counters["TN"]) / total
        return {
            "Stats": self.counters,
            "Accuracy": f"{accuracy:.2%}",
            "TotalEvents": total
        }
