# -*- coding: utf-8 -*-
from rich.console import Console
from rich.layout import Layout
from rich.panel import Panel
from rich.table import Table
from rich.live import Live
from rich.text import Text
from datetime import datetime
import time

class IronDomeMonitor:
    def __init__(self):
        self.console = Console()
        self.layout = Layout()
        self.event_log = []
        self.max_logs = 10
        self.start_time = datetime.now()
        self.mode = "MONITOR" # "MONITOR" or "BIOS"
        self.bios_selection = 0
        self.bios_options = [
            "Sensitivity Threshold",
            "Dissipation Coefficient (S)",
            "Hamiltonian Factor (H)",
            "Alert Silence Mode",
            "Recalibrate Core",
            "Exit BIOS"
        ]
        
        self.layout.split(
            Layout(name="header", size=3),
            Layout(name="main"),
            Layout(name="footer", size=3)
        )
        self.layout["main"].split_row(
            Layout(name="stats", ratio=1),
            Layout(name="logs", ratio=2)
        )

    def set_mode(self, mode):
        self.mode = mode

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
        if self.mode == "BIOS":
            self.layout["main"].update(self._create_bios_view())
            self.layout["header"].update(self._create_header(title="IRON DOME BIOS/UEFI v2.0"))
            self.layout["footer"].update(Panel("[bold yellow]↑↓ Navigate | ENTER Select | TAB Exit[/]", border_style="blue"))
        else:
            self.layout["header"].update(self._create_header())
            self.layout["stats"].update(self._create_stats_panel(diagnostics, stats_summary))
            self.layout["logs"].update(self._create_logs_table())
            
            # Footer with Threat Meter
            meter = f"[bold]Threat Level:[/bold] [red]{'█' * int(current_threat_level * 20)}[/red]" if current_threat_level > 0.1 else "[bold]Threat Level:[/bold] [green]SAFE[/green]"
            self.layout["footer"].update(Panel(meter, border_style="white"))
        
        return self.layout

    def _create_header(self, title="IRON DOME v2.0"):
        uptime = str(datetime.now() - self.start_time).split(".")[0]
        grid = Table.grid(expand=True)
        grid.add_column(justify="left", ratio=1)
        grid.add_column(justify="center", ratio=1)
        grid.add_column(justify="right", ratio=1)
        
        style = "white on blue" if self.mode == "BIOS" else "default"
        grid.add_row(
            "🛡️  [bold cyan]" + title + "[/]",
            "[bold white]METRIPLECTIC SURVEILLANCE[/]",
            f"Uptime: [yellow]{uptime}[/]"
        )
        return Panel(grid, style=style)

    def _create_bios_view(self):
        table = Table(title="Setup Utility", expand=True, box=None)
        table.add_column("Option", style="white")
        table.add_column("Value", justify="right")

        for i, option in enumerate(self.bios_options):
            style = "bold white on blue" if i == self.bios_selection else "dim white"
            table.add_row(f" > {option}" if i == self.bios_selection else f"   {option}", "[ ENABLED ]" if "Alert" in option else "[ AUTO ]", style=style)

        return Panel(table, title="[bold white]BIOS Setup Utility[/]", border_style="blue", style="white on blue")

    def _create_stats_panel(self, d, s):
        table = Table.grid(padding=(0, 1))
        table.add_column(style="cyan")
        table.add_column(style="magenta")
        
        table.add_row("[bold white]Metriplectic Health[/]", "")
        table.add_row("  H (Symp):", f"{d.get('H', 0):.2f}")
        table.add_row("  S (Metr):", f"{d.get('S', 0):.2f}")
        table.add_row("  Entropy:", f"{d.get('entropy', 0):.2f}")
        table.add_row("", "")
        table.add_row("[bold white]Memory Geometry[/]", "")
        table.add_row("  Z-Finch:", f"{d.get('centroid_z', 0):.4f}")
        
        # Confinement state based on entropy and Reynolds proxy
        # If entropy is low and coherence is high -> Coherent
        ent = d.get('entropy', 0)
        conf = "Coherent" if ent < 0.2 else ("Thermal" if ent < 0.6 else "Evaporated")
        table.add_row("  Confinement:", conf)
        table.add_row("", "")
        table.add_row("[bold white]Security[/]", "")
        table.add_row("  True Positives:", f"[green]{s.get('TP', 0)}[/green]")
        table.add_row("  False Positives:", f"[red]{s.get('FP', 0)}[/red]")
        table.add_row("  True Negatives:", f"[blue]{s.get('TN', 0)}[/blue]")
        table.add_row("", "")
        
        # Hotkeys Help
        table.add_row("[bold white]Controls[/bold white]", "")
        table.add_row("  [TAB]", "Enter BIOS")
        table.add_row("  [q] ", "Quit System")
        table.add_row("  [r] ", "Recalibrate")
        
        return Panel(table, title="[bold]System Status[/bold]", border_style="cyan")

    def _create_logs_table(self):
        table = Table(expand=True, box=None)
        table.add_column("Time", width=10)
        table.add_column("Target", width=15)
        table.add_column("State", width=25)
        table.add_column("Result")
        
        for event in reversed(self.event_log[-self.max_logs:]):
            target_style = "bold red" if "Drone" in event['target'] or "Threat" in event['target'] else "green"
            table.add_row(
                event['time'],
                Text(event['target'], style=target_style),
                event['state'],
                event['result']
            )
        return Panel(table, title="[bold]Recent Events (TUI Circular Log)[/bold]", border_style="magenta")

    def start_live(self):
        return Live(self.layout, refresh_per_second=4, screen=True)
