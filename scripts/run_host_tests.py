#!/usr/bin/env python3
"""Build and run the firmware's host regressions without a connected device."""
import pathlib
import subprocess
import tempfile
from watchlist_codegen import generate

ROOT = pathlib.Path(__file__).resolve().parents[1]
FAKES = {
    "screen_render": "screen_render_fakes",
    "cache_paging": "wifi_fakes",
    "wifi_reconnect_persistence": "services_fakes",
    "screen_render_memory": "screen_render_fakes",
    "terminal_modifier": "ssh_ui_fakes",
    "cache_recovery": "wifi_fakes",
    "editor_escape": "wifi_fakes",
    "unsaved_back": "wifi_fakes",
    "light_sleep": "power_fakes",
    "ssh_worker": "ssh_worker_fakes",
    "ssh_app": "ssh_ui_fakes",
    "ssh_persistence": "services_fakes",
    "capture_reminders": "wifi_fakes",
    "startup_layout": "display_fakes",
    "reminder_persistence": "services_fakes",
    "system_terminal": "system_fakes",
    "input": "fakes",
    "capture": "wifi_fakes",
    "capture_timeout": "wifi_fakes",
    "wifi_form": "wifi_fakes",
    "weather_layout": "display_fakes",
    "crypto_layout": "display_fakes",
    "calendar_layout": "display_fakes",
    "idle_board_layout": "display_fakes",
    "status_layout": "wifi_fakes",
    "service_persistence": "services_fakes",
    "location_settings": "wifi_fakes",
    "settings_terminal_ui": "wifi_fakes",
    "home_themes": "wifi_fakes",
}

def main():
    generate(ROOT / "data/crypto/watchlist.json", ROOT / ".pio/generated/Watchlist.h")
    sources = sorted((ROOT / "tests").glob("*.cpp"))
    with tempfile.TemporaryDirectory(prefix="idkos-tests-") as output:
        for source in sources:
            executable = pathlib.Path(output) / source.stem
            command = ["g++", "-std=c++11", "-Wall", "-Wextra", "-I",
                       str(ROOT / ".pio/libdeps/cardputer-adv/ArduinoJson/src"),
                       "-I", str(ROOT / ".pio/generated")]
            if source.stem in FAKES:
                command += ["-I", str(ROOT / "tests" / FAKES[source.stem])]
            if source.stem == "ssh_worker":
                command += ["-pthread"]
            command += [str(source), "-o", str(executable)]
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(executable)], cwd=ROOT, check=True)
            print(f"PASS {source.stem}", flush=True)
    print(f"All {len(sources)} host regressions passed.")

if __name__ == "__main__":
    main()
