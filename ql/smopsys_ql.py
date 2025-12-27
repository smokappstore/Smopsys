import re
from typing import List, Dict, Tuple, Optional, Any
from dataclasses import dataclass
from enum import Enum, auto
import json
import sys

# ============================================================
# TOKENS Y ENUMERACIONES
# ============================================================

class TokenType(Enum):
    # Palabras clave
    PULSE = auto()
    WAIT = auto()
    MEASURE = auto()
    ENTANGLE = auto()
    BROADCAST = auto()
    THERMAL = auto()
    SYNC = auto()
    IF = auto()
    ELSE = auto()
    LOOP = auto()
    BREAK = auto()
    FUNCTION = auto()
    RETURN = auto()
    
    # Identificadores y literales
    IDENTIFIER = auto()
    NUMBER = auto()
    STRING = auto()
    WAVELENGTH = auto()  # 1550nm, 405nm
    DURATION = auto()     # 100ns, 1us
    POLARIZATION = auto() # H, V, D, L
    
    # Operadores
    PLUS = auto()
    MINUS = auto()
    MULTIPLY = auto()
    DIVIDE = auto()
    EQUAL = auto()
    NOT_EQUAL = auto()
    LESS = auto()
    GREATER = auto()
    ASSIGN = auto()
    
    # Delimitadores
    LPAREN = auto()
    RPAREN = auto()
    LBRACE = auto()
    RBRACE = auto()
    COMMA = auto()
    SEMICOLON = auto()
    COLON = auto()
    
    # Especiales
    EOF = auto()
    NEWLINE = auto()

@dataclass
class Token:
    type: TokenType
    value: Any
    line: int
    column: int

# ============================================================
# INSTRUCCIONES COMPILADAS
# ============================================================

class InstructionType(Enum):
    PULSE_LASER = auto()
    WAIT = auto()
    MEASURE = auto()
    ENTANGLE = auto()
    BROADCAST = auto()
    THERMAL_PAGE = auto()
    SYNC_PHASE = auto()
    JUMP = auto()
    CONDITIONAL = auto()
    CALL = auto()

@dataclass
class Instruction:
    """Instrucción compilada"""
    type: InstructionType
    operands: Dict[str, Any]
    opcode: int = 0  # Para enviar al kernel
    laser_params: Optional[Dict] = None
    quantum_state: Optional[Dict] = None
    
    def to_c_code(self) -> str:
        """Genera código C para el kernel"""
        if self.type == InstructionType.PULSE_LASER:
            wavelength = self.operands.get('wavelength', '1550nm')
            duration = self.operands.get('duration', '100ns')
            polarization = self.operands.get('polarization', 'H')
            return f"laser_pulse_emit(\"{wavelength}\", \"{duration}\", '{polarization}');"
        
        elif self.type == InstructionType.WAIT:
            time_ns = self.operands.get('time', 0)
            return f"busy_wait_ns({int(time_ns)});"
        
        elif self.type == InstructionType.MEASURE:
            qubit = self.operands.get('qubit', 'q0')
            return f"measure_qubit(\"{qubit}\");"
        
        elif self.type == InstructionType.BROADCAST:
            message = self.operands.get('message', '')
            return f"serial_putstr(\"{message}\\n\");"
        
        elif self.type == InstructionType.THERMAL_PAGE:
            addr = self.operands.get('address', 0)
            threshold = self.operands.get('threshold', 0.7)
            return f"check_thermal_page(0x{addr:x}, {threshold});"
        
        elif self.type == InstructionType.SYNC_PHASE:
            phase = self.operands.get('phase', 0)
            return f"sync_metriplectc_phase({phase});"
        
        return "// Unknown instruction"
    
    def to_json(self) -> Dict:
        """Serializa a JSON para enviar al kernel"""
        return {
            'type': self.type.name,
            'operands': self.operands,
            'opcode': self.opcode,
            'laser_params': self.laser_params,
            'quantum_state': self.quantum_state
        }

# ============================================================
# LEXER (Tokenizador)
# ============================================================

class Lexer:
    """Analiza código SmopsysQL en tokens"""
    
    def __init__(self, code: str):
        self.code = code
        self.position = 0
        self.line = 1
        self.column = 1
        self.tokens: List[Token] = []
        
        self.keywords = {
            'PULSE': TokenType.PULSE,
            'WAIT': TokenType.WAIT,
            'MEASURE': TokenType.MEASURE,
            'ENTANGLE': TokenType.ENTANGLE,
            'BROADCAST': TokenType.BROADCAST,
            'THERMAL': TokenType.THERMAL,
            'SYNC': TokenType.SYNC,
            'IF': TokenType.IF,
            'ELSE': TokenType.ELSE,
            'LOOP': TokenType.LOOP,
            'BREAK': TokenType.BREAK,
            'FUNCTION': TokenType.FUNCTION,
            'RETURN': TokenType.RETURN,
        }
    
    def tokenize(self) -> List[Token]:
        """Tokeniza el código"""
        while self.position < len(self.code):
            self._skip_whitespace_and_comments()
            
            if self.position >= len(self.code):
                break
            
            ch = self.code[self.position]
            
            if ch == '\n':
                self.tokens.append(Token(TokenType.NEWLINE, '\n', self.line, self.column))
                self.position += 1
                self.line += 1
                self.column = 1
            
            elif ch == '"':
                self._read_string()
            
            elif ch.isdigit():
                self._read_number()
            
            elif ch.isalpha() or ch == '_':
                self._read_identifier()
            
            elif ch == '(':
                self.tokens.append(Token(TokenType.LPAREN, '(', self.line, self.column))
                self.position += 1
                self.column += 1
            
            elif ch == ')':
                self.tokens.append(Token(TokenType.RPAREN, ')', self.line, self.column))
                self.position += 1
                self.column += 1
            
            elif ch == '{':
                self.tokens.append(Token(TokenType.LBRACE, '{', self.line, self.column))
                self.position += 1
                self.column += 1
            
            elif ch == '}':
                self.tokens.append(Token(TokenType.RBRACE, '}', self.line, self.column))
                self.position += 1
                self.column += 1
            
            elif ch == ',':
                self.tokens.append(Token(TokenType.COMMA, ',', self.line, self.column))
                self.position += 1
                self.column += 1
            
            elif ch == ';':
                self.tokens.append(Token(TokenType.SEMICOLON, ';', self.line, self.column))
                self.position += 1
                self.column += 1
            
            elif ch == '=':
                self.position += 1
                self.column += 1
                if self.position < len(self.code) and self.code[self.position] == '=':
                    self.tokens.append(Token(TokenType.EQUAL, '==', self.line, self.column - 1))
                    self.position += 1
                    self.column += 1
                else:
                    self.tokens.append(Token(TokenType.ASSIGN, '=', self.line, self.column - 1))
            
            elif ch == '+':
                self.tokens.append(Token(TokenType.PLUS, '+', self.line, self.column))
                self.position += 1
                self.column += 1
            
            else:
                self.position += 1
                self.column += 1
        
        self.tokens.append(Token(TokenType.EOF, None, self.line, self.column))
        return self.tokens
    
    def _skip_whitespace_and_comments(self):
        """Salta espacios y comentarios"""
        while self.position < len(self.code):
            ch = self.code[self.position]
            
            if ch in ' \t':
                self.position += 1
                self.column += 1
            elif ch == '/' and self.position + 1 < len(self.code) and self.code[self.position + 1] == '/':
                # Comentario de línea
                while self.position < len(self.code) and self.code[self.position] != '\n':
                    self.position += 1
            else:
                break
    
    def _read_string(self):
        """Lee un string"""
        start_col = self.column
        self.position += 1  # Saltar "
        self.column += 1
        
        value = ""
        while self.position < len(self.code) and self.code[self.position] != '"':
            value += self.code[self.position]
            self.position += 1
            self.column += 1
        
        self.position += 1  # Saltar "
        self.column += 1
        
        self.tokens.append(Token(TokenType.STRING, value, self.line, start_col))
    
    def _read_number(self):
        """Lee un número"""
        start_col = self.column
        value = ""
        
        while self.position < len(self.code) and (self.code[self.position].isdigit() or self.code[self.position] == '.' or (self.code[self.position] == 'x' and value == '0')):
            value += self.code[self.position]
            self.position += 1
            self.column += 1
        
        # Puede ser wavelength (1550nm) o duration (100ns)
        if self.position < len(self.code):
            if self.code[self.position].isalpha():
                # Leer unidades
                units = ""
                while self.position < len(self.code) and self.code[self.position].isalpha():
                    units += self.code[self.position]
                    self.position += 1
                    self.column += 1
                
                value += units
                
                if units in ['nm', 'um', 'mm']:
                    self.tokens.append(Token(TokenType.WAVELENGTH, value, self.line, start_col))
                elif units in ['ns', 'us', 'ms', 's']:
                    self.tokens.append(Token(TokenType.DURATION, value, self.line, start_col))
                else:
                    self.tokens.append(Token(TokenType.NUMBER, self._parse_val(value), self.line, start_col))
            else:
                self.tokens.append(Token(TokenType.NUMBER, self._parse_val(value), self.line, start_col))
        else:
            self.tokens.append(Token(TokenType.NUMBER, self._parse_val(value), self.line, start_col))

    def _parse_val(self, s: str) -> float:
        try:
            if s.startswith('0x'):
                return float(int(s, 16))
            return float(s)
        except:
            return 0.0

    def _read_identifier(self):
        """Lee identificador o palabra clave"""
        start_col = self.column
        value = ""
        
        while self.position < len(self.code) and (self.code[self.position].isalnum() or self.code[self.position] == '_'):
            value += self.code[self.position]
            self.position += 1
            self.column += 1
        
        # Chequear si es palabra clave
        upper_value = value.upper()
        if upper_value in self.keywords:
            self.tokens.append(Token(self.keywords[upper_value], value, self.line, start_col))
        else:
            # Chequear si es polarización (H, V, D, L)
            if value.upper() in ['H', 'V', 'D', 'L', 'R']:
                self.tokens.append(Token(TokenType.POLARIZATION, value.upper(), self.line, start_col))
            else:
                self.tokens.append(Token(TokenType.IDENTIFIER, value, self.line, start_col))

# ============================================================
# PARSER (Analizador Sintáctico)
# ============================================================

class Parser:
    """Convierte tokens en AST"""
    
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.position = 0
        self.instructions: List[Instruction] = []
    
    def parse(self) -> List[Instruction]:
        """Parsea los tokens"""
        while not self._is_at_end():
            self._skip_newlines()
            
            if self._is_at_end():
                break
            
            instr = self._parse_statement()
            if instr:
                self.instructions.append(instr)
        
        return self.instructions
    
    def _parse_statement(self) -> Optional[Instruction]:
        """Parsea una sentencia"""
        token = self._peek()
        
        if token.type == TokenType.PULSE:
            return self._parse_pulse()
        elif token.type == TokenType.WAIT:
            return self._parse_wait()
        elif token.type == TokenType.MEASURE:
            return self._parse_measure()
        elif token.type == TokenType.BROADCAST:
            return self._parse_broadcast()
        elif token.type == TokenType.THERMAL:
            return self._parse_thermal()
        elif token.type == TokenType.SYNC:
            return self._parse_sync()
        
        self._advance()
        return None
    
    def _parse_pulse(self) -> Instruction:
        """PULSE <wavelength> <duration> <polarization>"""
        self._consume(TokenType.PULSE, "Expected PULSE")
        
        wavelength_token = self._consume(TokenType.WAVELENGTH, "Expected wavelength")
        duration_token = self._consume(TokenType.DURATION, "Expected duration")
        polarization_token = self._consume(TokenType.POLARIZATION, "Expected polarization")
        self._consume_optional(TokenType.SEMICOLON)
        
        wavelength = wavelength_token.value
        duration = duration_token.value
        polarization = polarization_token.value
        
        return Instruction(
            type=InstructionType.PULSE_LASER,
            operands={
                'wavelength': wavelength,
                'duration': duration,
                'polarization': polarization
            },
            laser_params={
                'frequency_nm': float(wavelength.replace('nm', '')),
                'duration_ns': float(duration.replace('ns', '')) if 'ns' in duration else 0,
                'polarization': polarization
            }
        )
    
    def _parse_wait(self) -> Instruction:
        """WAIT <duration>"""
        self._consume(TokenType.WAIT, "Expected WAIT")
        duration_token = self._consume(TokenType.DURATION, "Expected duration")
        self._consume_optional(TokenType.SEMICOLON)
        
        # Convertir a nanosegundos
        duration_str = duration_token.value
        time_ns = self._parse_time_to_ns(duration_str)
        
        return Instruction(
            type=InstructionType.WAIT,
            operands={'time': time_ns}
        )
    
    def _parse_measure(self) -> Instruction:
        """MEASURE <qubit>"""
        self._consume(TokenType.MEASURE, "Expected MEASURE")
        qubit_token = self._consume(TokenType.IDENTIFIER, "Expected qubit identifier")
        self._consume_optional(TokenType.SEMICOLON)
        
        return Instruction(
            type=InstructionType.MEASURE,
            operands={'qubit': qubit_token.value}
        )
    
    def _parse_broadcast(self) -> Instruction:
        """BROADCAST <string>"""
        self._consume(TokenType.BROADCAST, "Expected BROADCAST")
        message_token = self._consume(TokenType.STRING, "Expected message")
        self._consume_optional(TokenType.SEMICOLON)
        
        return Instruction(
            type=InstructionType.BROADCAST,
            operands={'message': message_token.value}
        )
    
    def _parse_thermal(self) -> Instruction:
        """THERMAL <address> <threshold>"""
        self._consume(TokenType.THERMAL, "Expected THERMAL")
        
        addr_token = self._consume(TokenType.NUMBER, "Expected address")
        threshold_token = self._consume(TokenType.NUMBER, "Expected threshold")
        self._consume_optional(TokenType.SEMICOLON)
        
        return Instruction(
            type=InstructionType.THERMAL_PAGE,
            operands={
                'address': int(addr_token.value),
                'threshold': float(threshold_token.value)
            }
        )
    
    def _parse_sync(self) -> Instruction:
        """SYNC <phase>"""
        self._consume(TokenType.SYNC, "Expected SYNC")
        phase_token = self._consume(TokenType.NUMBER, "Expected phase")
        self._consume_optional(TokenType.SEMICOLON)
        
        return Instruction(
            type=InstructionType.SYNC_PHASE,
            operands={'phase': float(phase_token.value)}
        )
    
    def _parse_time_to_ns(self, time_str: str) -> float:
        """Convierte string de tiempo a nanosegundos"""
        if 'ns' in time_str:
            return float(time_str.replace('ns', ''))
        elif 'us' in time_str:
            return float(time_str.replace('us', '')) * 1000
        elif 'ms' in time_str:
            return float(time_str.replace('ms', '')) * 1e6
        else:
            return float(time_str)
    
    # Utilidades
    def _peek(self) -> Token:
        if self.position < len(self.tokens):
            return self.tokens[self.position]
        return self.tokens[-1]  # EOF
    
    def _advance(self) -> Token:
        token = self._peek()
        self.position += 1
        return token
    
    def _consume(self, expected: TokenType, message: str) -> Token:
        token = self._peek()
        if token.type != expected:
            raise SyntaxError(f"{message} at line {token.line}, got {token.type}")
        return self._advance()
    
    def _consume_optional(self, token_type: TokenType) -> bool:
        if self._peek().type == token_type:
            self._advance()
            return True
        return False
    
    def _skip_newlines(self):
        while self._peek().type == TokenType.NEWLINE:
            self._advance()
    
    def _is_at_end(self) -> bool:
        return self._peek().type == TokenType.EOF

# ============================================================
# COMPILADOR
# ============================================================

class Compiler:
    """Compila código SmopsysQL a C y JSON"""
    
    def __init__(self):
        self.instructions: List[Instruction] = []
    
    def compile(self, code: str) -> Dict:
        """Compila código fuente"""
        # Lexing
        lexer = Lexer(code)
        tokens = lexer.tokenize()
        
        # Parsing
        parser = Parser(tokens)
        self.instructions = parser.parse()
        
        # Generación de código
        return {
            'c_code': self._generate_c(),
            'json_instructions': [instr.to_json() for instr in self.instructions],
            'instruction_count': len(self.instructions),
            'laser_instructions': [i for i in self.instructions if i.type == InstructionType.PULSE_LASER],
            'memory_instructions': [i for i in self.instructions if i.type == InstructionType.THERMAL_PAGE]
        }
    
    def _generate_c(self) -> str:
        """Genera código C para el kernel"""
        code = "/* AUTO-GENERATED SMOPSYSQL CODE */\n"
        code += "#include \"ql_bridge.h\"\n\n"
        code += "void quantum_program(void) {\n"
        
        for instr in self.instructions:
            code += f"    {instr.to_c_code()}\n"
        
        code += "}\n"
        return code

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python smopsys_ql.py <input.sql> <output.c>")
        sys.exit(1)
        
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    with open(input_file, 'r') as f:
        code = f.read()
        
    compiler = Compiler()
    result = compiler.compile(code)
    
    with open(output_file, 'w') as f:
        f.write(result['c_code'])
    
    print(f"Compiled {input_file} to {output_file}")
