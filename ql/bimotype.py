import json
import time
import logging
from typing import Dict, List, Union, Any, Optional
from dataclasses import dataclass
from enum import Enum
import numpy as np

# Configuración de logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class TipoDecaimiento(Enum):
    """Tipos de decaimiento radiactivo (Opcional para BiMOtype)."""
    ALPHA = "alpha"
    BETA = "beta"
    GAMMA = "gamma"
    FISSION = "fission"
    ELECTRON_CAPTURE = "electron_capture"
    NONE = "none"

@dataclass
class EstadoComplejo:
    """Representa un estado cuántico complejo."""
    alpha: complex
    beta: complex
    
    def normalize(self):
        """Normaliza el estado cuántico."""
        norm = np.sqrt(abs(self.alpha)**2 + abs(self.beta)**2)
        if norm > 0:
            self.alpha /= norm
            self.beta /= norm
    
    def validate(self) -> bool:
        """Valida que el estado sea físicamente válido."""
        return abs(abs(self.alpha)**2 + abs(self.beta)**2 - 1.0) < 1e-10

@dataclass
class FirmaRadiactiva:
    """Firma radiactiva opcional (Analogía a Hawking)."""
    isotopo: str = "vacuum"
    energia_pico_ev: float = 0.0
    tipo_decaimiento: TipoDecaimiento = TipoDecaimiento.NONE
    vida_media_s: float = float('inf')
    spin_nuclear: float = 0.0
    mahalanobis_distance: Optional[float] = None
    
    def validate(self) -> bool:
        return True

@dataclass
class QuantumRadiationState:
    """Estado cuántico-radiactivo unificado."""
    isotope: str
    energy_level: float
    decay_rate: float
    spin_state: EstadoComplejo
    entanglement_phase: float
    coherence_time: float
    firma_radiactiva: Optional[FirmaRadiactiva] = None
    timestamp: float = None
    
    def __post_init__(self):
        if self.timestamp is None:
            self.timestamp = time.time()
        self.spin_state.normalize()
    
    def validate(self) -> bool:
        return self.spin_state.validate()

@dataclass
class PaqueteBiMoType:
    """Paquete BiMOtype que contiene el mensaje codificado."""
    mensaje_original: str
    timestamp: float
    estados_cuanticos: List[QuantumRadiationState]
    metadatos: Dict[str, Any]
    checksum: Optional[str] = None
    
    def __post_init__(self):
        if self.checksum is None:
            self.checksum = self._calcular_checksum()
    
    def _calcular_checksum(self) -> str:
        data = f"{self.mensaje_original}{self.timestamp}{len(self.estados_cuanticos)}"
        return str(hash(data))

class LaserMorseSystem:
    """
    Sistema BiMOtype: Pulsos láser -> Morse -> Quantum States.
    """
    
    MORSE_TABLE = {
        'A': '.-', 'B': '-...', 'C': '-.-.', 'D': '-..', 'E': '.', 'F': '..-.',
        'G': '--.', 'H': '....', 'I': '..', 'J': '.---', 'K': '-.-', 'L': '.-..',
        'M': '--', 'N': '-.', 'O': '---', 'P': '.--.', 'Q': '--.-', 'R': '.-.',
        'S': '...', 'T': '-', 'U': '..-', 'V': '...-', 'W': '.--', 'X': '-..-',
        'Y': '-.--', 'Z': '--..', '0': '-----', '1': '.----', '2': '..---',
        '3': '...--', '4': '....-', '5': '.....', '6': '-....', '7': '--...',
        '8': '---..', '9': '----.', ' ': '/'
    }

    # Mapeo Morse-Cuántico simplificado (sin firmas obligatorias)
    MORSE_QUANTUM_MAP = {
        'A': {'morse': '.-', 'phase': np.pi/4, 'isotope': 'U235'},
        'B': {'morse': '-...', 'phase': np.pi, 'isotope': 'Pu239'},
        'C': {'morse': '-.-.', 'phase': 3*np.pi/4, 'isotope': 'Sr90'},
        'D': {'morse': '-..', 'phase': np.pi/2, 'isotope': 'Co60'},
        'E': {'morse': '.', 'phase': 0, 'isotope': 'Cs137'},
        ' ': {'morse': '/', 'phase': 0, 'isotope': 'vacuum'}
    }

    def __init__(self):
        logger.info("Sistema BiMOtype inicializado")

    def encode_message(self, message: str) -> PaqueteBiMoType:
        timestamp = time.time()
        estados = []
        
        for char in message.upper():
            if char in self.MORSE_TABLE:
                morse = self.MORSE_TABLE[char]
                # Simular estado cuántico basado en la fase del mapeo
                # Para caracteres no en MORSE_QUANTUM_MAP, usamos un default
                map_data = self.MORSE_QUANTUM_MAP.get(char, {'phase': np.random.random() * 2 * np.pi, 'isotope': 'unknown'})
                
                phase = map_data['phase']
                spin = EstadoComplejo(alpha=complex(np.cos(phase/2), 0), beta=complex(np.sin(phase/2), 0))
                
                state = QuantumRadiationState(
                    isotope=map_data['isotope'],
                    energy_level=1.0,
                    decay_rate=0.1,
                    spin_state=spin,
                    entanglement_phase=phase,
                    coherence_time=100.0
                )
                estados.append(state)
        
        return PaqueteBiMoType(
            mensaje_original=message,
            timestamp=timestamp,
            estados_cuanticos=estados,
            metadatos={"version": "1.0.0-BiMO"}
        )

    def to_c_header(self, paquete: PaqueteBiMoType) -> str:
        """Genera un array de C para el kernel."""
        c_code = "/* BiMOtype Encoded Message */\n"
        c_code += f"#define BIMOTYPE_MSG_LEN {len(paquete.estados_cuanticos)}\n\n"
        c_code += "typedef struct {\n"
        c_code += "    float phase;\n"
        c_code += "    int is_dash;\n"
        c_code += "} BiMOPulse;\n\n"
        c_code += "BiMOPulse bimotype_sequence[] = {\n"
        
        for state in paquete.estados_cuanticos:
            # Simplificamos a pulsos Morse para el parpadeo
            char = state.isotope # Dummy check
            morse = self.MORSE_TABLE.get(paquete.mensaje_original[paquete.estados_cuanticos.index(state)].upper(), ".")
            for m in morse:
                is_dash = 1 if m == '-' else 0
                c_code += f"    {{ {state.entanglement_phase:.4f}f, {is_dash} }},\n"
        
        c_code += "};\n"
        return c_code
