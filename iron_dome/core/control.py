# -*- coding: utf-8 -*-
import sys
import tty
import termios
import select
import threading
from queue import Queue

class CommandInterface:
    """Non-blocking keyboard listener for Linux terminal."""
    def __init__(self):
        self.command_queue = Queue()
        self.running = False
        self.old_settings = termios.tcgetattr(sys.stdin)
        self.thread = None

    def _get_char(self):
        tty.setraw(sys.stdin.fileno())
        select.select([sys.stdin], [], [], 0)
        char = sys.stdin.read(1)
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_settings)
        return char

    def start(self):
        self.running = True
        self.thread = threading.Thread(target=self._listener_loop, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_settings)

    def _listener_loop(self):
        while self.running:
            if select.select([sys.stdin], [], [], 0.1)[0]:
                char = sys.stdin.read(1)
                if char:
                    self.command_queue.put(char)

    def get_next_command(self):
        if not self.command_queue.empty():
            return self.command_queue.get()
        return None
