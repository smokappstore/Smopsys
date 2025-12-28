# -*- coding: utf-8 -*-
import sys
import tty
import termios
import select
import threading
from queue import Queue

class CommandInterface:
    """Non-blocking keyboard listener for Linux terminal supporting special keys."""
    
    # Key Constants
    UP = "\x1b[A"
    DOWN = "\x1b[B"
    RIGHT = "\x1b[C"
    LEFT = "\x1b[D"
    ENTER = "\r"
    TAB = "\t"
    ESC = "\x1b"
    BACKSPACE = "\x7f"

    def __init__(self):
        self.command_queue = Queue()
        self.running = False
        self.old_settings = termios.tcgetattr(sys.stdin)
        self.thread = None

    def start(self):
        self.running = True
        self.thread = threading.Thread(target=self._listener_loop, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            # Send a character to break the select loop if needed
            pass 
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_settings)

    def _listener_loop(self):
        # Set to cbreak mode
        tty.setcbreak(sys.stdin.fileno())
        try:
            while self.running:
                if select.select([sys.stdin], [], [], 0.1)[0]:
                    char = sys.stdin.read(1)
                    if char == "\x1b": # Escape sequence
                        # Check if more chars are coming (for arrows)
                        if select.select([sys.stdin], [], [], 0.01)[0]:
                            next_char = sys.stdin.read(2)
                            char += next_char
                    self.command_queue.put(char)
        finally:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_settings)

    def get_next_command(self):
        if not self.command_queue.empty():
            return self.command_queue.get()
        return None
