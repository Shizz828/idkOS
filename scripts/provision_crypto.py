"""Copy the configured Demo key to idkOS over USB without displaying it."""
import argparse
import json
from pathlib import Path
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument("--port", default="/dev/ttyACM0")
parser.add_argument("--key-file", type=Path, default=Path(__file__).resolve().parents[1] / "secrets/coingecko.json")
args = parser.parse_args()
credentials = json.loads(args.key_file.read_text())
key = credentials.get("api_key", "")
if credentials.get("plan") != "demo" or not key.startswith("CG-") or not key.isascii() or not 3 < len(key) < 128 or any(c.isspace() for c in key):
    raise SystemExit("Configure a valid Demo key locally first.")
try:
    # Open without asserting DTR/RTS: on the ESP32-S3 USB port that line
    # sequence can reset the Cardputer into download mode.
    device = serial.Serial()
    device.port, device.baudrate, device.timeout = args.port, 115200, 1
    device.dtr = device.rts = False
    device.open()
    with device:
        time.sleep(2)
        device.reset_input_buffer()
        device.write(("IDKOS_CRYPTO_KEY " + key + "\n").encode("ascii"))
        device.flush()
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            response = device.readline().strip()
            # Never print arbitrary device output: only recognize protocol replies.
            if response == b"IDKOS_CRYPTO_KEY_SAVED":
                print("SUCCESS: Demo key saved on Cardputer. Open Crypto with Fn+3.")
                break
            if response in (b"IDKOS_CRYPTO_KEY_REJECTED", b"IDKOS_CRYPTO_KEY_SAVE_FAILED"):
                raise SystemExit("Device rejected or could not save the key. No key was displayed.")
            if b"waiting for download" in response:
                raise SystemExit("Cardputer is in download mode. Power it off and on without G0, then retry.")
        else:
            raise SystemExit("No save confirmation. Check idkOS is running and close serial monitors before retrying.")
except serial.SerialException:
    raise SystemExit("Cannot open the device port. Check USB connection, port name and other serial programs.")
