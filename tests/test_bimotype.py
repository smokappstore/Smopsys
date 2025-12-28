import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from ql.bimotype import LaserMorseSystem


def test_bimotype():
    print("=== Testing BiMOtype Encoding ===")
    bimo = LaserMorseSystem()
    msg = "SOS"
    paquete = bimo.encode_message(msg)
    
    print(f"Original: {paquete.mensaje_original}")
    print(f"Quantum States: {len(paquete.estados_cuanticos)}")
    
    header = bimo.to_c_header(paquete)
    print("\nGenerated C Header Snippet:")
    print("\n".join(header.split("\n")[:10]))
    
    assert len(paquete.estados_cuanticos) == 3
    print("\n[PASS] BiMOtype encoding verified.")

if __name__ == "__main__":
    test_bimotype()
