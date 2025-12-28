# -*- coding: utf-8 -*-
import time
from datetime import datetime
from rich.console import Console
from rich.layout import Layout
from rich.panel import Panel
from rich.table import Table
from rich.live import Live
from rich.progress import BarColumn, Progress, TextColumn
from rich import box

class IronDomeMonitor:
    def __init__(self):
        self.console = Console()
        self.layout = Layout()
        self.event_log = []
        self.max_logs = 10
        self.start_time = time.time()
        
        # Initialize Layout
        self.layout.split_column(
            Layout(name="header", size=3),
            Layout(name="main"),
            Layout(name="footer", size=3)
        )
        self.layout["main"].split_row(
            Layout(name="stats", ratio=1),
            Layout(name="logs", ratio=2)
        )
        
    def _create_header(self):
        elapsed = time.time() - self.start_time
        grid = Table.grid(expand=True)
        grid.add_column(justify="left", ratio=1)
        grid.add_column(justify="center", ratio=1)
        grid.add_column(justify="right", ratio=1)
        grid.add_row(
            "[bold cyan]🛡️  IRON DOME v2.0[/bold cyan]",
            "[bold white]METRIPLECTIC SURVEILLANCE[/bold white]",
            f"[yellow]Uptime: {elapsed:.1f}s[/yellow]"
        )
        return Panel(grid, style="white on blue")

    def _create_stats_panel(self, diagnostics, stats_summary):
        table = Table.grid(expand=True)
        table.add_column(style="bold yellow")
        table.add_column(justify="right")
        
        # Physics Stats
        table.add_row("Metriplectic Health", "")
        table.add_row("  H (Symp):", f"{diagnostics.get('symp_mag', 0):.2f}")
        table.add_row("  S (Metr):", f"{diagnostics.get('metr_mag', 0):.2f}")
        table.add_row("  Entropy:", f"{diagnostics.get('entropy', 0):.4f}")
        table.add_row("", "")
        
        # Security Stats
        table.add_row("Security Analytics", "")
        table.add_row("  Accuracy:", f"{stats_summary.get('Accuracy', '100%')}")
        s = stats_summary.get('Stats', {})
        table.add_row("  True Positives:", f"[green]{s.get('TP', 0)}[/green]")
        table.add_row("  False Positives:", f"[red]{s.get('FP', 0)}[/red]")
        table.add_row("  True Negatives:", f"[blue]{s.get('TN', 0)}[/blue]")
        
        return Panel(table, title="[bold]System Status[/bold]", border_style="cyan")

    def _create_logs_table(self):
        table = Table(
            expand=True, 
            box=box.SIMPLE, 
            header_style="bold magenta",
            show_edge=False
        )
        table.add_column("Time", width=10)
        table.add_column("Target", width=20)
        table.add_column("State", width=25)
        table.add_column("Result")
        
        for event in reversed(self.event_log[-self.max_logs:]):
            target_style = "bold red" if "Drone" in event['target'] or "Threat" in event['target'] else "green"
            table.add_row(
                event['time'],
                f"[{target_style}]{event['target']}[/]",
                event['state'],
                event['result']
            )
        return Panel(table, title="[bold]Recent Events (TUI Circular Log)[/bold]", border_style="magenta")

    def add_event(self, target, state, result):
        self.event_log.append({
            'time': datetime.now().strftime("%H:%M:%S"),
            'target': target,
            'state': state,
            'result': result
        })
        if len(self.event_log) > 100:
            self.event_log.pop(0)

    def update_view(self, diagnostics, stats_summary, current_threat_level):
        self.layout["header"].update(self._create_header())
        self.layout["stats"].update(self._create_stats_panel(diagnostics, stats_summary))
        self.layout["logs"].update(self._create_logs_table())
        
        # Footer with Threat Meter
        meter = f"[bold]Threat Level:[/bold] [red]{'█' * int(current_threat_level * 20)}[/red]" if current_threat_level > 0.1 else "[bold]Threat Level:[/bold] [green]SAFE[/green]"
        self.layout["footer"].update(Panel(meter, border_style="white"))
        
        return self.layout

    def start_live(self):
        return Live(self.layout, refresh_per_second=4, screen=True)
