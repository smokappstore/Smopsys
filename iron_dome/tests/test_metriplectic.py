# -*- coding: utf-8 -*-
import pytest
import numpy as np
from iron_dome.core.metriplectic_engine import MetriplecticSystem
from iron_dome.config import MetriplecticConfig

def test_metriplectic_mandate_rule_1():
    """Verify H and S competition and conservation."""
    system = MetriplecticSystem(dim=2)
    H = np.array([[1, 0], [0, -1]])
    S = np.array([[0.1, 0], [0, 0.1]])
    
    initial_trace = np.trace(system.rho)
    system.step(H, S, dt=0.01)
    final_trace = np.trace(system.rho)
    
    # Trace preservation (Conservation of probability)
    assert np.isclose(final_trace, initial_trace, atol=1e-7)

def test_metriplectic_mandate_rule_1_3():
    """Verify minimal dissipation is enforced if S is too small."""
    system = MetriplecticSystem(dim=2)
    H = np.eye(2)
    S = np.zeros((2, 2)) # Purely conservative attempt
    
    system.step(H, S, dt=0.01)
    diagnostics = system.get_diagnostics()
    
    # Metr magnitude should be at least MIN_DISSIPATION
    assert diagnostics['metr_mag'] >= MetriplecticConfig.MIN_DISSIPATION

def test_golden_operator():
    system = MetriplecticSystem(dim=2)
    val = system.golden_operator(1.0)
    # n=1: cos(pi) * cos(pi * phi) = -1 * cos(pi * phi)
    expected = np.cos(np.pi) * np.cos(np.pi * (1 + np.sqrt(5))/2)
    assert np.isclose(val, expected)
